// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_TRAIL_SERVICE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_TRAIL_SERVICE_H_

#include <memory>
#include <optional>
#include <string>

#include "capi/lynx_service_center_capi.h"
#include "capi/lynx_trail_service_capi.h"
#include "lynx_service_center.h"

namespace lynx {
namespace pub {

class LynxTrailService : public LynxServiceBase,
                         public std::enable_shared_from_this<LynxTrailService> {
 public:
  LynxTrailService() = default;
  ~LynxTrailService() override {
    if (trail_service_) {
      lynx_trail_service_release(trail_service_);
      trail_service_ = nullptr;
    }
  }

  void InitIfNeeded() override {
    if (trail_service_) {
      return;
    }
    // LynxTrailService must be owned by std::shared_ptr before InitIfNeeded()
    // is called; otherwise weak_from_this() is empty and callbacks return null.
    std::weak_ptr<LynxTrailService> self = weak_from_this();
    trail_service_ = lynx_trail_service_create_with_finalizer(
        new std::weak_ptr<LynxTrailService>(self),
        [](lynx_trail_service_t* trail_service, void* user_data) {
          std::weak_ptr<LynxTrailService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxTrailService>*>(user_data);
          delete weak_ptr;
        });
    lynx_trail_service_bind(
        trail_service_,
        [](lynx_trail_service_t* trail_service, const char* key) {
          std::weak_ptr<LynxTrailService>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxTrailService>*>(
                  lynx_trail_service_get_user_data(trail_service));
          std::shared_ptr<LynxTrailService> shared_trail_service =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (!shared_trail_service) {
            return static_cast<const char*>(nullptr);
          }
          std::optional<std::string> value =
              shared_trail_service->GetStringValueForTrailKey(key ? key : "");
          if (!value) {
            return static_cast<const char*>(nullptr);
          }
          // The returned pointer is valid until the next callback on the same
          // thread. Callers must copy it immediately.
          static thread_local std::string result;
          result = *value;
          return result.c_str();
        });
  }

  lynx_service_type_e GetServiceType() override { return kServiceTypeTrail; }

  void* Impl() override { return trail_service_; }

  virtual std::optional<std::string> GetStringValueForTrailKey(
      const std::string& key) = 0;

 private:
  lynx_trail_service_t* trail_service_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_TRAIL_SERVICE_H_
