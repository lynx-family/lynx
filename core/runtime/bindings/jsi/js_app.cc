// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/js_app.h"

#include <time.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "base/include/debug/lynx_error.h"
#include "base/include/expected.h"
#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/to_underlying.h"
#include "base/include/value/base_string.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/data/lynx_view_data_manager.h"
#include "core/renderer/dom/vdom/radon/node_select_options.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/jsi/api_call_back.h"
#include "core/runtime/bindings/jsi/console.h"
#include "core/runtime/bindings/jsi/js_object_destruction_observer.h"
#include "core/runtime/bindings/jsi/lynx.h"
#include "core/runtime/bindings/jsi/lynx_js_error.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/piper/js/lynx_api_handler.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"
#include "core/services/timing_handler/timing_utils.h"
#include "core/value_wrapper/value_impl_piper.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/testbench_base_recorder.h"
#endif

namespace lynx {
namespace piper {

namespace {

// TODO(douyanlin): corejs uses this map
inline std::unordered_map<std::string, JsContent>& GetJSAssetsMap() {
  static base::NoDestructor<std::unordered_map<std::string, JsContent>>
      js_assets_map_;
  return *js_assets_map_;
}

// default resource loader timeout is 5 seconds.
constexpr long DEFAULT_RESOURCE_TIMEOUT = 5;

}  // namespace

#if ENABLE_TRACE_PERFETTO
static void HandleProfileNameAndOption(const piper::Value* args, size_t count,
                                       Runtime& rt,
                                       std::weak_ptr<App> native_app,
                                       lynx::perfetto::EventContext& ctx) {
  // arg0: trace_name
  if (count < 1 || !args[0].isString()) {
    return;
  }
  std::string trace_name = args[0].getString(rt).utf8(rt);
  ctx.event()->set_name(trace_name);

  // args1: TraceOption
  if (count > 1) {
    auto ptr = native_app.lock();
    if (!ptr || ptr->IsDestroying()) {
      return;
    }
    auto option =
        ptr->ParseJSValueToLepusValue(std::move(args[1]), PAGE_GROUP_ID);
    if (!option || !option->IsObject()) {
      return;
    }
    auto trace_args = option->GetProperty(BASE_STATIC_STRING(runtime::kArgs));
    if (trace_args.IsObject()) {
      tasm::ForEachLepusValue(trace_args, [&ctx](const lepus::Value& key,
                                                 const lepus::Value& value) {
        if (key.IsString() && value.IsString()) {
          ctx.event()->add_debug_annotations(key.StdString(),
                                             value.StdString());
        }
      });
    }
    ctx.event()->add_debug_annotations("runtime_id",
                                       std::to_string(rt.getRuntimeId()));
    auto flow_id = option->GetProperty(BASE_STATIC_STRING(runtime::kFlowId));
    if (flow_id.IsNumber()) {
      ctx.event()->add_flow_ids(flow_id.Number());
    }
  }
}
#endif

Value AppProxy::get(Runtime* rt, const PropNameID& name) {
  auto methodName = name.utf8(*rt);
  if (methodName == "id") {
    auto native_app = native_app_.lock();
    if (!native_app || native_app->IsDestroying()) {
      return piper::Value::undefined();
    }
    auto guid = piper::String::createFromUtf8(*rt, native_app->getAppGUID());
    return piper::Value(*rt, guid);
  } else if (methodName == "__pageUrl") {
    auto native_app = native_app_.lock();
    if (!native_app) {
      return piper::Value::undefined();
    }
    return piper::Value(
        *rt, piper::String::createFromUtf8(*rt, native_app->GetPageUrl()));
  } else if (methodName == "loadScript") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "loadScript"), 1,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          std::shared_ptr<Runtime> js_runtime = rt_.lock();
          if (!js_runtime) {
            return Value::undefined();
          }

          if (count < 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("loadScript arg count must > 0"));
          }

