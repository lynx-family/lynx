// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMarkdownInlineViewHandle.h"

#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIOwner.h>

#import "LynxMarkdownResourceLoader.h"

static LynxMeasureMode LynxMarkdownToLynxMeasureMode(ServalMarkdownLayoutMode mode) {
  switch (mode) {
    case kServalMarkdownLayoutModeDefinite:
      return LynxMeasureModeDefinite;
    case kServalMarkdownLayoutModeAtMost:
      return LynxMeasureModeAtMost;
    case kServalMarkdownLayoutModeIndefinite:
    default:
      return LynxMeasureModeIndefinite;
  }
}

@implementation LynxMarkdownInlineViewHandle {
  __weak LynxNativeLayoutNode *_layoutNode;
  __weak id<LynxMarkdownResourceLoaderHost> _host;
  __weak LynxUI *_ui;
  CGFloat _width;
  CGFloat _height;
  CGFloat _baseline;
  CGFloat _left;
  CGFloat _top;
}

- (instancetype)initWithLayoutNode:(LynxNativeLayoutNode *)layoutNode
                              host:(id<LynxMarkdownResourceLoaderHost>)host {
  self = [super init];
  if (self != nil) {
    _layoutNode = layoutNode;
    _host = host;
    _width = 0.f;
    _height = 0.f;
    _baseline = 0.f;
    _left = 0.f;
    _top = 0.f;
  }
  return self;
}

- (void)requestMeasure {
}

- (void)requestAlign {
}

- (void)requestDraw {
}

- (ServalMarkdownMeasureResult)measureByWidth:(CGFloat)width
                                    WidthMode:(ServalMarkdownLayoutMode)widthMode
                                       Height:(CGFloat)height
                                   HeightMode:(ServalMarkdownLayoutMode)heightMode {
  LynxNativeLayoutNode *layoutNode = _layoutNode;
  if (layoutNode == nil || layoutNode.isDestroy) {
    return (ServalMarkdownMeasureResult){0.f, 0.f, 0.f};
  }
  MeasureParam *param =
      [[MeasureParam alloc] initWithWidth:width
                                WidthMode:LynxMarkdownToLynxMeasureMode(widthMode)
                                   Height:height
                               HeightMode:LynxMarkdownToLynxMeasureMode(heightMode)];
  MeasureResult result = [layoutNode measureWithMeasureParam:param
                                              MeasureContext:[_host markdownHostMeasureContext]];
  _width = result.size.width;
  _height = result.size.height;
  _baseline = result.baseline;
  return (ServalMarkdownMeasureResult){(float)_width, (float)_height, (float)_baseline};
}

- (void)align:(CGFloat)left top:(CGFloat)top {
  LynxNativeLayoutNode *layoutNode = _layoutNode;
  if (layoutNode == nil || layoutNode.isDestroy) {
    return;
  }
  _left = left;
  _top = top;

  AlignParam *alignParam = [[AlignParam alloc] init];
  [alignParam SetAlignOffsetWithLeft:left Top:top];
  AlignContext *alignContext = [_host markdownHostAlignContext];
  if (alignContext == nil) {
    alignContext = [[AlignContext alloc] init];
  }
  [layoutNode alignWithAlignParam:alignParam AlignContext:alignContext];
}

- (CGSize)getSize {
  return CGSizeMake(_width, _height);
}

- (CGPoint)getPosition {
  return CGPointMake(_left, _top);
}

- (void)setSize:(CGFloat)width height:(CGFloat)height {
  _width = width;
  _height = height;

  LynxUI *ui = [self findUI];
  if (ui == nil || ui.view == nil) {
    return;
  }
  void (^updateUI)(void) = ^{
    CGRect frame = ui.frame;
    frame.size = CGSizeMake(width, height);
    ui.frame = frame;
  };
  if ([NSThread isMainThread]) {
    updateUI();
  } else {
    dispatch_async(dispatch_get_main_queue(), updateUI);
  }
}

- (void)setPosition:(CGFloat)left top:(CGFloat)top {
  _left = left;
  _top = top;

  LynxUI *ui = [self findUI];
  if (ui == nil || ui.view == nil) {
    return;
  }
  void (^updateUI)(void) = ^{
    CGRect frame = ui.frame;
    frame.origin = CGPointMake(left, top);
    ui.frame = frame;
  };
  if ([NSThread isMainThread]) {
    updateUI();
  } else {
    dispatch_async(dispatch_get_main_queue(), updateUI);
  }
}

- (void)setVisibility:(BOOL)visible {
  LynxUI *ui = [self findUI];
  if (ui == nil || ui.view == nil) {
    return;
  }
  void (^updateUI)(void) = ^{
    ui.view.hidden = !visible;
    [ui.backgroundManager setHidden:!visible];
  };
  if ([NSThread isMainThread]) {
    updateUI();
  } else {
    dispatch_async(dispatch_get_main_queue(), updateUI);
  }
}

- (LynxUI *)findUI {
  if (_ui != nil) {
    return _ui;
  }
  LynxUIOwner *uiOwner = [_host markdownHostUIOwner];
  LynxNativeLayoutNode *layoutNode = _layoutNode;
  if (uiOwner == nil || layoutNode == nil) {
    return nil;
  }
  _ui = [uiOwner findUIBySign:layoutNode.sign];
  return _ui;
}

@end
