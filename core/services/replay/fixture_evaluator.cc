// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_evaluator.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "quickjs/include/quickjs.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace tasm {
namespace replay {
namespace {

using runtime::js::Function;
using runtime::js::HostFunctionType;
using runtime::js::JSINativeException;
using runtime::js::Object;
using runtime::js::PropNameID;
using runtime::js::Runtime;
using runtime::js::Scope;
using runtime::js::StartupData;
using runtime::js::String;
using runtime::js::StringBuffer;
using runtime::js::Value;

struct FixtureEvaluationState {
  const FixtureEvaluationLimits& limits;
  std::chrono::steady_clock::time_point deadline;
  size_t result_entries = 0;
  size_t result_bytes = 0;
  bool timed_out = false;
  bool result_limit_exceeded = false;
  bool callback_failed = false;
};

int InterruptFixtureEvaluation(LEPUSRuntime*, void* opaque) {
  auto* state = static_cast<FixtureEvaluationState*>(opaque);
  if (std::chrono::steady_clock::now() < state->deadline) {
    return 0;
  }
  state->timed_out = true;
  return 1;
}

class ScopedQuickJsEvaluationLimits {
 public:
  ScopedQuickJsEvaluationLimits(LEPUSRuntime* runtime,
                                FixtureEvaluationState& state)
      : runtime_(runtime) {
    LEPUS_SetMemoryLimit(runtime_, state.limits.memory_limit_bytes);
    LEPUS_SetInterruptHandler(runtime_, InterruptFixtureEvaluation, &state);
  }

  ~ScopedQuickJsEvaluationLimits() {
    LEPUS_SetInterruptHandler(runtime_, nullptr, nullptr);
  }

  ScopedQuickJsEvaluationLimits(const ScopedQuickJsEvaluationLimits&) = delete;
  ScopedQuickJsEvaluationLimits& operator=(
      const ScopedQuickJsEvaluationLimits&) = delete;

 private:
  LEPUSRuntime* runtime_;
};

std::string ReadFile(const std::string& path, size_t max_bytes) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream.is_open()) {
    return "";
  }
  stream.seekg(0, std::ios::end);
  const std::streamoff size = stream.tellg();
  if (size <= 0 || static_cast<uint64_t>(size) > max_bytes) {
    return "";
  }
  stream.seekg(0, std::ios::beg);
  std::string content(static_cast<size_t>(size), '\0');
  stream.read(content.data(), static_cast<std::streamsize>(size));
  if (!stream) {
    return "";
  }
  return content;
}

bool IsSafeAssetPath(const std::string& path) {
  // Keep drive- and scheme-like paths invalid even on hosts where ':' is an
  // ordinary filename character.
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

std::string ReadAsset(const std::string& fixture_directory, std::string path,
                      size_t max_bytes) {
  constexpr char kAssetsPrefix[] = "assets/";
  if (path.rfind(kAssetsPrefix, 0) == 0) {
    path.erase(0, sizeof(kAssetsPrefix) - 1);
  }
  if (!IsSafeAssetPath(path)) {
    return "";
  }
  return ReadFile(fixture_directory + "/assets/" + path, max_bytes);
}

std::string ValueToJson(Runtime& runtime, const Value& value,
                        const char* fallback) {
  auto json = value.toJsonString(runtime);
  if (!json.has_value() || !json->isString()) {
    return fallback;
  }
  return json->getString(runtime).utf8(runtime);
}

bool ReserveResult(FixtureEvaluationState& state, size_t bytes) {
  if (state.result_entries >= state.limits.max_result_entries ||
      bytes > state.limits.max_result_bytes - state.result_bytes) {
    state.result_limit_exceeded = true;
    return false;
  }
  ++state.result_entries;
  state.result_bytes += bytes;
  return true;
}

bool AppendAction(FixtureEvaluationResult& result,
                  FixtureEvaluationState& state,
                  const std::string& function_name, Runtime& runtime,
                  const Value* args, size_t count) {
  std::string params_json =
      count == 0 ? "{}" : ValueToJson(runtime, args[0], "{}");
  if (!ReserveResult(state, function_name.size() + params_json.size())) {
    return false;
  }
  result.actions.push_back({function_name, 0, std::move(params_json)});
  return true;
}

Function CreateActionRecorder(Runtime& runtime, const char* function_name,
                              FixtureEvaluationResult& result,
                              FixtureEvaluationState& state) {
  return Function::createFromHostFunction(
      runtime, PropNameID::forAscii(runtime, function_name), 1,
      HostFunctionType(
          [&result, &state, function_name](
              Runtime& current_runtime, const Value&, const Value* args,
              size_t count) -> base::expected<Value, JSINativeException> {
            if (!AppendAction(result, state, function_name, current_runtime,
                              args, count)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "Fixture evaluation output limit exceeded"));
            }
            return Value::undefined();
          }));
}

