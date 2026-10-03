// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_context.h"

#include <memory>
#include <string>
#include <utility>

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
namespace replay {

using runtime::js::Function;
using runtime::js::HostFunctionType;
using runtime::js::JSINativeException;
using runtime::js::Object;
using runtime::js::PropNameID;
using runtime::js::Runtime;
using runtime::js::Value;

namespace {

Function CreateNoop(Runtime& runtime, const char* name) {
  return Function::createFromHostFunction(
      runtime, PropNameID::forAscii(runtime, name), 0,
      HostFunctionType([](Runtime&, const Value&, const Value*,
                          size_t) -> base::expected<Value, JSINativeException> {
        return Value::undefined();
      }));
}

}  // namespace

struct FixtureContext::CallbackSink {
  std::unique_ptr<Object> token;
  std::vector<FixtureDispatchResult::CallbackCall> calls;
  size_t bytes = 0;
  bool failed = false;
};

struct FixtureContext::Handler {
  Handler(Runtime& runtime, Function function)
      : function(std::move(function)), state(runtime) {
    state.setProperty(runtime, "n", 0);
  }
  Function function;
  Object state;
};

FixtureContext::FixtureContext() = default;
FixtureContext::~FixtureContext() { Destroy(); }

bool FixtureContext::Initialize(const std::string& fixture_directory,
                                const FixtureContextLimits& limits) {
  Destroy();
  if (!limits.IsValid()) {
    return false;
  }
  std::string source =
      ReadFixtureScript(fixture_directory, limits.max_script_bytes);
  if (source.empty()) {
    return false;
  }
  limits_ = limits;
  runtime_ = CreateQuickJsFixtureRuntime(limits);
  if (!runtime_) {
    return false;
  }
  {
    // Install fixed host bindings before script allocation limits apply. JSI
    // host function creation has no recoverable allocation-failure result.
    runtime::js::Scope scope(runtime_->runtime());
    RegisterCtxObject(fixture_directory);
    if (InitializeCallbackFactory()) {
      ScopedFixtureExecution execution(*runtime_);
      auto result = runtime_->EvaluateScript(std::move(source));
      initialized_ =
          result.has_value() && !runtime_->timed_out() && !registration_failed_;
    }
  }
  if (!initialized_) {
    LOGE("FixtureContext initialization failed");
    Destroy();
  }
  return initialized_;
}

void FixtureContext::Destroy() {
  active_sink_.reset();
  callback_factory_.reset();
  handlers_.clear();
  runtime_.reset();
  initialized_ = false;
  registration_failed_ = false;
  handler_name_bytes_ = 0;
}

bool FixtureContext::HasHandler(const std::string& module,
                                const std::string& method) const {
  return handlers_.find({module, method}) != handlers_.end();
}

void FixtureContext::RegisterCtxObject(const std::string& fixture_directory) {
  auto& runtime = runtime_->runtime();
  Object ctx(runtime);
  ctx.setProperty(
      runtime, "register",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "register"), 3,
          HostFunctionType([this](Runtime& rt, const Value&, const Value* args,
                                  size_t count)
                               -> base::expected<Value, JSINativeException> {
            if (count < 3 || !args[0].isString() || !args[1].isString() ||
                !args[2].isObject()) {
              return Value::undefined();
            }
            auto function = args[2].getObject(rt).asFunction(rt);
            if (!function.has_value()) {
              return Value::undefined();
            }
            auto key = std::make_pair(args[0].getString(rt).utf8(rt),
                                      args[1].getString(rt).utf8(rt));
            const bool is_new = handlers_.find(key) == handlers_.end();
            const size_t key_bytes = key.first.size() + key.second.size();
            if (is_new && (handlers_.size() >= limits_.max_handlers ||
                           key_bytes > limits_.max_handler_name_bytes -
                                           handler_name_bytes_)) {
              registration_failed_ = true;
              return base::unexpected(
                  BUILD_JSI_NATIVE_EXCEPTION("Fixture handler limit exceeded"));
            }
            if (is_new) {
              handler_name_bytes_ += key_bytes;
            }
            handlers_.insert_or_assign(
                std::move(key),
                std::make_shared<Handler>(rt, std::move(*function)));
            return Value::undefined();
          })));
  ctx.setProperty(runtime, "readAsset",
                  CreateFixtureReadAssetFunction(runtime, fixture_directory,
                                                 limits_.max_asset_bytes));
  const char* noop_methods[] = {
      "setThreadStrategy", "updateViewPort",   "setGlobalProps",
      "loadTemplate",      "reloadTemplate",   "after",
      "sendGlobalEvent",   "sendEventAndroid", "sendCustomEvent",
      "sendTouchEvent",    "sharedData",       "dispatch"};
  for (const char* name : noop_methods) {
    ctx.setProperty(runtime, name, CreateNoop(runtime, name));
  }
  runtime.global().setProperty(runtime, "ctx", std::move(ctx));
}

