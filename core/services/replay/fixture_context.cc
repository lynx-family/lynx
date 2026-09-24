// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_context.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "quickjs/include/quickjs.h"

namespace lynx {
namespace runtime {
namespace js {

namespace {

// Mirrors fixture_evaluator.cc: non-finite or non-positive delays become 0
// (immediate); finite values beyond int64_t saturate instead of invoking UB.
int64_t NormalizeDelayMs(double delay_ms) {
  if (!std::isfinite(delay_ms) || delay_ms <= 0) {
    return 0;
  }
  constexpr double kMaxDelayMs =
      static_cast<double>(std::numeric_limits<int64_t>::max());
  if (delay_ms >= kMaxDelayMs) {
    return std::numeric_limits<int64_t>::max();
  }
  return static_cast<int64_t>(delay_ms);
}

bool IsSafeAssetPath(const std::string& path) {
  if (path.empty() || path.front() == '/' ||
      path.find('\\') != std::string::npos ||
      path.find(':') != std::string::npos) {
    return false;
  }
  size_t start = 0;
  while (start <= path.size()) {
    size_t end = path.find('/', start);
    std::string component = path.substr(start, end - start);
    if (component.empty() || component == "." || component == "..") {
      return false;
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }
  return true;
}

std::string ReadWholeFile(const std::string& path, size_t max_bytes) {
  std::ifstream ifs(path, std::ios::binary);
  if (!ifs.is_open()) {
    return "";
  }
  ifs.seekg(0, std::ios::end);
  const std::streamoff size = ifs.tellg();
  if (size <= 0 || static_cast<uint64_t>(size) > max_bytes) {
    return "";
  }
  ifs.seekg(0, std::ios::beg);
  std::string content(static_cast<size_t>(size), '\0');
  ifs.read(content.data(), static_cast<std::streamsize>(size));
  if (!ifs) {
    return "";
  }
  return content;
}

// Adds a no-op host function with the given name to `ctx`. Lifecycle methods
// (setThreadStrategy, loadTemplate, after, ...) are extracted from fixture.js
// during the offline-eval pass on the loader thread; during the live dispatch
// session they only need to not throw so the top-level fixture body can run.
void InstallNoop(Runtime& rt, Object& ctx, const char* name) {
  ctx.setProperty(
      rt, name,
      Function::createFromHostFunction(
          rt, PropNameID::forAscii(rt, name), 1,
          HostFunctionType([](Runtime&, const Value&, const Value*, size_t)
                               -> base::expected<Value, JSINativeException> {
            return Value::undefined();
          })));
}

}  // namespace

FixtureContext::FixtureContext() = default;
FixtureContext::~FixtureContext() { Destroy(); }

int FixtureContext::InterruptHandler(LEPUSRuntime*, void* opaque) {
  auto* self = static_cast<FixtureContext*>(opaque);
  if (std::chrono::steady_clock::now() < self->interrupt_deadline_) {
    return 0;
  }
  self->interrupt_timed_out_ = true;
  return 1;
}

bool FixtureContext::Initialize(const std::string& fixture_dir,
                                const FixtureContextLimits& limits) {
  if (initialized_) {
    return true;
  }
  fixture_dir_ = fixture_dir;
  limits_ = limits;

  runtime_ = lynx::runtime::js::makeQuickJsRuntime();
  if (!runtime_) {
    LOGE("FixtureContext failed to create the fixture runtime, dir: "
         << fixture_dir_);
    return false;
  }
  StartupData startup_data{};
  vm_ = runtime_->createVM(&startup_data);
  js_context_ = runtime_->createContext(vm_);
  runtime_->InitRuntime(js_context_);

  // Bound the live session the same way the offline fixture evaluation is
  // bounded: a memory cap plus an interrupt handler with a per-execution
  // deadline, so a buggy or hostile fixture.js cannot hang or OOM the host.
  auto* quickjs_runtime = static_cast<QuickjsRuntime*>(runtime_.get());
  LEPUSRuntime* lepus_runtime = quickjs_runtime->getJSRuntime();
  if (limits_.memory_limit_bytes > 0) {
    LEPUS_SetMemoryLimit(lepus_runtime, limits_.memory_limit_bytes);
  }
  LEPUS_SetInterruptHandler(lepus_runtime, &FixtureContext::InterruptHandler,
                            this);

  RegisterCtxObject();
  if (!LoadFixtureScript()) {
    Destroy();
    return false;
  }
  initialized_ = true;
  return true;
}

void FixtureContext::Destroy() {
  handler_map_.clear();
  handler_state_map_.clear();
  js_context_.reset();
  vm_.reset();
  runtime_.reset();
  interrupt_deadline_ = std::chrono::steady_clock::time_point::max();
  interrupt_timed_out_ = false;
  initialized_ = false;
}

bool FixtureContext::HasHandler(const std::string& module,
                                const std::string& method) const {
  return handler_map_.find(module + "." + method) != handler_map_.end();
}

void FixtureContext::RegisterCtxObject() {
  Scope scope(*runtime_);
  Object ctx(*runtime_);

  auto register_fn = Function::createFromHostFunction(
      *runtime_, PropNameID::forAscii(*runtime_, "register"), 3,
      HostFunctionType(
          [this](Runtime& rt, const Value& thisVal, const Value* args,
                 size_t count) -> base::expected<Value, JSINativeException> {
            if (count < 3 || !args[0].isString() || !args[1].isString() ||
                !args[2].isObject()) {
              return Value::undefined();
            }
            auto module = args[0].getString(rt).utf8(rt);
            auto method = args[1].getString(rt).utf8(rt);
            auto func_opt = args[2].getObject(rt).asFunction(rt);
            if (!func_opt.has_value()) {
              return Value::undefined();
            }
            std::string key = module + "." + method;
            handler_map_.insert_or_assign(key, std::move(*func_opt));
            Object state(rt);
            state.setProperty(rt, "n", 0);
            handler_state_map_.insert_or_assign(key, std::move(state));
            return Value::undefined();
          }));

  auto read_asset_fn = Function::createFromHostFunction(
      *runtime_, PropNameID::forAscii(*runtime_, "readAsset"), 1,
      HostFunctionType(
          [this](Runtime& rt, const Value& thisVal, const Value* args,
                 size_t count) -> base::expected<Value, JSINativeException> {
            if (count < 1 || !args[0].isString()) {
              return Value::undefined();
            }
            auto path = args[0].getString(rt).utf8(rt);
            std::string content = ReadAssetFile(path);
            if (content.empty()) {
              return Value::undefined();
            }
            bool is_json =
                path.size() >= 5 && path.substr(path.size() - 5) == ".json";
            if (is_json) {
              auto json_val = Value::createFromJsonUtf8(
                  rt, reinterpret_cast<const uint8_t*>(content.data()),
                  content.size());
              if (json_val.has_value()) {
                return std::move(*json_val);
              }
              return Value::undefined();
            }
            return Value(String::createFromUtf8(rt, content));
          }));

  ctx.setProperty(*runtime_, "register", std::move(register_fn));
  ctx.setProperty(*runtime_, "readAsset", std::move(read_asset_fn));

  // Lifecycle / event APIs are no-ops during the live dispatch session.
  InstallNoop(*runtime_, ctx, "setThreadStrategy");
  InstallNoop(*runtime_, ctx, "updateViewPort");
  InstallNoop(*runtime_, ctx, "setGlobalProps");
  InstallNoop(*runtime_, ctx, "loadTemplate");
  InstallNoop(*runtime_, ctx, "loadTemplateBundle");
  InstallNoop(*runtime_, ctx, "reloadTemplate");
  InstallNoop(*runtime_, ctx, "after");
  InstallNoop(*runtime_, ctx, "sendGlobalEvent");
  InstallNoop(*runtime_, ctx, "sendEventAndroid");
  InstallNoop(*runtime_, ctx, "sendCustomEvent");
  InstallNoop(*runtime_, ctx, "sendTouchEvent");
  InstallNoop(*runtime_, ctx, "sharedData");
  InstallNoop(*runtime_, ctx, "dispatch");

  runtime_->global().setProperty(*runtime_, "ctx", std::move(ctx));
}

bool FixtureContext::LoadFixtureScript() {
  std::string source =
      ReadWholeFile(fixture_dir_ + "/fixture.js", limits_.max_script_bytes);
  if (source.empty()) {
    LOGE(
        "FixtureContext failed to read fixture.js (missing, empty, or larger "
        "than "
        << limits_.max_script_bytes << " bytes), dir: " << fixture_dir_);
    return false;
  }

  // fixture.js is authored as an ES module ("export default function(ctx)").
  // Rewrite it into a plain immediate invocation against the global ctx.
  size_t export_pos = source.find("export default function");
  std::string transformed;
  if (export_pos != std::string::npos) {
    transformed = source;
    transformed.replace(export_pos, std::strlen("export default function"),
                        "var __fixture_main__ = function");
    transformed += "\n__fixture_main__(ctx);\n";
  } else {
    transformed = "(function(ctx) {\n" + source + "\n})(ctx);\n";
  }

  Scope scope(*runtime_);
  auto buffer = std::make_shared<StringBuffer>(std::move(transformed));
  interrupt_timed_out_ = false;
  interrupt_deadline_ = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(limits_.timeout_ms);
  auto result = runtime_->evaluateJavaScript(buffer, "fixture.js");
  const bool timed_out = interrupt_timed_out_;
  interrupt_deadline_ = std::chrono::steady_clock::time_point::max();
  if (timed_out) {
    LOGE("FixtureContext fixture.js evaluation timed out after "
         << limits_.timeout_ms << "ms, dir: " << fixture_dir_);
    return false;
  }
  if (!result.has_value()) {
    LOGE("FixtureContext failed to evaluate fixture.js in dir: "
         << fixture_dir_ << " (script error or unsupported export form)");
    return false;
  }
  return true;
}

FixtureDispatchResult FixtureContext::Dispatch(
    const std::string& module, const std::string& method,
    const std::string& args_json,
    const std::vector<size_t>& callback_positions) {
  FixtureDispatchResult out;
  if (!initialized_) {
    return out;
  }
  std::string key = module + "." + method;
  auto handler_it = handler_map_.find(key);
  if (handler_it == handler_map_.end()) {
    return out;
  }
  auto state_it = handler_state_map_.find(key);
  if (state_it == handler_state_map_.end()) {
    return out;
  }
  out.handled = true;

  Scope scope(*runtime_);

  // args param: parse the JSON array string into a fixture-runtime array.
  Value args_value = Value::undefined();
  if (!args_json.empty()) {
    auto parsed = Value::createFromJsonUtf8(
        *runtime_, reinterpret_cast<const uint8_t*>(args_json.data()),
        args_json.size());
    if (parsed.has_value()) {
      args_value = std::move(*parsed);
    }
  }
  if (!args_value.isObject()) {
    auto empty = Array::createWithLength(*runtime_, 0);
    if (empty.has_value()) {
      args_value = Value(*runtime_, *empty);
    }
  }

  // Restore function-typed placeholders at their original positions. The
  // generated fixture matcher treats the string "function" in recorded args
  // as a type sentinel, so leaving the JSON transport's null placeholders in
  // place would make every callback-bearing call fail to match.
  if (args_value.isObject()) {
    auto args_array_opt = args_value.getObject(*runtime_).asArray(*runtime_);
    if (args_array_opt.has_value()) {
      auto& args_array = *args_array_opt;
      for (size_t callback_position : callback_positions) {
        if (callback_position >= args_array.size(*runtime_)) {
          continue;
        }
        auto placeholder = Function::createFromHostFunction(
            *runtime_, PropNameID::forAscii(*runtime_, "callbackPlaceholder"),
            0,
            HostFunctionType([](Runtime&, const Value&, const Value*, size_t)
                                 -> base::expected<Value, JSINativeException> {
              return Value::undefined();
            }));
        args_array.setValueAtIndex(*runtime_, callback_position,
                                   std::move(placeholder));
      }
    }
  }

  // callbacks param: an array of proxy host functions. Each proxy records the
  // JSON value the handler passes so the caller can replay it asynchronously.
  // The proxies own their result sink via shared_ptr: a handler is expected to
  // call its callbacks synchronously, but nothing prevents it from stashing a
  // proxy and invoking it during a later dispatch. Capturing `out` (a stack
  // local of Dispatch) by reference would turn such deferred calls into a
  // use-after-free, so the sink must outlive the proxies.
  auto callback_sink =
      std::make_shared<std::vector<FixtureDispatchResult::CallbackCall>>();
  auto callbacks_array_opt =
      Array::createWithLength(*runtime_, callback_positions.size());
  if (!callbacks_array_opt.has_value()) {
    return out;
  }
  auto& callbacks_array = *callbacks_array_opt;
  for (size_t i = 0; i < callback_positions.size(); i++) {
    size_t cb_index = i;
    auto proxy = Function::createFromHostFunction(
        *runtime_, PropNameID::forAscii(*runtime_, "callback"), 1,
        HostFunctionType([callback_sink, cb_index](
                             Runtime& rt, const Value& thisVal,
                             const Value* cb_args, size_t cb_count)
                             -> base::expected<Value, JSINativeException> {
          std::string value_json;
          if (cb_count >= 1) {
            if (cb_args[0].isUndefined() || cb_args[0].isNull()) {
              value_json = "null";
            } else {
              auto j = cb_args[0].toJsonString(rt);
              if (j.has_value() && j->isString()) {
                value_json = j->getString(rt).utf8(rt);
              }
            }
          } else {
            value_json = "null";
          }
          int64_t delay_ms = 0;
          if (cb_count >= 2 && cb_args[1].isNumber()) {
            // NaN/Infinity/huge values must not reach the int64_t cast
            // (UB); mirror the evaluator's normalization.
            delay_ms = NormalizeDelayMs(cb_args[1].getNumber());
          }
          callback_sink->push_back({cb_index, std::move(value_json), delay_ms});
          return Value::undefined();
        }));
    callbacks_array.setValueAtIndex(*runtime_, i, std::move(proxy));
  }

  // this.n bookkeeping so handlers can branch on call ordinal.
  auto n_opt = state_it->second.getProperty(*runtime_, "n");
  double n_val =
      (n_opt.has_value() && n_opt->isNumber()) ? n_opt->getNumber() : 0;
  state_it->second.setProperty(*runtime_, "n", n_val + 1);

  Value invoke_args[2] = {std::move(args_value),
                          Value(*runtime_, callbacks_array)};
  // Bound one handler execution with the configured deadline; callWithThis is
  // exception-free (errors come back via the return value), so the deadline is
  // always disarmed right after the call.
  interrupt_timed_out_ = false;
  interrupt_deadline_ = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(limits_.timeout_ms);
  auto result = handler_it->second.callWithThis(*runtime_, state_it->second,
                                                invoke_args, 2);
  const bool timed_out = interrupt_timed_out_;
  interrupt_deadline_ = std::chrono::steady_clock::time_point::max();

  if (timed_out) {
    LOGE("FixtureContext dispatch timed out after " << limits_.timeout_ms
                                                    << "ms, module: " << module
                                                    << ", method: " << method);
    // Report the call as unmatched so ModuleFixtureReplay falls back to
    // invoking the original callbacks with null instead of replaying the
    // partial results recorded before the interrupt.
    out.handled = false;
    return out;
  }

  out.callbacks = std::move(*callback_sink);
  if (result.has_value() && !result->isUndefined()) {
    auto rj = result->toJsonString(*runtime_);
    if (rj.has_value() && rj->isString()) {
      out.return_value_json = rj->getString(*runtime_).utf8(*runtime_);
    }
  }
  return out;
}

std::string FixtureContext::ReadAssetFile(const std::string& path) {
  std::string relative_path = path;
  constexpr char kAssetsPrefix[] = "assets/";
  if (relative_path.rfind(kAssetsPrefix, 0) == 0) {
    relative_path.erase(0, sizeof(kAssetsPrefix) - 1);
  }
  if (!IsSafeAssetPath(relative_path)) {
    return "";
  }
  return ReadWholeFile(fixture_dir_ + "/assets/" + relative_path,
                       limits_.max_asset_bytes);
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