          auto sourceURL = args[0].asString(rt);
          if (!sourceURL) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "loadScript args[0] must be a string."));
          }
          std::string entryName = tasm::DEFAULT_ENTRY_NAME;
          if (count > 1 && args[1].isString()) {
            entryName = args[1].getString(rt).utf8(rt);
          }

          long timeout = DEFAULT_RESOURCE_TIMEOUT;
          if (count > 2 && args[2].isNumber()) {
            timeout = static_cast<long>(args[2].getNumber());
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }
          return native_app->loadScript(entryName, sourceURL->utf8(rt),
                                        timeout);
        });
  } else if (methodName == "readScript") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "readScript"), 1,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          std::shared_ptr<Runtime> js_runtime = rt_.lock();
          if (!js_runtime) {
            return Value::undefined();
          }

          if (count < 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("readScript arg count must > 0"));
          }

          auto sourceURL = args[0].asString(rt);
          if (!sourceURL) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "readScript args[0] must be a string."));
          }
          std::string entry_name = tasm::DEFAULT_ENTRY_NAME;
          long timeout = DEFAULT_RESOURCE_TIMEOUT;
          if (count > 1 && args[1].isObject()) {
            auto entry_opt =
                args[1].getObject(rt).getProperty(rt, "dynamicComponentEntry");
            if (!entry_opt) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "readScript args[1] must have "
                  "dynamicComponentEntry property."));
            }
            auto entry_name_opt = entry_opt->asString(rt);
            if (!entry_name_opt) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "readScript dynamicComponentEntry must be a string."));
            }
            entry_name = entry_name_opt->utf8(rt);
            auto timeout_opt = args[1].getObject(rt).getProperty(rt, "timeout");
            if (timeout_opt && timeout_opt->isNumber()) {
              timeout = static_cast<long>(timeout_opt->getNumber());
            }
          }
          if (count > 2 && args[2].isObject()) {
            auto timeout = args[2].getObject(rt).getProperty(rt, "timeout");
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }
          return native_app->readScript(entry_name, sourceURL->utf8(rt),
                                        timeout);
        });
  } else if (methodName == "readDynamicComponentScripts") {
    // deprecated
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "readDynamicComponentScripts"), 1,
        [](Runtime& rt, const Value& thisVal, const Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          return piper::Value::undefined();
        });
  } else if (methodName == "updateData") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "updateData"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "updateData");
          int32_t instance_id = static_cast<int32_t>(rt.getRuntimeId());
          tasm::timing::LongTaskMonitor::Scope long_task_scope(
              instance_id, tasm::timing::kUpdateDataByJSTask,
              tasm::timing::kTaskNameJSAppUpdateData);
          if (count < 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("updateData arg count must be 1"));
          }

          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          auto lepus_value_opt =
              ptr->ParseJSValueToLepusValue(std::move(args[0]), PAGE_GROUP_ID);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in updateData"));
          }
          if (!lepus_value_opt->IsObject()) {
            return piper::Value::undefined();
          }

          runtime::UpdateDataType update_data_type;
          if (count >= 2 && args[1].isNumber()) {
            // updateDataType here is an optional argument
            // updateData(data, updateDataType);
            update_data_type = runtime::UpdateDataType(args[1].getNumber());
          }

          ApiCallBack callback;
          if (count >= 2 && args[1].isObject() &&
              args[1].getObject(rt).isFunction(rt)) {
            // mini-app do not has updateDataType, keep behavior of 2.4
            // callback here are optional arguments
            // updateData(data, callback);
            callback =
                ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
          }
          if (count >= 3 && args[2].isObject() &&
              args[2].getObject(rt).isFunction(rt)) {
            // updateDataType and callback here are optional arguments
            // updateData(data, updateDataType, callback);
            callback =
                ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "UpdateDataToTASM",
                      [&](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            "CallbackID", std::to_string(callback.id()));
                      });
          ptr->appDataChange(std::move(*lepus_value_opt), callback,
                             std::move(update_data_type));
          return piper::Value::undefined();
        });
  } else if (methodName == "batchedUpdateData") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "batchedUpdateData"), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          int32_t instance_id = static_cast<int32_t>(rt.getRuntimeId());
          tasm::timing::LongTaskMonitor::Scope long_task_scope(
              instance_id, tasm::timing::kUpdateDataByJSTask,
              tasm::timing::kTaskNameJSAppBatchedUpdateData);
          if (count < 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "batchedUpdateData arg count must be 1"));
          }

          auto native_app = native_app_.lock();
          if (native_app && !native_app->IsDestroying()) {
            auto opt_jsi_native_exception =
                native_app->batchedUpdateData(args[0]);
            if (opt_jsi_native_exception) {
              ADD_STACK((*opt_jsi_native_exception));
              return base::unexpected(std::move(*opt_jsi_native_exception));
            }
          }
          return piper::Value::undefined();
        });
  } else if (methodName == "setCard") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "setCard"), 1,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX PageProxy get -> setCard");
          if (count != 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("setCard arg count must be 1"));
          }

          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }

          if (args[0].isObject()) {
            native_app->setJsAppObj(args[0].getObject(rt));
          }

          return piper::Value::undefined();
        });
  } else if (methodName == "setTimeout") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "setTimeout"), 1,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGV("LYNX App get -> setTimeout");

          if (count < 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("setTimeout args count must >= 1"));
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (args[0].isObject()) {
            int interval =
                (count >= 2 && args[1].isNumber())
                    ? std::max(static_cast<int>(args[1].getNumber()), 0)
                    : 0;

            auto callback = args[0].getObject(rt).asFunction(rt);
            if (!callback) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "setTimeout args[0] isn't a function."));
            }
            return native_app->setTimeout(std::move(*callback), interval);
          } else {
            return piper::Value::undefined();
          }
        });
  } else if (methodName == "setInterval") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "setInterval"), 2,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGV("LYNX App get -> setInterval");
          if (count != 2) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("setInterval arg count must be 2"));
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (args[0].isObject() && args[1].isNumber()) {
            auto callback = args[0].getObject(rt).asFunction(rt);
            if (!callback) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "setInterval args[0] isn't a function."));
            }
            int interval = std::max(static_cast<int>(args[1].getNumber()), 0);
            return native_app->setInterval(std::move(*callback), interval);
          } else {
            return piper::Value::undefined();
          }
        });
  } else if (methodName == "clearTimeout") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "clearTimeout"), 1,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGV("LYNX App get -> clearTimeout");
          if (count != 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("clearTimeout arg count must be 1"));
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }

          if (args[0].isNumber()) {
            native_app->clearTimeout(args[0].getNumber());
          }

          return piper::Value::undefined();
        });
  } else if (methodName == "clearInterval") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "clearInterval"), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGV("LYNX App get -> clearInterval");
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "clearInterval arg count must be 1"));
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }

          // also use clearTimeout
          if (args[0].isNumber()) {
            native_app->clearTimeout(args[0].getNumber());
          }

          return piper::Value::undefined();
        });
  } else if (methodName == "nativeModuleProxy") {
    auto native_app = native_app_.lock();
    if (!native_app || native_app->IsDestroying()) {
      return piper::Value::undefined();
    }

    return native_app->nativeModuleProxy();
  } else if (methodName == "reportException") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "reportException"), 3,
        [this](Runtime& rt, const Value& thisVal, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          std::shared_ptr<Runtime> js_runtime = rt_.lock();
          if (!js_runtime) {
            return Value::undefined();
          }

          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "the arg count of reportException must be equal to 2"));
          }
          if (!args[0].isObject() || !args[1].isObject()) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("the args type is error."));
          }
          auto error_obj = args[0].asObject(rt);
          common::JSErrorInfo error_info;
          auto value = error_obj->getProperty(rt, "message");
          if (!value || !value->isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "the first args must has string type `message`."));
          }
          error_info.message = value->asString(rt)->utf8(rt);
          value = error_obj->getProperty(rt, "name");
          if (value && value->isString()) {
            error_info.name = value->asString(rt)->utf8(rt);
          }
          value = error_obj->getProperty(rt, "kind");
          if (value && value->isString()) {
            error_info.kind = value->asString(rt)->utf8(rt);
          }
          value = error_obj->getProperty(rt, "stack");
          if (value && value->isString()) {
            error_info.stack = value->asString(rt)->utf8(rt);
          }
          value = error_obj->getProperty(rt, "cause");
          if (value && value->isString()) {
            error_info.cause = value->asString(rt)->utf8(rt);
          }

          auto options_obj = args[1].asObject(rt);
          value = options_obj->getProperty(rt, "filename");
          if (value && value->isString()) {
            error_info.file_name = value->asString(rt)->utf8(rt);
          }
          value = options_obj->getProperty(rt, "release");
          if (value && value->isString()) {
            error_info.release = value->asString(rt)->utf8(rt);
          }
          value = options_obj->getProperty(rt, "buildVersion");
          if (value && value->isString()) {
            error_info.build_version = value->asString(rt)->utf8(rt);
          }
          value = options_obj->getProperty(rt, "versionCode");
          if (value && value->isString()) {
            error_info.version_code = value->asString(rt)->utf8(rt);
          }
          value = options_obj->getProperty(rt, "errorCode");
          if (value && value->isNumber()) {
            error_info.error_code =
                static_cast<int32_t>(std::round(value->getNumber()));
          }
          value = options_obj->getProperty(rt, "errorLevel");
          if (value && value->isNumber()) {
            int num = static_cast<int>(value->getNumber());
            if (num > static_cast<int>(base::LynxErrorLevel::Warn) ||
                num < static_cast<int>(base::LynxErrorLevel::Fatal)) {
              num = static_cast<int>(base::LynxErrorLevel::Error);
            }
            error_info.error_level = static_cast<base::LynxErrorLevel>(num);
          }

          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            LOGE("js_app reportException when native_app is destroying: "
                 << error_info.message);
            return piper::Value::undefined();
          }
          native_app->ReportException(std::move(error_info));
          return piper::Value::undefined();
        });
  } else if (methodName == "updateComponentData") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "updateComponentData"), 4,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 3) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "updateComponentData arg count must >= 3"));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "updateComponentData");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          int32_t instance_id = static_cast<int32_t>(rt.getRuntimeId());
          tasm::timing::LongTaskMonitor::Scope long_task_scope(
              instance_id, tasm::timing::kUpdateDataByJSTask,
              tasm::timing::kTaskNameJSAppUpdateComponentData);
          std::string id;
          if (args[0].isString()) {
            id = args[0].getString(rt).utf8(rt);
          }
          auto lepus_value_opt = ptr->ParseJSValueToLepusValue(args[1], id);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in updateComponentData"));
          }
          if (lepus_value_opt->IsObject()) {
            ApiCallBack callback;
            if (args[2].isObject() && args[2].getObject(rt).isFunction(rt)) {
              // callback here is a required argument
              // updateComponentData(id, data, callback);
              callback =
                  ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));
            }

            runtime::UpdateDataType update_data_type;
            if (count >= 4) {
              // there are two ways to call updateComponentData
              // - updateComponentData(id, data, callback);
              // - updateComponentData(id, data, callback, updateDataType);
              if (args[3].isNumber()) {
                // updateDataType here is a optional argument
                update_data_type = runtime::UpdateDataType(args[3].getNumber());
              }
            }

            TRACE_EVENT(
                LYNX_TRACE_CATEGORY, "updateComponentDataToTASM",
                [&](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_debug_annotations(
                      "CallbackID", std::to_string(callback.id()));
                  ctx.event()->add_debug_annotations(
                      "update_data_type",
                      std::to_string(static_cast<uint32_t>(update_data_type)));
                });
            ptr->updateComponentData(id, std::move(*lepus_value_opt), callback,
                                     std::move(update_data_type));
          }

          return piper::Value::undefined();
        });
  } else if (methodName == "triggerLepusGlobalEvent") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "triggerLepusGlobalEvent"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX PageProxy get -> triggerLepusGlobalEvent" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "triggerLepusGlobalEvent arg count must be 2"));
          }
          std::string event;
          if (args[0].isString()) {
            event = args[0].getString(rt).utf8(rt);
          }
          if (!args[1].isObject()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "triggerLepusGlobalEvent arg error"));
          }
          auto ptr = native_app_.lock();
          if (ptr && !ptr->IsDestroying()) {
            auto lepus_value_opt = ptr->ParseJSValueToLepusValue(
                std::move(args[1]), PAGE_GROUP_ID);
            if (!lepus_value_opt) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ParseJSValueToLepusValue error in triggerLepusGlobalEvent"));
            }
            ptr->triggerLepusGlobalEvent(event, std::move(*lepus_value_opt));
          }
          return piper::Value::undefined();
        });
  } else if (methodName == "triggerComponentEvent") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "triggerComponentEvent"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX PageProxy get -> triggerComponentEvent" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "triggerComponentEvent arg count must be 2"));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "triggerComponentEvent");
          std::string id;
          if (args[0].isString()) {
            id = args[0].getString(rt).utf8(rt);
          }

          auto ptr = native_app_.lock();
          if (ptr && !ptr->IsDestroying()) {
            auto lepus_value_opt = ptr->ParseJSValueToLepusValue(
                std::move(args[1]), PAGE_GROUP_ID);
            if (!lepus_value_opt) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ParseJSValueToLepusValue error in triggerComponentEvent"));
            }
            TRACE_EVENT(LYNX_TRACE_CATEGORY, "triggerComponentEventToTASM");
            ptr->triggerComponentEvent(id, std::move(*lepus_value_opt));
          }

          return piper::Value::undefined();
        });
  } else if (methodName == "selectComponent") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "selectComponent"), 0,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> selectComponent");
          if (count < 4) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "selectComponent args count must be 4"));
          }

          TRACE_EVENT(LYNX_TRACE_CATEGORY, "selectComponent");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string comp_id;
          if (args[0].isString()) {
            comp_id = args[0].getString(rt).utf8(rt);
          }

          std::string id_selector;
          if (args[1].isString()) {
            id_selector = args[1].getString(rt).utf8(rt);
          }

          bool single = true;
          if (args[2].isBool()) {
            single = args[2].getBool();
          }

          ApiCallBack callback;
          if (count >= 4 && args[3].isObject() &&
              args[3].getObject(rt).isFunction(rt)) {
            callback =
                ptr->CreateCallBack(args[3].getObject(rt).getFunction(rt));
          }
          ptr->selectComponent(comp_id, id_selector, single, callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "loadScriptAsync") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "loadScriptAsync"), 2,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> loadScriptAsync" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "loadScriptAsync arg count must be 2"));
          }

          auto ptr = native_app_.lock();
          // not allow invoke when destroy lifecycle
          if (ptr == nullptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string url;
          if (args[0].isString()) {
            url = args[0].getString(rt).utf8(rt);
          }

          ApiCallBack callback;
          if (args[1].isObject() && args[1].getObject(rt).isFunction(rt)) {
            callback =
                ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
          }

          ptr->LoadScriptAsync(url, callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "onPiperInvoked") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "onPiperInvoked"), 1,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> onPiperInvoked" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "onPiperInvoked arg count must be 2"));
          }

          auto ptr = native_app_.lock();
          // not allow invoke when destroy lifecycle
          if (ptr == nullptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          std::string module_name, method_name;
          if (args[0].isString()) {
            module_name = args[0].getString(rt).utf8(rt);
          }
          if (args[1].isString()) {
            method_name = args[1].getString(rt).utf8(rt);
          }

          ptr->onPiperInvoked(module_name, method_name);
          return piper::Value::undefined();
        });
  } else if (methodName == "getPathInfo") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getPathInfo"), 5,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> getPathInfo");
          if (count < 5) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("getPathInfo args count must be 5"));
          }

          TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::getPathInfo");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (!(args[0].isNumber() && args[1].isString() &&
                args[2].isString() && args[3].isBool() && args[4].isObject() &&
                args[4].getObject(rt).isFunction(rt))) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("getPathInfo args type is error"));
          }
          tasm::NodeSelectOptions::IdentifierType identifier_type =
              static_cast<tasm::NodeSelectOptions::IdentifierType>(
                  args[0].getNumber());
          std::string identifier = args[1].getString(rt).utf8(rt);
          std::string component_id = args[2].getString(rt).utf8(rt);
          bool first_only = args[3].getBool();
          ApiCallBack callback =
              ptr->CreateCallBack(args[4].getObject(rt).getFunction(rt));
          auto info = tasm::NodeSelectOptions(identifier_type, identifier);
          info.component_only = false;
          info.only_current_component = true;
          info.first_only = first_only;
          tasm::NodeSelectRoot root =
              count >= 6 && args[5].isNumber()
                  ? tasm::NodeSelectRoot::ByUniqueId(args[5].getNumber())
                  : tasm::NodeSelectRoot::ByComponentId(component_id);
          ptr->GetPathInfo(std::move(root), std::move(info), callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "getEnv") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getEnv"), 1,
        [this](
            Runtime& rt, const piper::Value& this_val, const piper::Value* args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          if (count < 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("getEnv args count must be 1"));
          }

          TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::getEnv");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return Value::undefined();
          }

          auto key = static_cast<tasm::LynxEnv::Key>(args[0].getNumber());

          if (key >= tasm::LynxEnv::Key::END_MARK) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "unknown env key " + std::to_string(base::to_underlying(key))));
          }

          std::optional<std::string> env =
              tasm::LynxEnv::GetInstance().GetStringEnv(key);

          if (env) {
            return String::createFromUtf8(rt, *env);
          }
          return Value::undefined();
        });
  } else if (methodName == "invokeUIMethod") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "invokeUIMethod"), 6,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 6) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "invokeUIMethod args count must be 6"));
          }

          TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::invokeUIMethod");

          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          tasm::NodeSelectOptions::IdentifierType identifier_type =
              static_cast<tasm::NodeSelectOptions::IdentifierType>(
                  args[0].getNumber());
          std::string identifier = args[1].getString(rt).utf8(rt);
          std::string component_id = args[2].getString(rt).utf8(rt);
          std::string method = args[3].getString(rt).utf8(rt);
          const piper::Value* params = &args[4];
          ApiCallBack callback =
              ptr->CreateCallBack(args[5].getObject(rt).getFunction(rt));
          auto options = tasm::NodeSelectOptions(identifier_type, identifier);
          options.component_only = false;
          options.only_current_component = true;
          options.first_only = true;
          tasm::NodeSelectRoot root =
              count >= 7 && args[6].isNumber()
                  ? tasm::NodeSelectRoot::ByUniqueId(args[6].getNumber())
                  : tasm::NodeSelectRoot::ByComponentId(component_id);
          ptr->InvokeUIMethod(std::move(root), std::move(options),
                              std::move(method), params, callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "getFields") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getFields"), 6,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> getFields");
          if (count < 6) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("getFields args count must be 6"));
          }

          TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::getFields");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          tasm::NodeSelectOptions::IdentifierType identifier_type =
              static_cast<tasm::NodeSelectOptions::IdentifierType>(
                  args[0].getNumber());
          std::string identifier = args[1].getString(rt).utf8(rt);
          std::string component_id = args[2].getString(rt).utf8(rt);
          bool first_only = args[3].getBool();
          piper::Array fields = args[4].getObject(rt).getArray(rt);
          std::vector<std::string> fields_native;
          for (size_t i = 0; i < fields.length(rt); i++) {
            fields_native.push_back(
                fields.getValueAtIndex(rt, i)->getString(rt).utf8(rt));
          }

          ApiCallBack callback =
              ptr->CreateCallBack(args[5].getObject(rt).getFunction(rt));
          auto info = tasm::NodeSelectOptions(identifier_type, identifier);
          info.component_only = false;
          info.only_current_component = true;
          info.first_only = first_only;
          tasm::NodeSelectRoot root =
              count >= 7 && args[6].isNumber()
                  ? tasm::NodeSelectRoot::ByUniqueId(args[6].getNumber())
                  : tasm::NodeSelectRoot::ByComponentId(component_id);
          ptr->GetFields(std::move(root), std::move(info),
                         std::move(fields_native), callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "setNativeProps") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "setNativeProps"), 5,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (count < 5) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.setNativeProps args count must be 5"));
          }
          if (!(args[0].isNumber() && args[1].isString() &&
                args[2].isString() && args[3].isBool())) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "setNativeProps args type is error"));
          }
          tasm::NodeSelectOptions::IdentifierType identifier_type =
              static_cast<tasm::NodeSelectOptions::IdentifierType>(
                  args[0].getNumber());
          std::string identifier = args[1].getString(rt).utf8(rt);
          std::string component_id = args[2].getString(rt).utf8(rt);
          bool first_only = args[3].getBool();
          auto info = tasm::NodeSelectOptions(identifier_type, identifier);
          info.component_only = false;
          info.only_current_component = true;
          info.first_only = first_only;
          auto lepus_value_opt =
              ptr->ParseJSValueToLepusValue(std::move(args[4]), PAGE_GROUP_ID);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in setNativeProps"));
          }
          tasm::NodeSelectRoot root =
              count >= 6 && args[5].isNumber()
                  ? tasm::NodeSelectRoot::ByUniqueId(args[5].getNumber())
                  : tasm::NodeSelectRoot::ByComponentId(component_id);
          ptr->SetNativeProps(std::move(root), std::move(info),
                              std::move(*lepus_value_opt));
          return piper::Value::undefined();
        });
  } else if (methodName == "getSessionStorageItem") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getSessionStorageItem"), 2,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "getSessionStorageItem args count must be 2."));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (!args[0].isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "getSessionStorageItem's first param must be string!."));
          }
          auto key_opt = ptr->ParseJSValueToLepusValue(args[0], PAGE_GROUP_ID);
          if (!key_opt || !key_opt->IsString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in getSessionStorageItem."));
          }

          ApiCallBack callback;
          if (args[1].isObject() && args[1].getObject(rt).isFunction(rt)) {
            callback =
                ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
          }
          ptr->GetSessionStorageItem(key_opt->String(), callback);
          return piper::Value::undefined();
        });
  } else if (methodName == "subscribeSessionStorage") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "subscribeSessionStorage"), 3,
        [this](Runtime& rt, const piper::Value& this_val,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 3) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "subscribeSessionStorage's args count must be 3."));
          }

          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (!args[0].isString()) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("subscribeSessionStorage's first "
                                           "param must be string!."));
          }

          auto key_opt = ptr->ParseJSValueToLepusValue(args[0], PAGE_GROUP_ID);
          if (!key_opt || !key_opt->IsString()) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("ParseJSValueToLepusValue error in "
                                           "subscribeSessionStorage."));
          }

          if (!args[1].isNumber()) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("subscribeSessionStorage's second "
                                           "param must be number!"));
          }

          auto listener_id_opt =
              ptr->ParseJSValueToLepusValue(args[1], PAGE_GROUP_ID);
          if (!listener_id_opt || !listener_id_opt->IsNumber()) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("ParseJSValueToLepusValue error in "
                                           "subscribeSessionStorage."));
          }

          ApiCallBack callback;
          if (args[2].isObject() && args[2].getObject(rt).isFunction(rt)) {
            callback =
                ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));
          }
          ptr->SubscribeSessionStorage(key_opt->String(),
                                       listener_id_opt->Number(), callback);

          return piper::Value::undefined();
        });
  } else if (methodName == "callLepusMethod") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "callLepusMethod"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size >= 2
          // [0] method name -> String
          // [1] args -> Object
          // [2] optional JS callback -> Function
          // [3] optional group_id -> String
          LOGI("LYNX PageProxy get -> callLepusMethod" << this);
          if (count < 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "callLepusMethod arg count must >= 2"));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "callLepusMethod");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          int32_t instance_id = static_cast<int32_t>(rt.getRuntimeId());
          tasm::timing::LongTaskMonitor::Scope long_task_scope(
              instance_id, tasm::timing::kJSFuncTask,
              tasm::timing::kTaskNameJSAppCallLepusMethod);

          std::string method_name;
          if (args[0].isString()) {
            method_name = args[0].getString(rt).utf8(rt);
          }
          tasm::timing::LongTaskTiming* timing =
              tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
          if (timing != nullptr) {
            timing->task_info_ = method_name;
          }

          // PAGE_GROUP_ID (-1) is the root component. If you want to avoid
          // overlapping with the function of the root component, use a negative
          // number other than -1, such as -2
          std::string group_id = PAGE_GROUP_ID;
          if (count >= 4 && args[3].isString()) {
            group_id = args[3].getString(rt).utf8(rt);
          }

          std::string stacks;
          if (count >= 5 && args[4].isString()) {
            stacks = args[4].getString(rt).utf8(rt);
          }

          auto lepus_value_opt =
              ptr->ParseJSValueToLepusValue(args[1], group_id);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in callLepusMethod"));
          }
          if (lepus_value_opt->IsObject()) {
            ApiCallBack callback;
            if (count >= 3 && args[2].isObject() &&
                args[2].getObject(rt).isFunction(rt)) {
              // callback here is a optional argument
              callback =
                  ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));
            }

            ptr->CallLepusMethod(method_name, std::move(*lepus_value_opt),
                                 std::move(callback), std::move(stacks));
          }
          return piper::Value::undefined();
        });
  } else if (methodName == "generatePipelineOptions") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "GeneratePipelineOptions"), 0,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size == 0
          LOGI("LYNX PageProxy get -> GeneratePipelineOptions" << this);
          if (count != 0) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "GeneratePipelineOptions arg count must == 0"));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "generatePipelineOptions");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          tasm::PipelineOptions options;
          piper::Object options_obj = piper::Object(rt);
          options_obj.setProperty(rt, tasm::kPipelineID, options.pipeline_id);
          options_obj.setProperty(rt, tasm::kPipelineNeedTimestamps,
                                  options.need_timestamps);
          return options_obj;
        });
  } else if (methodName == "onPipelineStart") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "OnPipelineStart"), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size == 1 or == 2.
          // [0] pipeline id -> String
          // [1] pipeline origin -> String
          LOGI("LYNX PageProxy get -> OnPipelineStart" << this);
          if (count < 1 || count > 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "OnPipelineStart arg count must == 1 or == 2"));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string pipeline_id;
          if (args[0].isString()) {
            pipeline_id = args[0].getString(rt).utf8(rt);
          }
          std::string pipeline_origin;
          if (count > 1 && args[1].isString()) {
            pipeline_origin = args[1].getString(rt).utf8(rt);
          }

          ptr->OnPipelineStart(pipeline_id, pipeline_origin);
          return piper::Value::undefined();
        });
  } else if (methodName == "markPipelineTiming") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "MarkPipelineTiming"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size == 2
          // [0] pipeline id -> String
          // [1] key -> String
          LOGI("LYNX PageProxy get -> MarkPipelineTiming" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "MarkPipelineTiming arg count must == 2"));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string pipeline_id;
          if (args[0].isString()) {
            pipeline_id = args[0].getString(rt).utf8(rt);
          }

          std::string key;
          if (args[1].isString()) {
            key = args[1].getString(rt).utf8(rt);
          }
          ptr->MarkPipelineTiming(pipeline_id, key);
          return piper::Value::undefined();
        });
  } else if (methodName == "bindPipelineIdWithTimingFlag") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "BindPipelineIdWithTimingFlag"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size == 2
          // [0] pipeline id -> String
          // [1] timing_flag -> String
          LOGI("LYNX PageProxy get -> BindPipelineIdWithTimingFlag" << this);
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "BindPipelineIdWithTimingFlag arg count must == 2"));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string pipeline_id;
          if (args[0].isString()) {
            pipeline_id = args[0].getString(rt).utf8(rt);
          }

          std::string timing_flag;
          if (args[1].isString()) {
            timing_flag = args[1].getString(rt).utf8(rt);
          }
          ptr->BindPipelineIDWithTimingFlag(pipeline_id, timing_flag);
          return piper::Value::undefined();
        });
  } else if (methodName == "markTiming") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "MarkTiming"), 2,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size == 2
          // [0] Timing flag -> String
          // [1] key -> String
          LOGI("LYNX PageProxy get -> MarkTiming" << this);
          if (count != 2) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("MarkTiming arg count must == 2"));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          std::string timing_flag;
          if (args[0].isString()) {
            timing_flag = args[0].getString(rt).utf8(rt);
          }

          std::string key;
          if (args[1].isString()) {
            key = args[1].getString(rt).utf8(rt);
          }
          ptr->MarkTimingWithTimingFlag(timing_flag, key);
          return piper::Value::undefined();
        });
  } else if (methodName == "triggerWorkletFunction") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "triggerWorkletFunction"), 5,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size >= 3
          // [0] component_id -> string
          // [1] worklet_module_name -> string
          // [2] method_name -> string
          // [3] args -> Object
          // [4] optional JS callback -> Function

          if (count < 3) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "triggerWorkletFunction arg count must >= 3"));
          }
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "triggerWorkletFunction");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }

          if (!(args[0].isString() && args[1].isString() &&
                args[2].isString())) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "triggerWorkletFunction args error"));
          }

          std::string component_id = args[0].getString(rt).utf8(rt);
          std::string worklet_module_name = args[1].getString(rt).utf8(rt);
          std::string method_name = args[2].getString(rt).utf8(rt);

          std::optional<lepus::Value> lepus_value_opt = lepus::Value();
          ApiCallBack callback;

          if (count > 3) {
            lepus_value_opt =
                ptr->ParseJSValueToLepusValue(args[3], component_id);
            if (!lepus_value_opt) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ParseJSValueToLepusValue error in triggerWorkletFunction"));
            }
          }

          if (count > 4 && args[4].isObject() &&
              args[4].getObject(rt).isFunction(rt)) {
            // callback here is a optional argument
            callback =
                ptr->CreateCallBack(args[4].getObject(rt).getFunction(rt));
          }

          ptr->triggerWorkletFunction(
              std::move(component_id), std::move(worklet_module_name),
              std::move(method_name), std::move(*lepus_value_opt),
              std::move(callback));

          return piper::Value::undefined();
        });
  } else if (methodName == "featureCount") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "featureCount"), 1,
        [this](
            Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          // parameter size == 1
          // [0] feature -> LynxFeature
          LOGI("LYNX PageProxy get -> featureCount" << this);
          if (count != 1) {
            return base::unexpected(
                BUILD_JSI_NATIVE_EXCEPTION("featureCount arg count must == 1"));
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (args[0].isNumber()) {
            tasm::report::LynxFeature feature =
                static_cast<tasm::report::LynxFeature>(args[0].getNumber());
            tasm::report::FeatureCounter::Instance()->Count(feature);
          }
          return piper::Value::undefined();
        });
  } else if (methodName == "createJSObjectDestructionObserver") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "createJSObjectDestructionObserver"), 1,
        [this](
            Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          // parameter size == 1
          // [0]: () => {}
          LOGI("LYNX PageProxy get -> createJSObjectDestructionObserver"
               << this);
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "createJSObjectDestructionObserver arg count must == 1"));
            return base::expected<piper::Value, JSINativeException>();
          }
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (!args[0].isObject() || !args[0].getObject(rt).isFunction(rt)) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "the first argument of createJSObjectDestructionObserver must "
                "be a function"));
          }

          const auto callback =
              ptr->CreateCallBack(args[0].getObject(rt).getFunction(rt));

          return piper::Value(Object::createFromHostObject(
              rt, std::make_shared<JSObjectDestructionObserver>(
                      native_app_, std::move(callback))));
        });
  } else if (methodName == "pauseGcSuppressionMode") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "pauseGcSuppressionMode"), 0,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> pauseGcSuppressionMode");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          ptr->PauseGcSuppressionMode();
          return piper::Value::undefined();
        });
  } else if (methodName == "resumeGcSuppressionMode") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "resumeGcSuppressionMode"), 0,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> resumeGcSuppressionMode");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          ptr->ResumeGcSuppressionMode();
          return piper::Value::undefined();
        });
  } else if (methodName == "__SetSourceMapRelease") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "__SetSourceMapRelease"), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> __SetSourceMapRelease");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "__SetSourceMapRelease arg count must == 1"));
          }
          if (!args[0].isObject()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "the first argument of __SetSourceMapRelease must "
                "be an object."));
          }
          common::JSErrorInfo error_info;
          auto js_error = args[0].asObject(rt);
          auto message = js_error->getProperty(rt, "message");
          if (!message || !message->isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "__SetSourceMapRelease arg must has message property."));
          }
          error_info.message = message->asString(rt)->utf8(rt);
          auto stack = js_error->getProperty(rt, "stack");
          if (!stack || !stack->isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "__SetSourceMapRelease arg must has stack property."));
          }
          error_info.stack = stack->asString(rt)->utf8(rt);
          auto name = js_error->getProperty(rt, "name");
          if (name && name->isString()) {
            error_info.name = name->asString(rt)->utf8(rt);
          }
          ptr->SetSourceMapRelease(std::move(error_info));
          return piper::Value::undefined();
        });
  } else if (methodName == "__GetSourceMapRelease") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "__GetSourceMapRelease"), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get -> __GetSourceMapRelease");
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          std::string url;
          if (count >= 1 && args[0].isString()) {
            url = args[0].asString(rt)->utf8(rt);
          }
          auto ret = ptr->GetSourceMapRelease(url);
          if (ret.empty()) {
            return piper::Value::undefined();
          } else {
            return piper::String::createFromAscii(rt, ret);
          }
        });
  } else if (methodName == "requestAnimationFrame") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "requestAnimationFrame"), 1,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "requestAnimationFrame arg count must be 1"));
          }

          if (args[0].isObject()) {
            if (!args[0].getObject(rt).isFunction(rt)) {
              return base::unexpected(
                  BUILD_JSI_NATIVE_EXCEPTION("Args[0] must be a function."));
            }
            auto func = args[0].getObject(rt).asFunction(rt);
            return ptr->RequestAnimationFrame(std::move(*func));
          } else {
            return piper::Value::undefined();
          }
        });
  } else if (methodName == "cancelAnimationFrame") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "cancelAnimationFrame"), 1,

        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "cancelAnimationFrame arg count must be 1"));
          }

          // also use clearTimeout
          if (args[0].isNumber()) {
            ptr->CancelAnimationFrame(args[0].getNumber());
          }

          return piper::Value::undefined();
        });
  } else if (methodName == runtime::kAddReporterCustomInfo) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kAddReporterCustomInfo), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGI("LYNX App get ->" << runtime::kAddReporterCustomInfo);
          auto ptr = native_app_.lock();
          if (!ptr || ptr->IsDestroying()) {
            return piper::Value::undefined();
          }
          if (count >= 1 && args[0].isObject()) {
            auto obj = args[0].asObject(rt);
            auto ary = obj->getPropertyNames(rt);
            if (!ary) {
              return piper::Value::undefined();
            }
            auto length = (*ary).length(rt);
            if (length) {
              std::unordered_map<std::string, std::string> info;
              for (size_t i = 0; i < *length; i++) {
                auto property_name = (*ary).getValueAtIndex(rt, i);
                if (!property_name || !property_name->isString()) {
                  return piper::Value::undefined();
                }
                auto property_name_str = property_name->getString(rt).utf8(rt);
                auto property_value =
                    obj->getProperty(rt, property_name_str.c_str());
                if (property_value->isString()) {
                  info[property_name_str] =
                      property_value->getString(rt).utf8(rt);
                }
              }
              if (!info.empty()) {
                ptr->AddReporterCustomInfo(info);
              }
            }
          }
          return piper::Value::undefined();
        });
  } else if (methodName == runtime::kProfileStart) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kProfileStart), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size >= 1
          // [0] trace name -> String
          // optional ->  Object  {args: {}, flowId: number}
          auto native_app = native_app_;

          TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY_JAVASCRIPT, nullptr,
                            [&args, &count, &rt,
                             &native_app](lynx::perfetto::EventContext ctx) {
                              HandleProfileNameAndOption(args, count, rt,
                                                         native_app, ctx);
                            });
          return piper::Value::undefined();
        });

  } else if (methodName == runtime::kProfileEnd) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kProfileEnd), 0,
        [](Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          TRACE_EVENT_END(LYNX_TRACE_CATEGORY_JAVASCRIPT);
          return piper::Value::undefined();
        });
  } else if (methodName == runtime::kProfileMark) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kProfileMark), 1,
        [this](Runtime& rt, const piper::Value& thisVal,
               const piper::Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
          // parameter size >= 1
          // [0] trace name -> String
          // optional: -> Object {args: {}, flowId: number}
          auto native_app = native_app_;

          TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_JAVASCRIPT, nullptr,
                              [&args, &count, &rt,
                               &native_app](lynx::perfetto::EventContext ctx) {
                                HandleProfileNameAndOption(args, count, rt,
                                                           native_app, ctx);
                              });
          return piper::Value::undefined();
        });
  } else if (methodName == runtime::kProfileFlowId) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kProfileFlowId), 0,
        [](Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
          uint64_t flow_id = TRACE_FLOW_ID();
          return piper::Value(static_cast<int>(flow_id));
        });
  } else if (methodName == runtime::kIsProfileRecording) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kIsProfileRecording), 0,
        [](Runtime& rt, const piper::Value& thisVal, const piper::Value* args,
           size_t count) -> base::expected<Value, JSINativeException> {
#if ENABLE_TRACE_PERFETTO
          return piper::Value(
              TRACE_EVENT_CATEGORY_ENABLED(LYNX_TRACE_CATEGORY_JAVASCRIPT));
#elif ENABLE_TRACE_SYSTRACE
          return piper::Value(true);
#else
          return piper::Value(false);
#endif
        });
  }

  return piper::Value::undefined();
}

