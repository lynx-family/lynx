// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/lynx_service/lynx_service_center_priv.h"

LYNX_EXTERN_C lynx_service_center_t* lynx_service_get_center_instance() {
  static lynx_service_center_t global_service_center;
  return &global_service_center;
}

LYNX_EXTERN_C void lynx_service_register_service(
    lynx_service_center_t* service_center, lynx_service_type_e type,
    void* service) {
  if (!service) {
    return;
  }
  // Must be a type of LynxServiceBase.
  lynx::embedder::LynxServiceBase* service_base =
      reinterpret_cast<lynx::embedder::LynxServiceBase*>(service);
  auto it = service_center->services.find(type);
  if (it != service_center->services.end() && it->second) {
    it->second->Release();
  }
  service_base->AddRef();
  service_center->services[type] = service_base;
}

LYNX_EXTERN_C void lynx_service_unregister_service(
    lynx_service_center_t* service_center, lynx_service_type_e type,
    void* service) {
  if (!service) {
    return;
  }
  auto it = service_center->services.find(type);
  if (it != service_center->services.end() && it->second == service) {
    it->second->Release();
    service_center->services.erase(it);
  }
}

LYNX_EXTERN_C void* lynx_service_get_service(
    lynx_service_center_t* service_center, lynx_service_type_e type) {
  auto it = service_center->services.find(type);
  if (it != service_center->services.end()) {
    return it->second;
  }
  return nullptr;
}
