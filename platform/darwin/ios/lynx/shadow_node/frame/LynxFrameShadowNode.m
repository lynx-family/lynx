// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxFrameShadowNode.h>
#import <Lynx/LynxNativeLayoutNode.h>

@implementation LynxFrameShadowNode

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("frame")
#else
LYNX_REGISTER_SHADOW_NODE("frame")
#endif

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    _intrinsicContentSize = CGSizeZero;
    self.hasCustomLayout = YES;
  }
  return self;
}

- (void)updateIntrinsicContentSize:(CGSize)size {
  if (!CGSizeEqualToSize(_intrinsicContentSize, size)) {
    _intrinsicContentSize = size;
    [self setNeedsLayout];
  }
}

- (MeasureResult)customMeasureLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)context {
  if (CGSizeEqualToSize(_intrinsicContentSize, CGSizeZero)) {
    return (MeasureResult){{param.width, param.height}, 0.f};
  }
  CGFloat width =
      (param.widthMode == LynxMeasureModeDefinite) ? param.width : _intrinsicContentSize.width;
  CGFloat height =
      (param.heightMode == LynxMeasureModeDefinite) ? param.height : _intrinsicContentSize.height;
  return (MeasureResult){{width, height}, 0.f};
}

@end
