// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_evaluator.h"

#include <string>
#include <utility>

#include "core/runtime/js/jsi/jsi.h"
#include "core/services/replay/fixture_common.h"
#include "core/services/replay/fixture_js_runtime.h"
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
using runtime::js::Value;

struct FixtureEvaluationState {
  const FixtureEvaluationLimits& limits;
  size_t result_entries = 0;
  size_t result_bytes = 0;
  bool result_limit_exceeded = false;
  bool callback_failed = false;
};

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
      count == 0 ? "{}" : FixtureValueToJson(runtime, args[0], "{}");
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
    std::string json = FixtureValueToJson(runtime, args[2], "{}");
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
                    count == 0
                        ? "{}"
                        : FixtureValueToJson(current_runtime, args[0], "{}");
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
      CreateFixtureReadAssetFunction(runtime, fixture_directory,
                                     state.limits.max_asset_bytes));

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
                      FixtureValueToJson(current_runtime, args[1], "null");
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

}  // namespace

base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory) {
  return EvaluateFixture(fixture_directory, FixtureEvaluationLimits{});
}

base::expected<FixtureEvaluationResult, std::string> EvaluateFixture(
    const std::string& fixture_directory,
    const FixtureEvaluationLimits& limits) {
  const FixtureRuntimeLimits runtime_limits{limits.timeout_ms,
                                            limits.memory_limit_bytes};
  if (!runtime_limits.IsValid() || limits.max_result_entries == 0 ||
      limits.max_result_bytes == 0 || limits.max_script_bytes == 0 ||
      limits.max_asset_bytes == 0) {
    return base::unexpected(
        std::string("Fixture evaluation limits must be positive"));
  }

  std::string source =
      ReadFixtureScript(fixture_directory, limits.max_script_bytes);
  if (source.empty()) {
    return base::unexpected(std::string("fixture.js is missing or empty"));
  }

  FixtureEvaluationResult result;
  FixtureEvaluationState state{limits};
  auto fixture_runtime = CreateQuickJsFixtureRuntime(runtime_limits);
  if (!fixture_runtime) {
    return base::unexpected(
        std::string("Unable to create the fixture runtime"));
  }
  auto& js_runtime = fixture_runtime->runtime();
  Scope scope(js_runtime);
  RegisterFixtureContext(js_runtime, fixture_directory, result, state);

  auto evaluation = fixture_runtime->EvaluateScript(std::move(source));
  if (fixture_runtime->timed_out()) {
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
