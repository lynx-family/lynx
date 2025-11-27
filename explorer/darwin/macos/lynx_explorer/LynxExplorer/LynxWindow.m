// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxWindow.h"
#import "ViewController.h"

@interface LynxWindow ()

@property(nonatomic, strong) NSButton *refresh_btn;

@end

@implementation LynxWindow

- (instancetype)init {
  self = [super init];
  if (self) {
    [self initUI];
  }
  return self;
}

- (void)initUI {
  NSUInteger style = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
                     NSWindowStyleMaskTitled | NSWindowStyleMaskResizable;
  [self setStyleMask:style];
  self.titlebarAppearsTransparent = YES;
}

@end
