// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutNodeManager.h"
#import <Foundation/Foundation.h>
#import "LynxCustomMeasureDelegate+Internal.h"
#import "LynxLayoutNode.h"
#import "LynxMeasureFuncDarwin.h"

#include "lynx/core/public/layout_node_manager.h"

using namespace lynx::tasm;

@implementation LynxLayoutNodeManager {
  LayoutNodeManager *layout_node_manager;
}

- (instancetype)initWithNativePtr:(void *)nativePtr {
  self = [super init];
  if (self) {
    layout_node_manager = static_cast<LayoutNodeManager *>(nativePtr);
  }
  return self;
}

- (LynxFlexDirection)getFlexDirection:(NSInteger)sign {
  return (LynxFlexDirection)layout_node_manager->GetFlexDirection((int32_t)sign);
}

// Now only supports computed length, length with auto and percentage will be 0
- (CGFloat)getMarginLeft:(NSInteger)sign {
  return layout_node_manager->GetMarginLeft((int32_t)sign);
}

- (CGFloat)getMarginRight:(NSInteger)sign {
  return layout_node_manager->GetMarginRight((int32_t)sign);
}

- (CGFloat)getMarginTop:(NSInteger)sign {
  return layout_node_manager->GetMarginTop((int32_t)sign);
}

- (CGFloat)getMarginBottom:(NSInteger)sign {
  return layout_node_manager->GetMarginBottom((int32_t)sign);
}

- (CGFloat)getPaddingLeft:(NSInteger)sign {
  return layout_node_manager->GetPaddingLeft((int32_t)sign);
}

- (CGFloat)getPaddingRight:(NSInteger)sign {
  return layout_node_manager->GetPaddingRight((int32_t)sign);
}

- (CGFloat)getPaddingTop:(NSInteger)sign {
  return layout_node_manager->GetPaddingTop((int32_t)sign);
}

- (CGFloat)getPaddingBottom:(NSInteger)sign {
  return layout_node_manager->GetPaddingBottom((int32_t)sign);
}

- (CGFloat)getWidth:(NSInteger)sign {
  return layout_node_manager->GetWidth((int32_t)sign);
}

- (CGFloat)getHeight:(NSInteger)sign {
  return layout_node_manager->GetHeight((int32_t)sign);
}

- (CGFloat)getMinWidth:(NSInteger)sign {
  return layout_node_manager->GetMinHeight((int32_t)sign);
}
// if max-width is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)getMaxWidth:(NSInteger)sign {
  return layout_node_manager->GetMaxWidth((int32_t)sign);
}

- (CGFloat)getMinHeight:(NSInteger)sign {
  return layout_node_manager->GetMinHeight((int32_t)sign);
}
// if max-height is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
- (CGFloat)getMaxHeigh:(NSInteger)sign {
  return layout_node_manager->GetMaxHeight((int32_t)sign);
}

- (void)setMeasureFuncWithSign:(NSInteger)sign LayoutNode:(LynxLayoutNode *)layoutNode {
  return layout_node_manager->SetMeasureFunc((int32_t)sign,
                                             std::make_unique<LynxMeasureFuncDarwin>(layoutNode));
}

- (void)markDirtyAndRequestLayout:(NSInteger)sign {
  return layout_node_manager->MarkDirtyAndRequestLayout((int32_t)sign);
}

- (void)markDirtyAndForceLayout:(NSInteger)sign {
  return layout_node_manager->MarkDirtyAndForceLayout((int32_t)sign);
}

- (bool)isDirty:(NSInteger)sign {
  return layout_node_manager->IsDirty((int32_t)sign);
}

- (MeasureResult)measureWithSign:(NSInteger)sign
                    MeasureParam:(MeasureParam *)param
                  MeasureContext:(MeasureContext *)context {
  LayoutResult size = layout_node_manager->UpdateMeasureByPlatform(
      (int32_t)sign, param.width, (int32_t)param.widthMode, param.height, (int32_t)param.heightMode,
      context.finalMeasure);
  MeasureResult result;
  result.size = CGSizeMake(size.width_, size.height_);
  result.baseline = size.baseline_;
  return result;
}

- (void)alignWithSign:(NSInteger)sign
           AlignParam:(AlignParam *)param
         AlignContext:(AlignContext *)context {
  layout_node_manager->AlignmentByPlatform((int32_t)sign, param.topOffset, param.leftOffset);
}

@end
