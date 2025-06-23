// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/harmony/lynx_trail_hub_impl_harmony.h"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/services/fluency/fluency_tracer.h"

namespace lynx {
namespace tasm {
namespace harmony {
namespace {

LynxTrailHubImplHarmony::ConcurrentTrailMap& InternalTrailMap() {
  static base::NoDestructor<LynxTrailHubImplHarmony::ConcurrentTrailMap>
      trail_map;
  return *trail_map;
}

napi_value SetTrailMap(napi_env env, napi_callback_info info) {
  /**
   * 0 - trailValues: Record<string, string>
   */
  napi_value js_object;
  size_t argc = 1;
  napi_value argv[argc];
  if (napi_ok !=
      napi_get_cb_info(env, info, &argc, argv, &js_object, nullptr)) {
    return nullptr;
  }
  std::unordered_map<std::string, std::string> trail_map{};
  if (!base::NapiUtil::ConvertToMap(env, argv[0], trail_map)) {
    return nullptr;
  }
  LOGI("SetTrailMap, size: " << trail_map.size());

  InternalTrailMap().SetMap(std::move(trail_map));
  FluencyTracer::SetNeedCheck();
  return nullptr;
}

napi_status RegisterLynxTrailHubImpl(napi_env env, napi_value exports) {
  constexpr char kClassName[] = "LynxTrailHubImpl";
  constexpr char kMethodName[] = "setTrailMap";
  constexpr char kInvokeNAPIFailed[] = "%s %s failed";
  napi_property_descriptor properties[] = {{kMethodName, nullptr, SetTrailMap,
                                            nullptr, nullptr, nullptr,
                                            napi_static, nullptr}};
  // define class
  napi_value constructor;
  napi_status status = napi_define_class(
      env, kClassName, NAPI_AUTO_LENGTH, SetTrailMap, nullptr,
      sizeof(properties) / sizeof(properties[0]), properties, &constructor);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kInvokeNAPIFailed, "define",
                              kClassName);

  // register class
  status = napi_set_named_property(env, exports, kClassName, constructor);
  NAPI_THROW_IF_FAILED_STATUS(env, status, kInvokeNAPIFailed, "register",
                              kClassName);

  return status;
}

}  // namespace

napi_value LynxTrailHubImplHarmony::Init(napi_env env, napi_value exports) {
  RegisterLynxTrailHubImpl(env, exports);
  return exports;
}

std::optional<std::string> LynxTrailHubImplHarmony::GetStringForTrailKey(
    const std::string& key) {
  auto value = InternalTrailMap().GetValue(key);
  LOGI("GetString: " << *value << ", ForTrailKey: " << key);
  return value;
}

void LynxTrailHubImplHarmony::ConcurrentTrailMap::SetMap(
    std::unordered_map<std::string, std::string> map) {
  fml::UniqueLock lock{*mutex_};
  map_ = std::move(map);
}

std::optional<std::string>
LynxTrailHubImplHarmony::ConcurrentTrailMap::GetValue(const std::string& key) {
  fml::SharedLock lock{*mutex_};
  auto it = map_.find(key);
  return it != map_.end() ? std::optional<std::string>(it->second)
                          : std::nullopt;
}

}  // namespace harmony

std::unique_ptr<LynxTrailHub::TrailImpl> LynxTrailHub::TrailImpl::Create() {
  return std::make_unique<harmony::LynxTrailHubImplHarmony>();
}

}  // namespace tasm
}  // namespace lynx
