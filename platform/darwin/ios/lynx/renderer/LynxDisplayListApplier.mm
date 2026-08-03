// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundDrawable.h>
#import <Lynx/LynxBackgroundUtils.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxTextLayer.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/UIView+Lynx.h>
#import "LynxDisplayListApplier+Internal.h"
#import "LynxTextraLayer.h"

#include <stack>
#include "base/include/vector.h"
#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/dom/fragment/rounded_rectangle.h"

using namespace lynx;
using namespace lynx::tasm;

namespace {

// Renderer hosts need renderer callbacks and decoration-layer synchronization. Legacy LynxUI-backed
// views have no renderer, so update only their host geometry here.
bool UpdateLegacyViewLayoutOffsetIfNeeded(UIView *view, CGPoint offset) {
  if (view == nil) {
    return false;
  }

  CALayer *layer = view.layer;
  if (CATransform3DIsIdentity(layer.transform)) {
    CGRect frame = view.frame;
    if (CGPointEqualToPoint(frame.origin, offset)) {
      return false;
    }
    frame.origin = offset;
    [view setFrame:frame];
    return true;
  }

  BOOL anchorPointChanged = !CGPointEqualToPoint(layer.anchorPoint, CGPointZero);
  BOOL positionChanged = !CGPointEqualToPoint(layer.position, offset);
  if (!anchorPointChanged && !positionChanged) {
    return false;
  }
  layer.anchorPoint = CGPointZero;
  layer.position = offset;
  return true;
}

}  // namespace

@interface LynxDisplayListApplier ()

- (void)insertHostDecorationLayer:(CALayer *)layer;
- (void)insertLayer:(CALayer *)layer forOp:(DisplayListOpType)op;
- (BOOL)shouldInsertAsHostDecorationForOp:(DisplayListOpType)op;
- (void)applyRoundedRect:(const RoundedRectangle &)box toLayer:(CALayer *)layer;
- (CGRect)rectForRoundedRectangle:(const RoundedRectangle &)rect applyingOffsets:(BOOL)applyOffsets;
- (CALayer *)createLinearGradientLayerWithAngle:(float)angle
                                         colors:(NSArray<NSNumber *> *)colors
                                          stops:(NSArray<NSNumber *> *)stops
                                      originBox:(const RoundedRectangle &)originBox
                                        clipBox:(const RoundedRectangle &)clipBox
                                        repeatX:(int32_t)repeatX
                                        repeatY:(int32_t)repeatY;
- (LynxBorderRadii)borderRadiiWithRoundedRectangle:(const RoundedRectangle &)rect;
- (void)applyRoundedMaskToLayer:(CALayer *)layer
                      maskFrame:(CGRect)maskFrame
                           rect:(CGRect)rect
                    borderRadii:(LynxBorderRadii)radii;

@end

@implementation LynxDisplayListApplier {
  __weak UIView<LynxRendererHost> *_view;
  LynxRendererContext *_renderer_context;
  DisplayListReader reader_;

  std::stack<float> x_stack_;
  float left_offset_;

  std::stack<float> y_stack_;
  float top_offset_;

  std::stack<int32_t> sign_stack_;

  base::Vector<RoundedRectangle> box_array_;

  CALayer *_refLayer;
  CALayer *_hostDecorationRefLayer;

  NSMutableArray<UIImageView *> *_contentImageViews;
  NSMutableArray<CALayer *> *_contentLayers;
  NSMutableArray<CALayer *> *_hostDecorationLayers;
}

- (instancetype)initWithView:(UIView<LynxRendererHost> *)view
                  andContext:(LynxRendererContext *)context {
  self = [super init];
  if (self) {
    _view = view;
    _renderer_context = context;

    reader_ = DisplayListReader();

    left_offset_ = 0;
    top_offset_ = 0;

    _refLayer = nil;
    _hostDecorationRefLayer = nil;
    _contentImageViews = [NSMutableArray new];
    _contentLayers = [NSMutableArray new];
    _hostDecorationLayers = [NSMutableArray new];
  }
  return self;
}

- (BOOL)isTextServiceModeOn {
  LynxContext *lynxContext = _renderer_context.uiContext.lynxContext;
  return lynxContext != nil && lynxContext.isTextServiceModeOn;
}

