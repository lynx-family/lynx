// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/event/context_proxy_in_js.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/event/event_listener.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/bindings/jsi/event/js_event_listener.h"
#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/common/utils.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace piper {

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

ContextProxyInJS::ContextProxyInJS(runtime::ContextProxy::Delegate &delegate,
                                   runtime::ContextProxy::Type target_type,
                                   std::weak_ptr<Runtime> rt,
                                   std::weak_ptr<App> native_app)
    : runtime::ContextProxy(delegate, runtime::ContextProxy::Type::kJSContext,
                            target_type),
      rt_(rt),
      native_app_(native_app) {}

runtime::MessageEvent ContextProxyInJS::CreateMessageEvent(
    Runtime &rt, std::shared_ptr<App> native_app, const piper::Value &event) {
  return runtime::MessageEvent(
      event.getObject(rt)
          .getProperty(rt, runtime::kType)
          ->asString(rt)
          ->utf8(rt),
      GetOriginType(), GetTargetType(),
      *(native_app->ParseJSValueToLepusValue(
          *(event.getObject(rt).getProperty(rt, runtime::kData)),
          PAGE_GROUP_ID)));
}

Value ContextProxyInJS::get(Runtime *rt, const PropNameID &name) {
  if (rt == nullptr) {
    return piper::Value::undefined();
  }

  auto method_name = name.utf8(*rt);
  auto type = ConvertPropStringToPropType(name.utf8(*rt));
  if (type < PropType::kPostMessage || type >= PropType::kUnknown) {
    return piper::Value::undefined();
  }

  if (type < PropType::kFunctionPropEnd) {
    return Function::createFromHostFunction(
        *rt, PropNameID::forAscii(*rt, method_name), 0,
        [this, method_name = std::move(method_name), type](
            Runtime &rt, const piper::Value &this_val, const piper::Value *args,
            size_t count) -> base::expected<piper::Value, JSINativeException> {
          if (type == PropType::kPostMessage) {
            if (count < 1) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the args count must >= 1!"));
            }

            auto app = native_app_.lock();
            if (!app) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since native_app_ is nullptr!"));
            }
            auto option_value = app->ParseJSValueToLepusValue(
                std::move(args[0]), PAGE_GROUP_ID);
            if (!option_value.has_value()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since convert arg0 to lepus::Value failed!"));
            }

            PostMessage(*option_value);
            return piper::Value::undefined();
          } else if (type == PropType::kDispatchEvent) {
            if (count < 1) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the args count must >= 1!"));
            }

            auto app = native_app_.lock();
            if (!app) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since native_app_ is nullptr!"));
            }

            if (!args[0].isObject()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since arg0 must be object!"));
            }

            auto event = args[0].getObject(rt);
            if (!event.hasProperty(rt, runtime::kType) ||
                !event.getProperty(rt, runtime::kType)->isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since arg0 must contain type "
                  "property and the value must be string!"));
            }

            if (!event.hasProperty(rt, runtime::kData)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since arg0 must contain data property!"));
            }

            auto message_event = CreateMessageEvent(rt, app, event);
            auto success = DispatchEvent(message_event);
            return piper::Value(static_cast<int>(success));
          } else if (type == PropType::kAddEventListener) {
            if (count < 2) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the args count must >= 2!"));
            }

            auto app = native_app_.lock();
            if (!app) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since native_app_ is nullptr!"));
            }

            if (!args[0].isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the arg0 must be string!"));
            }

            if (!args[1].isObject() || !args[1].asObject(rt)->isFunction(rt)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the arg1 must be closure or function!"));
            }

            AddEventListener(args[0].asString(rt)->utf8(rt),
                             std::make_unique<JSClosureEventListener>(
                                 rt_.lock(), native_app_.lock(), args[1]));

            return piper::Value::undefined();
          } else if (type == PropType::kRemoveEventListener) {
            if (count < 2) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the args count must >= 2!"));
            }

            auto app = native_app_.lock();
            if (!app) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since native_app_ is nullptr!"));
            }

            if (!args[0].isString()) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the arg0 must be string!"));
            }

            if (!args[1].isObject() || !args[1].asObject(rt)->isFunction(rt)) {
              return base::unexpected(BUILD_JSI_NATIVE_EXCEPTION(
                  "ContextProxy's " + method_name +
                  " failed, since the arg1 must be closure or function!"));
            }

            RemoveEventListener(args[0].asString(rt)->utf8(rt),
                                std::make_unique<JSClosureEventListener>(
                                    rt_.lock(), native_app_.lock(), args[1]));

            return piper::Value::undefined();
          }
          return piper::Value::undefined();
        });
  }
  if (type == PropType::kOnTriggerEvent) {
    if (event_listener_ == nullptr ||
        event_listener_->type() !=
            event::EventListener::Type::kJSClosureEventListener) {
      return piper::Value::undefined();
    }

    auto event_listener_ptr =
        static_cast<JSClosureEventListener *>(event_listener_.get());
    return event_listener_ptr->GetClosure();
  }
  return piper::Value::undefined();
}

void ContextProxyInJS::set(Runtime *rt, const PropNameID &name,
                           const Value &value) {
  if (rt == nullptr) {
    return;
  }

  auto name_str = name.utf8(*rt);
  if (name_str == runtime::kOnTriggerEvent) {
    SetListenerBeforePublishEvent(std::make_unique<JSClosureEventListener>(
        rt_.lock(), native_app_.lock(), piper::Value(*rt, value)));
  }
  return;
}

std::vector<PropNameID> ContextProxyInJS::getPropertyNames(Runtime &rt) {
  std::vector<PropNameID> vec;
  vec.push_back(piper::PropNameID::forUtf8(rt, runtime::kPostMessage));
  vec.push_back(piper::PropNameID::forUtf8(rt, runtime::kDispatchEvent));
  vec.push_back(piper::PropNameID::forUtf8(rt, runtime::kAddEventListener));
  vec.push_back(piper::PropNameID::forUtf8(rt, runtime::kRemoveEventListener));
  vec.push_back(piper::PropNameID::forUtf8(rt, runtime::kOnTriggerEvent));
  return vec;
}

}  // namespace piper
}  // namespace lynx
