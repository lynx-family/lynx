// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutStyle.h"
#import <Foundation/Foundation.h>

@implementation LynxLayoutStyle {
  LynxLayoutNodeManager* layoutNodeManager_;
}

- (instancetype)initWithSign:(NSInteger)sign
           layoutNodeManager:(LynxLayoutNodeManager*)layoutNodeManager {
  self = [super init];
  if (self) {
    _sign = sign;
    layoutNodeManager_ = layoutNodeManager;
  }
  return self;
}

- (LynxFlexDirection)flexDirection {
  return [layoutNodeManager_ getFlexDirection:_sign];
}

- (CGFloat)computedMarginLeft {
  return [layoutNodeManager_ getMarginLeft:_sign];
}

- (CGFloat)computedMarginRight {
  return [layoutNodeManager_ getMarginRight:_sign];
}

- (CGFloat)computedMarginTop {
  return [layoutNodeManager_ getMarginTop:_sign];
}

- (CGFloat)computedMarginBottom {
  return [layoutNodeManager_ getMarginBottom:_sign];
}

- (CGFloat)computedPaddingLeft {
  return [layoutNodeManager_ getPaddingLeft:_sign];
}

- (CGFloat)computedPaddingRight {
  return [layoutNodeManager_ getPaddingRight:_sign];
}

- (CGFloat)computedPaddingTop {
  return [layoutNodeManager_ getPaddingTop:_sign];
}

- (CGFloat)computedPaddingBottom {
  return [layoutNodeManager_ getPaddingBottom:_sign];
}

- (CGFloat)computedWidth {
  return [layoutNodeManager_ getWidth:_sign];
}

- (CGFloat)computedHeight {
  return [layoutNodeManager_ getHeight:_sign];
}

- (CGFloat)computedMinWidth {
  return [layoutNodeManager_ getMinWidth:_sign];
}

- (CGFloat)computedMaxWidth {
  return [layoutNodeManager_ getMaxWidth:_sign];
}

- (CGFloat)computedMinHeight {
  return [layoutNodeManager_ getMinHeight:_sign];
}

- (CGFloat)computedMaxHeight {
  return [layoutNodeManager_ getMaxHeigh:_sign];
}

@end