std::string BuildLoadTemplateParams(Runtime& runtime, const Value* args,
                                    size_t count) {
  rapidjson::Document params(rapidjson::kObjectType);
  auto& allocator = params.GetAllocator();
  std::string url;
  std::string template_asset;
  if (count >= 1 && args[0].isString()) {
    url = args[0].getString(runtime).utf8(runtime);
  }
  if (count >= 2 && args[1].isString()) {
    template_asset = args[1].getString(runtime).utf8(runtime);
  }
  params.AddMember("url", rapidjson::Value(url.c_str(), allocator), allocator);
  params.AddMember("source", rapidjson::Value("", allocator), allocator);
  params.AddMember("templateAsset",
                   rapidjson::Value(template_asset.c_str(), allocator),
                   allocator);

  rapidjson::Document template_data;
  if (count >= 3) {
    std::string json = ValueToJson(runtime, args[2], "{}");
    template_data.Parse(json.c_str());
  }
  if (template_data.HasParseError() || !template_data.IsObject()) {
    template_data.SetObject();
  }
  params.AddMember("templateData", rapidjson::Value(template_data, allocator),
                   allocator);
  params.AddMember("isCSR", true, allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  params.Accept(writer);
  return buffer.GetString();
}

void RegisterFixtureContext(Runtime& runtime,
                            const std::string& fixture_directory,
                            FixtureEvaluationResult& result,
                            FixtureEvaluationState& state) {
  Object context(runtime);

  context.setProperty(
      runtime, "setThreadStrategy",
      CreateActionRecorder(runtime, "setThreadStrategy", result, state));
  context.setProperty(
      runtime, "updateViewPort",
      CreateActionRecorder(runtime, "updateViewPort", result, state));

  context.setProperty(
      runtime, "setGlobalProps",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "setGlobalProps"), 1,
          HostFunctionType(
              [&result, &state](
                  Runtime& current_runtime, const Value&, const Value* args,
                  size_t count) -> base::expected<Value, JSINativeException> {
                std::string value =
                    count == 0 ? "{}"
                               : ValueToJson(current_runtime, args[0], "{}");
                std::string params_json =
                    std::string("{\"global_props\":") + value + "}";
                if (!ReserveResult(state, sizeof("setGlobalProps") - 1 +
                                              params_json.size())) {
                  return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                      "Fixture evaluation output limit exceeded"));
                }
                result.actions.push_back(
                    {"setGlobalProps", 0, std::move(params_json)});
                return Value::undefined();
              })));

  context.setProperty(
      runtime, "loadTemplate",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "loadTemplate"), 3,
          HostFunctionType(
              [&result, &state](
                  Runtime& current_runtime, const Value&, const Value* args,
                  size_t count) -> base::expected<Value, JSINativeException> {
                std::string params_json =
                    BuildLoadTemplateParams(current_runtime, args, count);
                if (!ReserveResult(state, sizeof("loadTemplate") - 1 +
                                              params_json.size())) {
                  return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                      "Fixture evaluation output limit exceeded"));
                }
                result.actions.push_back(
                    {"loadTemplate", 0, std::move(params_json)});
                return Value::undefined();
              })));

  context.setProperty(
      runtime, "after",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "after"), 2,
          HostFunctionType([&result, &state](Runtime& current_runtime,
                                             const Value&, const Value* args,
                                             size_t count)
                               -> base::expected<Value, JSINativeException> {
            if (count < 2) {
              return Value::undefined();
            }
            int64_t delay_ms = 0;
            if (args[0].isNumber()) {
              delay_ms = NormalizeDelayMs(args[0].getNumber());
            }
            if (args[1].isObject()) {
              auto function = args[1]
                                  .getObject(current_runtime)
                                  .asFunction(current_runtime);
              if (function.has_value()) {
                size_t action_count = result.actions.size();
                if (!function->call(current_runtime, nullptr, 0).has_value()) {
                  state.callback_failed = true;
                  return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                      "Fixture after callback evaluation failed"));
                }
                for (size_t i = action_count; i < result.actions.size(); ++i) {
                  result.actions[i].delay_ms = delay_ms;
                }
              }
            }
            return Value::undefined();
          })));

  context.setProperty(
      runtime, "sendGlobalEvent",
      CreateActionRecorder(runtime, "sendGlobalEvent", result, state));
  context.setProperty(
      runtime, "sendEventAndroid",
      CreateActionRecorder(runtime, "sendEventAndroid", result, state));
  context.setProperty(
      runtime, "sendCustomEvent",
      CreateActionRecorder(runtime, "SendCustomEvent", result, state));
  context.setProperty(
      runtime, "sendTouchEvent",
      CreateActionRecorder(runtime, "SendTouchEvent", result, state));
  context.setProperty(
      runtime, "reloadTemplate",
      CreateActionRecorder(runtime, "reloadTemplate", result, state));

  context.setProperty(
      runtime, "readAsset",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "readAsset"), 1,
          HostFunctionType(
              [&fixture_directory, &state](
                  Runtime& current_runtime, const Value&, const Value* args,
                  size_t count) -> base::expected<Value, JSINativeException> {
                if (count < 1 || !args[0].isString()) {
                  return Value::undefined();
                }
                std::string path =
                    args[0].getString(current_runtime).utf8(current_runtime);
                std::string content = ReadAsset(fixture_directory, path,
                                                state.limits.max_asset_bytes);
                if (content.empty()) {
                  return Value::undefined();
                }
                bool is_json =
                    path.size() >= 5 && path.substr(path.size() - 5) == ".json";
                if (is_json) {
                  auto value = Value::createFromJsonUtf8(
                      current_runtime,
                      reinterpret_cast<const uint8_t*>(content.data()),
                      content.size());
                  if (value.has_value()) {
                    return std::move(*value);
                  }
                }
                return Value(String::createFromUtf8(current_runtime, content));
              })));

  context.setProperty(
      runtime, "register",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "register"), 3,
          HostFunctionType([](Runtime&, const Value&, const Value*, size_t)
                               -> base::expected<Value, JSINativeException> {
            return Value::undefined();
          })));

  context.setProperty(
      runtime, "sharedData",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "sharedData"), 2,
          HostFunctionType(
              [&result, &state](
                  Runtime& current_runtime, const Value&, const Value* args,
                  size_t count) -> base::expected<Value, JSINativeException> {
                if (count >= 2 && args[0].isString()) {
                  std::string key =
                      args[0].getString(current_runtime).utf8(current_runtime);
                  std::string value_json =
                      ValueToJson(current_runtime, args[1], "null");
                  if (!ReserveResult(state, key.size() + value_json.size())) {
                    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                        "Fixture evaluation output limit exceeded"));
                  }
                  result.shared_data.push_back(
                      {std::move(key), std::move(value_json)});
                }
                return Value::undefined();
              })));

  context.setProperty(
      runtime, "dispatch",
      Function::createFromHostFunction(
          runtime, PropNameID::forAscii(runtime, "dispatch"), 2,
          HostFunctionType(
              [&result, &state](
                  Runtime& current_runtime, const Value&, const Value* args,
                  size_t count) -> base::expected<Value, JSINativeException> {
                if (count >= 1 && args[0].isString()) {
                  std::string function_name =
                      args[0].getString(current_runtime).utf8(current_runtime);
                  if (!AppendAction(result, state, function_name,
                                    current_runtime,
                                    count >= 2 ? args + 1 : nullptr,
                                    count >= 2 ? 1 : 0)) {
                    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                        "Fixture evaluation output limit exceeded"));
                  }
                }
                return Value::undefined();
              })));

  runtime.global().setProperty(runtime, "ctx", std::move(context));
}

