// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRendererHost.h>
#import "LynxDisplayListApplier+Internal.h"

#include <stack>
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

using namespace lynx;
using namespace lynx::tasm;

@implementation LynxDisplayListApplier {
  __weak UIView<LynxRendererHost> *_view;
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
}

- (instancetype)initWithView:(UIView<LynxRendererHost> *)view {
  self = [super init];
  if (self) {
    _view = view;

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
  NSArray<UIView *> *views = [_view subviews];

  while (content_op_index_ < list_->GetContentOpTypesSize()) {
    auto op = static_cast<DisplayListOpType>(list_->GetOpAtIndex(content_op_index_++));
    auto int_count = list_->GetIntAtIndex(content_int_index_++);
    auto float_count = list_->GetIntAtIndex(content_int_index_++);

    size_t next_int_index = content_int_index_ + int_count;
    size_t next_float_index = content_float_index_ + float_count;

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

        auto &box = box_array_[clip_index];
        CGRect rect = CGRectMake(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
        rect.origin.x += left_offset_;
        rect.origin.y += top_offset_;
        // TODO(songshourui.null): consider the border radius later.
        [layer setFrame:rect];

        if (_refLayer == nil) {
          [_view.layer addSublayer:layer];
        } else {
          [_view.layer insertSublayer:layer above:_refLayer];
        }
        _refLayer = layer;
        break;
      }
      case DisplayListOpType::kDrawView: {
        if (int_count == 1) {
          [[maybe_unused]] auto view_id = [self nextContentInt];
        }
        UIView *nextView = views[view_index++];
        _refLayer = nextView.layer;
        break;
      }
      case DisplayListOpType::kText: {
        // TODO(songshourui.null): implement the kText action later.
        break;
      }
      case DisplayListOpType::kImage: {
        // TODO(songshourui.null): implement the kImage action later.
        break;
      }
      case DisplayListOpType::kBorder: {
        // TODO(songshourui.null): implement the kBorder action later.
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

    // Ensure alignment
    content_int_index_ = next_int_index;
    content_float_index_ = next_float_index;
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

@end