void AppProxy::set(Runtime* rt, const PropNameID& name, const Value& value) {}

std::vector<PropNameID> AppProxy::getPropertyNames(Runtime& rt) {
  static const char* kProps[] = {
      "id",
      "__pageUrl",
      "loadScript",
      "readScript",
      "readDynamicComponentScripts",
      "updateData",
      "batchedUpdateData",
      "setCard",
      "setTimeout",
      "setInterval",
      "clearTimeout",
      "clearInterval",
      "nativeModuleProxy",
      "reportException",
      "updateComponentData",
      "triggerLepusGlobalEvent",
      "triggerComponentEvent",
      "selectComponent",
      "loadScriptAsync",
      "onPiperInvoked",
      "getPathInfo",
      "getEnv",
      "invokeUIMethod",
      "getFields",
      "setNativeProps",
      "callLepusMethod",
      "markTiming",
      "triggerWorkletFunction",
      "createJSObjectDestructionObserver",
      "featureCount",
      "pauseGcSuppressionMode",
      "resumeGcSuppressionMode",
      "getSessionStorageItem",
      "subscribeSessionStorage",
      "generatePipelineOptions",
      "onPipelineStart",
      "bindPipelineIdWithTimingFlag",
      "markPipelineTiming",
      "__SetSourceMapRelease",
      "__GetSourceMapRelease",
      "requestAnimationFrame",
      "cancelAnimationFrame",
      runtime::kAddReporterCustomInfo,
      runtime::kProfileStart,
      runtime::kProfileEnd,
      runtime::kProfileMark,
      runtime::kProfileFlowId,
      runtime::kIsProfileRecording,
  };

  static constexpr size_t kPropsCount = sizeof(kProps) / sizeof(kProps[0]);

  std::vector<PropNameID> vec;
  vec.reserve(kPropsCount);
  for (size_t i = 0; i < kPropsCount; i++) {
    vec.push_back(
        piper::PropNameID::forAscii(rt, kProps[i], std::strlen(kProps[i])));
  }
  return vec;
}

