// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxServiceTextProtocol.h>

@implementation LynxTextInfo

- (instancetype)initWithWidth:(CGFloat)width {
  return [self initWithWidth:width height:0 content:nil];
}

- (instancetype)initWithWidth:(CGFloat)width content:(nullable NSArray<NSString *> *)content {
  return [self initWithWidth:width height:0 content:content];
}

- (instancetype)initWithWidth:(CGFloat)width height:(CGFloat)height {
  return [self initWithWidth:width height:height content:nil];
}

- (instancetype)initWithWidth:(CGFloat)width
                       height:(CGFloat)height
                      content:(nullable NSArray<NSString *> *)content {
  self = [super init];
  if (self) {
    _width = width;
    _height = height;
    _content = [content copy];
  }
  return self;
}

@end
