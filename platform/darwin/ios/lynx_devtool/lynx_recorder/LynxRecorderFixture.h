// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Owns extracted assets until the replay view and JS context release the package.
@interface LynxRecorderFixture : NSObject
@property(nonatomic, readonly) NSString *directory;
@property(nonatomic, readonly) NSDictionary *record;
+ (nullable instancetype)openData:(NSData *)data error:(NSError **)error;
@end

NS_ASSUME_NONNULL_END
