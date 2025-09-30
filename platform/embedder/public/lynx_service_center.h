// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_SERVICE_CENTER_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_SERVICE_CENTER_H_

#include <memory>
#include <unordered_map>

#include "capi/lynx_service_center_capi.h"

namespace lynx {
namespace pub {

class LynxServiceBase {
 public:
  LynxServiceBase() = default;
  virtual ~LynxServiceBase() = default;

  virtual void InitIfNeeded() = 0;

  virtual lynx_service_type_e GetServiceType() = 0;

  virtual void* Impl() = 0;
};

/**
 * @brief LynxServiceCenter is the service center of Lynx. It provides methods
 * for registering services.
 */
class LynxServiceCenter {
 public:
  static LynxServiceCenter& GetInstance() {
    static LynxServiceCenter instance;
    return instance;
  }

  LynxServiceCenter(const LynxServiceCenter&) = delete;
  LynxServiceCenter& operator=(const LynxServiceCenter&) = delete;

  /**
   * @brief RegisterService is used to register a service.
   * @param service The service to be registered.
   */
  void RegisterService(std::shared_ptr<LynxServiceBase> service) {
    service->InitIfNeeded();
    lynx_service_register_service(service_center_, service->GetServiceType(),
                                  service->Impl());
    services_[service->GetServiceType()] = service;
  }

 private:
  LynxServiceCenter() { service_center_ = lynx_service_get_center_instance(); }
  ~LynxServiceCenter() = default;

  lynx_service_center_t* service_center_ = nullptr;
  std::unordered_map<lynx_service_type_e, std::shared_ptr<LynxServiceBase> >
      services_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_SERVICE_CENTER_H_