void App::SetCSSVariable(const std::string& component_id,
                         const std::string& id_selector,
                         const lepus::Value& properties) {
  auto rt = rt_.lock();
  if (!rt) {
    return;
  }
  tasm::PipelineOptions pipeline_options;
  delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                             pipeline_options.pipeline_origin,
                             pipeline_options.pipeline_start_timestamp);
  delegate_->SetCSSVariables(component_id, id_selector, properties,
                             std::move(pipeline_options));
}

void App::Init() {
  GetContextProxy(runtime::ContextProxy::Type::kJSContext)
      ->AddEventListener(runtime::kMessageEventTypeForceGcJSIObjectWrapper,
                         std::make_unique<event::ClosureEventListener>(
                             [this](lepus::Value args) {
                               jsi_object_wrapper_manager_->ForceGcOnJSThread();
                             }));
  auto core_context_proxy =
      GetContextProxy(runtime::ContextProxy::Type::kCoreContext);
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypeOnAppEnterForeground,
      std::make_unique<event::ClosureEventListener>(
          [this](lepus::Value args) { ResumeAnimationFrame(); }));
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypeOnAppEnterBackground,
      std::make_unique<event::ClosureEventListener>(
          [this](lepus::Value args) { PauseAnimationFrame(); }));
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypeSendPageEvent,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        auto args_array = args.Array();
        if (args.IsArray() && args_array->size() == 3) {
          const auto& page_name = args_array->get(0).StdString();
          const auto& handler = args_array->get(1).StdString();
          const auto& info = args_array->get(2);
          SendPageEvent(page_name, handler, info);
        }
      }));
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypePublishComponentEvent,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        auto args_array = args.Array();
        if (args.IsArray() && args_array->size() == 3) {
          const auto& component_id = args_array->get(0).StdString();
          const auto& handler = args_array->get(1).StdString();
          const auto& info = args_array->get(2);
          PublishComponentEvent(component_id, handler, info);
        }
      }));
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypeCallJSFunctionInLepusEvent,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        auto args_array = args.Array();
        if (args.IsArray() && args_array->size() == 3) {
          const auto& component_id = args_array->get(0).StdString();
          const auto& name = args_array->get(1).StdString();
          const auto& params = args_array->get(2);
          CallJSFunctionInLepusEvent(component_id, name, params);
        }
      }));
  core_context_proxy->AddEventListener(
      runtime::kMessageEventTypeSendGlobalEvent,
      std::make_unique<event::ClosureEventListener>([this](lepus::Value args) {
        auto args_array = args.Array();
        if (args.IsArray() && args_array->size() == 2) {
          const auto& name = args_array->get(0).StdString();
          const auto& params = args_array->get(1);
          SendGlobalEvent(name, params);
        }
      }));
}

void App::destroy() {
  auto rt = rt_.lock();
  if (rt && js_app_.isObject()) {
    LOGI("App::destroy " << this);

    Scope scope(*rt);

    piper::Object global = rt->global();

    auto destroyCard = global.getPropertyAsFunction(*rt, "destroyCard");
    if (destroyCard) {
      size_t count = 1;
      piper::String id_str = piper::String::createFromUtf8(*rt, app_guid_);
      piper::Value id_value(*rt, id_str);
      const Value args[1] = {std::move(id_value)};
      destroyCard->call(*rt, args, count);
      LOGI("App::destroy end " << this);
    }
  }

  if (jsi_object_wrapper_manager_) {
    jsi_object_wrapper_manager_->DestroyOnJSThread();
  }

  exception_handler_->Destroy();
}

void App::CallDestroyLifetimeFun() {
  state_ = State::kDestroying;

  LOGI(" App::CallDestroyLifetimeFun start " << this);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CardLifeTimeCallback:onDestroy");
  auto rt = rt_.lock();
  if (rt && js_app_.isObject()) {
    Scope scope(*rt);

    piper::Object global = rt->global();

    auto on_destroy =
        global.getPropertyAsFunction(*rt, "callDestroyLifetimeFun");
    if (on_destroy) {
      size_t count = 1;
      piper::String id_str = piper::String::createFromUtf8(*rt, app_guid_);
      piper::Value id_value(*rt, id_str);
      const Value args[1] = {std::move(id_value)};
      on_destroy->call(*rt, args, count);
    }
  }
  // when destroy, internal js api callbacks, timed task callbacks, and
  // animation frame callbacks are not necessory to handle.
  api_callback_manager_.Destroy();
  js_task_adapter_.reset();
  // destroy raf
  if (animation_frame_handler_) {
    animation_frame_handler_->Destroy();
  }
  LOGI(" App::CallDestroyLifetimeFun end " << this);
}

void App::loadApp(tasm::TasmRuntimeBundle bundle,
                  const lepus::Value& global_props,
                  tasm::PackageInstanceDSL dsl,
                  tasm::PackageInstanceBundleModuleMode bundle_module_mode,
                  const std::string& url) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "LoadJSApp", "url", url);
  card_bundle_ = std::move(bundle);
  init_global_props_ = global_props;
  app_dsl_ = dsl;
  bundle_module_mode_ = bundle_module_mode;
  url_ = url;

  auto rt = rt_.lock();
  if (!rt) {
    handleLoadAppFailed("js runtime is null!");
    return;
  }
  GCPauseSuppressionMode mode(GetRuntime().get());

  Scope scope(*rt.get());
  state_ = State::kStarted;
  LOGI(" App::loadApp start " << this);
  piper::Object global = rt->global();
  auto load_app_func = global.getPropertyAsFunction(*rt, "loadCard");
  if (!load_app_func) {
    handleLoadAppFailed("LoadApp fail: get loadCard from js global fail!");
    return;
  }

  auto page_proxy = std::make_shared<piper::AppProxy>(rt, shared_from_this());
  piper::Object page_object =
      piper::Object::createFromHostObject(*rt, page_proxy);

  lepus::Value encoded_data = card_bundle_.encoded_data;
  lepus::Value init_card_config_data = card_config_;

  const auto& init_data = card_bundle_.init_data;

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LepusValueToJSValue");
  auto js_encoded_data = valueFromLepus(*rt, encoded_data);
  if (!js_encoded_data) {
    handleLoadAppFailed(
        " App::loadApp error! js_encoded_data valueFromLepus fail. ");
    return;
  }

  auto js_init_data = valueFromLepus(*rt, init_data.GetValue());
  if (!js_init_data) {
    handleLoadAppFailed(
        " App::loadApp error! js_init_data valueFromLepus fail. ");
    return;
  }

  const auto& cache_data = card_bundle_.cache_data;
  auto js_cache_data = Array::createWithLength(*rt, cache_data.size());
  if (!js_cache_data) {
    handleLoadAppFailed(" App::loadApp error! init js_cache_data fail. ");
    return;
  }
  for (size_t i = 0; i < cache_data.size(); ++i) {
    const auto& data = cache_data[i];
    piper::Object js_obj(*rt);
    const auto& js_data = valueFromLepus(*rt, data.GetValue());
    if (!js_data) {
      handleLoadAppFailed(
          " App::loadApp error! js_data in js_cache_data valueFromLepus "
          "fail. ");
      return;
    }

    bool is_successful =
        js_obj.setProperty(*rt, tasm::kData, *js_data) &&
        js_obj.setProperty(
            *rt, tasm::kProcessorName,
            Value(String::createFromUtf8(*rt, data.PreprocessorName())));
    if (!is_successful) {
      handleLoadAppFailed(
          " App::loadApp error! construct js_obj in cache data setProperty "
          "fail. ");
      return;
    }

    js_cache_data->setValueAtIndex(*rt, i, std::move(js_obj));
  }

  auto js_init_card_config_data = valueFromLepus(*rt, init_card_config_data);
  if (!js_init_card_config_data) {
    handleLoadAppFailed(
        " App::loadApp error! js_init_card_config_data "
        "valueFromLepus fail. ");
    return;
  }

  piper::Object page_config_subset(*rt);
  if (!page_config_subset.setProperty(
          *rt, runtime::kEnableMicrotaskPromisePolyfill,
          card_bundle_.enable_microtask_promise_polyfill)) {
    handleLoadAppFailed(" App::loadApp error! page_config_subset init fail.");
    return;
  }

  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  const char* str_dsl = tasm::GetDSLName(dsl);
  piper::Value card_type(piper::String::createFromUtf8(*rt, str_dsl));

  piper::Object params(*rt);

  // As long as there is a return value of `setProperty` is false, we consider
  // the `loadApp` failed.
  bool is_successful =
      params.setProperty(*rt, "initData", *js_encoded_data) &&
      params.setProperty(*rt, "updateData", *js_init_data) &&
      params.setProperty(
          *rt, tasm::kProcessorName,
          Value(String::createFromUtf8(*rt, init_data.PreprocessorName()))) &&
      params.setProperty(*rt, tasm::kCacheData, *js_cache_data) &&
      params.setProperty(*rt, "initConfig", *js_init_card_config_data) &&
      params.setProperty(*rt, "cardType", card_type) &&
      params.setProperty(*rt, "appGUID", getAppGUID()) &&
      params.setProperty(
          *rt, "bundleSupportLoadScript",
          bundle_module_mode ==
              tasm::PackageInstanceBundleModuleMode::RETURN_BY_FUNCTION_MODE) &&
      params.setProperty(*rt, "srcName", url) &&
      params.setProperty(*rt, "pageConfigSubset", page_config_subset);
  if (!is_successful) {
    handleLoadAppFailed("LoadApp fail: setProperty fail!");
    return;
  }

  lynx_proxy_ = std::make_shared<piper::LynxProxy>(rt, shared_from_this());
  piper::Object lynx_object =
      piper::Object::createFromHostObject(*rt, lynx_proxy_);

  piper::Value pageValue(*rt, page_object);
  piper::Value paramValue(*rt, params);
  piper::Value lynxValue(*rt, lynx_object);
  const Value args[3] = {std::move(pageValue), std::move(paramValue),
                         std::move(lynxValue)};
  size_t count = 3;
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "RunningInJS");
  auto ret = load_app_func->call(*rt, args, count);
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  if (!ret || !ret->isBool() || !ret->getBool()) {
    LOGE("LoadApp fail: call load_app_func fail!");
    return;
  }
  LOGI(" App::loadApp end " << this);
}

