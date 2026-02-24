// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxTextLayer.h>
#import "LynxDisplayListApplier+Internal.h"

#include <stack>
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

using namespace lynx;
using namespace lynx::tasm;

@implementation LynxDisplayListApplier {
  __weak UIView<LynxRendererHost> *_view;
  LynxRendererContext *_renderer_context;
  DisplayList *list_;

  size_t content_op_index_;
  size_t content_int_index_;
  size_t content_float_index_;

  std::stack<float> x_stack_;
  float left_offset_;

  std::stack<float> y_stack_;
  float top_offset_;

  base::Vector<RoundedRectangle> box_array_;

  CALayer *_refLayer;

  NSMutableDictionary<NSNumber *, LynxImageManager *> *_imageManagers;
}

- (instancetype)initWithView:(UIView<LynxRendererHost> *)view
                  andContext:(LynxRendererContext *)context {
  self = [super init];
  if (self) {
    _view = view;
    _renderer_context = context;

    content_op_index_ = 0;
    content_int_index_ = 0;
    content_float_index_ = 0;

    left_offset_ = 0;
    top_offset_ = 0;

    _refLayer = nil;
  }
  return self;
}

- (int32_t)nextContentInt {
  return list_->GetIntAtIndex(content_int_index_++);
}

- (float)nextContentFloat {
  return list_->GetFloatAtIndex(content_float_index_++);
}

