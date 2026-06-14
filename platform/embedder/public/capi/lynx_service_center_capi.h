// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SERVICE_CENTER_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SERVICE_CENTER_CAPI_H_

#include <stdint.h>
#include <stdlib.h>

#include "lynx_export.h"

LYNX_EXTERN_C_BEGIN

typedef struct lynx_service_center_t lynx_service_center_t;

// Enumeration of service types available in the Lynx service center. And new
// service types can be appended here in the future.
typedef enum lynx_service_type_e {
  kServiceTypeHttp = 1,
  kServiceTypeSecurity,
  kServiceTypeEventReporter,
  kServiceTypeTrail,
} lynx_service_type_e;

// Retrieves the singleton instance of the Lynx service center.
LYNX_CAPI_EXPORT lynx_service_center_t* lynx_service_get_center_instance();

// Register a service with the Lynx service center. This function allows you to
// register a service of a specific type with the Lynx service center. The
// service center can then manage and provide access to this service as needed.
LYNX_CAPI_EXPORT void lynx_service_register_service(lynx_service_center_t*,
                                                    lynx_service_type_e type,
                                                    void* service);

// Unregister a service from the Lynx service center. This function allows you
// to unregister a service of a specific type from the Lynx service center.
LYNX_CAPI_EXPORT void lynx_service_unregister_service(lynx_service_center_t*,
                                                      lynx_service_type_e type,
                                                      void* service);

// Retrieves a service from the Lynx service center. This function allows you to
// retrieve a service of a specific type from the Lynx service center. If the
// service of the specified type is registered in the service center, a pointer
// to the service will be returned. Otherwise, it may return NULL.
LYNX_CAPI_EXPORT void* lynx_service_get_service(lynx_service_center_t*,
                                                lynx_service_type_e type);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_SERVICE_CENTER_CAPI_H_