void App::handleLoadAppFailed(std::string error_msg) {
  state_ = State::kAppLoadFailed;
  auto rt = rt_.lock();
  if (rt) {
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(error_msg));
  }
}

void App::LoadScriptAsync(const std::string& url, ApiCallBack callback) {
  TRACE_EVENT_FUNC_NAME(LYNX_TRACE_CATEGORY, "url", url);
  LOGI("App::LoadScriptAsync " << url << " " << this);
  delegate_->LoadScriptAsync(url, callback);
}

void App::EvaluateScript(const std::string& url, std::string script,
                         ApiCallBack callback) {
  TRACE_EVENT_FUNC_NAME(LYNX_TRACE_CATEGORY, "url", url);
  LOGI("App::EvaluateScript:" << url);
#if ENABLE_TESTBENCH_RECORDER
  tasm::recorder::TestBenchBaseRecorder::GetInstance().RecordScripts(
      url.c_str(), script.c_str());
#endif

  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt);
    auto prepared_script = rt->prepareJavaScript(
        std::make_shared<StringBuffer>(std::move(script)), url);
    auto ret = rt->evaluatePreparedJavaScript(prepared_script);
    if (!ret.has_value()) {
      auto error_str = ret.error().ToString();
      auto js_error_value = piper::Value(piper::String::createFromUtf8(
          *rt, "load external js script failed! url: " + url +
                   " error:" + error_str));
      rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
          "load external js script failed! url: " + url + error_str));
      return api_callback_manager_.InvokeWithValue(rt.get(), callback.id(),
                                                   std::move(js_error_value));
    }

    return api_callback_manager_.InvokeWithValue(
        rt.get(), callback.id(), piper::Value::null(), std::move(ret.value()));
  }
}

void App::onAppReload(tasm::TemplateData init_data) {
  auto rt = rt_.lock();
  if (rt && js_app_.isObject()) {
    Scope scope(*rt);
    auto js_app = js_app_.getObject(*rt);

    auto on_app_reload = js_app.getPropertyAsFunction(*rt, "onAppReload");
    if (!on_app_reload) {
      return;
    }
    auto js_init_data = valueFromLepus(*rt, init_data.GetValue(),
                                       jsi_object_wrapper_manager_.get());
    if (!js_init_data) {
      return;
    }

    piper::Object options(*rt);
    if (!options.setProperty(
            *rt, tasm::kProcessorName,
            Value(String::createFromUtf8(*rt, init_data.PreprocessorName())))) {
      LOGE("App::onAppReload since options setProperty failed");
      return;
    }

    size_t count = 2;
    const Value args[2] = {std::move(*js_init_data), std::move(options)};
    on_app_reload->callWithThis(*rt, js_app, args, count);
  }
}

void App::CallJSFunctionInLepusEvent(const std::string& component_id,
                                     const std::string& name,
                                     const lepus::Value& params) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallbackToLepusEvent");
  LOGI("App::CallJSFunctionInLepusEvent,func name: " << name << " " << this);
  std::optional<Value> res;
  constexpr const static char* kPageComponentID = "0";
  if (component_id == kPageComponentID) {
    res = SendPageEvent("", name, params);
  } else {
    res = PublishComponentEvent(component_id, name, params);
  }
  constexpr const static char kEventCallbackId[] = "callbackId";

  // if need callback(in worklet), it need to callback to lepus event
  // function
  int32_t callbackId = params.Table()
                           .get()
                           ->GetValue(BASE_STATIC_STRING(kEventCallbackId))
                           .Int32();
  // if callback id is negative, means no need to callback
  if (callbackId >= 0) {
    if (res.has_value()) {
      auto rt = rt_.lock();
      auto data_lepusValue = ParseJSValueToLepusValue(*res, PAGE_GROUP_ID);
      if (data_lepusValue.has_value()) {
        delegate_->InvokeLepusComponentCallback(
            callbackId, tasm::DEFAULT_ENTRY_NAME, *data_lepusValue);
      }
    } else {
      // if not have return value, it also need to callback,
      // because the callback is stored in set wait for callback to remove
      delegate_->InvokeLepusComponentCallback(
          callbackId, tasm::DEFAULT_ENTRY_NAME, lepus::Value());
    }
  }
}

std::optional<Value> App::SendPageEvent(const std::string& page_name,
                                        const std::string& handler,
                                        const lepus::Value& info) {
  LOGI("App::SendPageEvent,handler: " << handler << " " << this);
  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid()) {
    int32_t instance_id = static_cast<int32_t>(rt->getRuntimeId());
    tasm::timing::LongTaskMonitor::Scope long_task_scope(
        instance_id, tasm::timing::kJSFuncTask,
        tasm::timing::kTaskNameJSAppSendPageEvent, handler);
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "SendPageEvent",
                [&](lynx::perfetto::EventContext ctx) {
                  ctx.event()->add_debug_annotations("event", handler);
                });
    Scope scope(*rt);
    Object js_app = js_app_.getObject(*rt);

    auto publishEvent = js_app.getPropertyAsFunction(*rt, "publishEvent");
    if (!publishEvent) {
      return std::nullopt;
    }

    piper::String strName = piper::String::createFromUtf8(*rt, handler);
    piper::Value jsName(*rt, strName);
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LepusValueToJSValue");
    auto data = valueFromLepus(*rt, info, jsi_object_wrapper_manager_.get());
    TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
    if (!data) {
      return std::nullopt;
    }
    piper::Value id(0);

    const Value args[3] = {std::move(jsName), std::move(*data), std::move(id)};
    size_t count = 3;
    const piper::Object& thisObj = js_app;
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "RunningInJS");
    auto res = publishEvent->callWithThis(*rt, thisObj, args, count);
    // get js function return value
    return res;
  }
  return std::nullopt;
}

void App::SendGlobalEvent(const std::string& name,
                          const lepus::Value& arguments) {
  constexpr char kGlobalEventModuleName[] = "GlobalEventEmitter";
  constexpr char kGlobalEventMethodName[] = "emit";
  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid()) {
    Scope scope(*rt);
    auto param = Array::createWithLength(*rt, 1);
    if (!param) {
      return;
    }
    auto arg = Array::createWithLength(*rt, 2);
    if (!arg) {
      return;
    }
    auto element =
        valueFromLepus(*rt, arguments, jsi_object_wrapper_manager_.get());
    if (!element) {
      return;
    }
    // As long as there is a return value of `setValueAtIndex` is false, we
    // consider the `SendGlobalEvent` failed.
    bool is_successful = (*param).setValueAtIndex(*rt, 0, *element) &&
                         (*arg).setValueAtIndex(*rt, 0, name) &&
                         (*arg).setValueAtIndex(*rt, 1, std::move(*param));
    if (!is_successful) {
      return;
    }
    CallFunction(kGlobalEventModuleName, kGlobalEventMethodName,
                 std::move(*arg));
  }
}

void App::SetupSsrJsEnv() {
  constexpr char kCreateGlobalEventEmitter[] = "__createEventEmitter";
  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt);
    piper::Object global = rt->global();
    auto create_event_emitterFunc =
        global.getPropertyAsFunction(*rt, kCreateGlobalEventEmitter);
    if (!create_event_emitterFunc) {
      rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
          "SSR: exception has happened in getting function "
          "__createEventEmitter"));
      return;
    }
    // Create SSR global Event Emitter
    auto ret = create_event_emitterFunc->call(*rt, nullptr, 0);
    if (ret) {
      ssr_global_event_emitter_ = piper::Value(*rt, *ret);
    } else {
      rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
          "SSR: exception has happened in creating ssr global event emit"));
    }
  }
}

// SSR script be like:
// (function(){ return {func : function(SsrGlobalEventEmitter,SsrNativeModules){
//         //Captured Scripts
//  }}})();
void App::LoadSsrScript(const std::string& script) {
  LOGI("LoadSsrScript: " << script);
  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt);
    auto script_result = rt->evaluateJavaScript(
        std::make_shared<StringBuffer>(std::move(script)), "ssr-script.js");
    if (!script_result.has_value()) {
      rt->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("SSR: exception has happened in getting "
                                     "ssr-script.js returned object error:" +
                                     script_result.error().ToString()));
      return;
    }

    auto ssr_returned_object = (*script_result).asObject(*rt);
    auto ssr_returned_function =
        ssr_returned_object->getPropertyAsFunction(*rt, "func");

    if (!ssr_returned_function) {
      rt->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("SSR: exception has happened in getting "
                                     "ssr-script.js returned function"));
      return;
    }

    size_t count = 2;
    piper::Value global_event_emit(*rt, ssr_global_event_emitter_);
    piper::Value native_modules(*rt, nativeModuleProxy());
    const Value args[2] = {std::move(global_event_emit),
                           std::move(native_modules)};

    auto ret = ssr_returned_function->call(*rt, args, count);
    if (!ret) {
      rt->reportJSIException(
          BUILD_JSI_NATIVE_EXCEPTION("SSR: exception has happened in calling "
                                     "ssr-script.js returned function"));
      return;
    }
  }
}

void App::SendSsrGlobalEvent(const std::string& name,
                             const lepus::Value& arguments) {
  constexpr char kSsrGlobalEventEmitterFun[] = "emit";
  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt);

    if (ssr_global_event_emitter_.isNull()) {
      LOGE(
          "SSR: exception has happened in getting native "
          "ssr_global_event_emitter_ "
          << name << "  " << this);
      return;
    }

    auto ssr_event_emitter = ssr_global_event_emitter_.asObject(*rt);

    auto emit_func = ssr_event_emitter->getPropertyAsFunction(
        *rt, kSsrGlobalEventEmitterFun);
    if (!emit_func) {
      LOGE("SSR: exception has happened in getting SSR global event emitter"
           << name << "  " << this);
      return;
    }

    auto piper_arguments =
        valueFromLepus(*rt, arguments, jsi_object_wrapper_manager_.get());
    if (!piper_arguments) {
      LOGE("SSR: exception has happened in parsing ssr global event arguments"
           << name << "  " << this);
      return;
    }

    piper::Value event_name(piper::String::createFromUtf8(*rt, name));
    const Value args[2] = {std::move(event_name), std::move(*piper_arguments)};
    size_t count = 2;

    const piper::Object& ssr_event_emitter_obj = *ssr_event_emitter;

    auto ret = emit_func->callWithThis(*rt, ssr_event_emitter_obj, args, count);
    if (!ret) {
      LOGE("SSR: exception has happened in emitting ssr global event:"
           << name << "  " << this);
      return;
    }
    LOGI("SSR: end emit ssr global event:" << name << "  " << this);
  }
}

void App::CallFunction(const std::string& module_id,
                       const std::string& method_id,
                       const piper::Array& arguments,
                       bool force_call_despite_app_state) {
  auto rt = rt_.lock();
  if (rt && (IsJsAppStateValid() ||
             (force_call_despite_app_state && js_app_.isObject()))) {
    Scope scope(*rt);
    Object js_app = js_app_.getObject(*rt);

    std::string first_arg_str;
    std::optional<piper::Value> first_arg_opt;
    if (arguments.length(*rt) > 0) {
      first_arg_opt = arguments.getValueAtIndex(*rt, 0);
    }
    if (first_arg_opt && first_arg_opt->isString()) {
      first_arg_str = first_arg_opt->getString(*rt).utf8(*rt);
    }
    LOGI("call jsmodule:" << module_id << "." << method_id << "."
                          << first_arg_str << " " << this);

    auto publishEvent = js_app.getPropertyAsFunction(*rt, "callFunction");
    if (!publishEvent) {
      return;
    }

    piper::String str_module = piper::String::createFromUtf8(*rt, module_id);
    piper::Value jsName(*rt, str_module);
    piper::String str_method = piper::String::createFromUtf8(*rt, method_id);
    piper::Value jsMethod(*rt, str_method);

    piper::Value args[3];
    args[0] = std::move(jsName);
    args[1] = std::move(jsMethod);
    piper::Value method_args(*rt, arguments);
    args[2] = std::move(method_args);
    const piper::Object& thisObj = js_app;
    auto ret = publishEvent->callWithThis(*rt, thisObj, args, 3);
    if (!ret) {
      LOGI("exception has happened in call jsmodule. module:"
           << module_id << " method:" << method_id << this);
      return;
    }
    LOGV("end  call jsmodule. module:" << module_id << " method:" << method_id
                                       << "." << first_arg_str << " " << this);
  }
}

