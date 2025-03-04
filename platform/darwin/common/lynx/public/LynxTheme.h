// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNXTHEME_H_
#define DARWIN_COMMON_LYNX_LYNXTHEME_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxTheme : NSObject

@property(readwrite, copy) NSDictionary *themeConfig;

- (BOOL)updateValue:(nullable NSString *)value forKey:(nonnull NSString *)key;
- (nullable NSString *)valueForKey:(nonnull NSString *)key;
- (nullable NSArray<NSString *> *)allKeys;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_LYNXTHEME_H_
