// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxCustomMeasureShadowNode.h>

#import "LynxTransferShadowNode.h"

@implementation LynxTransferShadowNode {
  CGFloat _hostWidth;
  LynxMeasureMode _hostWidthMode;
  CGFloat _hostHeight;
  LynxMeasureMode _hostHeightMode;
}

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_SHADOW_NODE("transfer-view")
#else
LYNX_REGISTER_SHADOW_NODE("transfer-view")
#endif

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString*)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self) {
    self.hasCustomLayout = YES;
    _hostWidthMode = LynxMeasureModeIndefinite;
    _hostHeightMode = LynxMeasureModeIndefinite;
  }
  return self;
}

- (void)updateHostConstraintsWithWidth:(CGFloat)width
                             widthMode:(LynxMeasureMode)widthMode
                                height:(CGFloat)height
                            heightMode:(LynxMeasureMode)heightMode {
  if (fabs(_hostWidth - width) <= CGFLOAT_EPSILON && _hostWidthMode == widthMode &&
      fabs(_hostHeight - height) <= CGFLOAT_EPSILON && _hostHeightMode == heightMode) {
    return;
  }
  _hostWidth = width;
  _hostWidthMode = widthMode;
  _hostHeight = height;
  _hostHeightMode = heightMode;
  [self setNeedsLayout];
}

- (MeasureResult)customMeasureLayoutNode:(MeasureParam*)param
                          measureContext:(MeasureContext*)context {
  MeasureParam* childParam = [[MeasureParam alloc]
      initWithWidth:(_hostWidthMode == LynxMeasureModeIndefinite ? param.width : _hostWidth)
          WidthMode:(_hostWidthMode == LynxMeasureModeIndefinite ? param.widthMode : _hostWidthMode)
             Height:(_hostHeightMode == LynxMeasureModeIndefinite ? param.height : _hostHeight)
         HeightMode:(_hostHeightMode == LynxMeasureModeIndefinite ? param.heightMode
                                                                  : _hostHeightMode)];
  for (LynxShadowNode* child in self.children) {
    if (![child isVirtual]) {
      [self.layoutNodeManager measureWithSign:child.sign
                                 MeasureParam:childParam
                               MeasureContext:context];
    }
  }
  return (MeasureResult){CGSizeZero, 0.f};
}

- (void)customAlignLayoutNode:(AlignParam*)param alignContext:(AlignContext*)context {
  AlignParam* childParam = [AlignParam new];
  [childParam SetAlignOffsetWithLeft:0.f Top:0.f];
  for (LynxShadowNode* child in self.children) {
    if (![child isVirtual]) {
      [self.layoutNodeManager alignWithSign:child.sign AlignParam:childParam AlignContext:context];
    }
  }
}

@end
