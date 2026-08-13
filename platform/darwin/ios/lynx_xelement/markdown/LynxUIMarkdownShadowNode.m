// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxUIMarkdownShadowNode.h>

#import <UIKit/UIKit.h>

#import <Lynx/LynxContext.h>
#import <Lynx/LynxEvent.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxWeakProxy.h>
#import <ServalMarkdown/MarkdownMeasurer.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

#import "adaptor/LynxMarkdownBundle.h"
#import "adaptor/LynxMarkdownEventDispatcher.h"
#import "adaptor/LynxMarkdownResourceLoader.h"

@interface LynxUIMarkdownShadowNodeV2 () <LynxMarkdownResourceLoaderHost,
                                          LynxMarkdownEventDispatcherHost>
@end

static ServalMarkdownLayoutMode LynxMarkdownToServalLayoutMode(LynxMeasureMode mode) {
  switch (mode) {
    case LynxMeasureModeDefinite:
      return kServalMarkdownLayoutModeDefinite;
    case LynxMeasureModeAtMost:
      return kServalMarkdownLayoutModeAtMost;
    case LynxMeasureModeIndefinite:
    default:
      return kServalMarkdownLayoutModeIndefinite;
  }
}

static ServalMarkdownAnimationType LynxMarkdownToServalAnimationType(NSString *type) {
  if ([type isEqualToString:@"typewriter"]) {
    return kServalMarkdownAnimationTypeTypewriter;
  }
  return kServalMarkdownAnimationTypeNone;
}

static uint32_t LynxMarkdownColorComponentToByte(CGFloat value) {
  if (value < 0.f) {
    value = 0.f;
  } else if (value > 1.f) {
    value = 1.f;
  }
  return (uint32_t)(value * 255.f + 0.5f);
}

static uint32_t LynxUIMarkdownColorToARGB(UIColor *color) {
  if (color == nil) {
    return 0;
  }
  CGFloat red = 0.f;
  CGFloat green = 0.f;
  CGFloat blue = 0.f;
  CGFloat alpha = 0.f;
  if (![color getRed:&red green:&green blue:&blue alpha:&alpha]) {
    CGFloat white = 0.f;
    if ([color getWhite:&white alpha:&alpha]) {
      red = white;
      green = white;
      blue = white;
    }
  }
  return (LynxMarkdownColorComponentToByte(alpha) << 24) |
         (LynxMarkdownColorComponentToByte(red) << 16) |
         (LynxMarkdownColorComponentToByte(green) << 8) | LynxMarkdownColorComponentToByte(blue);
}

@implementation LynxUIMarkdownShadowNodeV2 {
  MarkdownMeasurer *_markdownMeasurer;
  LynxMarkdownResourceLoader *_resourceLoader;
  LynxMarkdownEventDispatcher *_eventDispatcher;
  MeasureContext *_measureContext;
  AlignContext *_alignContext;
  CGSize _measuredSize;
  CGPoint _contentOffset;
  NSString *_contentID;
  NSString *_content;
  CADisplayLink *_displayLink;
  NSRunLoop *_layoutLoop;
}

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self != nil) {
    _resourceLoader = [[LynxMarkdownResourceLoader alloc] initWithHost:self];
    _eventDispatcher = [[LynxMarkdownEventDispatcher alloc] initWithHost:self];
    _markdownMeasurer = [[MarkdownMeasurer alloc] init];
    _markdownMeasurer.resourceDelegate = _resourceLoader;
    _markdownMeasurer.eventDelegate = _eventDispatcher;
    _markdownMeasurer.exposureDelegate = _eventDispatcher;
    __weak typeof(self) weakSelf = self;
    _markdownMeasurer.requestMeasureCallback = ^{
      [weakSelf setNeedsLayout];
    };
    _measuredSize = CGSizeZero;
    _contentOffset = CGPointZero;
    _contentID = @"";
    _content = @"";
    _layoutLoop = [NSRunLoop currentRunLoop];
    [self setCustomMeasureDelegate:self];
    [self createDisplayLink];
  }
  return self;
}

