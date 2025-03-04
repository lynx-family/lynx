// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRoute.h"

@implementation LynxRoute

- (instancetype)initWithUrl:(NSString *)url param:(NSDictionary *)param {
  return [self initWithUrl:url routeName:url param:param];
}

- (instancetype)initWithUrl:(NSString *)url
                  routeName:(NSString *)routeName
                      param:(NSDictionary *)param {
  self = [super init];
  if (self) {
    self.templateUrl = url;
    self.routeName = url;
    self.param = param;
  }
  return self;
}

- (id)copyWithZone:(NSZone *)zone {
  return self;
}

@end
