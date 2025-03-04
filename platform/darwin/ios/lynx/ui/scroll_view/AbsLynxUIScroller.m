// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "AbsLynxUIScroller.h"
#import <Foundation/Foundation.h>
#import "LynxPropsProcessor.h"

@implementation AbsLynxUIScroller

LYNX_PROP_SETTER("scroll-y", setScrollY, BOOL) {}

LYNX_PROP_SETTER("scroll-x", setScrollX, BOOL) {}

LYNX_PROP_SETTER("scroll-orientation", setScrollOrientation, NSString *) {}

LYNX_PROP_SETTER("scroll-x-reverse", setScrollXReverse, BOOL) {}

LYNX_PROP_SETTER("scroll-y-reverse", setScrollYReverse, BOOL) {}

LYNX_PROP_SETTER("scroll-bar-enable", setScrollBarEnable, BOOL) {}

LYNX_PROP_SETTER("upper-threshold", setUpperThreshold, NSInteger) {}

LYNX_PROP_SETTER("lower-threshold", setLowerThreshold, NSInteger) {}

LYNX_PROP_SETTER("scroll-top", setScrollTop, int) {}

LYNX_PROP_SETTER("scroll-left", setScrollLeft, int) {}

LYNX_PROP_SETTER("initial-scroll-offset", setInitialScrollOffset, int) {}

LYNX_PROP_SETTER("scroll-to-index", setScrollToIndex, int) {}

LYNX_PROP_SETTER("initial-scroll-to-index", setInitialScrollToIndex, int) {}

LYNX_PROP_SETTER("bounces", setBounces, BOOL) {}

LYNX_PROP_SETTER("enable-scroll", setEnableScroll, BOOL) {}

LYNX_PROP_SETTER("enable-scroll-monitor", setEnableScrollMonitor, BOOL) {}

LYNX_PROP_SETTER("scroll-monitor-tag", setScrollMonitorTag, NSString *) {}

LYNX_PROP_SETTER("enable-nested-scroll", setEnableNested, BOOL) {}

LYNX_PROP_SETTER("fading-edge-length", setFadingEdge, NSString *) {}

LYNX_PROP_SETTER("experimental-ios-gesture-frequency-increasing", exprIOSIncreaseFrequency, BOOL) {
  // On iOS, if the default scrolling capability of UIScrollView in UIKit is not used, the CPU
  // frequency may be reduced, resulting in slower rendering of <list-item>. To increase the CPU
  // main frequency, in the gesture scenario, we attempt to turn on the scrolling of UIScrollView,
  // but we do not consume the scrolling dispatched from the system to be compatible with this
  // scenario.
}

- (void)scrollInto:(LynxUI *)value alignToTop:(BOOL)alignToTop {
}

- (void)scrollInto:(LynxUI *)value
          isSmooth:(BOOL)isSmooth
         blockType:(NSString *)blockType
        inlineType:(NSString *)inlineTyle {
}

- (void)sendScrollEvent:(NSString *)name
              scrollTop:(float)top
              scollleft:(float)left
           scrollHeight:(float)height
            scrollWidth:(float)width
                 deltaX:(float)x
                 deltaY:(float)y {
}

- (BOOL)canScroll:(ScrollDirection)direction {
  return NO;
}

- (void)scrollByX:(float)delta {
}

- (void)scrollByY:(float)delta {
}

- (void)flickX:(float)velocity {
}

- (void)flickY:(float)velocity {
}

- (void)addScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate {
}
- (void)removeScrollerDelegate:(id<LynxUIScrollerDelegate>)delegate {
}

- (BOOL)notifyParent {
  return YES;
}

@end
