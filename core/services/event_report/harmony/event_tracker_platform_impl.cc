// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/event_report/event_tracker_platform_impl.h"

#include "base/include/platform/harmony/napi_util.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/services/event_report/harmony/event_tracker_harmony.h"

namespace lynx {
namespace tasm {
namespace report {
namespace harmony {
using GenericInfoMapBuilder = base::MoveOnlyClosure<void, napi_env, napi_value>;

static napi_env env_{nullptr};
static napi_ref js_self_ref_{nullptr};
static napi_ref js_on_event_func_{nullptr};
static napi_ref js_update_generic_info_func_{nullptr};
static napi_ref js_clear_cache_func_{nullptr};

napi_value RegisterJSMethods(napi_env env, napi_callback_info info) {
  napi_value js_object;
  /**
   * 0 - js object ref
   * 1 - onEvent js function ref
   * 2 - updateGenericInfo js function ref
   * 3 - clearCache js function ref
   */
  size_t argc = 4;
  napi_value argv[argc];
  env_ = env;
  napi_get_cb_info(env, info, &argc, argv, &js_object, nullptr);
  napi_create_reference(env, argv[0], 0, &js_self_ref_);
  napi_create_reference(env, argv[1], 0, &js_on_event_func_);
  napi_create_reference(env, argv[2], 0, &js_update_generic_info_func_);
  napi_create_reference(env, argv[3], 0, &js_clear_cache_func_);
  return js_object;
}

napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {{"registerJSMethods", nullptr,
                                            RegisterJSMethods, nullptr, nullptr,
                                            nullptr, napi_static, nullptr}};