- (BOOL)needsEventSet {
  return YES;
}

- (BOOL)isChildDirty {
  for (LynxShadowNode *child in self.children) {
    if (![child isKindOfClass:LynxNativeLayoutNode.class]) {
      continue;
    }
    if ([child needsLayout]) {
      return YES;
    }
  }
  return NO;
}

- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(MeasureContext *)context {
  _measureContext = context;
  MarkdownMeasurer *markdownMeasurer = _markdownMeasurer;
  if (markdownMeasurer == nil) {
    _measuredSize = CGSizeZero;
    return (MeasureResult){CGSizeZero, 0.f};
  }
  if ([self isChildDirty]) {
    [markdownMeasurer markDirty];
  }
  ServalMarkdownMeasureResult result =
      [markdownMeasurer measureWithWidth:param.width
                               widthMode:LynxMarkdownToServalLayoutMode(param.widthMode)
                                  height:param.height
                              heightMode:LynxMarkdownToServalLayoutMode(param.heightMode)];
  _measuredSize = CGSizeMake(result.width, result.height);
  return (MeasureResult){_measuredSize, result.baseline};
}

- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)context {
  _alignContext = context;
  [_markdownMeasurer align:param.leftOffset top:param.topOffset];
}

- (id)getExtraBundle {
  return [[LynxMarkdownBundleV2 alloc] initWithMarkdownMeasurer:_markdownMeasurer
                                                     shadowNode:self
                                                   measuredSize:_measuredSize];
}

LYNX_PROP_SETTER("content", setContent, NSString *) {
  if (requestReset || value == nil) {
    value = @"";
  }
  _content = value;
  _markdownMeasurer.content = value;
}

LYNX_PROP_SETTER("text-selection", setEnableTextSelection, BOOL) {
  if (requestReset) {
    value = NO;
  }
  [_markdownMeasurer setBooleanProp:kServalMarkdownPropsEnableTextSelection Value:value];
}

LYNX_PROP_SETTER("selection-background-color", setSelectionBackgroundColor, UIColor *) {
  if (requestReset) {
    value = nil;
  }
  [_markdownMeasurer setColorProp:kServalMarkdownPropsSelectionHighlightColor
                            Value:LynxUIMarkdownColorToARGB(value)];
}

LYNX_PROP_SETTER("selection-handle-color", setSelectionHandleColor, UIColor *) {
  if (requestReset) {
    value = nil;
  }
  [_markdownMeasurer setColorProp:kServalMarkdownPropsSelectionHandleColor
                            Value:LynxUIMarkdownColorToARGB(value)];
}

LYNX_PROP_SETTER("selection-handle-size", setSelectionHandleSize, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 0.f;
  }
  [_markdownMeasurer setNumberProp:kServalMarkdownPropsSelectionHandleSize Value:value];
}

LYNX_PROP_SETTER("markdown-effect", setMarkdownEffect, NSDictionary *) {
  if (requestReset) {
    value = nil;
  }
  [_markdownMeasurer setMapProp:kServalMarkdownPropsMarkdownEffect Value:value];
}

LYNX_PROP_SETTER("text-mark-attachments", setTextMarkAttachments, NSArray *) {
  if (requestReset) {
    value = nil;
  }
  [_markdownMeasurer setArrayProp:kServalMarkdownPropsTextMarkAttachments Value:value];
}

LYNX_PROP_SETTER("content-id", setContentID, NSString *) {
  if (requestReset || value == nil) {
    _contentID = @"";
  } else {
    _contentID = value;
  }
}

