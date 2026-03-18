// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundUtils.h>
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
  NSMutableArray<UIImageView *> *_contentImageViews;
  NSMutableArray<CALayer *> *_contentLayers;
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
    _contentImageViews = [NSMutableArray new];
    _contentLayers = [NSMutableArray new];
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

  NSArray<UIView *> *hostSubviewsSnapshot = [_view.subviews copy];
  int32_t view_index = 0;
  UIView *refView = nil;

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
        [self applyRectToLayer:layer withBoxIndex:clip_index];
        [self insertLayer:layer];
        break;
      }
      case DisplayListOpType::kDrawView: {
        if (int_count == 1) {
          [[maybe_unused]] auto view_id = [self nextContentInt];
        }
        if (static_cast<NSUInteger>(view_index) < hostSubviewsSnapshot.count) {
          refView = hostSubviewsSnapshot[view_index++];
          _refLayer = refView.layer;
        } else {
          refView = nil;
          _refLayer = nil;
        }
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
          [_contentImageViews addObject:imageView];

          view_index++;
        }
        break;
      }
      case DisplayListOpType::kBorder: {
        if (int_count >= 10) {
          int out_box_index = [self nextContentInt];
          int inner_box_index = [self nextContentInt];

          // 4 colors: Top, Right, Bottom, Left (ARGB int)
          UIColor *topColor = [self colorFromARGB:[self nextContentInt]];
          UIColor *rightColor = [self colorFromARGB:[self nextContentInt]];
          UIColor *bottomColor = [self colorFromARGB:[self nextContentInt]];
          UIColor *leftColor = [self colorFromARGB:[self nextContentInt]];

          // 4 styles: Top, Right, Bottom, Left (0=none, 1=solid, 2=dashed, 3=dotted)
          int topStyle = [self nextContentInt];
          int rightStyle = [self nextContentInt];
          int bottomStyle = [self nextContentInt];
          int leftStyle = [self nextContentInt];

          // Validate box indices
          if (out_box_index < 0 || static_cast<size_t>(out_box_index) >= box_array_.size() ||
              inner_box_index < 0 || static_cast<size_t>(inner_box_index) >= box_array_.size()) {
            break;
          }

          const RoundedRectangle &outBox = box_array_[out_box_index];
          const RoundedRectangle &innerBox = box_array_[inner_box_index];

          // Create border image
          UIImage *borderImage =
              [self createBorderImageWithOutBox:outBox
                                          inner:innerBox
                                         colors:@[ topColor, rightColor, bottomColor, leftColor ]
                                         styles:@[
                                           @(topStyle), @(rightStyle), @(bottomStyle), @(leftStyle)
                                         ]];

          if (borderImage) {
            CALayer *borderLayer = [CALayer layer];
            borderLayer.contents = (id)borderImage.CGImage;

            // Set frame with offset
            CGRect frame = CGRectMake(outBox.GetX() + left_offset_, outBox.GetY() + top_offset_,
                                      outBox.GetWidth(), outBox.GetHeight());
            borderLayer.frame = frame;

            [self insertLayer:borderLayer];
          }
        }
        break;
      }
      case DisplayListOpType::kClipRect: {
        // Clear previous clip state to avoid state leakage when multiple kClipRect in same frame
        _view.layer.mask = nil;
        _view.layer.cornerRadius = 0;
        _view.layer.masksToBounds = NO;

        float left = [self nextContentFloat];
        float top = [self nextContentFloat];
        float width = [self nextContentFloat];
        float height = [self nextContentFloat];

        CGRect rect = CGRectMake(left + left_offset_, top + top_offset_, width, height);

        if (float_count >= 12) {
          CGFloat tlX = [self nextContentFloat];
          CGFloat tlY = [self nextContentFloat];
          CGFloat trX = [self nextContentFloat];
          CGFloat trY = [self nextContentFloat];
          CGFloat brX = [self nextContentFloat];
          CGFloat brY = [self nextContentFloat];
          CGFloat blX = [self nextContentFloat];
          CGFloat blY = [self nextContentFloat];

          CGRect viewBounds = _view.bounds;
          BOOL isFullView = CGRectEqualToRect(rect, viewBounds);
          BOOL isUniformRadius = (tlX == tlY && tlX == trX && tlX == trY && tlX == brX &&
                                  tlX == brY && tlX == blX && tlX == blY && tlX > 0);

          if (isFullView && isUniformRadius) {
            _view.layer.cornerRadius = tlX;
            _view.layer.masksToBounds = YES;
          } else {
            LynxBorderRadii radii;
            radii.topLeftX.val = tlX;
            radii.topLeftX.unit = LynxBorderValueUnitDefault;
            radii.topLeftY.val = tlY;
            radii.topLeftY.unit = LynxBorderValueUnitDefault;
            radii.topRightX.val = trX;
            radii.topRightX.unit = LynxBorderValueUnitDefault;
            radii.topRightY.val = trY;
            radii.topRightY.unit = LynxBorderValueUnitDefault;
            radii.bottomRightX.val = brX;
            radii.bottomRightX.unit = LynxBorderValueUnitDefault;
            radii.bottomRightY.val = brY;
            radii.bottomRightY.unit = LynxBorderValueUnitDefault;
            radii.bottomLeftX.val = blX;
            radii.bottomLeftX.unit = LynxBorderValueUnitDefault;
            radii.bottomLeftY.val = blY;
            radii.bottomLeftY.unit = LynxBorderValueUnitDefault;

            CAShapeLayer *maskLayer = [CAShapeLayer layer];
            maskLayer.frame = _view.bounds;
            CGPathRef path = [LynxBackgroundUtils createBezierPathWithRoundedRect:rect
                                                                      borderRadii:radii];
            maskLayer.path = path;
            CGPathRelease(path);
            _view.layer.mask = maskLayer;
          }
        } else {
          if (CGRectEqualToRect(rect, _view.bounds)) {
            _view.layer.masksToBounds = YES;
          } else {
            CAShapeLayer *maskLayer = [CAShapeLayer layer];
            maskLayer.frame = _view.bounds;
            maskLayer.path = [UIBezierPath bezierPathWithRect:rect].CGPath;
            _view.layer.mask = maskLayer;
          }
        }
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
  _refLayer = nil;

  // Clear previous clip state
  if (_view) {
    _view.layer.mask = nil;
    _view.layer.cornerRadius = 0;
    _view.layer.masksToBounds = NO;
  }

  for (UIImageView *imageView in _contentImageViews) {
    [imageView removeFromSuperview];
  }
  [_contentImageViews removeAllObjects];

  for (CALayer *layer in _contentLayers) {
    [layer removeFromSuperlayer];
  }
  [_contentLayers removeAllObjects];
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
  [_contentLayers addObject:layer];
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

#pragma mark - Border Helpers

- (UIColor *)colorFromARGB:(int)argb {
  CGFloat a = ((argb >> 24) & 0xFF) / 255.0;
  CGFloat r = ((argb >> 16) & 0xFF) / 255.0;
  CGFloat g = ((argb >> 8) & 0xFF) / 255.0;
  CGFloat b = (argb & 0xFF) / 255.0;
  return [UIColor colorWithRed:r green:g blue:b alpha:a];
}

- (LynxBorderStyle)lynxBorderStyleFromInt:(int)style {
  switch (style) {
    case 0:
      return LynxBorderStyleSolid;
    case 1:
      return LynxBorderStyleDashed;
    case 2:
      return LynxBorderStyleDotted;
    case 3:
      return LynxBorderStyleDouble;
    case 4:
      return LynxBorderStyleGroove;
    case 5:
      return LynxBorderStyleRidge;
    case 6:
      return LynxBorderStyleInset;
    case 7:
      return LynxBorderStyleOutset;
    case 8:
      return LynxBorderStyleHidden;
    case 9:
      return LynxBorderStyleNone;
    default:
      return LynxBorderStyleNone;
  }
}

- (LynxBorderRadii)convertToLynxBorderRadii:(const RoundedRectangle &)rect {
  LynxBorderRadii radii;

  // Top-Left
  radii.topLeftX.val = rect.GetRadiusXTopLeft();
  radii.topLeftX.unit = LynxBorderValueUnitDefault;
  radii.topLeftY.val = rect.GetRadiusYTopLeft();
  radii.topLeftY.unit = LynxBorderValueUnitDefault;

  // Top-Right
  radii.topRightX.val = rect.GetRadiusXTopRight();
  radii.topRightX.unit = LynxBorderValueUnitDefault;
  radii.topRightY.val = rect.GetRadiusYTopRight();
  radii.topRightY.unit = LynxBorderValueUnitDefault;

  // Bottom-Right
  radii.bottomRightX.val = rect.GetRadiusXBottomRight();
  radii.bottomRightX.unit = LynxBorderValueUnitDefault;
  radii.bottomRightY.val = rect.GetRadiusYBottomRight();
  radii.bottomRightY.unit = LynxBorderValueUnitDefault;

  // Bottom-Left
  radii.bottomLeftX.val = rect.GetRadiusXBottomLeft();
  radii.bottomLeftX.unit = LynxBorderValueUnitDefault;
  radii.bottomLeftY.val = rect.GetRadiusYBottomLeft();
  radii.bottomLeftY.unit = LynxBorderValueUnitDefault;

  return radii;
}

- (UIImage *)createBorderImageWithOutBox:(const RoundedRectangle &)outBox
                                   inner:(const RoundedRectangle &)innerBox
                                  colors:(NSArray<UIColor *> *)colors
                                  styles:(NSArray<NSNumber *> *)styles {
  // Validate output box dimensions
  const CGFloat width = outBox.GetWidth();
  const CGFloat height = outBox.GetHeight();
  if (width <= 0 || height <= 0) {
    return nil;
  }
  CGSize size = CGSizeMake(width, height);

  // Calculate border insets with non-negative constraint: {top, left, bottom, right}
  const CGFloat topInset = MAX(0.0f, innerBox.GetY() - outBox.GetY());
  const CGFloat leftInset = MAX(0.0f, innerBox.GetX() - outBox.GetX());
  const CGFloat bottomInset =
      MAX(0.0f, outBox.GetY() + outBox.GetHeight() - innerBox.GetY() - innerBox.GetHeight());
  const CGFloat rightInset =
      MAX(0.0f, outBox.GetX() + outBox.GetWidth() - innerBox.GetX() - innerBox.GetWidth());

  UIEdgeInsets borderInsets = UIEdgeInsetsMake(topInset, leftInset, bottomInset, rightInset);

  // Check if there's any valid border width
  if (borderInsets.top == 0 && borderInsets.left == 0 && borderInsets.bottom == 0 &&
      borderInsets.right == 0) {
    return nil;
  }

  // Convert to LynxBorderColors (order: top, left, bottom, right)
  LynxBorderColors borderColors;
  borderColors.top = colors[0].CGColor;
  borderColors.left = colors[3].CGColor;
  borderColors.bottom = colors[2].CGColor;
  borderColors.right = colors[1].CGColor;

  // Convert to LynxBorderStyles (order: top, left, bottom, right)
  LynxBorderStyles borderStyles;
  borderStyles.top = [self lynxBorderStyleFromInt:styles[0].intValue];
  borderStyles.left = [self lynxBorderStyleFromInt:styles[3].intValue];
  borderStyles.bottom = [self lynxBorderStyleFromInt:styles[2].intValue];
  borderStyles.right = [self lynxBorderStyleFromInt:styles[1].intValue];

  // Get corner radii
  LynxBorderRadii cornerRadii = [self convertToLynxBorderRadii:outBox];

  // Generate border image using existing utility
  return LynxGetBorderLayerImage(borderStyles, size, cornerRadii, borderInsets, borderColors, NO);
}

@end
