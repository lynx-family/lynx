// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTheme.h"

#pragma mark - LynxTheme
@interface LynxTheme () {
  NSMutableDictionary<NSString *, NSString *> *_themeConfig;
}
@end

@implementation LynxTheme

- (nullable NSString *)valueForKey:(nonnull NSString *)key {
  if (_themeConfig == nil || key == nil) {
    return nil;
  }
  return [_themeConfig objectForKey:key];
}

- (nullable NSArray<NSString *> *)allKeys {
  return [_themeConfig allKeys];
}

- (BOOL)updateValue:(nullable NSString *)value forKey:(nonnull NSString *)key {
  if ([key length] == 0 || [key hasPrefix:@"__"]) {
    return false;
  }
  if (value == nil) {
    [_themeConfig removeObjectForKey:key];
    return true;
  }
  if (_themeConfig == nil) {
    _themeConfig = [NSMutableDictionary new];
  }
  [_themeConfig setObject:value forKey:key];
  return true;
}

- (NSDictionary *)themeConfig {
  if (_themeConfig == nil) {
    return [NSDictionary dictionary];
  }
  return [_themeConfig copy];
}

- (void)setThemeConfig:(NSDictionary *)themeConfig {
  _themeConfig = [themeConfig mutableCopy];
}

@end
