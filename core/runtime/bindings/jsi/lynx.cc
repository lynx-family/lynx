// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/lynx.h"

#include <string>
#include <utility>

#include "base/include/expected.h"
#include "base/include/log/logging.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/jsi/java_script_element.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/jsi/jsi.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace piper {
Value LynxProxy::get(lynx::piper::Runtime *rt,
                     const lynx::piper::PropNameID &name) {
  auto methodName = name.utf8(*rt);
  if (methodName == "__globalProps") {
    auto native_app = native_app_.lock();
    if (!native_app) {
      return piper::Value::undefined();
    }
    auto global_props_opt = native_app->getInitGlobalProps();
    if (!global_props_opt) {
      // TODO(wujintian): return optional here.
      return piper::Value::undefined();
    }
    return std::move(*global_props_opt);
  }

  if (methodName == "__presetData") {
    auto native_app = native_app_.lock();
    if (!native_app) {
      return piper::Value::undefined();
    }
    auto data_opt = native_app->getPresetData();
    if (!data_opt) {
      return piper::Value::undefined();
    }
    return std::move(*data_opt);
  }

  if (methodName == "getI18nResource") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getI18nResource"), 0,
        [this](Runtime &rt, const piper::Value &this_val,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto native_app = native_app_.lock();
          if (!native_app) {
            return piper::Value::undefined();
          }
          return native_app->getI18nResource();
        });
  }

  if (methodName == "getComponentContext") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "getComponentContext"), 3,
        [this](Runtime &rt, const piper::Value &this_val,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 3) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.getComponentContext args count must be 3"));
          }
          auto ptr = native_app_.lock();
          if (ptr && !ptr->IsDestroying()) {
            std::string id;
            if (args[0].isString()) {
              id = args[0].getString(rt).utf8(rt);
            }
            std::string key;
            if (args[1].isString()) {
              key = args[1].getString(rt).utf8(rt);
            }
            ApiCallBack callback;
            if (args[2].isObject() && args[2].getObject(rt).isFunction(rt)) {
              callback =
                  ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));
            }
            ptr->getContextDataAsync(id, key, callback);
          }
          return piper::Value::undefined();
        });
  }

  if (methodName == "createElement") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "createElement"), 2,
        [this](Runtime &rt, const piper::Value &this_val,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.createNativeElement args count must be 1"));
          }
          std::string root_id;
          if (args[0].isString()) {
            root_id = args[0].getString(rt).utf8(rt);
          }
          std::string id;
          if (args[1].isString()) {
            id = args[1].getString(rt).utf8(rt);
          }
          return piper::Value(Object::createFromHostObject(
              rt, std::make_shared<JavaScriptElement>(rt_, native_app_, root_id,
                                                      id)));
        });
  }

  if (methodName == "fetchDynamicComponent") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "fetchDynamicComponent"), 3,

        [this](Runtime &rt, const piper::Value &thisVal,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          if (count < 3) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.fetchDynamicComponent args count must "
                "be no less than 3"));
          }

          auto ptr = native_app_.lock();
          if (ptr) {
            std::string url;
            ApiCallBack callback;

            if (!args[0].isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "lynx.fetchDynamicComponent args0 must be string"));
            }
            url = args[0].getString(rt).utf8(rt);

            if (!args[2].isObject() || !args[2].getObject(rt).isFunction(rt)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "lynx.fetchDynamicComponent args2 must be ApiCallBack"));
            }
            callback =
                ptr->CreateCallBack(args[2].getObject(rt).getFunction(rt));

            std::vector<std::string> ids;

            // args[3] is the ids of the dynamic components which should be
            // dispatched after loading.
            if (count > 3) {
              if (!ConvertPiperValueToStringVector(rt, args[3], ids)) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "lynx.fetchDynamicComponent args3 must be string[]"));
              }
            }

            ptr->QueryComponent(url, callback, ids);
          }

          return piper::Value::undefined();
        });
  }

  // js reload api
  if (methodName == "reload") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "reload"), 2,
        [this](Runtime &rt, const piper::Value &thisVal,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (ptr) {
            lepus::Value value(lepus::Dictionary::Create());
            ApiCallBack callback;
            if (count > 0 && args[0].isObject()) {
              if (!args[0].isObject()) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "lynx.reload's first params must be object."));
              }
              auto lepus_value_opt = ptr->ParseJSValueToLepusValue(
                  std::move(args[0]), PAGE_GROUP_ID);
              if (!lepus_value_opt) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "ParseJSValueToLepusValue error in lynx.reload."));
              }
              if (!lepus_value_opt->IsObject()) {
                return piper::Value::undefined();
              }
              value = std::move(*lepus_value_opt);
            }

            if (count > 1 && args[1].isObject() &&
                args[1].getObject(rt).isFunction(rt)) {
              if (!args[1].isObject() ||
                  !args[1].getObject(rt).isFunction(rt)) {
                return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                    "lynx.reload's second params must be function."));
              }
              // lynx.reload has one optional callback param.
              callback =
                  ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
            }
            ptr->ReloadFromJS(value, callback);
          }
          return piper::Value::undefined();
        });
  }

  if (methodName == "QueryComponent") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "QueryComponent"), 2,
        [this](Runtime &rt, const piper::Value &thisVal,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (ptr) {
            std::string url;
            ApiCallBack callback;
            if (!args[0].isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "lynx.QueryComponent's first params must be String."));
            }
            url = args[0].getString(rt).utf8(rt);
            if (!args[1].isObject() || !args[1].getObject(rt).isFunction(rt)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "lynx.QueryComponent's second params must be function."));
            }

            callback =
                ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
            ptr->QueryComponent(url, callback, {});
          }

          return piper::Value::undefined();
        });
  }

  if (methodName == "addFont") {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, "addFont"), 2,
        [this](
            Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          auto ptr = native_app_.lock();
          if (!ptr) {
            return piper::Value::undefined();
          }
          if (count != 2) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx.addFont's args count must be 2."));
          }
          if (!args[0].isObject()) {
            return piper::Value::undefined();
          }
          auto lepus_value_opt =
              ptr->ParseJSValueToLepusValue(std::move(args[0]), PAGE_GROUP_ID);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in lynx.addFont."));
          }
          if (!lepus_value_opt->IsObject()) {
            return piper::Value::undefined();
          }
          lepus::Value value = std::move(*lepus_value_opt);
          if (!args[1].isObject() || !args[1].getObject(rt).isFunction(rt)) {
            return piper::Value::undefined();
          }
          ApiCallBack callback =
              ptr->CreateCallBack(args[1].getObject(rt).getFunction(rt));
          ptr->AddFont(value, std::move(callback));

          return piper::Value::undefined();
        });
  }

  if (methodName == runtime::kGetDevTool ||
      methodName == runtime::kGetCoreContext ||
      methodName == runtime::kGetJSContext ||
      methodName == runtime::kGetUIContext) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, methodName), 0,
        [this, methodName = std::move(methodName)](
            Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          auto app = native_app_.lock();
          if (!app) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx." + methodName +
                " failed, since native_app_ is nullptr"));
          }

          auto type = runtime::ContextProxy::Type::kUnknown;
          if (methodName == runtime::kGetDevTool) {
            type = runtime::ContextProxy::Type::kDevTool;
          } else if (methodName == runtime::kGetJSContext) {
            type = runtime::ContextProxy::Type::kJSContext;
          } else if (methodName == runtime::kGetCoreContext) {
            type = runtime::ContextProxy::Type::kCoreContext;
          } else if (methodName == runtime::kGetUIContext) {
            type = runtime::ContextProxy::Type::kUIContext;
          }

          auto proxy = app->GetContextProxy(type);
          if (proxy == nullptr) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "lynx." + methodName +
                " failed, since GetContextProxy return nullptr"));
          }

          return piper::Object::createFromHostObject(rt, proxy);
        });
  }

  if (methodName == runtime::kGetCustomSectionSync) {
    return GetCustomSectionSync(*rt, methodName.c_str());
  }

  if (methodName == runtime::kQueueMicrotask) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, runtime::kQueueMicrotask), 1,
        [this](Runtime &rt, const piper::Value &thisVal,
               const piper::Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          LOGV("LYNX App get -> queueMicrotask");

          if (count != 1) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "queueMicrotask args count must = 1"));
          }
          auto native_app = native_app_.lock();
          if (!native_app || native_app->IsDestroying()) {
            return piper::Value::undefined();
          }
          bool success = false;
          if (args[0].isObject()) {
            auto maybe_callback = args[0].getObject(rt);
            if (maybe_callback.isFunction(rt)) {
              auto callback = maybe_callback.asFunction(rt);
              native_app->QueueMicrotask(std::move(*callback));
              success = true;
            }
          }
          if (!success) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "queueMicrotask args[0] isn't a function."));
          }

          return piper::Value::undefined();
        });
  }

  return piper::Value::undefined();
}

