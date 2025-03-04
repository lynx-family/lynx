// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_DARWIN_LYNXTRACEEVENT_H_
#define BASE_TRACE_DARWIN_LYNXTRACEEVENT_H_

#import "LynxTraceEventWrapper.h"

#if ENABLE_TRACE_PERFETTO
#define LYNX_TRACE_SECTION_WITH_INFO(category, name, info) \
  [LynxTraceEvent beginSection:category withName:name debugInfo:info];

#define LYNX_TRACE_SECTION(category, name) [LynxTraceEvent beginSection:category withName:name];

#define LYNX_TRACE_END_SECTION(category) [LynxTraceEvent endSection:category];

#define LYNX_TRACE_END_SECTION_WITH_NAME(category, name) \
  [LynxTraceEvent endSection:category withName:name]

#define LYNX_TRACE_INSTANT(category, name) [LynxTraceEvent instant:(category) withName:(name)];

#define LYNX_TRACE_INSTANT_WITH_DEBUG_INFO(category, name, info) \
  [LynxTraceEvent instant:(category) withName:(name)debugInfo:(info)];
#else

#define LYNX_TRACE_SECTION_WITH_INFO(category, name, debugInfo)
#define LYNX_TRACE_SECTION(category, name)
#define LYNX_TRACE_END_SECTION(category)
#define LYNX_TRACE_INSTANT(category, name)
#define LYNX_TRACE_INSTANT_WITH_DEBUG_INFO(category, name, info)

#endif

#define DEPRECATED_API __attribute__((deprecated))

@interface LynxTraceEvent : NSObject

+ (NSString *)getRandomColor;

+ (void)beginSection:(NSString *)category
            withName:(NSString *)name
           debugInfo:(NSDictionary *)keyValues;

+ (void)beginSection:(NSString *)category withName:(NSString *)name;

+ (void)endSection:(NSString *)category;

+ (void)endSection:(NSString *)category
          withName:(NSString *)name
         debugInfo:(NSDictionary *)keyValues;

+ (void)endSection:(NSString *)category withName:(NSString *)name;

+ (void)instant:(NSString *)category withName:(NSString *)name;

+ (void)instant:(NSString *)category withName:(NSString *)name withColor:(NSString *)color;

+ (void)instant:(NSString *)category withName:(NSString *)name debugInfo:(NSDictionary *)keyValues;

+ (void)instant:(NSString *)category withName:(NSString *)name withTimestamp:(int64_t)timestamp;

+ (void)instant:(NSString *)category
         withName:(NSString *)name
    withTimestamp:(int64_t)timestamp
        withColor:(NSString *)color;

+ (void)counter:(NSString *)category withName:(NSString *)name withCounterValue:(uint64_t)value;

+ (BOOL)categoryEnabled:(NSString *)category;

+ (void)instant:(NSString *)category
         withName:(NSString *)name
    withTimestamp:(int64_t)timestamp
        debugInfo:(NSDictionary *)keyValues DEPRECATED_API;

+ (BOOL)registerTraceBackend:(intptr_t)ptr DEPRECATED_API;

@end

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXTRACEEVENT_H_
