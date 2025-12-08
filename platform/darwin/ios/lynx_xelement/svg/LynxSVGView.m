// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxView.h>
#import <XElement/LynxSVGView.h>
#import <XElement/LynxUISVG.h>

@interface LynxSVGView ()
@property(nonatomic, assign) BOOL recreateLayerContents;
@property(nonatomic, assign) BOOL invalidated;
@end

@implementation LynxSVGView

// Mark view dirty, and need to refresh the svg content.
- (void)invalidate {
  self.invalidated = YES;
  [self.layer setNeedsDisplay];
}

- (instancetype)initWithFrame:(CGRect)frame ui:(LynxUISVG *)ui {
  if (self = [self initWithFrame:frame]) {
    self.ui = ui;
    self.recreateLayerContents = NO;
    self.invalidated = NO;
  }
  return self;
}

- (void)displayLayer:(CALayer *)layer {
  // Not invalidated. Apply the current Image to contents.
  if (!self.invalidated) {
    self.layer.contents = (__bridge id)self.image.CGImage;
    return;
  }

  // Invalidate during recreating new layer contents, do nothing.
  // New contents will be created after finishing current rendering task.
  if (self.recreateLayerContents && self.invalidated) {
    self.layer.contents = (__bridge id)self.image.CGImage;
    return;
  }

  // Clear invalidated flag and use a temp flag recreateLayerContents to
  // enable invalidate during rendering.
  if (!self.recreateLayerContents && self.invalidated) {
    self.recreateLayerContents = YES;
    self.invalidated = NO;
    [self.ui updateLayoutIfNeed];
  }
}

- (void)setImage:(UIImage *)image {
  // Call on UI thread.
  if (![NSThread isMainThread]) {
    return;
  }
  _image = image;
  self.recreateLayerContents = NO;
  self.layer.contents = (__bridge id)image.CGImage;
  if (self.invalidated) {
    [self.layer setNeedsDisplay];
  }
}

@end