piper::Value LynxProxy::GetCustomSectionSync(Runtime &rt,
                                             const char *prop_name) {
  return Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, prop_name), 1,
      [this](Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
             size_t count) -> base::expected<Value, JSINativeException> {
        if (count < 1) {
          return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
              std::string(runtime::kGetCustomSectionSync) +
              "'s args count must be 1."));
        }

        auto native_app = native_app_.lock();
        if (native_app) {
          std::string url;
          if (!args[0].isString()) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                std::string(runtime::kGetCustomSectionSync) +
                "'s first params must be String."));
          }
          auto key = args[0].getString(rt).utf8(rt);
          piper::Value res =
              *valueFromLepus(rt, native_app->GetCustomSectionSync(key));
          return res;
        }

        return piper::Value::undefined();
      });
}

void LynxProxy::set(Runtime *, const PropNameID &name, const Value &value) {}

std::vector<PropNameID> LynxProxy::getPropertyNames(Runtime &rt) {
  static const char *kProps[] = {
      "__globalProps",
      "__presetData",
      "getI18nResource",
      "getComponentContext",
      "createElement",
      "fetchDynamicComponent",
      "reload",
      "QueryComponent",
      "addFont",
      tasm::kGetTextInfo,
      runtime::kGetDevTool,
      runtime::kGetJSContext,
      runtime::kGetCoreContext,
      runtime::kGetUIContext,
      runtime::kGetCustomSectionSync,
      runtime::kQueueMicrotask,
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

}  // namespace piper
}  // namespace lynx
