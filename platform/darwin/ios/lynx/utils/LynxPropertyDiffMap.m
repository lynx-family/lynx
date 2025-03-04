// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxPropertyDiffMap.h"

@interface LynxPropertyDiffMap ()

// Restore all the props
@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *backingMap;

// Restore the updated props
@property(nonatomic, strong) NSMutableSet<NSString *> *dirtyPropertiesSet;

@end

@implementation LynxPropertyDiffMap

- (instancetype)init {
  if (self = [super init]) {
    _backingMap = [NSMutableDictionary dictionary];
    _dirtyPropertiesSet = [NSMutableSet set];
  }
  return self;
}

- (void)putValue:(id)value forKey:(NSString *)key {
  [_dirtyPropertiesSet addObject:key];
  [_backingMap setObject:value forKey:key];
}

- (void)deleteKey:(NSString *)key {
  [_backingMap removeObjectForKey:key];
  [_dirtyPropertiesSet removeObject:key];
}

- (id)getValueForKey:(NSString *)key {
  return [_backingMap objectForKey:key];
}

- (id)getValueForKey:(NSString *)key defaultValue:(id _Nullable)defaultValue {
  return [_backingMap objectForKey:key] ?: defaultValue;
}

- (id _Nullable)getUpdatedValueForKey:(NSString *)key {
  return [_dirtyPropertiesSet containsObject:key] ? [_backingMap objectForKey:key] : nil;
}

- (BOOL)valueChangedForKey:(NSString *)key updateTo:(NSObject *_Nullable __strong *_Nonnull)owner {
  if ([_dirtyPropertiesSet containsObject:key]) {
    *owner = [_backingMap objectForKey:key];
    return YES;
  }
  return NO;
}

- (BOOL)isValueForKeyUpdated:(NSString *)key {
  return [_dirtyPropertiesSet containsObject:key];
}

- (NSSet<NSString *> *)getUpdatedKeys {
  NSSet<NSString *> *ret = [_dirtyPropertiesSet copy];
  return ret;
}

- (void)clearDirtyRecords {
  [_dirtyPropertiesSet removeAllObjects];
}

@end
