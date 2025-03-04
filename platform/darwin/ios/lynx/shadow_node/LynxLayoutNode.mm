// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutNode.h"
#import "LynxCustomMeasureDelegate+Internal.h"
#import "LynxMeasureFuncDarwin.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#include "core/public/layout_node_manager.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

using namespace lynx::tasm;

@implementation MeasureParam

- (id)initWithWidth:(CGFloat)width
          WdithMode:(LynxMeasureMode)widthMode
             Height:(CGFloat)height
         HeightMode:(LynxMeasureMode)heightMode {
  return [self initWithWidth:width WidthMode:widthMode Height:height HeightMode:heightMode];
}

- (id)initWithWidth:(CGFloat)width
          WidthMode:(LynxMeasureMode)widthMode
             Height:(CGFloat)height
         HeightMode:(LynxMeasureMode)heightMode {
  if (self = [super init]) {
    _width = width;
    _widthMode = widthMode;
    _height = height;
    _heightMode = heightMode;
  }
  return self;
}

@end

@implementation MeasureContext

- (id)initWithFinalMeasure:(bool)finalMeasure {
  if (self = [super init]) {
    _finalMeasure = finalMeasure;
  }
  return self;
}

@end

@implementation AlignParam
- (void)SetAlignOffsetWithLeft:(CGFloat)leftOffset Top:(CGFloat)topOffset {
  _leftOffset = leftOffset;
  _topOffset = topOffset;
}
@end

@implementation AlignContext

@end

@implementation LynxLayoutNode

LayoutNodeManager *LynxGetLayoutNodeManager(void *layoutNodeManagerPtr) {
  return static_cast<LayoutNodeManager *>(layoutNodeManagerPtr);
}

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super init];
  if (self) {
    _sign = sign;
    _tagName = tagName;
  }
  return self;
}

- (void)turboNativeLayoutNode {
  if (_layoutNodeManagerPtr != nullptr && (_measureDelegate || _customMeasureDelegate)) {
    LynxGetLayoutNodeManager(_layoutNodeManagerPtr)
        ->SetMeasureFunc((int32_t)_sign, std::make_unique<LynxMeasureFuncDarwin>(self));
  }
}

- (void)adoptNativeLayoutNode:(int64_t)ptr {
  _style = [[LynxLayoutStyle alloc] initWithSign:_sign layoutNodeManager:_layoutNodeManagerPtr];
  [self turboNativeLayoutNode];
}

- (MeasureResult)measureWithWidth:(float)width
                        widthMode:(LynxMeasureMode)widthMode
                           height:(float)height
                       heightMode:(LynxMeasureMode)heightMode
                     finalMeasure:(bool)finalMeasure {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxLayoutNode::measure");
  MeasureResult result;
  if ([self measureDelegate] != nil) {
    CGSize resultSize = [[self measureDelegate] measureNode:self
                                                  withWidth:width
                                                  widthMode:widthMode
                                                     height:height
                                                 heightMode:heightMode];
    result.size = CGSizeMake(resultSize.width, resultSize.height);
  } else if ([self customMeasureDelegate] != nullptr) {
    MeasureContext *measure_context = [[MeasureContext alloc] initWithFinalMeasure:finalMeasure];
    MeasureParam *param = [[MeasureParam alloc] initWithWidth:width
                                                    WidthMode:widthMode
                                                       Height:height
                                                   HeightMode:heightMode];
    result = [[self customMeasureDelegate] measureWithMeasureParam:param
                                                    MeasureContext:measure_context];
  }
  return result;
}

- (void)align {
  AlignParam *param = [[AlignParam alloc] init];
  AlignContext *context = [[AlignContext alloc] init];
  [[self customMeasureDelegate] alignWithAlignParam:param AlignContext:context];
}

- (void)setMeasureDelegate:(id<LynxMeasureDelegate>)measureDelegate {
  _measureDelegate = measureDelegate;
  [self turboNativeLayoutNode];
}

- (void)setCustomMeasureDelegate:(id<LynxCustomMeasureDelegate>)measureDelegate {
  _customMeasureDelegate = measureDelegate;
  [self turboNativeLayoutNode];
}

- (void)updateLayoutWithFrame:(CGRect)frame {
  _frame = frame;
  [self layoutDidUpdate];
}

- (void)setNeedsLayout {
  if (_layoutNodeManagerPtr == nullptr) {
    return;
  }
  LynxGetLayoutNodeManager(_layoutNodeManagerPtr)->MarkDirtyAndRequestLayout((int32_t)_sign);
}

- (void)internalSetNeedsLayoutForce {
  if (_layoutNodeManagerPtr == nullptr) {
    return;
  }
  LynxGetLayoutNodeManager(_layoutNodeManagerPtr)->MarkDirtyAndForceLayout((int32_t)_sign);
}

- (BOOL)needsLayout {
  if (_layoutNodeManagerPtr == nullptr) {
    return NO;
  }
  return LynxGetLayoutNodeManager(_layoutNodeManagerPtr)->IsDirty((int32_t)_sign);
}

- (void)layoutDidStart {
}
- (void)layoutDidUpdate {
}

- (BOOL)hasCustomLayout {
  return NO;
}

@end
