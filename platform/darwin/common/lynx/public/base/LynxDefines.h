// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXDEFINES_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXDEFINES_H_

#ifndef LYNX_CONCAT
#define LYNX_CONCAT2(A, B) A##B
#define LYNX_CONCAT(A, B) LYNX_CONCAT2(A, B)
#endif

#if defined(__cplusplus)
#define LYNX_EXTERN extern "C" __attribute__((visibility("default")))
#else
#define LYNX_EXTERN extern __attribute__((visibility("default")))
#endif

/**
 * This attribute is used for static analysis.
 */
#if !defined LYNX_DYNAMIC
#if __has_attribute(objc_dynamic)
#define LYNX_DYNAMIC __attribute__((objc_dynamic))
#else
#define LYNX_DYNAMIC
#endif
#endif

#ifndef LYNX_NOT_IMPLEMENTED
#define LYNX_NOT_IMPLEMENTED(method)                                                           \
  method NS_UNAVAILABLE {                                                                      \
    NSString *msg = [NSString                                                                  \
        stringWithFormat:@"%s is not implemented in class %@", sel_getName(_cmd), self.class]; \
    @throw [NSException exceptionWithName:@"LxNotDesignatedInitializerException"               \
                                   reason:msg                                                  \
                                 userInfo:nil];                                                \
  }
#endif

#ifndef LynxMainThreadChecker
#define LynxMainThreadChecker()                               \
  NSAssert([NSThread currentThread] == [NSThread mainThread], \
           @"This method should be called on the main thread.")
#endif

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXDEFINES_H_
