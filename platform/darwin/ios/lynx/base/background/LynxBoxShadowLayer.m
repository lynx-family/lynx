// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBoxShadowLayer.h"
#import "LynxUI+Internal.h"
#import "LynxUIUnitUtils.h"

@implementation LynxBoxShadowLayer {
  // backend for the contents.
  UIImage* _image;
  // dirty mark.
  BOOL _invalidated;
  // the temporary mark of `invalidated` during re-rendering.
  //  Use a temporary mark when start recreating the layer contents to enable an new invalidate
  //  mark.
  BOOL _recreateLayerContents;
}

@synthesize customShadowPath = _customShadowPath;
@synthesize maskPath = _maskPath;

- (id)init {
  return [self initWithUi:nil];
}

- (id)initWithUi:(LynxUI* _Nullable)ui {
  self = [super init];
  if (self) {
    _ui = ui;
  }
  return self;
}

- (void)setCustomShadowPath:(CGPathRef)customShadowPath {
  CGPathRef path = CGPathRetain(customShadowPath);
  if (_customShadowPath) {
    // Release retained path.
    CGPathRelease(_customShadowPath);
  }
  _customShadowPath = path;
}

- (CGPathRef)customShadowPath {
  return _customShadowPath;
}

- (void)setMaskPath:(CGPathRef)maskPath {
  CGPathRef path = CGPathRetain(maskPath);
  if (_maskPath) {
    // Release retained path.
    CGPathRelease(_maskPath);
  }
  _maskPath = path;
}

- (CGPathRef)maskPath {
  return _maskPath;
}

- (void)dealloc {
  if (_customShadowPath) {
    CGPathRelease(_customShadowPath);
  }
  if (_maskPath) {
    CGPathRelease(_maskPath);
  }
}

#pragma mark - render functions

/// Invoke after changing props of the shadow. Let layer know the shadow need to be re-draw.
- (void)invalidate {
  _invalidated = YES;
  [self setNeedsDisplay];
}

/// Drawing the shadow to the given graphic context.
/// - Parameters:
///   - shadowPath: the shadow is based on this path.
///   - maskPath: the outside edge of the border
///   - inset: inset shadow
+ (void)doRender:(CGContextRef)ctx
    withShadowColor:(CGColorRef)shadowColor
      andBlurRadius:(CGFloat)blurRadius
          andOffset:(CGSize)offset
            andPath:(CGPathRef)shadowPath
        andMaskPath:(CGPathRef)maskPath
            isInset:(BOOL)inset {
  CGPathRef mask = CGPathRetain(maskPath);
  CGPathRef shadow = CGPathRetain(shadowPath);
  if (inset) {
    // All inset shadow should render inside the content area.
    CGContextAddPath(ctx, mask);
    CGContextClip(ctx);
  }

  // Draw shadow
  CGContextSetShadowWithColor(ctx, offset, blurRadius, shadowColor);

  if (!inset) {
    // Clip the content area out, the shadow should not overlap on the content area.

    // Add bounding area rect.
    CGContextAddRect(ctx, CGContextGetClipBoundingBox(ctx));
    CGContextClosePath(ctx);
    // Add the shadow path.
    CGContextAddPath(ctx, shadow);

    // Clip to the path use even-odd mode. Clip to the area between shadow path and bounding rect.
    CGContextEOClip(ctx);
  }

  CGContextAddPath(ctx, shadow);
  // Make this compatible to CALayer.shadowPath. Draw the stroke when inset, according to the EO
  // rule.
  CGColorRef strokeColor = inset ? shadowColor : [[UIColor clearColor] CGColor];
  CGContextSetStrokeColorWithColor(ctx, strokeColor);
  CGContextSetFillColorWithColor(ctx, shadowColor);

  CGContextDrawPath(ctx, kCGPathFill);
  CGPathRelease(mask);
  CGPathRelease(shadowPath);
}

- (void)display {
  // Not invalidated. Apply the current Image to contents.
  if (!_invalidated) {
    self.contents = (__bridge id)_image.CGImage;
    return;
  }

  // Invalidate during recreating new layer contents, do nothing.
  // New contents will be created after finishing current rendering task.
  if (_recreateLayerContents && _invalidated) {
    self.contents = (__bridge id)_image.CGImage;
    return;
  }

  // Clear invalidated flag and use a temp flag recreateLayerContents to
  // enable invalidate during rendering.
  if (!_recreateLayerContents && _invalidated) {
    _recreateLayerContents = YES;
    _invalidated = NO;
    [self onDraw];
  }
}

- (void)onDraw {
  __weak typeof(self) weakSelf = self;
  // Copy for block capture value. Prevent multi-thread value writings.
  CGFloat scale = [LynxUIUnitUtils screenScale];
  CGSize offset = _customShadowOffset;
  CGFloat blur = _customShadowBlur;
  UIColor* shadowColorCopy = [_customShadowColor copy];
  BOOL insetCopy = self.inset;
  CGPathRef shadowPathCopy = CGPathCreateCopy(_customShadowPath);
  CGPathRef maskPathCopy = CGPathCreateCopy(_maskPath);
  CGRect frame = self.frame;

  if (_ui) {
    [_ui
        displayComplexBackgroundAsynchronouslyWithDisplay:^UIImage*() {
          __strong typeof(weakSelf) strongSelf = weakSelf;
          if (strongSelf) {
            UIImage* resultImage = [LynxUI
                imageWithActionBlock:^(CGContextRef _Nonnull context) {
                  CGContextTranslateCTM(context, -frame.origin.x, -frame.origin.y);
                  [LynxBoxShadowLayer doRender:context
                               withShadowColor:shadowColorCopy.CGColor
                                 andBlurRadius:blur
                                     andOffset:offset
                                       andPath:shadowPathCopy
                                   andMaskPath:maskPathCopy
                                       isInset:insetCopy];
                }
                              opaque:NO
                               scale:scale
                                size:frame.size];
            return resultImage;
          }
          return nil;
        }
        completion:^(UIImage* image) {
          [weakSelf setImage:image];
          CGPathRelease(shadowPathCopy);
          CGPathRelease(maskPathCopy);
        }];
  } else {
    // Clear the dirty mark if _ui doesn't exist.
    [self setImage:nil];
    CGPathRelease(shadowPathCopy);
    CGPathRelease(maskPathCopy);
  }
}

- (void)setImage:(UIImage*)image {
  // Call on UI thread.
  __weak typeof(self) weakSelf = self;
  if (![NSThread isMainThread]) {
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf setImage:image];
    });
    return;
  }
  _image = image;
  // Clear the temp mark.
  _recreateLayerContents = NO;
  self.contents = (__bridge id)image.CGImage;
  // If props changed during re-rendering, request a new draw pass.
  if (_invalidated) {
    [self setNeedsDisplay];
  }
}

@end
