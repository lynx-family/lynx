// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SECURITY_SERVICE_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SECURITY_SERVICE_CAPI_H_

#include <stdint.h>
#include <stdlib.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_security_service_t lynx_security_service_t;

typedef enum lynx_tasm_type_e {
  kTypeTemplate = 1,
  kTypeDynamicComponent,
} lynx_tasm_type_e;

// Defines a function pointer type for content verification. It can be bound to
// a security service and will be invoked when content verification is required.
typedef int (*lynx_verification_func)(lynx_security_service_t*,
                                      uint8_t* content, size_t length,
                                      const char* url, lynx_tasm_type_e type,
                                      const char** error_msg);
LYNX_CAPI_EXPORT lynx_security_service_t* lynx_security_service_create(
    void* user_data);
// Create a new Security service instance with a finalizer.
// The finalizer will be called when the service is released.
// The instance's lifecycle is based on reference count.
LYNX_CAPI_EXPORT lynx_security_service_t*
lynx_security_service_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_security_service_t*, void*));

// Retrieves the user data associated with the security service.
LYNX_CAPI_EXPORT void* lynx_security_service_get_user_data(
    lynx_security_service_t*);

// Binds a verification function to the security service. This function
// associates a custom verification function with the specified security
// service. The verification function will be used to perform security checks on
// content.
LYNX_CAPI_EXPORT void lynx_security_service_bind(lynx_security_service_t*,
                                                 lynx_verification_func f);

// Releases the resources associated with the security service.
LYNX_CAPI_EXPORT void lynx_security_service_release(lynx_security_service_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SECURITY_SERVICE_CAPI_H_
