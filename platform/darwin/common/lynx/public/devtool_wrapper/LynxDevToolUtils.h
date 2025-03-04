// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevToolUtils : NSObject

+ (void)setDevtoolEnv:(BOOL)value forKey:(NSString *)key;

+ (BOOL)getDevtoolEnv:(NSString *)key withDefaultValue:(BOOL)value;

+ (void)setDevtoolEnv:(NSSet *)newGroupValues forGroup:(NSString *)groupKey;

+ (NSSet *)getDevtoolEnvWithGroupName:(NSString *)groupKey;

@end

NS_ASSUME_NONNULL_END

#if ENABLE_INSPECTOR
#define BLOCK_FOR_INSPECTOR(block) \
  do {                             \
    block();                       \
  } while (0)
#else
#define BLOCK_FOR_INSPECTOR(block)
#endif
