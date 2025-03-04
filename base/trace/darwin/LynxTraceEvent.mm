// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTraceEvent.h"

#include "base/trace/native/trace_event.h"

@implementation LynxTraceEvent

+ (NSString *)getRandomColor {
  NSMutableString *result = [NSMutableString stringWithCapacity:7];
  [result appendString:@"#"];
  for (int i = 0; i < 6; i++) {
    [result appendFormat:@"%X", arc4random() % 16];
  }
  return result;
}

+ (void)beginSection:(NSString *)category
            withName:(NSString *)name
           debugInfo:(NSDictionary *)keyValues {
  TRACE_EVENT_BEGIN([category UTF8String], nullptr, [&](lynx::perfetto::EventContext ctx) {
    auto event = ctx.event();
    event->set_name([name UTF8String]);
    [keyValues
        enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL *_Nonnull stop) {
          auto *debug = event->add_debug_annotations();
          debug->set_name([[NSString stringWithFormat:@"%@", key] UTF8String]);
          debug->set_string_value([[NSString stringWithFormat:@"%@", obj] UTF8String]);
        }];
  });
}

+ (void)beginSection:(NSString *)category withName:(NSString *)name {
  TRACE_EVENT_BEGIN([category UTF8String], nullptr, [&](lynx::perfetto::EventContext ctx) {
    ctx.event()->set_name([name UTF8String]);
  });
}

+ (void)endSection:(NSString *)category {
  TRACE_EVENT_END([category UTF8String]);
}

+ (void)endSection:(NSString *)category
          withName:(NSString *)name
         debugInfo:(NSDictionary *)keyValues {
  TRACE_EVENT_END([category UTF8String], [&](lynx::perfetto::EventContext ctx) {
    auto event = ctx.event();
    event->set_name([name UTF8String]);
    [keyValues
        enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL *_Nonnull stop) {
          auto *debug = event->add_debug_annotations();
          debug->set_name([[NSString stringWithFormat:@"%@", key] UTF8String]);
          debug->set_string_value([[NSString stringWithFormat:@"%@", obj] UTF8String]);
        }];
  });
}

+ (void)endSection:(NSString *)category withName:(NSString *)name {
  TRACE_EVENT_END([category UTF8String], [&](lynx::perfetto::EventContext ctx) {
    ctx.event()->set_name([name UTF8String]);
  });
}

+ (void)instant:(NSString *)category withName:(NSString *)name {
  [self instant:category withName:name withColor:[self getRandomColor]];
}

+ (void)instant:(NSString *)category withName:(NSString *)name withColor:(NSString *)color {
  TRACE_EVENT_INSTANT([category UTF8String], nullptr, [&](lynx::perfetto::EventContext ctx) {
    ctx.event()->set_name([name UTF8String]);
    auto *debug = ctx.event()->add_debug_annotations();
    debug->set_name("color");
    debug->set_string_value([color UTF8String]);
  });
}

+ (void)instant:(NSString *)category withName:(NSString *)name debugInfo:(NSDictionary *)keyValues {
  TRACE_EVENT_INSTANT([category UTF8String], nullptr, [&](lynx::perfetto::EventContext ctx) {
    auto event = ctx.event();
    event->set_name([name UTF8String]);
    [keyValues
        enumerateKeysAndObjectsUsingBlock:^(id _Nonnull key, id _Nonnull obj, BOOL *_Nonnull stop) {
          auto *debug = event->add_debug_annotations();
          debug->set_name([[NSString stringWithFormat:@"%@", key] UTF8String]);
          debug->set_string_value([[NSString stringWithFormat:@"%@", obj] UTF8String]);
        }];
  });
}

+ (void)counter:(NSString *)category withName:(NSString *)name withCounterValue:(uint64_t)value {
  TRACE_COUNTER([category UTF8String], lynx::perfetto::CounterTrack([name UTF8String]), value);
}

+ (BOOL)categoryEnabled:(NSString *)category {
  return YES;
}

+ (void)instant:(NSString *)category withName:(NSString *)name withTimestamp:(int64_t)timestamp {
  [self instant:category withName:name withColor:[self getRandomColor]];
}

+ (void)instant:(NSString *)category
         withName:(NSString *)name
    withTimestamp:(int64_t)timestamp
        withColor:(NSString *)color {
  [self instant:category withName:name withColor:color];
}

+ (void)instant:(NSString *)category
         withName:(NSString *)name
    withTimestamp:(int64_t)timestamp
        debugInfo:(NSDictionary *)keyValues {
  [self instant:category withName:name debugInfo:keyValues];
}

+ (BOOL)registerTraceBackend:(intptr_t)ptr {
  return NO;
}

@end