void App::InvokeApiCallBack(ApiCallBack id) {
  api_callback_manager_.InvokeWithValue(rt_.lock().get(), id);
}

void App::InvokeApiCallBackWithValue(ApiCallBack id, const lepus::Value& value,
                                     bool persist) {
  if (persist) {
    api_callback_manager_.InvokeWithValuePersist(rt_.lock().get(), id, value);
  } else {
    api_callback_manager_.InvokeWithValue(rt_.lock().get(), id, value);
  }
}

void App::InvokeApiCallBackWithValue(ApiCallBack id, piper::Value value) {
  api_callback_manager_.InvokeWithValue(rt_.lock().get(), id, std::move(value));
}

ApiCallBack App::CreateCallBack(piper::Function func) {
  return api_callback_manager_.createCallbackImpl(std::move(func));
}

void App::EraseApiCallBack(ApiCallBack callback) {
  api_callback_manager_.EraseWithCallback(callback);
}

void App::NotifyUpdatePageData() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::updateCardData");
  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid()) {
    auto updated_card_data = delegate_->FetchUpdatedCardData();

    // for react, don't need update data, react use "onReactCardRender"
    if (app_dsl_ == tasm::PackageInstanceDSL::REACT) {
      return;
    }

    for (const auto& data : updated_card_data) {
      Scope scope(*rt);
      LOGI("App::updateCardData" << this);

      Object js_app = js_app_.getObject(*rt);
      auto publishEvent = js_app.getPropertyAsFunction(*rt, "updateCardData");
      if (!publishEvent) {
        continue;
      }
      TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LepusValueToJSValue");
      auto jsValue = valueFromLepus(*rt, data.GetValue(),
                                    jsi_object_wrapper_manager_.get());
      if (!jsValue) {
        return;
      }

      auto op_type =
          valueFromLepus(*rt, lepus::Value(static_cast<int>(data.GetType())),
                         jsi_object_wrapper_manager_.get());
      if (!op_type) {
        LOGE(
            "App::NotifyUpdatePageData() since options generate op type "
            "failed");
        return;
      }

      piper::Object options(*rt);
      if (!(options.setProperty(*rt, tasm::kType, std::move(*op_type)) &&
            options.setProperty(
                *rt, tasm::kProcessorName,
                Value(String::createFromUtf8(*rt, data.ProcessorName()))))) {
        LOGE("App::NotifyUpdatePageData() since options setProperty failed");
        return;
      }

      TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
      const Value args[2] = {std::move(*jsValue), std::move(options)};
      size_t count = 2;
      const piper::Object& thisObj = js_app;
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "RunningInJS");
      publishEvent->callWithThis(*rt, thisObj, args, count);
    }  // end for
  }
}

void App::NotifyUpdateCardConfigData() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::NotifyJSUpdateCardConfigData");
  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid()) {
    Scope scope(*rt);
    LOGI("App::updateCardConfigData" << this);

    Object js_app = js_app_.getObject(*rt);

    auto publishEvent = js_app.getPropertyAsFunction(*rt, "processCardConfig");
    if (!publishEvent) {
      return;
    }
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LepusValueToJSValue");

    lepus::Value card_config_data = card_config_;
    auto jsValue = valueFromLepus(*rt, card_config_data,
                                  jsi_object_wrapper_manager_.get());
    if (!jsValue) {
      return;
    }
    TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
    const Value args[1] = {std::move(*jsValue)};
    size_t count = 1;
    const piper::Object& thisObj = js_app;
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "RunningInJS");
    publishEvent->callWithThis(*rt, thisObj, args, count);
  }
}

void App::OnAppJSError(const piper::JSIException& exception) {
  const std::string& msg = exception.message();
  LOGE("app::onAppJSError:" << exception.ToString());
  auto rt = rt_.lock();
  if (rt && js_app_.isObject()) {
    Scope scope(*rt);
    Object js_app = js_app_.getObject(*rt);

    auto onAppError = js_app.getPropertyAsFunction(*rt, "onAppError");
    if (!onAppError) {
      return;
    }

    piper::String msg_str = piper::String::createFromUtf8(*rt, msg);

    piper::Value js_message(*rt, msg_str);
    auto js_error = piper::Object::createFromHostObject(
        *rt, std::make_shared<LynxError>(exception));

    // The first argument is used for backward compatibility
    // since appbrand will use this API and we do not want to break them.
    const Value args[2] = {std::move(js_message), std::move(js_error)};
    onAppError->callWithThis(*rt, js_app, args, 2);
  } else {
    LOGE("reportJSException when js_app_ is not ready: " << msg);
    base::LynxError error{exception.errorCode(), msg};
    common::FormatErrorUrl(error, "");
    delegate_->OnErrorOccurred(std::move(error));
  }
}

void App::setJsAppObj(piper::Object&& obj) {
  auto rt = rt_.lock();
  if (!rt) {
    return;
  }

  js_app_ = piper::Value(*rt, obj);
  // check if there has cached data changes
  NotifyUpdatePageData();
}

void App::appDataChange(lepus_value&& data, ApiCallBack callback,
                        runtime::UpdateDataType update_data_type) {
  tasm::PipelineOptions pipeline_options;
  delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                             pipeline_options.pipeline_origin,
                             pipeline_options.pipeline_start_timestamp);
  const auto& timing_flag = tasm::GetTimingFlag(data);
  if (!timing_flag.empty()) {
    pipeline_options.need_timestamps = true;
    delegate_->BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                            timing_flag);
    tasm::TimingCollector::Scope<runtime::TemplateDelegate> scope(
        delegate_, pipeline_options);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kSetStateTrigger);
  }
  runtime::UpdateDataTask task(true, PAGE_GROUP_ID, std::move(data), callback,
                               std::move(update_data_type),
                               std::move(pipeline_options));
  delegate_->UpdateDataByJS(std::move(task));
}

std::optional<JSINativeException> App::batchedUpdateData(
    const piper::Value& args) {
  auto rt = rt_.lock();
  if (!rt || !args.isObject()) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
        "runtime is destroy or batchedUpdateData's args isn't an object."));
  }
  uint64_t update_task_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "batchedUpdateData",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(update_task_id);
              });
  auto data_obj = args.asObject(*rt);
  if (!data_obj) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
        "batchedUpdateData's args isn't an object."));
  }
  auto data_ary = data_obj->asArray(*rt);
  if (!data_ary) {
    return std::optional(
        BUILD_JSI_NATIVE_EXCEPTION("batchedUpdateData's args isn't an array."));
  }

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                    "batchedUpdateData:JSValueToLepusValue");
  std::vector<runtime::UpdateDataTask> tasks;
  auto size = data_ary->size(*rt);
  if (!size) {
    return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
        "batchedUpdateData's args don't have size property."));
  }
  tasks.reserve(*size);
  for (size_t i = 0; i < *size; i++) {
    auto val = data_ary->getValueAtIndex(*rt, i);
    if (!val) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
          "batchedUpdateData's data[" + std::to_string(i) + "] is null."));
    }
    auto item = val->asObject(*rt);
    if (!item) {
      return std::optional(
          BUILD_JSI_NATIVE_EXCEPTION("batchedUpdateData's data[" +
                                     std::to_string(i) + "] isn't an object."));
    }

    auto type_opt = item->getProperty(*rt, "type");
    if (!type_opt) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
          "batchedUpdateData's data[" + std::to_string(i) +
          "] don't have type property."));
    }
    auto type = type_opt->asString(*rt);
    if (!type) {
      return std::optional(
          BUILD_JSI_NATIVE_EXCEPTION("batchedUpdateData's data[" +
                                     std::to_string(i) + "] isn't a string."));
    }

    ApiCallBack callback;
    if (item->hasProperty(*rt, "callback")) {
      auto cb_opt = item->getProperty(*rt, "callback");
      if (!cb_opt) {
        return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
            "batchedUpdateData's data[" + std::to_string(i) +
            "]'s callback is null."));
      }
      auto cb = cb_opt->asObject(*rt);
      if (!cb || !cb->isFunction(*rt)) {
        return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
            "batchedUpdateData's data[" + std::to_string(i) +
            "]'s callback isn't a function."));
      }
      callback = CreateCallBack(cb->getFunction(*rt));
    }

    bool is_card = (type->utf8(*rt) == "card");
    std::string component_id = PAGE_GROUP_ID;
    if (!is_card) {
      auto comp_id_opt = item->getProperty(*rt, "componentId");
      if (!comp_id_opt) {
        return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
            "batchedUpdateData's data[" + std::to_string(i) +
            "]'s componentId is null."));
      }
      auto comp_id = comp_id_opt->asString(*rt);
      if (!comp_id) {
        return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
            "batchedUpdateData's data[" + std::to_string(i) +
            "]'s componentId isn't a string."));
      }
      component_id = comp_id->utf8(*rt);
    }
    auto data_opt = item->getProperty(*rt, "data");
    if (!data_opt) {
      return std::optional(
          BUILD_JSI_NATIVE_EXCEPTION("batchedUpdateData's data[" +
                                     std::to_string(i) + "]'s data is null."));
    }
    auto data_lepusValue = ParseJSValueToLepusValue(*data_opt, component_id);
    if (!data_lepusValue) {
      return std::optional(BUILD_JSI_NATIVE_EXCEPTION(
          "ParseJSValueToLepusValue error in batchedUpdateData"));
    }
    auto data_stacks = item->getProperty(*rt, "stackTraces");
    std::string stacks;
    if (data_stacks && data_stacks->isString()) {
      stacks = data_stacks->getString(*rt).utf8(*rt);
    }
    runtime::UpdateDataType update_data_type;
    auto js_update_data_type_opt = item->getProperty(*rt, "updateDataType");
    if (js_update_data_type_opt && js_update_data_type_opt->isNumber()) {
      update_data_type =
          runtime::UpdateDataType(js_update_data_type_opt->getNumber());
    }
    tasm::PipelineOptions pipeline_options;
    delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                               pipeline_options.pipeline_origin,
                               pipeline_options.pipeline_start_timestamp);
    const auto& timing_flag = tasm::GetTimingFlag(*data_lepusValue);
    if (!timing_flag.empty()) {
      pipeline_options.need_timestamps = true;
      delegate_->BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                              timing_flag);
      tasm::TimingCollector::Scope<runtime::TemplateDelegate> scope(
          delegate_, pipeline_options);
      tasm::TimingCollector::Instance()->Mark(tasm::timing::kSetStateTrigger);
    }
    tasks.emplace_back(is_card, component_id, *data_lepusValue, callback,
                       std::move(update_data_type), std::move(pipeline_options),
                       std::move(stacks));
  }
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "updateData:UpdateDataToTASM",
              [&](lynx::perfetto::EventContext ctx) {
                std::stringstream ss;
                for (runtime::UpdateDataTask& task : tasks) {
                  ss << task.callback_.id() << ",";
                }
                auto* debug = ctx.event()->add_debug_annotations();
                debug->set_name("CallbackID");
                debug->set_string_value(ss.str());
              });
  delegate_->UpdateBatchedDataByJS(std::move(tasks), update_task_id);
  return std::nullopt;
}

base::expected<Value, JSINativeException> App::loadScript(
    const std::string entry_name, const std::string& url, long timeout) {
  TRACE_EVENT_FUNC_NAME(LYNX_TRACE_CATEGORY, "url", url, "entry", entry_name);

  LOGI("loadscript:" << url);

  auto rt = rt_.lock();
  if (rt) {
    tasm::timing::LongTaskTiming* timing =
        tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
    if (timing != nullptr) {
      timing->task_name_ = url;
      timing->task_info_ = entry_name;
    }
    std::string source_url;
    // is the js file in lynx bundle?
    bool is_lynx_assets_source = runtime::IsLynxAssets(url);

    if (is_lynx_assets_source || base::BeginsWith(url, "http") ||
        url.front() == '/') {
      source_url = url;
    } else {
      source_url = "/" + url;
    }
    JsContent content = [this, is_lynx_assets_source, &source_url, &entry_name,
                         timeout]() -> JsContent {
      if (is_lynx_assets_source) {
        auto& lynx_js_assets_ = GetJSAssetsMap();
        if (auto iter = lynx_js_assets_.find(source_url);
            iter == lynx_js_assets_.cend()) {
          JsContent content{delegate_->GetLynxJSAsset(source_url),
                            JsContent::Type::SOURCE};
          lynx_js_assets_.emplace(source_url, content);
          return content;
        } else {
          return iter->second;
        }
      } else {
        return GetJSContent(entry_name, source_url, timeout);
      }
    }();

    if (content.IsError()) {
      auto buffer = std::move(content).GetBuffer();
      std::string error_msg{reinterpret_cast<const char*>(buffer->data()),
                            buffer->size()};
      return base::unexpected(
          BUILD_JSI_NATIVE_EXCEPTION(std::string("load script failed! url:") +
                                     source_url + " error:" + error_msg));
    }

    if (!entry_name.empty() && entry_name != tasm::DEFAULT_ENTRY_NAME) {
      source_url = GenerateDynamicComponentSourceUrl(entry_name, source_url);
    }
    bool is_app_service_js = runtime::IsAppServiceJS(source_url);

    // after this line, 'throwing_source' becomes a empty string
    auto prep =
        rt->prepareJavaScript(std::move(content).GetBuffer(), source_url);
    auto ret = rt->evaluatePreparedJavaScript(prep);
    if (is_app_service_js) {
      state_ = ret.has_value() ? State::kAppLoaded : State::kAppLoadFailed;
    }
    return ret;
  }
  return piper::Value::undefined();
}

