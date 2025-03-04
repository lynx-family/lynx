// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <TargetConditionals.h>
#ifdef __OBJC__
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#elif defined(TARGET_OS_IOS) && TARGET_OS_IOS
#import <UIKit/UIKit.h>
#elif defined(TARGET_OS_TV) && TARGET_OS_TV
#import <UIKit/UIKit.h>
#else
#error "unknown os"
#endif  // TARGET_OS_
#else   // __OBJC__
#ifndef BASE_TRACE_DARWIN_TRACE - PREFIX_H_
#if defined(__cplusplus)
#define BASE_TRACE_DARWIN_TRACE -PREFIX_H_ extern "C"
#else
#define FOUNDATION_EXPORT extern
#endif  // BASE_TRACE_DARWIN_TRACE-PREFIX_H_
#endif  // FOUNDATION_EXPORT
#endif  // __OBJC__
