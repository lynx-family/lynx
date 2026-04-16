// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxServalMarkdownViewWrapper.h"

#import <XElement/LynxMarkdownShadowNode.h>

@implementation LynxServalMarkdownViewWrapper

- (instancetype)init {
  return [self initWithShadowNode:nil];
}

- (instancetype)initWithShadowNode:(LynxMarkdownShadowNode *_Nullable)shadowNode {
  self = [super init];
  if (self != nil) {
    _shadowNode = shadowNode;
    self.opaque = NO;
    self.clipsToBounds = YES;
  }
  return self;
}

- (void)requestMeasure {
  [self.shadowNode setNeedsLayout];
  [self requestDraw];
}

- (void)requestAlign {
  [self requestMeasure];
}

- (void)requestDraw {
  if ([NSThread isMainThread]) {
    [super requestDraw];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [self setNeedsDisplay];
    });
  }
}

@end
