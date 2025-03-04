// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutStyle.h"
#import <Foundation/Foundation.h>
#import "core/public/layout_node_manager.h"

using namespace lynx::tasm;

@implementation LynxLayoutStyle {
  LayoutNodeManager* layoutNodeManager_;
}

- (instancetype)initWithSign:(NSInteger)sign layoutNodeManager:(void*)layoutNodeManagerPtr {
  self = [super init];
  if (self) {
    _sign = sign;
    layoutNodeManager_ = static_cast<LayoutNodeManager*>(layoutNodeManagerPtr);
  }
  return self;
}

- (LynxFlexDirection)flexDirection {
  return (LynxFlexDirection) static_cast<LayoutNodeManager*>(layoutNodeManager_)
      ->GetFlexDirection((int32_t)_sign);
}

- (CGFloat)computedMarginLeft {
  return layoutNodeManager_->GetMarginLeft((int32_t)_sign);
}

- (CGFloat)computedMarginRight {
  return layoutNodeManager_->GetMarginRight((int32_t)_sign);
}

- (CGFloat)computedMarginTop {
  return layoutNodeManager_->GetMarginTop((int32_t)_sign);
}

- (CGFloat)computedMarginBottom {
  return layoutNodeManager_->GetMarginBottom((int32_t)_sign);
}

- (CGFloat)computedPaddingLeft {
  return layoutNodeManager_->GetPaddingLeft((int32_t)_sign);
}

- (CGFloat)computedPaddingRight {
  return layoutNodeManager_->GetPaddingRight((int32_t)_sign);
}

- (CGFloat)computedPaddingTop {
  return layoutNodeManager_->GetPaddingTop((int32_t)_sign);
}

- (CGFloat)computedPaddingBottom {
  return layoutNodeManager_->GetPaddingBottom((int32_t)_sign);
}

- (CGFloat)computedWidth {
  return layoutNodeManager_->GetWidth((int32_t)_sign);
}

- (CGFloat)computedHeight {
  return layoutNodeManager_->GetHeight((int32_t)_sign);
}

- (CGFloat)computedMinWidth {
  return layoutNodeManager_->GetMinWidth((int32_t)_sign);
}

- (CGFloat)computedMaxWidth {
  return layoutNodeManager_->GetMaxWidth((int32_t)_sign);
}

- (CGFloat)computedMinHeight {
  return layoutNodeManager_->GetMaxHeight((int32_t)_sign);
}

- (CGFloat)computedMaxHeight {
  return layoutNodeManager_->GetMaxHeight((int32_t)_sign);
}

@end
