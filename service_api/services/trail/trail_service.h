// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef SERVICE_API_SERVICES_TRAIL_TRAIL_SERVICE_H_
#define SERVICE_API_SERVICES_TRAIL_TRAIL_SERVICE_H_

#include <service_api/service_api.h>

#include <optional>
#include <string>

#include "./trail_setting_key.h"

namespace lynx {
namespace service {
namespace trail_service {

class LYNX_SERVICE_DECLARE(LynxTrailService)
    : public BaseService<LynxTrailService> {
 public:
  ~LynxTrailService() override = default;

  // Reads a typed setting value. The key type controls both dispatch and the
  // return type, so callers can use one unified entry point for bool/string/
  // long settings.
  template <typename ValueType, bool HasDefault>
  typename SettingKey<ValueType, HasDefault>::return_type get_value(
      const SettingKey<ValueType, HasDefault>& key) {
    return SettingValueTraits<ValueType, HasDefault>::FromStringValue(
        get_value_impl(static_cast<const SettingKeyBase&>(key)),
        key.default_value());
  }

 protected:
  virtual std::optional<std::string> get_value_impl(
      const SettingKeyBase& key) = 0;
};

template <typename ValueType, bool HasDefault>
typename SettingKey<ValueType, HasDefault>::return_type
SettingKey<ValueType, HasDefault>::value() const {
  auto trail_service = lynx::service::get_service<LynxTrailService>();
  if (!trail_service) {
    return this->default_value();
  }
  return trail_service->get_value(*this);
}

}  // namespace trail_service
}  // namespace service
}  // namespace lynx

#endif  // SERVICE_API_SERVICES_TRAIL_TRAIL_SERVICE_H_