std::string App::GenerateDynamicComponentSourceUrl(
    const std::string& entry_name, const std::string& source_url) {
  std::string dynamic_component_source_url = runtime::kDynamicComponentJSPrefix;
  dynamic_component_source_url.append(entry_name);
  dynamic_component_source_url.append("/");
  dynamic_component_source_url.append(source_url);
  return dynamic_component_source_url;
}

std::shared_ptr<ContextProxyInJS> App::GetContextProxy(
    runtime::ContextProxy::Type type) {
  if (type >= runtime::ContextProxy::Type::kUnknown ||
      type < runtime::ContextProxy::Type::kJSContext) {
    return nullptr;
  }
  auto result = context_proxy_vector_[static_cast<int32_t>(type)];
  if (result == nullptr) {
    result = std::make_shared<ContextProxyInJS>(*delegate_, type, rt_,
                                                weak_from_this());
    context_proxy_vector_[static_cast<int32_t>(type)] = result;
  }
  return result;
}

base::expected<Value, JSINativeException> App::readScript(
    const std::string entry_name, const std::string& url, long timeout) {
  LOGI("readScript:" << url);

  auto rt = rt_.lock();
  if (!rt) {
    return piper::Value::undefined();
  }

  // is the js file in lynx bundle?
  bool is_lynx_assets_source = runtime::IsLynxAssets(url);
  std::string source_url;

  if (is_lynx_assets_source || base::BeginsWith(url, "http") ||
      url.front() == '/') {
    source_url = std::move(url);
  } else {
    source_url = "/" + url;
  }
  auto content = GetJSContent(entry_name, source_url, timeout);
  auto is_error = content.IsError();
  auto buffer = std::move(content).GetBuffer();
  std::string throwing_source{reinterpret_cast<const char*>(buffer->data()),
                              buffer->size()};
  if (is_error) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        std::string("readScript ") + source_url + " error:" + throwing_source));
  } else {
    return piper::Value(piper::String::createFromUtf8(*rt, throwing_source));
  }
}

piper::Value App::setTimeout(piper::Function func, int time) {
  auto rt = rt_.lock();
  if (!rt || !js_task_adapter_) {
    return piper::Value::undefined();
  }

  TRACE_EVENT("lynx", "BackgroundThread::SetTimeout", "delay", time,
              "instance_id", rt->getRuntimeId());
  return js_task_adapter_->SetTimeout(std::move(func), time);
}

piper::Value App::setInterval(piper::Function func, int time) {
  auto rt = rt_.lock();
  if (!rt || !js_task_adapter_) {
    return piper::Value::undefined();
  }
  TRACE_EVENT("lynx", "BackgroundThread::SetInterval", "delay", time,
              "instance_id", rt->getRuntimeId());
  return js_task_adapter_->SetInterval(std::move(func), time);
}

void App::clearTimeout(double task) {
  if (js_task_adapter_) {
    js_task_adapter_->RemoveTask(static_cast<uint32_t>(task));
  }
}

void App::QueueMicrotask(piper::Function func) {
  auto rt = rt_.lock();
  if (!rt || !js_task_adapter_) {
    return;
  }
  return js_task_adapter_->QueueMicrotask(std::move(func));
}

void App::RunOnJSThreadWhenIdle(base::closure closure) {
  if (auto rt = rt_.lock()) {
    delegate_->RunOnJSThreadWhenIdle(std::move(closure));
  }
}

piper::Value App::nativeModuleProxy() {
  auto rt = rt_.lock();
  if (!rt) {
    return piper::Value::undefined();
  }
  return piper::Value(*rt, nativeModuleProxy_);
}

std::optional<piper::Value> App::getInitGlobalProps() {
  auto rt = rt_.lock();
  if (!rt) {
    return piper::Value::undefined();
  }
  auto props =
      valueFromLepus(*rt, lepus::Value::ShallowCopy(init_global_props_));
  if (!props) {
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "getInitGlobalProps fail! exception happen in valueFromLepus."));
    return std::optional<piper::Value>();
  }
  return std::move(*props);
}

std::optional<piper::Value> App::getPresetData() {
  auto rt = rt_.lock();
  if (!rt) {
    return piper::Value::undefined();
  }
  auto props = valueFromLepus(*rt, preset_data_);
  if (!props) {
    rt->reportJSIException(BUILD_JSI_NATIVE_EXCEPTION(
        "preset_data fail! exception happen in valueFromLepus."));
    return std::optional<piper::Value>();
  }
  return std::move(*props);
}

piper::Value App::getI18nResource() {
  auto rt = rt_.lock();
  if (!rt) {
    return piper::Value::undefined();
  }
  piper::Value res(piper::String::createFromUtf8(*rt, i18_resource_));
  return res;
}

void App::getContextDataAsync(const std::string& component_id,
                              const std::string& key, ApiCallBack callback) {
  auto rt = rt_.lock();
  if (!rt) {
    return;
  }
  delegate_->GetComponentContextDataAsync(component_id, key, callback);
}

void App::QueryComponent(const std::string& url, ApiCallBack callback,
                         const std::vector<std::string>& ids) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::QueryComponent",
              [&url](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", url);
              });
  auto holder = weak_js_bundle_holder_.lock();
  if (holder) {
    auto js_bundle = holder->GetJSBundleFromBT(url);
    if (js_bundle) {
      js_bundles_.emplace(url, std::move(*js_bundle));
      BASE_STATIC_STRING_DECL(kHasReady, "__hasReady");
      InvokeApiCallBackWithValue(std::move(callback),
                                 lepus::Value(lepus::Dictionary::Create(
                                     {{kHasReady, lepus::Value(true)}})));
      return;
    }
  }

  delegate_->LoadDynamicComponentFromJS(url, callback, ids);
}

void App::AddFont(const lepus::Value& font, ApiCallBack callback) {
  auto rt = rt_.lock();
  if (!rt) {
    return;
  }
  delegate_->AddFont(font, std::move(callback));
}

void App::OnIntersectionObserverEvent(int32_t observer_id, int32_t callback_id,
                                      piper::Value data) {
  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid()) {
    Scope scope(*rt);
    Object js_app = js_app_.getObject(*rt);

    auto onIntersectionObserverEvent =
        js_app.getPropertyAsFunction(*rt, "onIntersectionObserverEvent");
    if (!onIntersectionObserverEvent) {
      return;
    }

    piper::Value args[3];
    args[0] = observer_id;
    args[1] = callback_id;
    args[2] = std::move(data);
    const piper::Object& thisObj = js_app;
    onIntersectionObserverEvent->callWithThis(*rt, thisObj, args, 3);
  }
}

std::optional<Value> App::PublishComponentEvent(const std::string& component_id,
                                                const std::string& handler,
                                                const lepus::Value& info) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "App::PublishComponentEvent",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("event", handler);
              });
  LOGI(" publishComponentEvent " << component_id << " " << handler << " "
                                 << this);

  auto rt = rt_.lock();
  if (rt && IsJsAppStateValid() && card_bundle_.support_component_js) {
    int32_t instance_id = static_cast<int32_t>(rt->getRuntimeId());
    tasm::timing::LongTaskMonitor::Scope long_task_scope(
        instance_id, tasm::timing::kUpdateDataByJSTask,
        tasm::timing::kTaskNameJSAppPublishComponentEvent, handler);
    Scope scope(*rt);
    Object js_app = js_app_.getObject(*rt);

    // TODO(liyanbo.monster): fix this publicComponentEvent typo, when we can
    // break.
    auto publish_component_event =
        js_app.getPropertyAsFunction(*rt, "publicComponentEvent");
    if (!publish_component_event) {
      return std::nullopt;
    }

    piper::Value js_id(piper::String::createFromUtf8(*rt, component_id));
    piper::Value js_handler(piper::String::createFromUtf8(*rt, handler));
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, "LepusValueToJSValue");
    auto data = valueFromLepus(*rt, info, jsi_object_wrapper_manager_.get());
    TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
    if (!data) {
      return std::nullopt;
    }
    const Value args[3] = {std::move(js_id), std::move(js_handler),
                           std::move(*data)};
    size_t count = 3;
    const piper::Object& thisObj = js_app;
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "RunningInJS");
    auto res = publish_component_event->callWithThis(*rt, thisObj, args, count);
    return res;
  }
  return std::nullopt;
}

void App::triggerComponentEvent(const std::string& event_name,
                                lepus_value&& msg) {
  LOGI(" triggerComponentEvent " << event_name << " " << this);
  delegate_->TriggerComponentEvent(event_name, std::move(msg));
}

void App::triggerLepusGlobalEvent(const std::string& event_name,
                                  lepus_value&& msg) {
  LOGI(" triggerLepusGlobalEvent: " << event_name << " " << this);
  delegate_->TriggerLepusGlobalEvent(event_name, std::move(msg));
}

void App::triggerWorkletFunction(std::string component_id,
                                 std::string worklet_module_name,
                                 std::string method_name, lepus::Value args,
                                 ApiCallBack callback) {
  LOGI(" triggerWorkletFunction: " << method_name << " " << this);
  delegate_->TriggerWorkletFunction(
      std::move(component_id), std::move(worklet_module_name),
      std::move(method_name), std::move(args), std::move(callback));
}

void App::updateComponentData(const std::string& component_id,
                              lepus_value&& data, ApiCallBack callback,
                              runtime::UpdateDataType update_data_type) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxJSUpdateComponentData");
  LOGI(" updateComponentData " << component_id << " " << this);
  tasm::PipelineOptions pipeline_options;
  delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                             pipeline_options.pipeline_origin,
                             pipeline_options.pipeline_start_timestamp);
  const auto& timing_flag = tasm::GetTimingFlag(data);
  if (!timing_flag.empty()) {
    pipeline_options.need_timestamps = true;
    delegate_->BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                            timing_flag);
    tasm::TimingCollector::Scope<runtime::TemplateDelegate> scope(
        delegate_, pipeline_options);
    tasm::TimingCollector::Instance()->Mark(tasm::timing::kSetStateTrigger);
  }
  runtime::UpdateDataTask task(false, component_id, std::move(data), callback,
                               std::move(update_data_type),
                               std::move(pipeline_options));
  delegate_->UpdateComponentData(std::move(task));
}

void App::selectComponent(const std::string& component_id,
                          const std::string& id_selector, const bool single,
                          ApiCallBack callBack) {
  LOGI(" selectComponent " << component_id << " " << this);
  delegate_->SelectComponent(component_id, id_selector, single, callBack);
}

void App::InvokeUIMethod(tasm::NodeSelectRoot root,
                         tasm::NodeSelectOptions options, std::string method,
                         const piper::Value* params, ApiCallBack callback) {
  LOGI(" InvokeUIMethod with root: "
       << root.ToPrettyString() << ", node: " << options.ToString()
       << ", method: " << method << ", App: " << this);
  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt);
    auto piper_value = std::make_unique<pub::ValueImplPiper>(*rt, *params);
    auto prop_bundle = delegate_->CreatePropBundle();
    prop_bundle->SetProps(*piper_value);
    delegate_->InvokeUIMethod(std::move(root), std::move(options),
                              std::move(method), std::move(prop_bundle),
                              callback);
  }
}

void App::GetPathInfo(tasm::NodeSelectRoot root,
                      tasm::NodeSelectOptions options, ApiCallBack callBack) {
  LOGI(" GetPathInfo with root: " << root.ToPrettyString() << ", node: "
                                  << options.ToString() << ", App: " << this);
  delegate_->GetPathInfo(std::move(root), std::move(options), callBack);
}

void App::GetFields(tasm::NodeSelectRoot root, tasm::NodeSelectOptions options,
                    std::vector<std::string> fields, ApiCallBack call_back) {
  LOGI(" GetFields with root: " << root.ToPrettyString() << ", node: "
                                << options.ToString() << ", App: " << this);
  delegate_->GetFields(std::move(root), std::move(options), std::move(fields),
                       call_back);
}

