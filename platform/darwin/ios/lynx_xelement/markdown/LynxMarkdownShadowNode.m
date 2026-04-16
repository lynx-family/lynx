// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxMarkdownShadowNode.h>

#import <Lynx/LynxEvent.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxWeakProxy.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

#import "adaptor/LynxMarkdownBundle.h"
#import "adaptor/LynxMarkdownEventDispatcher.h"
#import "adaptor/LynxMarkdownResourceLoader.h"
#import "adaptor/LynxServalMarkdownViewWrapper.h"

@interface LynxMarkdownShadowNode () <LynxMarkdownResourceLoaderHost,
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

@implementation LynxMarkdownShadowNode {
  LynxMarkdownBundle *_bundle;
  LynxServalMarkdownViewWrapper *_markdownView;
  LynxMarkdownResourceLoader *_resourceLoader;
  LynxMarkdownEventDispatcher *_eventDispatcher;
  MeasureContext *_measureContext;
  AlignContext *_alignContext;
  NSString *_contentID;
  NSString *_content;
  CADisplayLink *_displayLink;
  NSRunLoop *_layoutLoop;
}

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  self = [super initWithSign:sign tagName:tagName];
  if (self != nil) {
    _bundle = [[LynxMarkdownBundle alloc] init];
    _resourceLoader = [[LynxMarkdownResourceLoader alloc] initWithHost:self];
    _eventDispatcher = [[LynxMarkdownEventDispatcher alloc] initWithHost:self];
    _contentID = @"";
    _content = @"";
    _layoutLoop = [NSRunLoop currentRunLoop];
    [self setCustomMeasureDelegate:self];
  }
  return self;
}

- (BOOL)needsEventSet {
  return YES;
}

- (LynxServalMarkdownViewWrapper *)ensureMarkdownView {
  if (_markdownView != nil) {
    return _markdownView;
  }
  _markdownView = [[LynxServalMarkdownViewWrapper alloc] initWithShadowNode:self];
  _markdownView.resourceDelegate = _resourceLoader;
  _markdownView.eventDelegate = _eventDispatcher;
  _markdownView.exposureDelegate = _eventDispatcher;
  [_markdownView disableInternalVSync:true];
  [self createDisplayLink];
  _bundle.markdownView = _markdownView;
  return _markdownView;
}

- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(MeasureContext *)context {
  _measureContext = context;
  LynxServalMarkdownViewWrapper *markdownView = [self ensureMarkdownView];
  if (markdownView == nil) {
    return (MeasureResult){CGSizeZero, 0.f};
  }
  ServalMarkdownMeasureResult result =
      [markdownView measureByWidth:param.width
                         WidthMode:LynxMarkdownToServalLayoutMode(param.widthMode)
                            Height:param.height
                        HeightMode:LynxMarkdownToServalLayoutMode(param.heightMode)];
  return (MeasureResult){CGSizeMake(result.width, result.height), result.baseline};
}

- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)context {
  _alignContext = context;
  [[self ensureMarkdownView] align:param.leftOffset top:param.topOffset];
}

- (id)getExtraBundle {
  _bundle.markdownView = [self ensureMarkdownView];
  return _bundle;
}

LYNX_PROP_SETTER("content", setContent, NSString *) {
  if (requestReset || value == nil) {
    value = @"";
  }
  _content = value;
  [[self ensureMarkdownView] setContent:value];
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
  [[self ensureMarkdownView] setStyle:value];
}

LYNX_PROP_SETTER("animation-type", setAnimationType, NSString *) {
  if (requestReset || value == nil) {
    value = @"none";
  }
  [[self ensureMarkdownView] setAnimationType:LynxMarkdownToServalAnimationType(value)];
}

LYNX_PROP_SETTER("animation-velocity", setAnimationVelocity, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 1.f;
  }
  [[self ensureMarkdownView] setAnimationVelocity:value];
}

LYNX_PROP_SETTER("initial-animation-step", setInitialAnimationStep, NSInteger) {
  if (requestReset || value < 0) {
    value = 0;
  }
  [[self ensureMarkdownView] setInitialAnimationStep:(int)value];
}

LYNX_PROP_SETTER("text-maxline", setTextMaxLine, NSInteger) {
  if (requestReset) {
    value = -1;
  }
  [[self ensureMarkdownView] setNumberProp:kServalMarkdownPropsTextMaxline Value:value];
}

LYNX_PROP_SETTER("content-complete", setContentComplete, BOOL) {
  if (requestReset) {
    value = YES;
  }
  [[self ensureMarkdownView] setBooleanProp:kServalMarkdownPropsContentComplete Value:value];
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
  [[self ensureMarkdownView] setBooleanProp:kServalMarkdownPropsTypewriterDynamicHeight
                                      Value:value];
}

LYNX_PROP_SETTER("typewriter-height-transition-duration", setTransitionDuration, CGFloat) {
  if (requestReset || value < 0.f) {
    value = 0.f;
  }
  [[self ensureMarkdownView] setNumberProp:kServalMarkdownPropsTypewriterHeightTransitionDuration
                                     Value:value];
}

LYNX_PROP_SETTER("typewriter-height-transition-prefetch", setHeightPrefetch, BOOL) {
  if (requestReset) {
    value = NO;
  }
  [[self ensureMarkdownView] setBooleanProp:kServalMarkdownPropsTypewriterHeightTransitionPrefetch
                                      Value:value];
}

- (void)destroy {
  [_resourceLoader releaseResources];
  _measureContext = nil;
  _alignContext = nil;
  _markdownView.resourceDelegate = nil;
  _markdownView.eventDelegate = nil;
  _markdownView.exposureDelegate = nil;
  _bundle.markdownView = nil;
  _markdownView = nil;
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

- (void)onImageLoaded:(NSString *)url {
  if (self.isDestroy) {
    return;
  }
  LynxServalMarkdownViewWrapper *markdownView = [self ensureMarkdownView];
  [markdownView onImageLoaded:url];
}

- (void)onFontLoaded:(NSString *)family Weight:(int)weight Style:(int)style {
  if (self.isDestroy) {
    return;
  }
  LynxServalMarkdownViewWrapper *markdownView = [self ensureMarkdownView];
  [markdownView onFontLoaded:family Weight:weight Style:style];
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
  NSString *contentIDFromView = [[self ensureMarkdownView] getContentID];
  if (contentIDFromView.length > 0) {
    return contentIDFromView;
  }
  return _contentID ?: @"";
}

- (NSInteger)markdownContentLength {
  return _content.length;
}

- (void)createDisplayLink {
  if (_displayLink == nil && _layoutLoop != nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(displayLinkHandle:)];
    [_displayLink addToRunLoop:_layoutLoop forMode:NSRunLoopCommonModes];
  }
}

- (void)displayLinkHandle:(CADisplayLink *)sender {
  if (_markdownView != nil) {
    [_markdownView onLayoutFrame:sender.targetTimestamp * 1e9];
  }
}

@end