std::string WrapFixtureSource(std::string source) {
  constexpr char kExportDefaultFunction[] = "export default function";
  size_t export_position = source.find(kExportDefaultFunction);
  if (export_position == std::string::npos) {
    return "(function(ctx) {\n" + source + "\n})(ctx);\n";
  }
  source.replace(export_position, std::strlen(kExportDefaultFunction),
                 "var __fixture_main__ = function");
  source += "\n__fixture_main__(ctx);\n";
  return source;
}

}  // namespace

base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory) {
  return EvaluateFixture(fixture_directory, FixtureEvaluationLimits{});
}

base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory,
    const FixtureEvaluationLimits& limits) {
  if (limits.timeout_ms <= 0 || limits.memory_limit_bytes == 0 ||
      limits.max_result_entries == 0 || limits.max_result_bytes == 0 ||
      limits.max_script_bytes == 0 || limits.max_asset_bytes == 0) {
    return base::unexpected(
        std::string("Fixture evaluation limits must be positive"));
  }

  std::string source =
      ReadFile(fixture_directory + "/fixture.js", limits.max_script_bytes);
  if (source.empty()) {
    return base::unexpected(std::string("fixture.js is missing or empty"));
  }

  auto js_runtime = runtime::js::makeQuickJsRuntime();
  if (!js_runtime) {
    return base::unexpected(
        std::string("Unable to create the fixture runtime"));
  }
  StartupData startup_data{};
  auto vm = js_runtime->createVM(&startup_data);
  auto js_context = js_runtime->createContext(vm);
  js_runtime->InitRuntime(js_context);
  js_runtime->SetEnableJsBindingApiThrowException(true);

  FixtureEvaluationResult result;
  FixtureEvaluationState state{limits,
                               std::chrono::steady_clock::time_point::max()};
  auto* quickjs_runtime =
      static_cast<runtime::js::QuickjsRuntime*>(js_runtime.get());
  Scope scope(*js_runtime);
  RegisterFixtureContext(*js_runtime, fixture_directory, result, state);

  auto buffer =
      std::make_shared<StringBuffer>(WrapFixtureSource(std::move(source)));
  state.deadline = std::chrono::steady_clock::now() +
                   std::chrono::milliseconds(limits.timeout_ms);
  ScopedQuickJsEvaluationLimits scoped_limits(quickjs_runtime->getJSRuntime(),
                                              state);
  auto evaluation = js_runtime->evaluateJavaScript(buffer, "fixture.js");
  if (state.timed_out) {
    return base::unexpected(std::string("fixture.js evaluation timed out"));
  }
  if (state.result_limit_exceeded) {
    return base::unexpected(
        std::string("fixture.js evaluation output limit exceeded"));
  }
  if (state.callback_failed || !evaluation.has_value()) {
    return base::unexpected(std::string("fixture.js evaluation failed"));
  }
  return result;
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
