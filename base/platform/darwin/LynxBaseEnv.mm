// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxBase/LynxBaseEnv.h>

@implementation LynxBaseEnv

+ (instancetype)sharedInstance {
  static LynxBaseEnv *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxBaseEnv alloc] init];
  });

  return _instance;
}

- (bool)initialize {
  // TODO(yongjie): do something here.
  return true;
}

@end
