// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxWeakProxy.h>
#import <ServalMarkdown/MarkdownMeasurer.h>
#import <ServalMarkdown/ServalMarkdownView.h>
#import <XElement/LynxMarkdownView.h>
#import "adaptor/LynxMarkdownBundle.h"

@interface LynxMarkdownViewV2 ()

- (void)invalidateDisplayLink;

@end

@implementation LynxMarkdownViewV2 {
  ServalMarkdownView *_markdownView;
  MarkdownMeasurer *_markdownMeasurer;
  CADisplayLink *_displayLink;
  CGSize _measuredSize;
  CGPoint _contentOffset;
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

- (ServalMarkdownView *_Nullable)setBundle:(LynxMarkdownBundleV2 *_Nullable)bundle {
  MarkdownMeasurer *newMarkdownMeasurer = bundle.markdownMeasurer;
  if (newMarkdownMeasurer != _markdownMeasurer) {
    [_markdownView removeFromSuperview];
    _markdownView = nil;
    _markdownMeasurer = nil;

    if (newMarkdownMeasurer != nil) {
      ServalMarkdownView *newMarkdownView = [[ServalMarkdownView alloc] initWithCreateMeasurer:NO];
      if ([newMarkdownView attachMarkdownMeasurer:newMarkdownMeasurer]) {
        [newMarkdownView disableInternalVSync:YES];
        _markdownMeasurer = newMarkdownMeasurer;
        _markdownView = newMarkdownView;
        [self addSubview:_markdownView];
        [self sendSubviewToBack:_markdownView];
      }
    }
  }

  if (_markdownView != nil) {
    [self createDisplayLink];
  } else {
    [self invalidateDisplayLink];
  }
  _measuredSize = bundle != nil ? bundle.measuredSize : CGSizeZero;
  [self layoutMarkdownView];
  [self setNeedsDisplay];
  return _markdownView;
}

- (void)setContentOffset:(CGPoint)contentOffset {
  if (CGPointEqualToPoint(_contentOffset, contentOffset)) {
    return;
  }
  _contentOffset = contentOffset;
  [self layoutMarkdownView];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self layoutMarkdownView];
}

- (void)layoutMarkdownView {
  _markdownView.frame = (CGRect){_contentOffset, _measuredSize};
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
