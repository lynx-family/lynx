// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxScreenMetrics.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxViewBuilder.h>
#import <Lynx/LynxViewGroup.h>
#import <Lynx/LynxViewportMetrics.h>
#import <XCTest/XCTest.h>

@interface LynxScreenMetricsUnitTest : XCTestCase
@end

@implementation LynxScreenMetricsUnitTest

- (void)testBuilderScreenMetricsOverrideGroup {
  LynxViewGroup* group = [[LynxViewGroup alloc] init];
  group.screenSize = CGSizeMake(390, 844);
  group.screenScale = 3;

  LynxViewBuilder* builder = [[LynxViewBuilder alloc] init];
  builder.lynxViewGroup = group;
  XCTAssertTrue(CGSizeEqualToSize(builder.screenSize, CGSizeMake(390, 844)));
  XCTAssertEqualWithAccuracy(builder.screenScale, 3, 0.001);

  builder.screenSize = CGSizeMake(360, 800);
  builder.screenScale = 2;
  XCTAssertTrue(CGSizeEqualToSize(builder.screenSize, CGSizeMake(360, 800)));
  XCTAssertEqualWithAccuracy(builder.screenScale, 2, 0.001);
}

- (void)testInvalidBuilderScreenMetricsFallBackToGroup {
  LynxViewGroup* group = [[LynxViewGroup alloc] init];
  group.screenSize = CGSizeMake(390, 844);
  group.screenScale = 3;

  LynxViewBuilder* builder = [[LynxViewBuilder alloc] init];
  builder.lynxViewGroup = group;
  builder.screenSize = CGSizeMake(360, 0);
  builder.screenScale = NAN;

  XCTAssertTrue(CGSizeEqualToSize(builder.screenSize, CGSizeMake(390, 844)));
  XCTAssertEqualWithAccuracy(builder.screenScale, 3, 0.001);
}

- (void)testUIContextReplacesScreenMetricsSnapshot {
  LynxScreenMetrics* initial = [[LynxScreenMetrics alloc] initWithScreenSize:CGSizeMake(390, 844)
                                                                       scale:3];
  LynxUIContext* context = [[LynxUIContext alloc] initWithScreenMetrics:initial];
  LynxScreenMetrics* next = [[LynxScreenMetrics alloc] initWithScreenSize:CGSizeMake(360, 800)
                                                                    scale:2];

  [context updateScreenMetrics:next];

  XCTAssertEqual(context.screenMetrics, next);
  XCTAssertTrue(CGSizeEqualToSize(initial.screenSize, CGSizeMake(390, 844)));
  XCTAssertEqualWithAccuracy(initial.scale, 3, 0.001);
}

- (void)testUIContextViewportMetricsLifecycle {
  LynxScreenMetrics* screenMetrics =
      [[LynxScreenMetrics alloc] initWithScreenSize:CGSizeMake(390, 844) scale:3];
  LynxUIContext* context = [[LynxUIContext alloc] initWithScreenMetrics:screenMetrics];
  XCTAssertNil(context.viewportMetrics);

  LynxViewportMetrics* viewportMetrics =
      [[LynxViewportMetrics alloc] initWithSize:CGSizeMake(320, 600)
                                      widthMode:LynxViewSizeModeMax
                                     heightMode:LynxViewSizeModeExact];
  [context updateViewportMetrics:viewportMetrics];

  XCTAssertEqual(context.viewportMetrics, viewportMetrics);
  XCTAssertTrue(CGSizeEqualToSize(context.viewportMetrics.size, CGSizeMake(320, 600)));
  XCTAssertEqual(context.viewportMetrics.widthMode, LynxViewSizeModeMax);
  XCTAssertEqual(context.viewportMetrics.heightMode, LynxViewSizeModeExact);

  [context updateViewportMetrics:nil];
  XCTAssertNil(context.viewportMetrics);
}

@end
