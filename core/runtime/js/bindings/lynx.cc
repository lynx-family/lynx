// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/lynx.h"

#include <string>
#include <utility>

#include "base/include/expected.h"
#include "base/include/log/logging.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/resource/lazy_bundle/bundle_resource_info.h"
#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/common/bindings/resource/response_promise.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/js/bindings/java_script_element.h"
#include "core/runtime/js/bindings/resource/response_handler_in_js.h"
#include "core/runtime/js/jsi/jsi.h"
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
              rt,
              std::make_shared<JavaScriptElement>(native_app_, root_id, id)));
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
      methodName == runtime::kGetUIContext ||
      methodName == runtime::kGetNative || methodName == runtime::kGetEngine) {
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
          } else if (methodName == runtime::kGetNative) {
            type = runtime::ContextProxy::Type::kNative;
          } else if (methodName == runtime::kGetEngine) {
            type = runtime::ContextProxy::Type::kEngine;
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
          return Value::undefined();
        });
  }

  if (methodName == tasm::kLoadScript) {
    return LoadScript(*rt);
  }

  if (methodName == tasm::kFetchBundle) {
    return FetchBundle(*rt);
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
              "'s args must has 'key' argument."));
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
          std::string bundle_name = LEPUS_DEFAULT_CONTEXT_NAME;
          if (count > 1) {
            if (!args[1].isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  std::string(runtime::kGetCustomSectionSync) +
                  "'s second params must be String."));
            }
            bundle_name = args[1].getString(rt).utf8(rt);
          }
          piper::Value res = *valueFromLepus(
              rt, native_app->GetCustomSectionSync(key, bundle_name));
          return res;
        }

        return piper::Value::undefined();
      });
}

piper::Value LynxProxy::LoadScript(Runtime &rt) {
  return Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, tasm::kLoadScript), 1,
      [this](Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
             size_t count) -> base::expected<Value, JSINativeException> {
        auto native_app = native_app_.lock();
        if (!native_app || native_app->IsDestroying()) {
          return piper::Value::undefined();
        }
        if (count < 1) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION(std::string(tasm::kLoadScript) +
                                         "'s args must has 'key' argument."));
        }
        if (!args[0].isString()) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION(std::string(tasm::kLoadScript) +
                                         "'s first param must be string."));
        }
        auto key = args[0].getString(rt).utf8(rt);
        std::string bundle_name = LEPUS_DEFAULT_CONTEXT_NAME;
        bool use_module_wrapper = false;
        if (count > 1 && args[1].isObject()) {
          auto maybe_bundle_name = args[1].getObject(rt).getProperty(
              rt, piper::PropNameID::forAscii(rt, "bundleName"));
          if (maybe_bundle_name && maybe_bundle_name->isString()) {
            bundle_name = maybe_bundle_name->getString(rt).utf8(rt);
          }
          auto maybe_use_module_wrapper = args[1].getObject(rt).getProperty(
              rt, piper::PropNameID::forAscii(rt, "useModuleWrapper"));
          if (maybe_use_module_wrapper && maybe_use_module_wrapper->isBool()) {
            use_module_wrapper = maybe_use_module_wrapper->getBool();
          }
        }
        return native_app->LoadCustomSectionScript(key, bundle_name,
                                                   use_module_wrapper);
      });
}

piper::Value LynxProxy::FetchBundle(Runtime &rt) {
  return Function::createFromHostFunction(
      rt, PropNameID::forAscii(rt, tasm::kFetchBundle), 1,
      [this](Runtime &rt, const piper::Value &thisVal, const piper::Value *args,
             size_t count) -> base::expected<Value, JSINativeException> {
        auto native_app = native_app_.lock();
        if (!native_app || native_app->IsDestroying()) {
          return piper::Value::undefined();
        }

        if (count < 1) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION(std::string(tasm::kFetchBundle) +
                                         "'s args must has 'url' argument."));
        }

        if (!args[0].isString()) {
          return base::unexpected(
              BUILD_JSI_NATIVE_EXCEPTION(std::string(tasm::kFetchBundle) +
                                         "'s first param must be a string."));
        }

        auto bundle_url = args[0].getString(rt).utf8(rt);
        lepus::Value options;
        if (count > 1 && args[1].isObject()) {
          auto lepus_value_opt = native_app->ParseJSValueToLepusValue(
              std::move(args[1]), PAGE_GROUP_ID);
          if (!lepus_value_opt) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ParseJSValueToLepusValue error in FetchBundle."));
          }
          options = std::move(*lepus_value_opt);
        }

        auto response_promise = std::make_shared<
            runtime::ResponsePromise<tasm::BundleResourceInfo>>();
        // invoke fetchBundle & passing ResponsePromise to retrieve result.
        native_app->FetchBundle(bundle_url, response_promise);
        auto promise = std::make_shared<ResponseHandlerInJS>(
            native_app->GetDelegate(), bundle_url, std::move(response_promise),
            native_app_);
        return piper::Object::createFromHostObject(rt, promise);
      });
}

void LynxProxy::set(Runtime *, const PropNameID &name, const Value &value) {}

std::vector<PropNameID> LynxProxy::getPropertyNames(Runtime &rt) {
  static const char *kProps[] = {"__globalProps",
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
                                 runtime::kGetNative,
                                 runtime::kGetEngine,
                                 runtime::kGetCustomSectionSync,
                                 runtime::kQueueMicrotask,
                                 tasm::kLoadScript,
                                 tasm::kFetchBundle};
  static constexpr size_t kPropsCount = sizeof(kProps) / sizeof(kProps[0]);

  std::vector<PropNameID> vec;
  vec.reserve(kPropsCount);
  for (auto &kProp : kProps) {
    vec.push_back(piper::PropNameID::forAscii(rt, kProp, std::strlen(kProp)));
  }
  return vec;
}

}  // namespace piper
}  // namespace lynx
