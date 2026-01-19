// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SERVICE_API_SERVICE_LAZY_LOAD_H_
#define SERVICE_API_SERVICE_LAZY_LOAD_H_

#include <service_api/service_api_utils.h>

// This header provides a pure C++ lazy load registration macro set in iOS
// platform. The data layout and section names must stay consistent with
// ServiceLazyLoad.h/LynxLazyLoad.h

#if defined(OS_IOS)
enum ServiceLazyLoadType : unsigned long {
  ServiceLazyLoadTypeFunction = 1,
  ServiceLazyLoadTypeObjCMethod = 2,
  ServiceLazyLoadTypeFunctionInfo = 3,
};

struct ServiceLazyLoadData {
  const ServiceLazyLoadType type;
  const bool repeatable;
  const char* key;
  const void* value;
};

#ifdef __cplusplus
#define SERVICE_LAZY_LOAD_CPP_CAST(VALUE) reinterpret_cast<const void*>(VALUE)
#else
#define SERVICE_LAZY_LOAD_CPP_CAST(VALUE) (const void*)(VALUE)
#endif

#define SERVICE_LAZY_LOAD_CPP_DATA_DEFINE(KEY, REPEATABLE, TYPE, VALUE)      \
  __attribute__((                                                            \
      used, no_sanitize_address,                                             \
      section(                                                               \
          "__LYNX__DATA,__LYNX__SECTION"))) static const ServiceLazyLoadData \
  BASE_CONCAT(__LYNX_ID__, __COUNTER__) = {TYPE, REPEATABLE, KEY,            \
                                           SERVICE_LAZY_LOAD_CPP_CAST(VALUE)}

#define SERVICE_LAZY_LOAD_CPP(name)                                     \
  static void name();                                                   \
  SERVICE_LAZY_LOAD_CPP_DATA_DEFINE("LynxBaseInitKey", false,           \
                                    ServiceLazyLoadTypeFunction, name); \
  static void name()
#endif
#endif  // SERVICE_API_SERVICE_LAZY_LOAD_H_
