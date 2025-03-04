// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGroup.h"
#import "LynxGroup+Internal.h"
#import "LynxView.h"

#include "core/shared_data/lynx_white_board.h"

@interface LynxGroupOption ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *config;

@end

@implementation LynxGroupOption

- (instancetype)init {
  if (self = [super init]) {
    _config = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)setStringConfig:(nonnull NSString *)value forKey:(nonnull NSString *)key {
  if (!key || !value) {
    return;
  }
  [_config setObject:value forKey:key];
}

- (void)setBoolConfig:(BOOL)value forKey:(nonnull NSString *)key {
  if (!key) {
    return;
  }
  [_config setObject:@(value) forKey:key];
}

@end

#pragma mark - NextId
static int sNextId = 0;

#pragma mark - SingleGroupTag
static NSString *sSingleGroupTag = @"-1";

#pragma mark - LynxGroup

@interface LynxGroup ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, id> *config;

@end

@implementation LynxGroup {
  int _numberId;
  NSMutableArray<LynxView *> *_viewList;
  bool _enableJSGroupThread;
}

+ (nonnull NSString *)singleGroupTag {
  return sSingleGroupTag;
}

- (nonnull instancetype)initWithName:(NSString *)name {
  return [self initWithName:name withPreloadScript:nil];
}

- (nonnull instancetype)initWithName:(nonnull NSString *)name
                   withPreloadScript:(nullable NSArray *)extraJSPaths {
  if (self = [super init]) {
    _config = [NSMutableDictionary dictionary];
    _groupName = name;
    _numberId = ++sNextId;
    _identification = [NSString stringWithFormat:@"%d", _numberId];
    _preloadJSPaths = extraJSPaths;
    _viewList = [[NSMutableArray alloc] init];
    _whiteBoard = std::make_shared<lynx::tasm::WhiteBoard>();
  }
  return self;
}

- (instancetype)initWithName:(NSString *)name withLynxGroupOption:(LynxGroupOption *)option {
  if (self = [self initWithName:name withPreloadScript:option.preloadJSPaths]) {
    // all of the options can set up here.
    _enableJSGroupThread = option.enableJSGroupThread;
    _config = option.config;
  }
  return self;
}

- (void)addLynxView:(nonnull LynxView *)view {
  [_viewList addObject:view];
}

- (bool)enableJSGroupThread {
  return _enableJSGroupThread;
}

+ (NSString *)groupNameForLynxGroupOrDefault:(LynxGroup *)group {
  return (group ? [group groupName] : [LynxGroup singleGroupTag]);
}

+ (NSString *)jsThreadNameForLynxGroupOrDefault:(LynxGroup *)group {
  if (group.enableJSGroupThread) {
    return [group groupName];
  }

  return @"";
}

- (nullable NSString *)getStringConfig:(nonnull NSString *)key {
  id value = [_config objectForKey:key];
  if (![value isKindOfClass:[NSString class]]) {
    return nil;
  }
  return value;
}

- (BOOL)getBoolConfig:(nonnull NSString *)key {
  id value = [_config objectForKey:key];
  if (![value isKindOfClass:[@YES class]]) {
    return NO;
  }
  return [value boolValue];
}

@end
