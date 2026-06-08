// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_

#import <Foundation/Foundation.h>

// The Objective-C library marker macro shares this public class name. Keep it
// out before declaring or extending the class API.
#if defined(LynxElement)
#pragma push_macro("LynxElement")
#undef LynxElement
#define LYNX_RESTORE_LYNX_ELEMENT_HEADER 1
#endif

NS_ASSUME_NONNULL_BEGIN

@interface LynxElement : NSObject

- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;

/**
 * @apidoc
 * @brief Asynchronously serializes this LynxElement tree to a JSON string.
 * @param callback Receives the JSON string, or nil if the element is unavailable. The callback is
 * always invoked asynchronously on the main thread.
 */
- (void)toJSONString:(void (^_Nonnull)(NSString *_Nullable json))callback;

@end

NS_ASSUME_NONNULL_END

#if defined(LYNX_RESTORE_LYNX_ELEMENT_HEADER)
#pragma pop_macro("LynxElement")
#undef LYNX_RESTORE_LYNX_ELEMENT_HEADER
#endif

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXELEMENT_H_
