// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/event/context_proxy_in_js.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/event/event_listener.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/js/bindings/event/js_event_listener.h"
#include "core/runtime/js/bindings/js_app.h"
#include "core/runtime/js/utils.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx {
namespace runtime {
namespace js {
namespace {

enum class PropType : int32_t {
  kPostMessage = 0,
  kDispatchEvent,
  kAddEventListener,
  kRemoveEventListener,
  kFunctionPropEnd,
  kOnTriggerEvent,
  kUnknown,
};

PropType ConvertPropStringToPropType(const std::string &str) {
  PropType type = PropType::kUnknown;
  if (str == runtime::kPostMessage) {
    type = PropType::kPostMessage;
  } else if (str == runtime::kDispatchEvent) {
    type = PropType::kDispatchEvent;
  } else if (str == runtime::kAddEventListener) {
    type = PropType::kAddEventListener;
  } else if (str == runtime::kRemoveEventListener) {
    type = PropType::kRemoveEventListener;
  } else if (str == runtime::kOnTriggerEvent) {
    type = PropType::kOnTriggerEvent;
  }
  return type;
}

}  // namespace

ContextProxyInJSImpl::ContextProxyInJSImpl(
    runtime::ContextProxy::Delegate &delegate,
    runtime::ContextProxy::Type target_type,
    base::UnsafeWeakPtr<App> native_app)
    : runtime::ContextProxy(delegate, runtime::ContextProxy::Type::kJSContext,
                            target_type),
      native_app_(std::move(native_app)) {}

void ContextProxyInJSImpl::ReportError(base::LynxError error) {
  auto *app = native_app_.Lock();
  if (!app) {
    return;
  }
  app->GetDelegate().OnErrorOccurred(std::move(error));
}

fml::RefPtr<runtime::MessageEvent> ContextProxyInJSImpl::CreateMessageEvent(
    Runtime &rt, const Value &event) {
  return fml::MakeRefCounted<runtime::MessageEvent>(
      event.getObject(rt)
          .getProperty(rt, runtime::kType)
          ->asString(rt)
          ->utf8(rt),
      GetOriginType(), GetTargetType(),
      std::make_unique<pub::ValueImplPiper>(
          rt, *(event.getObject(rt).getProperty(rt, runtime::kData))));
}

base::expected<Value, JSINativeException>
ContextProxyInJSImpl::PostMessageFromJS(Runtime &rt,
                                        const std::string &method_name,
                                        const Value *args, size_t count) {
  if (count < 1) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the args count must >= 1!"));
  }

  auto *app = native_app_.Lock();
  if (!app) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since native_app_ is nullptr!"));
  }

  auto option_value =
      app->ParseJSValueToLepusValue(std::move(args[0]), PAGE_GROUP_ID);
  if (!option_value.has_value()) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "ContextProxy's " + method_name +
        " failed, since convert arg0 to lepus::Value failed!"));
  }

  PostMessage(*option_value);
  return Value::undefined();
}

base::expected<Value, JSINativeException>
ContextProxyInJSImpl::DispatchEventFromJS(Runtime &rt,
                                          const std::string &method_name,
                                          const Value *args, size_t count) {
  if (count < 1) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the args count must >= 1!"));
  }

  auto *app = native_app_.Lock();
  if (!app) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since native_app_ is nullptr!"));
  }

  if (!args[0].isObject()) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since arg0 must be object!"));
  }

  auto event = args[0].getObject(rt);
  if (!event.hasProperty(rt, runtime::kType) ||
      !event.getProperty(rt, runtime::kType)->isString()) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "ContextProxy's " + method_name +
        " failed, since arg0 must contain type property and the value must "
        "be string!"));
  }

  if (!event.hasProperty(rt, runtime::kData)) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "ContextProxy's " + method_name +
        " failed, since arg0 must contain data property!"));
  }

  auto message_event = CreateMessageEvent(rt, event);
  auto result = DispatchEvent(std::move(message_event));
  return Value(static_cast<int>(result.cancel_type));
}

base::expected<Value, JSINativeException>
ContextProxyInJSImpl::AddEventListenerFromJS(Runtime &rt,
                                             const std::string &method_name,
                                             const Value *args, size_t count) {
  if (count < 2) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the args count must >= 2!"));
  }

  auto *app = native_app_.Lock();
  if (!app || app->IsDestroying()) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since native_app_ is nullptr!"));
  }

  if (!args[0].isString()) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the arg0 must be string!"));
  }

  if (!args[1].isObject() || !args[1].asObject(rt)->isFunction(rt)) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "ContextProxy's " + method_name +
        " failed, since the arg1 must be closure or function!"));
  }

  AddEventListener(
      args[0].asString(rt)->utf8(rt),
      std::make_unique<JSClosureEventListener>(native_app_, args[1]));
  return Value::undefined();
}

