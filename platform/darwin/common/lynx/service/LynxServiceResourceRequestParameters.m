// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxServiceResourceRequestParameters.h"

@implementation LynxServiceResourceRequestParameters

- (instancetype)copyWithZone:(NSZone*)zone {
  LynxServiceResourceRequestParameters* copy = [[[self class] allocWithZone:zone] init];
  copy.enableRequestReuse = _enableRequestReuse;
  copy.resourceScene = _resourceScene;
  return copy;
}

- (NSNumber* _Nullable)waitLocalResourceUpdate {
  return nil;
}

- (NSNumber* _Nullable)disableLocalResource {
  return [NSNumber numberWithBool:NO];
}

- (NSNumber* _Nullable)disableRemoteResource {
  return nil;
}

- (NSNumber* _Nullable)disableRemoteResourceCache {
  return nil;
}

- (NSNumber* _Nullable)onlyLocal {
  return nil;
}

- (NSNumber* _Nullable)onlyPath {
  return nil;
}

@end
