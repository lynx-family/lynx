// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/*
 * A `LynxPropertyDiffMap` is designed to restore all the properties being set to a LynxUI and be
 * able to get the updated properties.
 */

@interface LynxPropertyDiffMap : NSObject

- (void)putValue:(id)value forKey:(NSString *)key;
- (void)deleteKey:(NSString *)key;

- (id _Nullable)getValueForKey:(NSString *)key;

- (id _Nullable)getValueForKey:(NSString *)key defaultValue:(id _Nullable)defaultValue;

- (id _Nullable)getUpdatedValueForKey:(NSString *)key;

- (BOOL)valueChangedForKey:(NSString *)key updateTo:(NSObject *_Nullable __strong *_Nonnull)owner;

- (BOOL)isValueForKeyUpdated:(NSString *)key;

- (NSSet<NSString *> *)getUpdatedKeys;

- (void)clearDirtyRecords;

@end

NS_ASSUME_NONNULL_END
