// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEnv.h"

@interface LynxEnv ()

+ (NSString *)_keyStringFromType:(LynxEnvKey)key;

+ (BOOL)stringValueToBool:(NSString *)value defaultValue:(BOOL)defaultValue;

- (BOOL)enableCreateUIAsync;

- (BOOL)enableAnimationSyncTimeOpt;

- (NSString *)_stringFromExternalEnv:(NSString *)key;

// Provide an interface for UT (Unit Testing) that can update the key value of
// externalEnvCacheForKey.
- (void)updateExternalEnvCacheForKey:(NSString *)key withValue:(NSString *)value;

@end
