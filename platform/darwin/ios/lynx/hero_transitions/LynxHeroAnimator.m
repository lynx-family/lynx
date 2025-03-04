// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxHeroAnimator.h"
#import <QuartzCore/QuartzCore.h>

@implementation LynxHeroAnimator {
  CADisplayLink* _displayLink;
}

- (void)displayLinkCallback:(CADisplayLink*)link {
  _timePassed += _isReversed ? -link.duration : link.duration;
  if (_isReversed && -_timePassed > _totalTime - 1.0 / 120) {
    if (_delegate) {
      [_delegate complete:true];
    }
    [self stop];
    return;
  }

  if (!_isReversed && _timePassed > _totalTime - 1.0 / 120) {
    if (_delegate) {
      [_delegate complete:true];
    }
    [self stop];
    return;
  }

  if (_delegate) {
    [_delegate updateProgress:(_timePassed / _totalTime)];
  }
}

- (void)startWithTimePassed:(NSTimeInterval)timePassed
                  totalTime:(NSTimeInterval)totalTime
                 isReversed:(BOOL)isReversed {
  [self stop];
  self.timePassed = timePassed;
  self.isReversed = isReversed;
  self.totalTime = totalTime;
  _displayLink = [CADisplayLink displayLinkWithTarget:self
                                             selector:@selector(displayLinkCallback:)];
  [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
}

- (void)stop {
  if (!_displayLink) return;
  _displayLink.paused = true;
  [_displayLink removeFromRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  _displayLink = nil;
}
@end
