// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_SERVICE_API_LYNXLAZYLOAD_H_
#define DARWIN_SERVICE_API_LYNXLAZYLOAD_H_

#import <Foundation/Foundation.h>

// The definitions in this file need be the same with LynxLazyLoad.h

typedef enum : NSUInteger {
  ServiceLazyLoadTypeFunction = 1,
  ServiceLazyLoadTypeObjCMethod = 2,
  ServiceLazyLoadTypeFunctionInfo = 3,  // The difference with function is that this type carries
                                        // the file information of the function
} ServiceLazyLoadType;

typedef struct ServiceLazyLoadData {
  const ServiceLazyLoadType type;
  const bool repeatable;
  const char *key;
  const void *value;
} ServiceLazyLoadData;

typedef struct _ServiceLazyLoadFunctionInfo {
  const void *function;
  const char *fileName;
  const int line;
} ServiceLazyLoadFunctionInfo;

#define SERVICE_LAZY_LOAD_BASE_INIT_KEY "LynxBaseInitKey"
#define SERVICE_LAZY_LOAD_LAZY_CONCAT(A, B) A##B

#define SERVICE_LAZY_LOAD_METHOD(KEY) SERVICE_LAZY_LOAD_EXPORT_OBJC_METHOD(KEY, false)
#define SERVICE_LAZY_LOAD_BASE_INIT_METHOD \
  SERVICE_LAZY_LOAD_METHOD(SERVICE_LAZY_LOAD_BASE_INIT_KEY);
#define SERVICE_LAZY_LOAD_EXPORT_OBJC_METHOD(KEY, REPEATABLE) \
  ServiceLazyLoadDataDefine(KEY, REPEATABLE, ServiceLazyLoadTypeObjCMethod, __func__)

#define ServiceLazyLoadSegmentName "__LYNX__DATA"
#define ServiceLazyLoadSectionName "__LYNX__SECTION"
#define ServiceLazyLoadSectionSeperator ","

#define ServiceLazyLoadSectionFullName \
  ServiceLazyLoadSegmentName ServiceLazyLoadSectionSeperator ServiceLazyLoadSectionName

#define ServiceLazyLoadIdentifier(COUNTER) SERVICE_LAZY_LOAD_LAZY_CONCAT(__LYNX_ID__, COUNTER)

#define ServiceLazyLoadUniqueIdentifier ServiceLazyLoadIdentifier(__COUNTER__)

#define ServiceLazyLoadDataDefine(KEY, REPEATABLE, TYPE, VALUE)                             \
  __attribute__((used, no_sanitize_address,                                                 \
                 section(ServiceLazyLoadSectionFullName))) static const ServiceLazyLoadData \
      ServiceLazyLoadUniqueIdentifier = (ServiceLazyLoadData) {                             \
    TYPE, REPEATABLE, KEY, (void *)VALUE                                                    \
  }

/**
 * Code will be executed util lynxEnv was initialized when LazyLoad is true
 */
#define SERVICE_LOAD_LAZY(Codec)       \
  +(void)lynxLazyLoad {                \
    SERVICE_LAZY_LOAD_BASE_INIT_METHOD \
    Codec                              \
  }

#endif  // DARWIN_SERVICE_API_LYNXLAZYLOAD_H_