LYNX_PROP_SETTER("markdown-style", setMarkdownStyle, NSDictionary *) {
  if (requestReset || value == nil) {
    value = @{};
  }
  _markdownMeasurer.style = value;
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("animation-type", setAnimationType, NSString *) {
  if (requestReset || value == nil) {
    value = @"none";
  }
  _markdownMeasurer.animationType = LynxMarkdownToServalAnimationType(value);
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("animation-velocity", setAnimationVelocity, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 1.f;
  }
  _markdownMeasurer.animationVelocity = value;
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("initial-animation-step", setInitialAnimationStep, NSInteger) {
  if (requestReset || value < 0) {
    value = 0;
  }
  _markdownMeasurer.initialAnimationStep = (int)value;
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("text-maxline", setTextMaxLine, NSInteger) {
  if (requestReset) {
    value = -1;
  }
  [_markdownMeasurer setNumberProp:kServalMarkdownPropsTextMaxline Value:value];
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("content-complete", setContentComplete, BOOL) {
  if (requestReset) {
    value = YES;
  }
  [_markdownMeasurer setBooleanProp:kServalMarkdownPropsContentComplete Value:value];
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("image-downsampling", setImageDownSampling, BOOL) {
  if (requestReset) {
    value = NO;
  }
  _resourceLoader.enableImageDownSampling = value;
}

LYNX_PROP_SETTER("typewriter-dynamic-height", setDynamicHeight, BOOL) {
  if (requestReset) {
    value = NO;
  }
  [_markdownMeasurer setBooleanProp:kServalMarkdownPropsTypewriterDynamicHeight Value:value];
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("typewriter-height-transition-duration", setTransitionDuration, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 0.f;
  }
  [_markdownMeasurer setNumberProp:kServalMarkdownPropsTypewriterHeightTransitionDuration
                             Value:value];
}

LYNX_PROP_SETTER("typewriter-height-transition-prefetch", setHeightPrefetch, BOOL) {
  if (requestReset) {
    value = NO;
  }
  [_markdownMeasurer setBooleanProp:kServalMarkdownPropsTypewriterHeightTransitionPrefetch
                              Value:value];
}

LYNX_PROP_SETTER("markdown-max-height", setMarkdownMaxHeight, CGFloat) {
  if (requestReset) {
    value = 0.f;
  }
  [_markdownMeasurer setNumberProp:kServalMarkdownPropsMarkdownMaxHeight Value:value];
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("content-range", setMarkdownContentRange, NSArray *) {
  if (requestReset || ![value isKindOfClass:NSArray.class]) {
    return;
  }
  if (value.count > 0 && [value[0] isKindOfClass:NSNumber.class]) {
    [_markdownMeasurer setNumberProp:kServalMarkdownPropsContentRangeStart
                               Value:((NSNumber *)value[0]).intValue];
  }
  if (value.count > 1 && [value[1] isKindOfClass:NSNumber.class]) {
    [_markdownMeasurer setNumberProp:kServalMarkdownPropsContentRangeEnd
                               Value:((NSNumber *)value[1]).intValue];
  }
  [_markdownMeasurer markDirty];
}

LYNX_PROP_SETTER("exposure-tags", setExposureTags, NSArray *) {
  if (requestReset) {
    value = nil;
  }
  [_markdownMeasurer setArrayProp:kServalMarkdownPropsExposureTags Value:value];
}

LYNX_PROP_SETTER("animation-frame-rate", setAnimationFrameRate, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 0.f;
  }
  [_markdownMeasurer setNumberProp:kServalMarkdownPropsAnimationFrameRate Value:value];
}

LYNX_PROP_SETTER("allow-break-around-punctuation", setAllowBreakAroundPunctuation, BOOL) {
  if (requestReset) {
    value = NO;
  }
  [_markdownMeasurer setBooleanProp:kServalMarkdownPropsAllowBreakAroundPunctuation Value:value];
}

- (void)destroy {
  [_displayLink invalidate];
  _displayLink = nil;
  [_resourceLoader releaseResources];
  _measureContext = nil;
  _alignContext = nil;
  _markdownMeasurer.requestMeasureCallback = nil;
  _markdownMeasurer.resourceDelegate = nil;
  _markdownMeasurer.eventDelegate = nil;
  _markdownMeasurer.exposureDelegate = nil;
  _markdownMeasurer = nil;
  _measuredSize = CGSizeZero;
  _content = @"";
  [super destroy];
}

- (NSString *)currentContentID {
  return [self markdownParseEndContentID];
}

#pragma mark - LynxMarkdownResourceLoaderHost

- (BOOL)markdownHostDestroyed {
  return self.isDestroy;
}

- (NSArray<LynxShadowNode *> *)markdownHostChildren {
  return self.children ?: @[];
}

- (LynxUIOwner *)markdownHostUIOwner {
  return self.uiOwner;
}

- (MeasureContext *)markdownHostMeasureContext {
  return _measureContext;
}

- (AlignContext *)markdownHostAlignContext {
  return _alignContext;
}

- (void)setMarkdownContentOffset:(CGPoint)contentOffset {
  @synchronized(self) {
    _contentOffset = contentOffset;
  }
}

- (CGPoint)markdownHostContentOffset {
  @synchronized(self) {
    return _contentOffset;
  }
}

- (void)onImageLoaded:(NSString *)url {
  __weak typeof(self) weakSelf = self;
  [self runOnLayoutThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf.isDestroy) {
      return;
    }
    [strongSelf->_markdownMeasurer onImageLoaded:url];
  }];
}

- (void)onFontLoaded:(NSString *)family Weight:(int)weight Style:(int)style {
  __weak typeof(self) weakSelf = self;
  [self runOnLayoutThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf.isDestroy) {
      return;
    }
    [strongSelf->_markdownMeasurer onFontLoaded:family Weight:weight Style:style];
  }];
}

#pragma mark - LynxMarkdownEventDispatcherHost

- (BOOL)isBindEvent:(NSString *)name {
  return [self.eventSet objectForKey:name] != nil;
}

- (void)dispatchMarkdownEvent:(NSString *)name detail:(NSDictionary *_Nullable)detail {
  if (name.length == 0 || ![self isBindEvent:name]) {
    return;
  }
  LynxDetailEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                      targetSign:self.sign
                                                          detail:detail];
  __weak typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf.isDestroy) {
      return;
    }
    [strongSelf.uiOwner.uiContext.eventEmitter dispatchCustomEvent:event];
  });
}