  napi_value constructor;
  napi_status status = napi_define_class(
      env, "EventReporter", NAPI_AUTO_LENGTH, RegisterJSMethods, nullptr,
      sizeof(properties) / sizeof(properties[0]), properties, &constructor);
  if (status != napi_ok) {
    napi_throw_error(env, nullptr, "Failed to define class");
    return nullptr;
  }
  status = napi_set_named_property(env, exports, "EventReporter", constructor);
  if (status != napi_ok) {
    napi_throw_error(env, nullptr, "Failed to set named property");
    return nullptr;
  }
  return exports;
}

napi_status CallJSUpdateGenericInfo(int32_t instance_id,
                                    GenericInfoMapBuilder builder) {
  if (!env_ || !js_self_ref_ || !js_update_generic_info_func_) {
    napi_throw_error(
        env_, nullptr,
        "env_, js_self_ref_ or js_update_generic_info_func_ not found.");
    return napi_invalid_arg;
  }
  base::NapiHandleScope scope(env_);
  /**
   * LynxEventReporter.updateGenericInfoCallByNative(instanceId, genericInfo);
   *
   * arg[0] - instance_id (number)
   * arg[1] - genericInfo (Record<string, LynxReportEventPropValue>)
   */
  size_t argc = 2;
  napi_value argv[argc];
  // arg[0] - instance_id (number)
  argv[0] = base::NapiUtil::CreateInt32(env_, instance_id);
  // arg[1] - genericInfo (Record<string, LynxReportEventPropValue>)
  napi_create_object(env_, &argv[1]);
  builder(env_, argv[1]);
  // Call JS Method :
  // LynxEventReporter.onEventCallByNative(instanceId, eventName, props)
  return base::NapiUtil::InvokeJsMethod(
      env_, js_self_ref_, js_update_generic_info_func_, argc, argv);
}

void DoReportEvent(MoveOnlyEvent&& event) {
  /**
   * LynxEventReporter.onEventCallByNative(instanceId, eventName,
   * props);
   *
   * arg[0] - instance_id (number)
   * arg[1] - event name (string)
   * arg[2] - props (Record<string, Object>)
   */
  assert(event.IsValidInstanceId());
  size_t argc = 3;
  napi_value argv[argc];
  // arg[0] - instance_id (number)
  argv[0] = base::NapiUtil::CreateInt32(harmony::env_, event.GetInstanceId());
  // arg[1] - event name
  auto& event_name = event.GetName();
  napi_create_string_utf8(harmony::env_, event_name.c_str(),
                          event_name.length(), &argv[1]);
  // arg[2] - props
  napi_create_object(harmony::env_, &argv[2]);
  for (const auto& prop : event.GetProps()) {
    switch (prop.GetType()) {
      case EventProp::Type::kString:
        base::NapiUtil::SetPropToJSMap(harmony::env_, argv[2], prop.GetKey(),
                                       prop.GetStringValue());
        break;
      case EventProp::Type::kInt32:
        base::NapiUtil::SetPropToJSMap(harmony::env_, argv[2], prop.GetKey(),
                                       prop.GetIntValue());
        break;
      case EventProp::Type::kDouble:
        base::NapiUtil::SetPropToJSMap(harmony::env_, argv[2], prop.GetKey(),
                                       prop.GetDoubleValue());
        break;
    }
  }
  // Call JS Method :
  // LynxEventReporter.onEventCallByNative(instanceId, eventName, props)
  base::NapiUtil::InvokeJsMethod(harmony::env_, harmony::js_self_ref_,
                                 harmony::js_on_event_func_, argc, argv);
}
}  // namespace harmony

void EventTrackerPlatformImpl::OnEvent(MoveOnlyEvent&& event) {
  base::UIThread::GetRunner()->PostTask([event = std::move(event)]() mutable {
    if (!harmony::env_ || !harmony::js_self_ref_ ||
        !harmony::js_on_event_func_) {
      napi_throw_error(harmony::env_, nullptr,
                       "env_, js_self_ref_ or js_on_event_func_ not found.");
      return;
    }
    base::NapiHandleScope scope(harmony::env_);
    harmony::DoReportEvent(std::move(event));
  });
}

void EventTrackerPlatformImpl::OnEvents(std::vector<MoveOnlyEvent> stack) {
  base::UIThread::GetRunner()->PostTask([stack = std::move(stack)]() mutable {
    if (!harmony::env_ || !harmony::js_self_ref_ ||
        !harmony::js_on_event_func_) {
      napi_throw_error(harmony::env_, nullptr,
                       "env_, js_self_ref_ or js_on_event_func_ not found.");
      return;
    }
    base::NapiHandleScope scope(harmony::env_);
    for (auto& event : stack) {
      harmony::DoReportEvent(std::move(event));
    }
  });
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id,
    std::unordered_map<std::string, std::string> generic_info) {
  base::UIThread::GetRunner()->PostTask(
      [instance_id, generic_info = std::move(generic_info)]() mutable {
        harmony::CallJSUpdateGenericInfo(
            instance_id, [&generic_info](napi_env env, napi_value map) {
              for (auto const& [key, value] : generic_info) {
                base::NapiUtil::SetPropToJSMap(env, map, key, value);
              }
            });
      });
}

void EventTrackerPlatformImpl::UpdateGenericInfo(
    int32_t instance_id, std::unordered_map<std::string, float> generic_info) {
  base::UIThread::GetRunner()->PostTask(
      [instance_id, generic_info = std::move(generic_info)]() mutable {
        harmony::CallJSUpdateGenericInfo(
            instance_id, [&generic_info](napi_env env, napi_value map) {
              for (auto const& [key, value] : generic_info) {
                base::NapiUtil::SetPropToJSMap(env, map, key, value);
              }
            });
      });
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const std::string& value) {
  base::UIThread::GetRunner()->PostTask([instance_id, key, value]() {
    harmony::CallJSUpdateGenericInfo(
        instance_id, [&key, value](napi_env env, napi_value map) {
          base::NapiUtil::SetPropToJSMap(env, map, key, value);
        });
  });
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 const float value) {
  base::UIThread::GetRunner()->PostTask([instance_id, key, value]() {
    harmony::CallJSUpdateGenericInfo(
        instance_id, [&key, value](napi_env env, napi_value map) {
          base::NapiUtil::SetPropToJSMap(env, map, key, value);
        });
  });
}

void EventTrackerPlatformImpl::UpdateGenericInfo(int32_t instance_id,
                                                 const std::string& key,
                                                 int64_t value) {
  base::UIThread::GetRunner()->PostTask([instance_id, key, value]() {
    harmony::CallJSUpdateGenericInfo(
        instance_id, [&key, value](napi_env env, napi_value js_map) {
          napi_value js_value;
          napi_create_int64(env, value, &js_value);
          napi_set_named_property(env, js_map, key.c_str(), js_value);
        });
  });
}

void EventTrackerPlatformImpl::ClearCache(int32_t instance_id) {
  base::UIThread::GetRunner()->PostTask([instance_id]() {
    if (!harmony::env_ || !harmony::js_self_ref_ ||
        !harmony::js_clear_cache_func_) {
      LOGI("env_, js_self_ref_ or js_clear_cache_func_ not found.");
      return;
    }
    base::NapiHandleScope scope(harmony::env_);
    /**
     * LynxEventReporter.clearCacheCallByNative(instanceId);
     *
     * arg[0] - instance_id (number)
     */
    size_t argc = 1;
    napi_value argv[argc];
    // arg[0] - instance_id (number)
    argv[0] = base::NapiUtil::CreateInt32(harmony::env_, instance_id);

    // Call JS Method :
    // LynxEventReporter.clearCacheCallByNative(instanceId)
    napi_status status = base::NapiUtil::InvokeJsMethod(
        harmony::env_, harmony::js_self_ref_, harmony::js_clear_cache_func_,
        argc, argv);
    if (status != napi_ok) {
      LOGI(
          "Failed to call "
          "LynxEventReporter.clearCacheCallByNative(instanceId)");
    }
  });
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