base::expected<Value, JSINativeException>
ContextProxyInJSImpl::RemoveEventListenerFromJS(Runtime &rt,
                                                const std::string &method_name,
                                                const Value *args,
                                                size_t count) {
  if (count < 2) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the args count must >= 2!"));
  }

  auto *app = native_app_.Lock();
  if (!app) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since native_app_ is nullptr!"));
  }

  if (!args[0].isString()) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("ContextProxy's " + method_name +
                                   " failed, since the arg0 must be string!"));
  }

  if (!args[1].isObject() || !args[1].asObject(rt)->isFunction(rt)) {
    return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
        "ContextProxy's " + method_name +
        " failed, since the arg1 must be closure or function!"));
  }

  RemoveEventListener(
      args[0].asString(rt)->utf8(rt),
      std::make_unique<JSClosureEventListener>(native_app_, args[1]));
  return Value::undefined();
}

Value ContextProxyInJSImpl::GetOnTriggerEvent(Runtime &rt) {
  if (event_listener_ == nullptr ||
      event_listener_->type() !=
          event::EventListener::Type::kJSClosureEventListener) {
    return Value::undefined();
  }

  auto *event_listener_ptr =
      static_cast<JSClosureEventListener *>(event_listener_.get());
  return event_listener_ptr->GetClosure();
}

void ContextProxyInJSImpl::SetOnTriggerEvent(Runtime &rt, const Value &value) {
  auto *app = native_app_.Lock();
  if (!app || app->IsDestroying()) {
    return;
  }

  SetListenerBeforePublishEvent(
      std::make_unique<JSClosureEventListener>(native_app_, Value(rt, value)));
}

ContextProxyInJS::ContextProxyInJS(runtime::ContextProxy::Type type,
                                   base::UnsafeWeakPtr<App> native_app)
    : type_(type), native_app_(std::move(native_app)) {}

ContextProxyInJSImpl *ContextProxyInJS::GetContextProxyImpl() const {
  auto *app = native_app_.Lock();
  if (!app) {
    return nullptr;
  }
  return app->GetOrCreateContextProxyImpl(type_);
}

Value ContextProxyInJS::get(Runtime *rt, const PropNameID &name) {
  if (rt == nullptr) {
    return Value::undefined();
  }

  auto method_name = name.utf8(*rt);
  auto type = ConvertPropStringToPropType(method_name);
  if (type < PropType::kPostMessage || type >= PropType::kUnknown) {
    return Value::undefined();
  }

  if (type < PropType::kFunctionPropEnd) {
    auto type_for_host_object = type_;
    auto native_app = native_app_;
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, method_name), 0,
        [native_app, type_for_host_object, method_name = std::move(method_name),
         type](Runtime &rt, const Value &this_val, const Value *args,
               size_t count) -> base::expected<Value, JSINativeException> {
          auto *app = native_app.Lock();
          if (!app) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ContextProxy's " + method_name +
                " failed, since native_app_ is nullptr!"));
          }

          auto *impl = app->GetOrCreateContextProxyImpl(type_for_host_object);
          if (!impl) {
            return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                "ContextProxy's " + method_name +
                " failed, since context proxy impl is nullptr!"));
          }

          if (type == PropType::kPostMessage) {
            return impl->PostMessageFromJS(rt, method_name, args, count);
          }
          if (type == PropType::kDispatchEvent) {
            return impl->DispatchEventFromJS(rt, method_name, args, count);
          }
          if (type == PropType::kAddEventListener) {
            return impl->AddEventListenerFromJS(rt, method_name, args, count);
          }
          if (type == PropType::kRemoveEventListener) {
            return impl->RemoveEventListenerFromJS(rt, method_name, args,
                                                   count);
          }
          return Value::undefined();
        });
  }

  if (type == PropType::kOnTriggerEvent) {
    auto *impl = GetContextProxyImpl();
    return impl ? impl->GetOnTriggerEvent(*rt) : Value::undefined();
  }
  return Value::undefined();
}

void ContextProxyInJS::set(Runtime *rt, const PropNameID &name,
                           const Value &value) {
  if (rt == nullptr) {
    return;
  }

  auto *impl = GetContextProxyImpl();
  if (!impl) {
    return;
  }

  auto name_str = name.utf8(*rt);
  if (name_str == runtime::kOnTriggerEvent) {
    impl->SetOnTriggerEvent(*rt, value);
  }
}

std::vector<PropNameID> ContextProxyInJS::getPropertyNames(Runtime &rt) {
  std::vector<PropNameID> vec;
  vec.push_back(PropNameID::forUtf8(rt, runtime::kPostMessage));
  vec.push_back(PropNameID::forUtf8(rt, runtime::kDispatchEvent));
  vec.push_back(PropNameID::forUtf8(rt, runtime::kAddEventListener));
  vec.push_back(PropNameID::forUtf8(rt, runtime::kRemoveEventListener));
  vec.push_back(PropNameID::forUtf8(rt, runtime::kOnTriggerEvent));
  return vec;
}

runtime::ContextProxy::Type ContextProxyInJS::GetTargetType() const {
  return type_;
}

}  // namespace js

}  // namespace runtime
}  // namespace lynx
