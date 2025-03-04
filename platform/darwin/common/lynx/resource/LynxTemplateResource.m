// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateResource.h"

@implementation LynxTemplateResource

- (instancetype)initWithNSData:(NSData *)data {
  if (self = [super init]) {
    _data = data;
  }
  return self;
}

- (instancetype)initWithBundle:(LynxTemplateBundle *)bundle {
  if (self = [super init]) {
    _bundle = bundle;
  }
  return self;
}

@end