- (void)processContentOperations {
  if (list_ == nullptr || list_->GetContentOpTypesSize() == 0) {
    return;
  }

  int32_t view_index = 0;
  UIView *refView = nil;

  while (content_op_index_ < list_->GetContentOpTypesSize()) {
    auto op = static_cast<DisplayListOpType>(list_->GetOpAtIndex(content_op_index_++));
    auto int_count = list_->GetIntAtIndex(content_int_index_++);
    auto float_count = list_->GetIntAtIndex(content_int_index_++);

    switch (op) {
      case DisplayListOpType::kBegin: {
        bool record_offset = false;
        if (int_count == 1) {
          record_offset = [[_view getRenderer] getSign] != [self nextContentInt];
        }

        if (float_count == 4) {
          x_stack_.emplace([self nextContentFloat]);
          if (record_offset) left_offset_ += x_stack_.top();

          y_stack_.emplace([self nextContentFloat]);
          if (record_offset) top_offset_ += y_stack_.top();

          [self nextContentFloat];
          [self nextContentFloat];
        }
        break;
      }
      case DisplayListOpType::kEnd: {
        left_offset_ -= x_stack_.top();
        x_stack_.pop();

        top_offset_ -= y_stack_.top();
        y_stack_.pop();
        break;
      }
      case DisplayListOpType::kFill: {
        auto argb = [self nextContentInt];
        auto clip_index = [self nextContentInt];

        CGFloat a = ((argb >> 24) & 0xFF) / 255.0;
        CGFloat r = ((argb >> 16) & 0xFF) / 255.0;
        CGFloat g = ((argb >> 8) & 0xFF) / 255.0;
        CGFloat b = (argb & 0xFF) / 255.0;

        CALayer *layer = [[CALayer alloc] init];
        layer.backgroundColor = [[UIColor alloc] initWithRed:r green:g blue:b alpha:a].CGColor;
        [self applyRectToLayer:layer withBoxIndex:clip_index];
        [self insertLayer:layer];
        break;
      }
      case DisplayListOpType::kDrawView: {
        if (int_count == 1) {
          [[maybe_unused]] auto view_id = [self nextContentInt];
        }
        refView = [_view subviews][view_index++];
        _refLayer = refView.layer;
        break;
      }
      case DisplayListOpType::kText: {
        if (int_count >= 2) {
          auto text_id = [self nextContentInt];
          auto box_index = [self nextContentInt];

          LynxTextLayer *layer = [[LynxTextLayer alloc]
              initWithLynxTextRenderer:(LynxTextRenderer *)[_renderer_context.textRenderManager
                                           takeTextRender:text_id]];
          [self applyRectToLayer:layer withBoxIndex:box_index];
          [self insertLayer:layer];
          [layer setNeedsDisplay];
        }
        break;
      }
      case DisplayListOpType::kImage: {
        if (int_count >= 2) {
          auto image_id = [self nextContentInt];
          [[maybe_unused]] auto box_index = [self nextContentInt];
          LynxImageManager *imageManager = [self imageManagerForID:image_id];

          UIImageView *imageView = [self createImageView];

          auto &box = box_array_[box_index];
          CGRect rect = CGRectMake(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
          rect.origin.x += left_offset_;
          rect.origin.y += top_offset_;
          [imageView setFrame:rect];

          [imageManager setTarget:imageView];

          if (refView == nil) {
            [_view insertSubview:imageView atIndex:0];
          } else {
            [_view insertSubview:imageView aboveSubview:refView];
          }

          view_index++;
        }
        break;
      }
      case DisplayListOpType::kBorder: {
        if (int_count >= 10) {
          [[maybe_unused]] int out_box_index = [self nextContentInt];
          [[maybe_unused]] int inner_box_index = [self nextContentInt];

          [[maybe_unused]] int top_color = [self nextContentInt];
          [[maybe_unused]] int right_color = [self nextContentInt];
          [[maybe_unused]] int bottom_color = [self nextContentInt];
          [[maybe_unused]] int left_color = [self nextContentInt];

          [[maybe_unused]] int top_style = [self nextContentInt];
          [[maybe_unused]] int right_style = [self nextContentInt];
          [[maybe_unused]] int bottom_style = [self nextContentInt];
          [[maybe_unused]] int left_style = [self nextContentInt];

          // TODO(songshourui.null): implement the kBorder action later.
        }
        break;
      }
      case DisplayListOpType::kClipRect: {
        [[maybe_unused]] auto left = [self nextContentFloat];
        [[maybe_unused]] auto top = [self nextContentFloat];
        [[maybe_unused]] auto width = [self nextContentFloat];
        [[maybe_unused]] auto height = [self nextContentFloat];

        [[maybe_unused]] std::array<float, 8> arr = {};
        if (float_count > 4) {
          arr[0] = [self nextContentFloat];
          arr[1] = [self nextContentFloat];
          arr[2] = [self nextContentFloat];
          arr[3] = [self nextContentFloat];
          arr[4] = [self nextContentFloat];
          arr[5] = [self nextContentFloat];
          arr[6] = [self nextContentFloat];
          arr[7] = [self nextContentFloat];
        }
        // TODO(songshourui.null): implement the kClipRect action later.
        break;
      }
      case DisplayListOpType::kRecordBox: {
        RoundedRectangle rect;

        rect.SetX([self nextContentFloat]);
        rect.SetY([self nextContentFloat]);
        rect.SetWidth([self nextContentFloat]);
        rect.SetHeight([self nextContentFloat]);

        if (float_count > 4) {
          rect.SetRadiusXTopLeft([self nextContentFloat]);
          rect.SetRadiusYTopLeft([self nextContentFloat]);
          rect.SetRadiusXTopRight([self nextContentFloat]);
          rect.SetRadiusYTopRight([self nextContentFloat]);
          rect.SetRadiusXBottomRight([self nextContentFloat]);
          rect.SetRadiusYBottomRight([self nextContentFloat]);
          rect.SetRadiusXBottomLeft([self nextContentFloat]);
          rect.SetRadiusYBottomLeft([self nextContentFloat]);
        }

        box_array_.emplace_back(std::move(rect));
        break;
      }
      default:
        break;
    }
  }
}

- (void)applyDisplayList:(lynx::tasm::DisplayList *)list {
  if (list == nullptr) {
    return;
  }

  if (_view == nil) {
    return;
  }

  [self reset];

  list_ = list;

  [self processContentOperations];
}

- (void)reset {
  content_op_index_ = 0;
  content_int_index_ = 0;
  content_float_index_ = 0;
  top_offset_ = 0;
  left_offset_ = 0;
  box_array_.clear();
}

- (void)applyRectToLayer:(CALayer *)layer withBoxIndex:(int32_t)index {
  auto &box = box_array_[index];
  CGRect rect = CGRectMake(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
  rect.origin.x += left_offset_;
  rect.origin.y += top_offset_;
  [layer setFrame:rect];
}

- (void)insertLayer:(CALayer *)layer {
  if (_refLayer == nil) {
    [_view.layer addSublayer:layer];
  } else {
    [_view.layer insertSublayer:layer above:_refLayer];
  }
  _refLayer = layer;
}

- (UIImageView *)createImageView {
  UIImageView *imageView = [[LynxImageLoader imageService] imageView];
  if (imageView) {
    // TODO(songshourui.null): handle AnimatedImageCallBack here.
    // [[LynxImageLoader imageService] addAnimatedImageCallBack:imageView UI:ui];
  } else {
    // fallback to create UIImageView if no imageService
    imageView = [UIImageView new];
  }
  imageView.clipsToBounds = YES;
  imageView.contentMode = UIViewContentModeScaleToFill;
  imageView.userInteractionEnabled = YES;
  return imageView;
}

- (LynxImageManager *)imageManagerForID:(int32_t)imageManagerID {
  if (_imageManagers == nil) {
    _imageManagers = [NSMutableDictionary new];
  }
  LynxImageManager *imageManager = _imageManagers[@(imageManagerID)];
  if (imageManager == nil) {
    imageManager = [_renderer_context takeImageManager:imageManagerID];
    if (imageManager != nil) {
      _imageManagers[@(imageManagerID)] = imageManager;
    }
  }
  return imageManager;
}

@end