- (void)processContentOperations {
  if (!reader_.HasNext()) {
    return;
  }

  NSArray<UIView *> *hostSubviewsSnapshot = [_view.subviews copy];
  int32_t view_index = 0;
  UIView *refView = nil;

  while (reader_.HasNext()) {
    const auto &item = reader_.Next();
    auto op = item.type;

    switch (op) {
      case DisplayListOpType::kBegin: {
        bool record_offset = _view.renderer.sign != item.payload.begin.id;
        sign_stack_.emplace(item.payload.begin.id);

        x_stack_.emplace(item.payload.begin.x);
        if (record_offset) left_offset_ += x_stack_.top();

        y_stack_.emplace(item.payload.begin.y);
        if (record_offset) top_offset_ += y_stack_.top();
        break;
      }
      case DisplayListOpType::kEnd: {
        if (!sign_stack_.empty()) {
          sign_stack_.pop();
        }
        left_offset_ -= x_stack_.top();
        x_stack_.pop();

        top_offset_ -= y_stack_.top();
        y_stack_.pop();
        break;
      }
      case DisplayListOpType::kFill: {
        auto argb = item.payload.fill.color;
        auto clip_index = item.payload.fill.clip_index;

        CGFloat a = ((argb >> 24) & 0xFF) / 255.0;
        CGFloat r = ((argb >> 16) & 0xFF) / 255.0;
        CGFloat g = ((argb >> 8) & 0xFF) / 255.0;
        CGFloat b = (argb & 0xFF) / 255.0;

        CALayer *layer = [[CALayer alloc] init];
        layer.backgroundColor = [[UIColor alloc] initWithRed:r green:g blue:b alpha:a].CGColor;
        [self applyRectToLayer:layer withBoxIndex:clip_index];
        if (clip_index >= 0 && static_cast<size_t>(clip_index) < box_array_.size()) {
          [self applyRoundedRect:box_array_[clip_index] toLayer:layer];
        }
        [self insertLayer:layer forOp:op];
        break;
      }
      case DisplayListOpType::kDrawView: {
        auto view_id = item.payload.draw_view.view_id;
        auto offset_x = item.payload.draw_view.offset_x;
        auto offset_y = item.payload.draw_view.offset_y;
        if (static_cast<NSUInteger>(view_index) < hostSubviewsSnapshot.count) {
          refView = hostSubviewsSnapshot[view_index++];
          _refLayer = refView.layer;
        } else {
          refView = nil;
          _refLayer = nil;
        }
        LynxRenderer *renderer = nil;
        if ([refView conformsToProtocol:@protocol(LynxRendererHost)]) {
          renderer = ((UIView<LynxRendererHost> *)refView).renderer;
        }
        NSNumber *refViewSign = refView.lynxSign;
        BOOL rendererSignMatches = renderer != nil && renderer.sign == view_id;
        BOOL legacyViewSignMatches =
            renderer == nil && refViewSign != nil && refViewSign.intValue == view_id;
        if (rendererSignMatches) {
          [renderer updateLayoutOffsetIfNeeded:CGPointMake(offset_x, offset_y)];
        } else if (legacyViewSignMatches) {
          UpdateLegacyViewLayoutOffsetIfNeeded(refView, CGPointMake(offset_x, offset_y));
        }
        break;
      }
      case DisplayListOpType::kText: {
        auto text_id = item.payload.text.text_id;
        auto box_index = item.payload.text.box_index;
        CALayer *layer = nil;
        if ([self isTextServiceModeOn]) {
          void *page = [_renderer_context getTextBundle:text_id];
          if (page != nullptr) {
            layer = [[LynxTextraLayer alloc] initWithTextID:text_id
                                            rendererContext:_renderer_context];
          }
        } else {
          LynxTextRenderer *textRenderer =
              (LynxTextRenderer *)[_renderer_context.textRenderManager takeTextRender:text_id];
          if (textRenderer != nil) {
            layer = [[LynxTextLayer alloc] initWithLynxTextRenderer:textRenderer];
          }
        }
        if (layer != nil) {
          [self applyRectToLayer:layer withBoxIndex:box_index];
          [self insertLayer:layer];
          [layer setNeedsDisplay];
        }
        break;
      }
      case DisplayListOpType::kImage: {
        auto image_id = item.payload.image.image_id;
        auto box_index = item.payload.image.box_index;
        if (box_index < 0 || static_cast<size_t>(box_index) >= box_array_.size()) {
          break;
        }
        LynxImageManager *imageManager = [self imageManagerForID:image_id];

        UIImageView *imageView = [self createImageView];

        auto &box = box_array_[box_index];
        CGRect rect = CGRectMake(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
        rect.origin.x += left_offset_;
        rect.origin.y += top_offset_;
        [imageView setFrame:rect];
        if (box.HasRadius()) {
          [self applyRoundedRect:box toLayer:imageView.layer];
        }

        [imageManager setTarget:imageView];

        if (refView == nil) {
          [_view insertSubview:imageView atIndex:0];
        } else {
          [_view insertSubview:imageView aboveSubview:refView];
        }

        [self insertLayer:imageView.layer forOp:op];

        refView = imageView;

        [_contentImageViews addObject:imageView];
        break;
      }
      case DisplayListOpType::kBorder: {
        int out_box_index = item.payload.border.out_index;
        int inner_box_index = item.payload.border.inner_index;

        // 4 colors: Top, Right, Bottom, Left (ARGB int)
        UIColor *topColor = [self colorFromARGB:item.payload.border.colors[0]];
        UIColor *rightColor = [self colorFromARGB:item.payload.border.colors[1]];
        UIColor *bottomColor = [self colorFromARGB:item.payload.border.colors[2]];
        UIColor *leftColor = [self colorFromARGB:item.payload.border.colors[3]];

        // 4 styles: Top, Right, Bottom, Left (0=none, 1=solid, 2=dashed, 3=dotted)
        int topStyle = item.payload.border.styles[0];
        int rightStyle = item.payload.border.styles[1];
        int bottomStyle = item.payload.border.styles[2];
        int leftStyle = item.payload.border.styles[3];

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

          [self insertLayer:borderLayer forOp:op];
        }
        break;
      }
      case DisplayListOpType::kClipRect: {
        // Clear previous clip state to avoid state leakage when multiple kClipRect in same frame
        _view.layer.mask = nil;
        _view.layer.cornerRadius = 0;
        _view.layer.masksToBounds = NO;

        RoundedRectangle roundedRect;
        roundedRect.SetX(item.payload.clip_rect.x + left_offset_);
        roundedRect.SetY(item.payload.clip_rect.y + top_offset_);
        roundedRect.SetWidth(item.payload.clip_rect.w);
        roundedRect.SetHeight(item.payload.clip_rect.h);

        CGRect rect = CGRectMake(roundedRect.GetX(), roundedRect.GetY(), roundedRect.GetWidth(),
                                 roundedRect.GetHeight());

        if (item.payload.clip_rect.has_radii) {
          roundedRect.SetRadiusXTopLeft(item.payload.clip_rect.radii[0]);
          roundedRect.SetRadiusYTopLeft(item.payload.clip_rect.radii[1]);
          roundedRect.SetRadiusXTopRight(item.payload.clip_rect.radii[2]);
          roundedRect.SetRadiusYTopRight(item.payload.clip_rect.radii[3]);
          roundedRect.SetRadiusXBottomRight(item.payload.clip_rect.radii[4]);
          roundedRect.SetRadiusYBottomRight(item.payload.clip_rect.radii[5]);
          roundedRect.SetRadiusXBottomLeft(item.payload.clip_rect.radii[6]);
          roundedRect.SetRadiusYBottomLeft(item.payload.clip_rect.radii[7]);

          CGRect viewBounds = _view.bounds;
          BOOL isFullView = CGRectEqualToRect(rect, viewBounds);

          if (isFullView && roundedRect.IsUniformRadius()) {
            _view.layer.cornerRadius = roundedRect.GetRadiusXTopLeft();
            _view.layer.masksToBounds = YES;
          } else {
            LynxBorderRadii radii = [self borderRadiiWithRoundedRectangle:roundedRect];
            [self applyRoundedMaskToLayer:_view.layer
                                maskFrame:_view.bounds
                                     rect:rect
                              borderRadii:radii];
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

        rect.SetX(item.payload.record_box.x);
        rect.SetY(item.payload.record_box.y);
        rect.SetWidth(item.payload.record_box.w);
        rect.SetHeight(item.payload.record_box.h);

        if (item.payload.record_box.has_radii) {
          rect.SetRadiusXTopLeft(item.payload.record_box.radii[0]);
          rect.SetRadiusYTopLeft(item.payload.record_box.radii[1]);
          rect.SetRadiusXTopRight(item.payload.record_box.radii[2]);
          rect.SetRadiusYTopRight(item.payload.record_box.radii[3]);
          rect.SetRadiusXBottomRight(item.payload.record_box.radii[4]);
          rect.SetRadiusYBottomRight(item.payload.record_box.radii[5]);
          rect.SetRadiusXBottomLeft(item.payload.record_box.radii[6]);
          rect.SetRadiusYBottomLeft(item.payload.record_box.radii[7]);
        }

        box_array_.emplace_back(std::move(rect));
        break;
      }
      case DisplayListOpType::kLinearGradient: {
        int32_t color_count = item.payload.linear_gradient.color_count;
        int32_t stop_count = item.payload.linear_gradient.stop_count;

        if (color_count < 0) {
          break;
        }

        NSMutableArray<NSNumber *> *colors = [NSMutableArray arrayWithCapacity:color_count];
        const uint32_t *color_data = reader_.Colors(item);
        for (int32_t i = 0; i < color_count; ++i) {
          [colors addObject:@(color_data[i])];
        }

        int32_t origin_index = item.payload.linear_gradient.tiling_index;
        int32_t clip_index = item.payload.linear_gradient.clip_index;
        int32_t repeat_x = item.payload.linear_gradient.repeat_x;
        int32_t repeat_y = item.payload.linear_gradient.repeat_y;

        if (origin_index < 0 || static_cast<size_t>(origin_index) >= box_array_.size() ||
            clip_index < 0 || static_cast<size_t>(clip_index) >= box_array_.size()) {
          break;
        }

        float angle = item.payload.linear_gradient.angle;
        NSMutableArray<NSNumber *> *stops = [NSMutableArray arrayWithCapacity:stop_count];
        const float *stop_data = reader_.Stops(item);
        for (int32_t i = 0; i < stop_count; ++i) {
          [stops addObject:@(stop_data[i] * 100.0f)];
        }

        CALayer *gradientLayer = [self createLinearGradientLayerWithAngle:angle
                                                                   colors:colors
                                                                    stops:stops
                                                                originBox:box_array_[origin_index]
                                                                  clipBox:box_array_[clip_index]
                                                                  repeatX:repeat_x
                                                                  repeatY:repeat_y];
        if (gradientLayer != nil) {
          [self insertLayer:gradientLayer];
        }
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

  reader_ = DisplayListReader(*list);

  [self processContentOperations];
  [_view setNeedsDisplay];
}

- (void)reset {
  reader_ = DisplayListReader();
  top_offset_ = 0;
  left_offset_ = 0;
  box_array_.clear();
  _refLayer = nil;
  _hostDecorationRefLayer = nil;
  sign_stack_ = std::stack<int32_t>();

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

  for (CALayer *layer in _hostDecorationLayers) {
    [layer removeFromSuperlayer];
  }
  [_hostDecorationLayers removeAllObjects];
}

- (void)detachHostDecorationLayers {
  _hostDecorationRefLayer = nil;
  for (CALayer *layer in _hostDecorationLayers) {
    [layer removeFromSuperlayer];
  }
}

- (void)reattachHostDecorationLayers {
  CALayer *hostLayer = _view.layer;
  CALayer *superLayer = hostLayer.superlayer;
  if (superLayer == nil || _hostDecorationLayers.count == 0) {
    return;
  }

  _hostDecorationRefLayer = nil;
  for (CALayer *layer in _hostDecorationLayers) {
    [layer removeFromSuperlayer];
    if (_hostDecorationRefLayer == nil) {
      [superLayer insertSublayer:layer below:hostLayer];
    } else {
      [superLayer insertSublayer:layer above:_hostDecorationRefLayer];
    }
    _hostDecorationRefLayer = layer;
  }
}

- (void)syncHostDecorationLayers {
  if (_view == nil || _hostDecorationLayers.count == 0) {
    return;
  }

  CALayer *hostLayer = _view.layer;
  float hostOpacity = hostLayer.opacity;
  BOOL hostHidden = _view.hidden || hostLayer.hidden;

  [CATransaction begin];
  [CATransaction setDisableActions:YES];

  for (CALayer *layer in _hostDecorationLayers) {
    layer.transform = CATransform3DIdentity;
    layer.anchorPoint = hostLayer.anchorPoint;
    layer.bounds = hostLayer.bounds;
    layer.position = hostLayer.position;
    layer.transform = hostLayer.transform;

    layer.opacity = hostOpacity;
    layer.hidden = hostHidden;
    layer.filters = hostLayer.filters;
  }

  [CATransaction commit];
}

- (void)onRebuildSubRenderers {
  for (UIImageView *imageView in _contentImageViews) {
    [imageView removeFromSuperview];
  }
  [_contentImageViews removeAllObjects];
}

- (void)applyRectToLayer:(CALayer *)layer withBoxIndex:(int32_t)index {
  if (index < 0 || static_cast<size_t>(index) >= box_array_.size()) {
    return;
  }
  auto &box = box_array_[index];
  CGRect rect = CGRectMake(box.GetX(), box.GetY(), box.GetWidth(), box.GetHeight());
  rect.origin.x += left_offset_;
  rect.origin.y += top_offset_;
  [layer setFrame:rect];
}

- (CGRect)rectForRoundedRectangle:(const RoundedRectangle &)rect
                  applyingOffsets:(BOOL)applyOffsets {
  CGRect boxRect = CGRectMake(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
  if (applyOffsets) {
    boxRect.origin.x += left_offset_;
    boxRect.origin.y += top_offset_;
  }
  return boxRect;
}

- (CALayer *)createLinearGradientLayerWithAngle:(float)angle
                                         colors:(NSArray<NSNumber *> *)colors
                                          stops:(NSArray<NSNumber *> *)stops
                                      originBox:(const RoundedRectangle &)originBox
                                        clipBox:(const RoundedRectangle &)clipBox
                                        repeatX:(int32_t)repeatX
                                        repeatY:(int32_t)repeatY {
  CGRect clipRect = [self rectForRoundedRectangle:clipBox applyingOffsets:YES];
  if (CGRectIsEmpty(clipRect)) {
    return nil;
  }

  CGRect localBorderRect = CGRectMake(0, 0, clipRect.size.width, clipRect.size.height);
  CGRect originRect = [self rectForRoundedRectangle:originBox applyingOffsets:YES];
  CGRect localPaintRect =
      CGRectOffset(originRect, -CGRectGetMinX(clipRect), -CGRectGetMinY(clipRect));

  LynxBackgroundLinearGradientDrawable *drawable =
      [[LynxBackgroundLinearGradientDrawable alloc] initWithArray:@[ @(angle), colors, stops ]];
  if (drawable == nil) {
    return nil;
  }

  drawable.repeatX = static_cast<LynxBackgroundRepeatType>(repeatX);
  drawable.repeatY = static_cast<LynxBackgroundRepeatType>(repeatY);
  [drawable prepareGradientWithBorderBox:localBorderRect
                             andPaintBox:localPaintRect
                             andClipRect:localBorderRect];

  CALayer *layer = drawable.verticalRepeatLayer;
  if (layer == nil) {
    return nil;
  }

  layer.frame = clipRect;

  if (clipBox.HasRadius()) {
    RoundedRectangle localClipBox = clipBox;
    localClipBox.SetX(0);
    localClipBox.SetY(0);
    [self applyRoundedRect:localClipBox toLayer:layer];
  }
  return layer;
}

- (void)applyRoundedRect:(const RoundedRectangle &)box toLayer:(CALayer *)layer {
  layer.mask = nil;
  layer.cornerRadius = 0;
  layer.masksToBounds = NO;

  if (!box.HasRadius()) {
    return;
  }

  if (box.IsUniformRadius()) {
    layer.cornerRadius = box.GetRadiusXTopLeft();
    layer.masksToBounds = YES;
    return;
  }

  LynxBorderRadii radii = [self borderRadiiWithRoundedRectangle:box];
  CGRect localRect = CGRectMake(0, 0, box.GetWidth(), box.GetHeight());
  [self applyRoundedMaskToLayer:layer maskFrame:layer.bounds rect:localRect borderRadii:radii];
}

- (LynxBorderRadii)borderRadiiWithRoundedRectangle:(const RoundedRectangle &)rect {
  LynxBorderRadii radii;
  radii.topLeftX.val = rect.GetRadiusXTopLeft();
  radii.topLeftX.unit = LynxBorderValueUnitDefault;
  radii.topLeftY.val = rect.GetRadiusYTopLeft();
  radii.topLeftY.unit = LynxBorderValueUnitDefault;
  radii.topRightX.val = rect.GetRadiusXTopRight();
  radii.topRightX.unit = LynxBorderValueUnitDefault;
  radii.topRightY.val = rect.GetRadiusYTopRight();
  radii.topRightY.unit = LynxBorderValueUnitDefault;
  radii.bottomRightX.val = rect.GetRadiusXBottomRight();
  radii.bottomRightX.unit = LynxBorderValueUnitDefault;
  radii.bottomRightY.val = rect.GetRadiusYBottomRight();
  radii.bottomRightY.unit = LynxBorderValueUnitDefault;
  radii.bottomLeftX.val = rect.GetRadiusXBottomLeft();
  radii.bottomLeftX.unit = LynxBorderValueUnitDefault;
  radii.bottomLeftY.val = rect.GetRadiusYBottomLeft();
  radii.bottomLeftY.unit = LynxBorderValueUnitDefault;
  return radii;
}

- (void)applyRoundedMaskToLayer:(CALayer *)layer
                      maskFrame:(CGRect)maskFrame
                           rect:(CGRect)rect
                    borderRadii:(LynxBorderRadii)radii {
  CAShapeLayer *maskLayer = [CAShapeLayer layer];
  maskLayer.frame = maskFrame;
  CGPathRef path = [LynxBackgroundUtils createBezierPathWithRoundedRect:rect borderRadii:radii];
  maskLayer.path = path;
  CGPathRelease(path);
  layer.mask = maskLayer;
}

- (void)insertLayer:(CALayer *)layer {
  if (_refLayer == nil) {
    [_view.layer insertSublayer:layer atIndex:0];
  } else {
    [_view.layer insertSublayer:layer above:_refLayer];
  }
  [_contentLayers addObject:layer];
  _refLayer = layer;
}

- (void)insertLayer:(CALayer *)layer forOp:(DisplayListOpType)op {
  if ([self shouldInsertAsHostDecorationForOp:op]) {
    [self insertHostDecorationLayer:layer];
  } else {
    [self insertLayer:layer];
  }
}

- (void)insertHostDecorationLayer:(CALayer *)layer {
  CALayer *hostLayer = _view.layer;
  CALayer *superLayer = hostLayer.superlayer;
  if (superLayer == nil) {
    [_hostDecorationLayers addObject:layer];
    return;
  }

  if (_hostDecorationRefLayer == nil) {
    [superLayer insertSublayer:layer below:hostLayer];
  } else {
    [superLayer insertSublayer:layer above:_hostDecorationRefLayer];
  }
  [_hostDecorationLayers addObject:layer];
  _hostDecorationRefLayer = layer;
}

- (BOOL)shouldInsertAsHostDecorationForOp:(DisplayListOpType)op {
  if (sign_stack_.empty() || sign_stack_.top() != _view.renderer.sign) {
    return NO;
  }
  return op == DisplayListOpType::kFill || op == DisplayListOpType::kBorder;
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
  return [_renderer_context imageManagerForID:imageManagerID];
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
  LynxBorderRadii cornerRadii = [self borderRadiiWithRoundedRectangle:outBox];

  // Generate border image using existing utility
  return LynxGetBorderLayerImage(borderStyles, size, cornerRadii, borderInsets, borderColors, NO);
}

@end
