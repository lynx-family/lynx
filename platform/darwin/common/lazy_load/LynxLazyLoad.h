// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LAZYLOAD_LYNXLAZYLOAD_H_
#define DARWIN_COMMON_LAZYLOAD_LYNXLAZYLOAD_H_

#import <Foundation/Foundation.h>

typedef enum : NSUInteger {
  LynxTypeFunction = 1,
  LynxTypeObjCMethod = 2,
  LynxTypeFunctionInfo = 3,  // The difference with function is that this type carries the file
                             // information of the function
} LynxType;

typedef struct LynxData {
  const LynxType type;
  const bool repeatable;
  const char *key;
  const void *value;
} LynxData;

typedef struct _LynxFunctionInfo {
  const void *function;
  const char *fileName;
  const int line;
} LynxFunctionInfo;

#define LYNX_BASE_INIT_KEY "LynxBaseInitKey"
#define LYNX_LAZY_CONCAT(A, B) A##B

#define LYNX_METHOD(KEY) LYNX_EXPORT_OBJC_METHOD(KEY, false)
#define LYNX_BASE_INIT_METHOD LYNX_METHOD(LYNX_BASE_INIT_KEY);
#define LYNX_EXPORT_OBJC_METHOD(KEY, REPEATABLE) \
  LynxDataDefine(KEY, REPEATABLE, LynxTypeObjCMethod, __func__)

#define LynxSegmentName "__LYNX__DATA"
#define LynxSectionName "__LYNX__SECTION"
#define LynxSectionSeperator ","

#define LynxSectionFullName LynxSegmentName LynxSectionSeperator LynxSectionName

#define LynxIdentifier(COUNTER) LYNX_LAZY_CONCAT(__LYNX_ID__, COUNTER)

#define LynxUniqueIdentifier LynxIdentifier(__COUNTER__)

#define LynxDataDefine(KEY, REPEATABLE, TYPE, VALUE)                                         \
  __attribute__((used, no_sanitize_address,                                                  \
                 section(LynxSectionFullName))) static const LynxData LynxUniqueIdentifier = \
      (LynxData) {                                                                           \
    TYPE, REPEATABLE, KEY, (void *)VALUE                                                     \
  }

/**
 * Register ui class when lynx was initialized firstly which will be used by LynxUIOwner.
 *
 * the order of registration is not exact. It's possible that the
 * previous ui will be replace by the current class with the same name.
 *
 * @param name, the tag name used for displaying in front-end
 */
#define LYNX_LAZY_REGISTER_UI(name)                                               \
  +(void)lynxLazyLoad {                                                           \
    LYNX_BASE_INIT_METHOD [LynxComponentRegistry registerUI:self withName:@name]; \
  }

/**
 * Register shadow node class when lynx was initialized firstly which will be used by
 * LynxShadowNodeOwner.
 *
 * the order of registration is not exact. It's possible that the
 * previous ui will be replace by the current class with the same name.
 *
 * @param name, the tag name used for displaying in front-end
 */

#define LYNX_LAZY_REGISTER_SHADOW_NODE(name)                                              \
  +(void)lynxLazyLoad {                                                                   \
    LYNX_BASE_INIT_METHOD [LynxComponentRegistry registerShadowNode:self withName:@name]; \
  }

/**
 * Code will be executed util lynxEnv was initialized when LazyLoad is true
 */
#define LYNX_LOAD_LAZY(Codec) \
  +(void)lynxLazyLoad {       \
    LYNX_BASE_INIT_METHOD     \
    Codec                     \
  }

#endif  // DARWIN_COMMON_LAZYLOAD_LYNXLAZYLOAD_H_