- (NSString *)markdownParseEndContentID {
  return _contentID ?: @"";
}

- (NSInteger)markdownContentLength {
  return _content.length;
}

- (BOOL)pauseAnimation {
  if (self.isDestroy || _markdownMeasurer == nil) {
    return NO;
  }
  __weak typeof(self) weakSelf = self;
  [self runOnLayoutThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf.isDestroy) {
      return;
    }
    [strongSelf->_markdownMeasurer pauseAnimation];
  }];
  return YES;
}

- (BOOL)resumeAnimation:(NSInteger)animationStep {
  if (self.isDestroy || _markdownMeasurer == nil) {
    return NO;
  }
  __weak typeof(self) weakSelf = self;
  [self runOnLayoutThread:^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil || strongSelf.isDestroy) {
      return;
    }
    if (animationStep >= 0) {
      [strongSelf->_markdownMeasurer resumeAnimation:(int)animationStep];
    } else {
      [strongSelf->_markdownMeasurer resumeAnimation];
    }
  }];
  return YES;
}

- (void)runOnLayoutThread:(dispatch_block_t)block {
  if (block == nil) {
    return;
  }
  LynxContext *lynxContext = self.uiOwner.uiContext.lynxContext;
  if (lynxContext != nil) {
    [lynxContext runOnLayoutThread:block];
    return;
  }
  if (_layoutLoop == nil || _layoutLoop == NSRunLoop.currentRunLoop) {
    block();
    return;
  }
  [_layoutLoop performBlock:block];
}

- (void)createDisplayLink {
  if (_displayLink == nil && _layoutLoop != nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(displayLinkHandle:)];
    [_displayLink addToRunLoop:_layoutLoop forMode:NSRunLoopCommonModes];
  }
}

- (void)displayLinkHandle:(CADisplayLink *)sender {
  if (_markdownMeasurer != nil) {
    [_markdownMeasurer onLayoutFrame:sender.targetTimestamp * 1e9];
  }
}

@end
