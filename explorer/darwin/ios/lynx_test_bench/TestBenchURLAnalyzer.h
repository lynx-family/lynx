// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface TestBenchURLAnalyzer : NSObject
+ (NSString*)getQueryStringParameter:(NSURL*)url forKey:(NSString*)key;
+ (BOOL)getQueryBooleanParameter:(NSURL*)url forKey:(NSString*)key defaultValue:(BOOL)defaultValue;
@end

NS_ASSUME_NONNULL_END
