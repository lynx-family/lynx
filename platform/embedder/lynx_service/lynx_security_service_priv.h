// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SECURITY_SERVICE_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SECURITY_SERVICE_PRIV_H_

#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/public/capi/lynx_security_service_capi.h"

struct lynx_security_service_t : public lynx::embedder::LynxServiceBase {
  lynx_security_service_t() = default;
  ~lynx_security_service_t();

  void* user_data = nullptr;
  void (*finalizer)(lynx_security_service_t*, void*) = nullptr;

  lynx_verification_func verification_func = nullptr;
};

int lynx_security_service_verify_tasm(lynx_security_service_t* security_service,
                                      uint8_t* content, size_t length,
                                      const char* url, lynx_tasm_type_e type,
                                      const char** error_msg);

#endif  // PLATFORM_EMBEDDER_LYNX_SERVICE_LYNX_SECURITY_SERVICE_PRIV_H_