void App::SetNativeProps(tasm::NodeSelectRoot root,
                         tasm::NodeSelectOptions options,
                         lepus::Value native_props) {
  tasm::PipelineOptions pipeline_options;
  delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                             pipeline_options.pipeline_origin,
                             pipeline_options.pipeline_start_timestamp);
  LOGI(" SetNativeProps with root: " << root.ToPrettyString()
                                     << ", node: " << options.ToString()
                                     << ", App: " << this);
  delegate_->SetNativeProps(std::move(root), std::move(options),
                            std::move(native_props),
                            std::move(pipeline_options));
}

void App::GetSessionStorageItem(const base::String& key,
                                const ApiCallBack& callback) {
  LOGI("JS: GetSessionStorageItem with key: " << key.str());
  delegate_->GetSessionStorageItem(key.str(), callback);
}

void App::SubscribeSessionStorage(const base::String& key, double listener_id,
                                  const ApiCallBack& callback) {
  LOGI("JS: SubscribeSessionStorage with key: " << key.str() << " listenerId: "
                                                << listener_id);
  delegate_->SubscribeSessionStorage(key.str(), listener_id, callback);
}

void App::ElementAnimate(const std::string& component_id,
                         const std::string& id_selector,
                         const lepus::Value& args) {
  LOGI(" element " << id_selector << " in " << component_id
                   << " exec element.Animate " << this);
  delegate_->ElementAnimate(component_id, id_selector, args);
}

void App::ReportException(common::JSErrorInfo error_info) {
  // fatal error should stop runloop to avoid more unreleated errors.
  if (error_info.error_level == base::LynxErrorLevel::Fatal) {
    state_ = State::kAppLoadFailed;
  }
  auto error = js_error_reporter_.SendBTError(error_info);
  if (error) {
    delegate_->OnErrorOccurred(std::move(*error));
  }
}

void App::AddReporterCustomInfo(
    const std::unordered_map<std::string, std::string>& info) {
  js_error_reporter_.AddCustomInfoToError(info);
}

std::shared_ptr<Runtime> App::GetRuntime() { return rt_.lock(); }

std::optional<lepus_value> App::ParseJSValueToLepusValue(
    const piper::Value& data, const std::string& component_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "JSValueToLepusValue");
  auto rt = rt_.lock();
  if (rt) {
    // only React dsl support parse js function
    // TT dsl don't support use js function as the prroperties of component
    JSValueCircularArray pre_object_vector;
    auto lepus_value_opt =
        ParseJSValue(*rt, data, jsi_object_wrapper_manager_.get(), component_id,
                     card_bundle_.target_sdk_version, pre_object_vector);
    if (!lepus_value_opt) {
      return std::optional<lepus_value>();
    }
    return lepus_value_opt;
  }
  return lepus::Value();
}

void App::ConsoleLogWithLevel(const std::string& level,
                              const std::string& msg) {
  auto rt = rt_.lock();
  if (rt) {
    Scope scope(*rt.get());
    piper::Object global = rt->global();
    auto console = global.getProperty(*rt, "nativeConsole");
    if (console && console->isObject()) {
      piper::Value msg_object(piper::String::createFromUtf8(*rt, msg));

      size_t count = 1;
      auto level_func =
          console->getObject(*rt).getPropertyAsFunction(*rt, level.c_str());
      if (!level_func) {
        return;
      }
      if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
        std::string msg_with_rid =
            "lepusRuntimeId:" + std::to_string(rt->getRuntimeId());
        piper::Value msg_with_rid_obj(
            piper::String::createFromUtf8(*rt, msg_with_rid));
        count = 2;
        const Value args[2] = {std::move(msg_with_rid_obj),
                               std::move(msg_object)};
        level_func->call(*rt, args, count);
      } else {
        const Value args[1] = {std::move(msg_object)};
        level_func->call(*rt, args, count);
      }
    }
  }
}

void App::I18nResourceChanged(const std::string& msg) { i18_resource_ = msg; }

void App::onPiperInvoked(const std::string& module_name,
                         const std::string& method_name) {
  if (tasm::LynxEnv::GetInstance().IsPiperMonitorEnabled()) {
    time_t timep;
    time(&timep);
    std::ostringstream time_s;
    time_s << timep;
    tasm::LynxEnv::onPiperInvoked(module_name, method_name, "", url_,
                                  time_s.str());
  }
}

void App::ReloadFromJS(const lepus::Value& value, ApiCallBack callback) {
  auto rt = rt_.lock();
  if (rt) {
    runtime::UpdateDataType update_data_type;
    tasm::PipelineOptions pipeline_options;
    delegate_->OnPipelineStart(pipeline_options.pipeline_id,
                               pipeline_options.pipeline_origin,
                               pipeline_options.pipeline_start_timestamp);
    const auto& timing_flag = tasm::GetTimingFlag(value);
    if (!timing_flag.empty()) {
      pipeline_options.need_timestamps = true;
      delegate_->BindPipelineIDWithTimingFlag(pipeline_options.pipeline_id,
                                              timing_flag);
      tasm::TimingCollector::Scope<runtime::TemplateDelegate> scope(
          delegate_, pipeline_options);
      tasm::TimingCollector::Instance()->Mark(
          tasm::timing::kReloadFromBackground);
    }
    runtime::UpdateDataTask task(true, PAGE_GROUP_ID, std::move(value),
                                 callback, update_data_type,
                                 std::move(pipeline_options));
    delegate_->ReloadFromJS(std::move(task));
  }
}

void App::CallLepusMethod(const std::string& method_name, lepus::Value args,
                          const ApiCallBack& callback, std::string stacks) {
  // This `trace_flow_id` is used to trace the flow of CallLepusMethod.
  // ApiCallBack's creation and invocation use different trace_flow_id
  // generated in ApiCallBack's constructor
  auto trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CallLepusMethodInner",
              [&](perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(trace_flow_id);
                ctx.event()->add_debug_annotations("method_name", method_name);
                ctx.event()->add_debug_annotations("stacks", stacks);
              });
  LOGI(" CallLepusMethod: " << method_name << " " << this);
  delegate_->CallLepusMethod(method_name, std::move(args), callback,
                             trace_flow_id);
}

void App::MarkTimingWithTimingFlag(const std::string& timing_flag,
                                   const std::string& key) {
  if (timing_flag.empty()) {
    return;
  }
  // This logic is to ensure compatibility with the old js_app markTiming API.
  // The old js_app markTiming API takes TimingFlag as a parameter and
  // uses it as the dimension for marking.
  // Now, we need to mark Timing using pipeline_id as the dimension.
  // However, The old js_app markTiming lacks the context related to
  // pipeline_id, so we can only mark using TimingFlag as the dimension. We will
  // additionally store this data using TimingFlag and later associate it. In
  // the long term, this API will be deprecated after most of the business
  // front-end frameworks are upgraded.
  static const base::NoDestructor<std::unordered_set<std::string>>
      supported_keys{{tasm::timing::kUpdateDiffVdomStart,
                      tasm::timing::kUpdateDiffVdomEnd,
                      tasm::timing::kUpdateSetStateTrigger}};
  if (supported_keys->find(key) != supported_keys->end()) {
    const auto us_timestamp = base::CurrentSystemTimeMicroseconds();
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY, nullptr,
        [&timing_flag, &key, us_timestamp](lynx::perfetto::EventContext ctx) {
          ctx.event()->set_name("Timing::MarkTimingWithTimingFlag." + key);
          ctx.event()->add_debug_annotations("timing_key", key);
          ctx.event()->add_debug_annotations("timing_flag", timing_flag);
          ctx.event()->add_debug_annotations("timestamp",
                                             std::to_string(us_timestamp));
        });

    delegate_->SetTimingWithTimingFlag(timing_flag, key, us_timestamp);
  }
}

void App::OnPipelineStart(const tasm::PipelineID& pipeline_id,
                          const tasm::PipelineOrigin& pipeline_origin) {
  auto us_timestamp = base::CurrentSystemTimeMicroseconds();
  delegate_->OnPipelineStart(pipeline_id, pipeline_origin, us_timestamp);
}

void App::BindPipelineIDWithTimingFlag(
    const tasm::PipelineID& pipeline_id,
    const tasm::timing::TimingFlag& timing_flag) {
  delegate_->BindPipelineIDWithTimingFlag(pipeline_id, timing_flag);
}

void App::MarkPipelineTiming(const tasm::PipelineID& pipeline_id,
                             const tasm::TimingKey& timing_key) {
  if (pipeline_id.empty() || timing_key.empty()) {
    return;
  }
  tasm::TimingCollector::Scope<runtime::TemplateDelegate> scope(delegate_,
                                                                pipeline_id);
  tasm::TimingCollector::Instance()->MarkFrameworkTiming(timing_key);
}

void App::PauseGcSuppressionMode() {
  if (!gc_pause_suppression_mode_) {
    gc_pause_suppression_mode_ =
        std::make_unique<GCPauseSuppressionMode>(GetRuntime().get());
  }
}

void App::ResumeGcSuppressionMode() {
  if (gc_pause_suppression_mode_) {
    gc_pause_suppression_mode_.reset();
  }
}

void App::OnStandaloneScriptAdded(const std::string& url, std::string source) {
  standalone_js_bundle_.AddJsContent(
      url, JsContent(std::move(source), JsContent::Type::SOURCE));
}

void App::OnSetPresetData(lepus::Value data) { preset_data_ = std::move(data); }

void App::OnComponentDecoded(tasm::TasmRuntimeBundle bundle) {
  std::string name = bundle.name;
  component_bundles_.emplace(std::move(name), std::move(bundle));
}

void App::OnCardConfigDataChanged(const lepus::Value& data) {
  card_config_ = data;
}

JsContent App::GetJSContent(const std::string& bundle_name,
                            const std::string& name, long timeout) {
  // get from TasmRuntimeBundles
  tasm::TasmRuntimeBundle* bundle = nullptr;
  if (bundle_name == tasm::DEFAULT_ENTRY_NAME) {
    bundle = &card_bundle_;
  } else {
    auto iter = component_bundles_.find(bundle_name);
    if (iter != component_bundles_.end()) {
      bundle = &iter->second;
    }
  }
  if (bundle != nullptr) {
    if (auto source = bundle->js_bundle.GetJsContent(name)) {
      return *source;
    }
  }

  // get from JsBundles
  auto js_bundle_iter = js_bundles_.find(bundle_name);
  if (js_bundle_iter != js_bundles_.end()) {
    if (auto source = js_bundle_iter->second.GetJsContent(name)) {
      return *source;
    }
  }

  // get from StandaloneJsBundle
  if (auto source = standalone_js_bundle_.GetJsContent(name)) {
    return *source;
  }

  // load from external
  return delegate_->GetJSContentFromExternal(bundle_name, name, timeout);
}

lepus::Value App::GetCustomSectionSync(const std::string& key) {
  return card_bundle_.custom_sections.GetProperty(key);
}

void App::SetSourceMapRelease(common::JSErrorInfo error_info) {
  js_error_reporter_.SetSourceMapRelease(std::move(error_info));
}

std::string App::GetSourceMapRelease(const std::string url) {
  return js_error_reporter_.GetSourceMapRelease(url);
}

piper::Value App::RequestAnimationFrame(piper::Function func) {
  auto rt = rt_.lock();
  if (!animation_frame_handler_ || !rt) {
    return piper::Value::undefined();
    //
  }
  // requestVSyncTick
  auto observer = delegate_->GetVSyncObserver();
  if (observer) {
    observer->RequestAnimationFrame(
        reinterpret_cast<uintptr_t>(this),
        [this](int64_t frame_start, int64_t frame_end) {
          DoFrame(frame_start);
        });
  }

  const int64_t id =
      animation_frame_handler_->RequestAnimationFrame(std::move(func));

  return piper::Value(static_cast<double>(id));
}

void App::CancelAnimationFrame(int64_t id) {
  if (animation_frame_handler_) {
    animation_frame_handler_->CancelAnimationFrame(id);
  }
}

void App::DoFrame(int64_t time_stamp) {
  static constexpr int64_t kNanoSecondsPerMilliSecond = 1e+6;
  if (animation_frame_handler_ && !has_paused_animation_frame_) {
    // W3C window.requestAnimationFrame request milliseconds
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "AnimationFrame", "timestamp", time_stamp);
    animation_frame_handler_->DoFrame(time_stamp / kNanoSecondsPerMilliSecond,
                                      rt_.lock().get());
    fluency_tracer_.Trigger(time_stamp);
  }
}

void App::PauseAnimationFrame() {
  if (animation_frame_handler_ &&
      animation_frame_handler_->HasPendingRequest()) {
    has_paused_animation_frame_ = true;
  }
}

void App::ResumeAnimationFrame() {
  auto observer = delegate_->GetVSyncObserver();
  if (has_paused_animation_frame_ && observer) {
    has_paused_animation_frame_ = false;
    observer->RequestAnimationFrame(
        reinterpret_cast<uintptr_t>(this),
        [this](int64_t frame_start, int64_t frame_end) {
          DoFrame(frame_start);
        });
  }
}

void App::SetJsBundleHolder(
    const std::weak_ptr<piper::JsBundleHolder>& holder) {
  weak_js_bundle_holder_ = holder;
}

}  // namespace piper
}  // namespace lynx
