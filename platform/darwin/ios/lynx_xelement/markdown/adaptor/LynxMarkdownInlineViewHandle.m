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
}

- (BOOL)isForNode:(LynxNativeLayoutNode *)layoutNode {
  return _layoutNode == layoutNode;
}

- (instancetype)initWithLayoutNode:(LynxNativeLayoutNode *)layoutNode
                              host:(id<LynxMarkdownResourceLoaderHost>)host {
  self = [super init];
  if (self != nil) {
    _layoutNode = layoutNode;
    _host = host;
  }
  return self;
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
  return (ServalMarkdownMeasureResult){(float)result.size.width, (float)result.size.height,
                                       (float)result.baseline};
}

- (void)align:(CGFloat)left top:(CGFloat)top {
  LynxNativeLayoutNode *layoutNode = _layoutNode;
  if (layoutNode == nil || layoutNode.isDestroy) {
    return;
  }

  AlignParam *alignParam = [[AlignParam alloc] init];
  // Native alignment uses the markdown content box as its coordinate origin. Lynx adds the
  // parent's padding and border when updating the UI frame below.
  [alignParam SetAlignOffsetWithLeft:left Top:top];
  AlignContext *alignContext = [_host markdownHostAlignContext];
  if (alignContext == nil) {
    alignContext = [[AlignContext alloc] init];
  }
  [layoutNode alignWithAlignParam:alignParam AlignContext:alignContext];
}

- (void)setRenderBounds:(CGRect)bounds {
  LynxUI *ui = [self findUI];
  if (ui == nil || ui.view == nil) {
    return;
  }
  CGPoint contentOffset = [_host markdownHostContentOffset];
  bounds.origin.x += contentOffset.x;
  bounds.origin.y += contentOffset.y;
  [ui setFrame:bounds];
  [ui frameDidChange];
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

- (ServalMarkdownVerticalAlign)getVerticalAlign {
  switch (_layoutNode.shadowNodeStyle.valign) {
    case LynxVerticalAlignTop:
      return kServalMarkdownVerticalAlignTop;
    case LynxVerticalAlignTextTop:
      return kServalMarkdownVerticalAlignTextTop;
    case LynxVerticalAlignCenter:
    case LynxVerticalAlignMiddle:
      return kServalMarkdownVerticalAlignCenter;
    case LynxVerticalAlignBottom:
      return kServalMarkdownVerticalAlignBottom;
    case LynxVerticalAlignTextBottom:
      return kServalMarkdownVerticalAlignTextBottom;
    case LynxVerticalAlignLength:
      return kServalMarkdownVerticalAlignLength;
    default:
      return kServalMarkdownVerticalAlignBaseline;
  }
}

- (CGFloat)getVerticalAlignLength {
  LynxShadowNodeStyle *style = _layoutNode.shadowNodeStyle;
  return style.valign == LynxVerticalAlignLength ? style.valignLength : 0.f;
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
