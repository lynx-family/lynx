// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundImageLayerInfo.h"

#import "LynxGradient.h"
@implementation LynxBackgroundImageLayerInfo

- (void)flushPropsToDrawable {
  _item.bounds = _clipRect;
  _item.origin = self.backgroundOrigin;
  _item.repeatX = self.repeatXType;
  _item.repeatY = self.repeatYType;
  _item.posX = self.backgroundPosX;
  _item.posY = self.backgroundPosY;
  _item.sizeX = self.backgroundSizeX;
  _item.sizeY = self.backgroundSizeY;
  _item.clip = self.backgroundClip;
}

- (void)drawInContext:(CGContextRef)ctx {
  if ([self.item isKindOfClass:[LynxBackgroundDrawable class]]) {
    BOOL toRestore = NO;
    CGPathRef clipPath = [self createClipPath];
    if (clipPath != nil) {
      CGContextSaveGState(ctx);
      toRestore = YES;
      CGContextAddPath(ctx, clipPath);
      CGContextClip(ctx);
      CGPathRelease(clipPath);
    }

    [self flushPropsToDrawable];
    // FIXME: once all background code is merged, this will be removed
    LynxBackgroundDrawable* drawable = (LynxBackgroundDrawable*)self.item;
    [drawable drawInContext:ctx
                 borderRect:_borderRect
                paddingRect:_paddingRect
                contentRect:_contentRect];
    if (toRestore) {
      CGContextRestoreGState(ctx);
    }
    return;
  } else {
    // not loaded
    return;
  }
}

/**
  Creates and returns a CGMutablePath. Note: Callers must release the returned path to prevent
  memory leaks.
*/
- (CGPathRef)createClipPath {
  if (_backgroundClip != LynxBackgroundClipBorderBox ||
      LynxCornerInsetsAreAboveThreshold(_cornerInsets)) {
    CGMutablePathRef clipPath = CGPathCreateMutable();
    LynxPathAddRoundedRect(clipPath, _clipRect, _cornerInsets);
    return clipPath;
  }
  return nil;
}

- (BOOL)prepareGradientLayers {
  if ([_item type] < LynxBackgroundImageLinearGradient) {
    return NO;
  }
  [self flushPropsToDrawable];
  [(LynxBackgroundGradientDrawable*)_item prepareGradientWithBorderBox:_borderRect
                                                           andPaintBox:_paintingRect
                                                           andClipRect:_clipRect];
  // Handle background-clip
  CGPathRef clipPath = [self createClipPath];
  if (clipPath != nil) {
    CAShapeLayer* maskLayer = [CAShapeLayer layer];
    maskLayer.path = clipPath;
    [[(LynxBackgroundGradientDrawable*)_item verticalRepeatLayer] setMask:maskLayer];
    CGPathRelease(clipPath);
  }
  return YES;
}

@end
