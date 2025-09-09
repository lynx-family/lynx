// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUIScrollView.h>

@implementation LynxUIScrollView

LYNX_LAZY_REGISTER_UI("scroll-view-new-arch")

LYNX_PROPS_GROUP_DECLARE(
    LYNX_PROP_DECLARE("scroll-orientation", setScrollOrientation, NSString *),

    LYNX_PROP_DECLARE("enable-scroll-bar", setEnableScrollBar, BOOL),

    LYNX_PROP_DECLARE("enable-scroll", setEnableScroll, BOOL),

    LYNX_PROP_DECLARE("bounces", setBounces, BOOL),

    LYNX_PROP_DECLARE("forwards-nested-scroll", setForwardsNestedScroll, NSInteger),

    LYNX_PROP_DECLARE("backwards-nested-scroll", setBackwardsNestedScroll, NSInteger),

    LYNX_PROP_DECLARE("initial-scroll-index", setInitialScrollIndex, NSInteger),

    LYNX_PROP_DECLARE("initial-scroll-offset", setInitialScrollOffset, NSString *),

    LYNX_PROP_DECLARE("upper-threshold", setUpperThreshold, NSString *),

    LYNX_PROP_DECLARE("lower-threshold", setLowerThreshold, NSString *),

    LYNX_PROP_DECLARE("scroll-event-throttle", setScrollEventThrottle, NSNumber *));

LYNX_UI_METHOD_CALL_SUPER(scrollTo)

LYNX_UI_METHOD_CALL_SUPER(scrollBy)

LYNX_UI_METHOD_CALL_SUPER(autoScroll)

@end
