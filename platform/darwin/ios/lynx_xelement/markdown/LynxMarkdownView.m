// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxWeakProxy.h>
#import <XElement/LynxMarkdownView.h>
#import "adaptor/LynxMarkdownBundle.h"
#import "adaptor/LynxServalMarkdownViewWrapper.h"

@interface LynxMarkdownView ()

- (void)invalidateDisplayLink;

@end

@implementation LynxMarkdownView {
  LynxServalMarkdownViewWrapper *_markdownView;
  CADisplayLink *_displayLink;
}

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    self.opaque = NO;
    self.clipsToBounds = YES;
  }
  return self;
}

- (void)dealloc {
  [self invalidateDisplayLink];
}

- (void)setBundle:(LynxMarkdownBundle *_Nullable)bundle {
  LynxServalMarkdownViewWrapper *newMarkdownView = bundle != nil ? bundle.markdownView : nil;
  if (newMarkdownView == _markdownView) {
    [self setNeedsDisplay];
    return;
  }
  if (newMarkdownView != nil) {
    [self createDisplayLink];
  } else {
    [self invalidateDisplayLink];
  }

  [_markdownView removeFromSuperview];
  _markdownView = newMarkdownView;

  if (_markdownView != nil) {
    [self addSubview:_markdownView];
    [self sendSubviewToBack:_markdownView];
  }
  [self setNeedsDisplay];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _markdownView.frame = self.bounds;
}

- (void)createDisplayLink {
  if (_displayLink == nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(displayLinkHandle:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  }
}

- (void)invalidateDisplayLink {
  [_displayLink invalidate];
  _displayLink = nil;
}

- (void)displayLinkHandle:(CADisplayLink *)sender {
  if (_markdownView != nil) {
    [_markdownView onRendererFrame:sender.targetTimestamp * 1e9];
  }
}
@end
