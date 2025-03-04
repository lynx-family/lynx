// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXPERFORMANCE_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXPERFORMANCE_H_

#import <Foundation/Foundation.h>

// The flag to mark it's ssr hydrate perf records.
static const int kLynxPerformanceIsSrrHydrateIndex = 20220425;

@interface LynxPerformance : NSObject

@property(nonatomic, assign, readonly) BOOL hasActualFMP;
@property(nonatomic, assign, readonly) double actualFMPDuration;
@property(nonatomic, assign, readonly) double actualFirstScreenEndTimeStamp;

- (instancetype _Nonnull)initWithPerformance:(NSDictionary* _Nonnull)dic
                                         url:(NSString* _Nonnull)url
                                    pageType:(NSString* _Nonnull)pageType
                                reactVersion:(NSString* _Nonnull)reactVersion;

- (NSDictionary* _Nonnull)toDictionary;

+ (NSString* _Nullable)toPerfKey:(int)index;
+ (NSString* _Nullable)toPerfKey:(int)index isSsrHydrate:(BOOL)isSsrHydrate;
+ (NSString* _Nullable)toPerfStampKey:(int)index;
@end

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXPERFORMANCE_H_
