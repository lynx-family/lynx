// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutModelLight.h>

@implementation LynxListLayoutModelLight

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super init];
  if (self) {
    _frame = frame;
    _type = LynxLayoutModelInvalid;
  }
  return self;
}

@end
