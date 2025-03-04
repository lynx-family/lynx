// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/DevToolOverlayDelegate.h>

@interface DevToolOverlayDelegate ()

@property(nonatomic, strong) id<OverlayService> service;

@end

@implementation DevToolOverlayDelegate

+ (instancetype)sharedInstance {
  static DevToolOverlayDelegate *instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[self alloc] init];
  });
  return instance;
}

- (NSArray<NSNumber *> *)getAllVisibleOverlaySign {
  return self.service ? [self.service getAllVisibleOverlaySign] : nil;
}

- (void)initWithService:(id<OverlayService>)service {
  self.service = service;
}

@end