bool FixtureContext::InitializeCallbackFactory() {
  auto& runtime = runtime_->runtime();
  auto emit = Function::createFromHostFunction(
      runtime, PropNameID::forAscii(runtime, "emitCallback"), 4,
      HostFunctionType([this](Runtime& rt, const Value&, const Value* values,
                              size_t count)
                           -> base::expected<Value, JSINativeException> {
        auto sink = active_sink_.lock();
        if (!sink || count != 4 || !values[0].isObject() ||
            !Object::strictEquals(rt, *sink->token, values[0].getObject(rt))) {
          return Value::undefined();
        }
        std::string json = FixtureValueToJson(rt, values[2], "null");
        if (sink->failed || sink->calls.size() >= limits_.max_callbacks ||
            json.size() > limits_.max_result_bytes - sink->bytes) {
          sink->failed = true;
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
              "Fixture callback output limit exceeded"));
        }
        const int64_t delay =
            values[3].isNumber() ? NormalizeDelayMs(values[3].getNumber()) : 0;
        sink->bytes += json.size();
        sink->calls.push_back({static_cast<size_t>(values[1].getNumber()),
                               std::move(json), delay});
        return Value::undefined();
      }));
  // Allocate per-dispatch proxies inside a checked JS call. Creating a JSI host
  // function directly can assert when a persistent heap reaches its limit.
  // The callbacks array is also the dispatch token, so retained proxies expire.
  auto source = std::make_shared<runtime::js::StringBuffer>(R"(
(function(emit) {
  const placeholder = function() {};
  return function(args, positions) {
    const callbacks = [];
    for (let i = 0; i < positions.length; ++i) {
      args[positions[i]] = placeholder;
      callbacks[i] = function(value, delay) { emit(callbacks, i, value, delay); };
    }
    return callbacks;
  };
})
)");
  auto evaluated = runtime.evaluateJavaScript(source, "fixture_callbacks.js");
  if (!evaluated.has_value() || !evaluated->isObject()) {
    return false;
  }
  auto factory = evaluated->getObject(runtime).asFunction(runtime);
  if (!factory.has_value()) {
    return false;
  }
  auto bound = factory->call(runtime, {Value(runtime, emit)});
  if (!bound.has_value() || !bound->isObject()) {
    return false;
  }
  auto function = bound->getObject(runtime).asFunction(runtime);
  if (!function.has_value()) {
    return false;
  }
  callback_factory_ = std::make_unique<Function>(std::move(*function));
  return true;
}

FixtureDispatchResult FixtureContext::Dispatch(
    const std::string& module, const std::string& method,
    const std::string& args_json,
    const std::vector<size_t>& callback_positions) {
  if (!initialized_) {
    return {};
  }
  auto found = handlers_.find({module, method});
  if (found == handlers_.end()) {
    return {};
  }
  ScopedFixtureExecution execution(*runtime_);
  auto& runtime = runtime_->runtime();
  // The running handler may replace itself or register another handler.
  auto handler = found->second;
  registration_failed_ = false;
  auto parsed = Value::createFromJsonUtf8(
      runtime, reinterpret_cast<const uint8_t*>(args_json.data()),
      args_json.size());
  if (!parsed.has_value() || !parsed->isObject()) {
    return {};
  }
  auto args = parsed->getObject(runtime).asArray(runtime);
  if (!args.has_value()) {
    return {};
  }
  const auto argument_count = args->size(runtime);
  if (!argument_count.has_value()) {
    return {};
  }
  for (size_t position : callback_positions) {
    if (position >= *argument_count) {
      return {};
    }
  }
  std::string positions_json = "[";
  for (size_t i = 0; i < callback_positions.size(); ++i) {
    if (i != 0) {
      positions_json += ',';
    }
    positions_json += std::to_string(callback_positions[i]);
  }
  positions_json += ']';
  // Parse through the checked JSI path: native Array creation may assert on
  // OOM.
  auto positions = Value::createFromJsonUtf8(
      runtime, reinterpret_cast<const uint8_t*>(positions_json.data()),
      positions_json.size());
  if (!positions.has_value()) {
    return {};
  }
  auto prepared = callback_factory_->call(
      runtime, {Value(runtime, *args), Value(runtime, *positions)});
  if (!prepared.has_value() || !prepared->isObject()) {
    return {};
  }
  auto callbacks = prepared->getObject(runtime).asArray(runtime);
  if (!callbacks.has_value()) {
    return {};
  }
  auto sink = std::make_shared<CallbackSink>();
  sink->token = std::make_unique<Object>(prepared->getObject(runtime));
  active_sink_ = sink;
  Value invoke_args[] = {Value(runtime, *args), Value(runtime, *callbacks)};
  auto result =
      handler->function.callWithThis(runtime, handler->state, invoke_args, 2);
  if (!result.has_value()) {
    LOGE("FixtureContext handler execution failed");
    return {};
  }
  std::string return_json = FixtureValueToJson(runtime, *result, "");
  if (runtime_->timed_out() || registration_failed_ || sink->failed ||
      return_json.size() > limits_.max_result_bytes - sink->bytes) {
    LOGE("FixtureContext dispatch limit exceeded");
    return {};
  }
  FixtureDispatchResult out;
  out.handled = true;
  out.return_value_json = std::move(return_json);
  out.callbacks = std::move(sink->calls);
  return out;
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
