// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGlobalObserver+Internal.h"
#import "LynxWeakProxy.h"

@interface LynxGlobalObserver ()

@property NSMutableArray<callback>* animationObservers;
@property NSMutableArray<callback>* layoutObservers;
@property NSMutableArray<callback>* scrollObservers;
@property NSMutableArray<callback>* propertyObservers;

@property(nonatomic) CADisplayLink* displayLink;
// The frequency of executing callback for animation.
@property(nonatomic) int32_t frameRate;
// Record animations that overlap in time.
@property(nonatomic) int32_t animationCount;

@end

@implementation LynxGlobalObserver

- (instancetype)init {
  _animationObservers = [NSMutableArray new];
  _layoutObservers = [NSMutableArray new];
  _scrollObservers = [NSMutableArray new];
  _propertyObservers = [NSMutableArray new];
  _frameRate = 20;
  return self;
}

- (void)dealloc {
  [_displayLink invalidate];
}

- (void)setObserverFrameRate:(int32_t)rate {
  if (rate < 0) {
    return;
  }
  if (rate > 60) {
    _frameRate = 60;
    return;
  }

  _frameRate = rate;
}

// ====================== Observing the CSS animation executing ======================
// 1. Add the callback to modify flag when init LynxUIExposure.
// 2. Start a timer to execute callback when recive the first animation event after last timer
// destroy.
// 3. Detect exposure in the next runloop after executing callback.
// 4. Remove the timer when all animation end, there may be more than one animation event.
// 5. Remove the callback when dealloc LynxUIExposure.

- (void)addAnimationObserver:(callback)callback {
  [_animationObservers addObject:callback];
}

- (void)removeAnimationObserver:(callback)callback {
  [_animationObservers removeObject:callback];
}

- (void)notifyAnimationStart {
  [self onAnimationStart];
}

- (void)onAnimationStart {
  _animationCount += 1;
  // If displaylink is nil, it means it's the first animation event.
  if (_displayLink == nil) {
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(onAnimation:)];
    if (@available(iOS 10.0, *)) {
      _displayLink.preferredFramesPerSecond = _frameRate;  // ms = 1000 / 20 = 50ms
    } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      _displayLink.frameInterval = 60 / _frameRate;  // ms = (1000/60) * 3 = 50ms
#pragma clang diagnostic pop
    }
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
  } else {
    [_displayLink setPaused:NO];
  }
}

- (void)notifyAnimationEnd {
  [self onAnimationEnd];
}

- (void)onAnimationEnd {
  _animationCount -= 1;
  if (_animationCount == 0) {
    [self onAnimation:_displayLink];
    [_displayLink setPaused:YES];
  }
}

- (void)onAnimation:(CADisplayLink*)sender {
  for (callback block in [NSArray arrayWithArray:_animationObservers]) {
    block(nil);
  }
}

// ====================== Observing layout finished for the component ======================
// 1. Add the callback to modify flag when init LynxUIExposure.
// 2. Execute callback to modify flag when component did finish layout.
// 3. Detect exposure in the next runloop after executing callback.
// 4. Remove the callback when dealloc LynxUIExposure.

- (void)addLayoutObserver:(callback)callback {
  [_layoutObservers addObject:callback];
}

- (void)removeLayoutObserver:(callback)callback {
  [_layoutObservers removeObject:callback];
}

- (void)notifyLayout:(NSDictionary*)options {
  [self onLayout:options];
}

- (void)onLayout:(NSDictionary*)options {
  for (callback block in [NSArray arrayWithArray:_layoutObservers]) {
    block(options);
  }
}

// ====================== Observing scrolling for scrollable components ======================
// 1. Add the callback to modify flag when init LynxUIExposure.
// 2. Execute callback to modify flag when scrollable components did scroll.
// 3. Detect exposure in the next runloop after executing callback.
// 4. Remove the callback when dealloc LynxUIExposure.

- (void)addScrollObserver:(callback)callback {
  [_scrollObservers addObject:callback];
}

- (void)removeScrollObserver:(callback)callback {
  [_scrollObservers removeObject:callback];
}

- (void)notifyScroll:(NSDictionary*)options {
  [self onScroll:options];
}

- (void)onScroll:(NSDictionary*)options {
  for (callback block in [NSArray arrayWithArray:_scrollObservers]) {
    block(options);
  }
}

// ====================== Observing property changed for the component ======================
// 1. Add the callback to modify flag when init LynxUIExposure.
// 2. Execute callback to modify flag when the component's property did change.
// 3. Detect exposure in the next runloop after executing callback.
// 4. Remove the callback when dealloc LynxUIExposure.

- (void)addPropertyObserver:(callback)callback {
  [_propertyObservers addObject:callback];
}

- (void)removePropertyObserver:(callback)callback {
  [_propertyObservers removeObject:callback];
}

- (void)notifyProperty:(NSDictionary*)options {
  [self onProperty:options];
}

- (void)onProperty:(NSDictionary*)options {
  for (callback block in [NSArray arrayWithArray:_propertyObservers]) {
    block(options);
  }
}

@end
