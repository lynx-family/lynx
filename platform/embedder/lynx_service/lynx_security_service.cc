// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/lynx_service/lynx_security_service_priv.h"

lynx_security_service_t::~lynx_security_service_t() {
  if (finalizer) {
    finalizer(this, user_data);
  }
}

LYNX_EXTERN_C lynx_security_service_t* lynx_security_service_create(
    void* user_data) {
  return lynx_security_service_create_with_finalizer(user_data, nullptr);
}

LYNX_EXTERN_C lynx_security_service_t*
lynx_security_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_security_service_t*, void*)) {
  auto* security_service = new lynx_security_service_t;
  security_service->user_data = user_data;
  security_service->finalizer = finalizer;
  // The ref count has been initialized to 1, there is no need to call AddRef.
  return security_service;
}

LYNX_EXTERN_C void* lynx_security_service_get_user_data(
    lynx_security_service_t* security_service) {
  return security_service->user_data;
}

LYNX_EXTERN_C void lynx_security_service_bind(
    lynx_security_service_t* security_service, lynx_verification_func f) {
  security_service->verification_func = f;
}

LYNX_EXTERN_C void lynx_security_service_release(
    lynx_security_service_t* security_service) {
  // Unref the security_service.
  security_service->Release();
}

int lynx_security_service_verify_tasm(lynx_security_service_t* security_service,
                                      uint8_t* content, size_t length,
                                      const char* url, lynx_tasm_type_e type,
                                      const char** error_msg) {
  if (security_service && security_service->verification_func) {
    return security_service->verification_func(security_service, content,
                                               length, url, type, error_msg);
  }
  return 0;
}
