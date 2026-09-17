// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxAnimationDelegate.h>
#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

// Keep test access private; no testing API is added to the framework's public headers.
@interface LynxKeyframeAnimator (IterationTesting)
- (CFTimeInterval)iterationActiveTime;
- (void)onIterationTimer:(NSTimer*)timer;
- (void)onIterationLifecycleChange:(NSNotification*)notification;
@end

@interface LynxIterationTestAnimator : LynxKeyframeAnimator
@property(nonatomic) CFTimeInterval testActiveTime;
@property(nonatomic) NSUInteger timeReadCount;
@property(nonatomic) BOOL useRealTime;
@property(nonatomic) NSUInteger lifecycleNotificationCount;
@end

@implementation LynxIterationTestAnimator
- (CFTimeInterval)iterationActiveTime {
  ++_timeReadCount;
  return _useRealTime ? [super iterationActiveTime] : _testActiveTime;
}
- (void)onIterationLifecycleChange:(NSNotification*)notification {
  ++_lifecycleNotificationCount;
  [super onIterationLifecycleChange:notification];
}
@end

@interface LynxKeyframeAnimatorUnitTest : XCTestCase
@property(nonatomic, strong) LynxUI* ui;
@property(nonatomic, strong) LynxIterationTestAnimator* animator;
@property(nonatomic, strong) LynxAnimationInfo* info;
@property(nonatomic, strong) NSMutableArray<NSString*>* events;
@property(nonatomic, strong) NSMutableArray<NSDictionary*>* eventParams;
@property(nonatomic, copy) void (^onIteration)(void);
@property(nonatomic, strong) id eventMock;
@property(nonatomic, strong) id applicationMock;
@property(nonatomic) UIApplicationState applicationState;
@end

@implementation LynxKeyframeAnimatorUnitTest

- (void)setUp {
  [super setUp];
  self.ui = [[LynxUI alloc] initWithView:[[UIView alloc] init]];
  [self setIterationBound:YES];
  self.animator = [[LynxIterationTestAnimator alloc] initWithUI:self.ui];
  self.animator.autoResumeAnimation = NO;
  self.info = [[LynxAnimationInfo alloc] initWithName:@"fade"];
  self.info.duration = 10;
  self.info.iterationCount = 5;
  self.info.playState = LynxAnimationPlayStateRunning;
  self.info.fillMode = kCAFillModeForwards;

  // Exercise apply/pause/resume with real CA animations, without depending on keyframe parsing.
  LynxKeyframeParsedData* data = [[LynxKeyframeParsedData alloc] init];
  data.keyframeValues[@"opacity"] = [@[ @0, @1 ] mutableCopy];
  data.keyframeTimes[@"opacity"] = [@[ @0, @1 ] mutableCopy];
  [self.animator setValue:data forKey:@"keyframeParsedData"];

  self.events = [NSMutableArray array];
  self.eventParams = [NSMutableArray array];
  __weak typeof(self) weakSelf = self;
  self.eventMock = OCMClassMock([LynxAnimationDelegate class]);
  OCMStub([self.eventMock sendAnimationEvent:[OCMArg any]
                                   eventName:[OCMArg any]
                                 eventParams:[OCMArg any]])
      .andDo(^(NSInvocation* invocation) {
        __unsafe_unretained NSString* name;
        __unsafe_unretained NSDictionary* params;
        [invocation getArgument:&name atIndex:3];
        [invocation getArgument:&params atIndex:4];
        [weakSelf.events addObject:name];
        [weakSelf.eventParams addObject:params];
        if ([name isEqualToString:@"animationiteration"] && weakSelf.onIteration) {
          weakSelf.onIteration();
        }
      });
  self.applicationState = UIApplicationStateActive;
  // Logic tests have no application host, so sharedApplication can be nil.
  self.applicationMock = OCMClassMock([UIApplication class]);
  OCMStub([self.applicationMock sharedApplication]).andReturn(self.applicationMock);
  OCMStub([self.applicationMock applicationState]).andDo(^(NSInvocation* invocation) {
    UIApplicationState state = weakSelf.applicationState;
    [invocation setReturnValue:&state];
  });
}

- (void)tearDown {
  self.onIteration = nil;
  [self.animator destroy];
  [self.ui.view.layer removeAllAnimations];
  [self.eventMock stopMocking];
  [self.applicationMock stopMocking];
  self.animator = nil;
  self.ui = nil;
  [super tearDown];
}

- (void)setIterationBound:(BOOL)bound {
  NSSet* events = bound ? [NSSet setWithObject:@"animationiteration(bindEvent)"] : [NSSet set];
  [self.ui setRawEvents:events andLepusRawEvents:[NSSet set]];
}

- (NSTimer*)timer {
  return [self.animator valueForKey:@"iterationTimer"];
}

- (void)applyAnimation {
  [self.animator apply:self.info];
  [self.events removeAllObjects];
  [self.eventParams removeAllObjects];
}

- (void)wakeAt:(CFTimeInterval)activeTime {
  self.animator.testActiveTime = activeTime;
  NSTimer* timer = [self timer];
  XCTAssertNotNil(timer);
  // Deliver the existing one-shot timer synchronously, without wall-clock sleeps.
  [timer fire];
}

- (void)completeAnimation {
  CAKeyframeAnimation* animation = [self.animator valueForKey:@"internalAnimators"][@"opacity"];
  [animation.delegate animationDidStop:animation finished:YES];
}

- (void)setBackground:(BOOL)background {
  self.applicationState = background ? UIApplicationStateBackground : UIApplicationStateActive;
  [[NSNotificationCenter defaultCenter]
      postNotificationName:background ? UIApplicationDidEnterBackgroundNotification
                                      : UIApplicationDidBecomeActiveNotification
                    object:nil];
}

- (void)testBindingControlsDetectionWithoutReplayingHistory {
  [self setIterationBound:NO];
  [self applyAnimation];
  [self setBackground:YES];
  [self setBackground:NO];
  XCTAssertNil([self timer]);
  XCTAssertNil([self.animator valueForKey:@"iterationTimerTarget"]);
  XCTAssertEqual(self.animator.timeReadCount, 0u);
  XCTAssertEqual(self.animator.lifecycleNotificationCount, 0u);

  [self setIterationBound:YES];
  self.animator.testActiveTime = 25;
  [self.animator apply:self.info];
  [self wakeAt:30];
  XCTAssertEqualObjects(self.events, (@[ @"animationiteration" ]));
  NSUInteger reads = self.animator.timeReadCount;
  [self setIterationBound:NO];
  [self wakeAt:40];
  XCTAssertNil([self timer]);
  XCTAssertEqual(self.animator.timeReadCount, reads);
  [self setBackground:YES];
  [self setBackground:NO];
  XCTAssertEqual(self.animator.lifecycleNotificationCount, 0u);
}

- (void)testUnbindingBeforePauseDoesNotReadProgress {
  [self applyAnimation];
  [self setIterationBound:NO];
  LynxAnimationInfo* paused = [self.info copy];
  paused.playState = LynxAnimationPlayStatePaused;
  [self.animator apply:paused];
  XCTAssertEqual(self.animator.timeReadCount, 0u);
  XCTAssertEqual(self.events.count, 0u);
  XCTAssertNil([self timer]);
}

- (void)testBoundarySchedulingAndDelayedDelivery {
  self.info.delay = 3;
  [self applyAnimation];
  NSTimer* timer = [self timer];
  id target = [self.animator valueForKey:@"iterationTimerTarget"];
  CFTimeInterval begin = [[self.animator valueForKey:@"keyframeStartTime"] doubleValue];
  XCTAssertEqualWithAccuracy([[self.animator valueForKey:@"iterationTimerDeadline"] doubleValue],
                             [self.ui.view.layer convertTime:begin + 13 toLayer:nil], 0.000001);
  NSDate* fireDate = timer.fireDate;
  [self.animator apply:[self.info copy]];
  XCTAssertEqual([self timer], timer);
  XCTAssertEqualObjects([self timer].fireDate, fireDate);
  [self wakeAt:9.9];
  XCTAssertEqual(self.events.count, 0u);
  XCTAssertFalse(timer.valid);
  [self wakeAt:35];
  XCTAssertEqualObjects(self.events,
                        (@[ @"animationiteration", @"animationiteration", @"animationiteration" ]));
  XCTAssertEqualObjects(
      self.eventParams[0],
      (@{@"animation_type" : @"keyframe-animation", @"animation_name" : @"fade"}));
  XCTAssertEqual(self.eventParams[0], self.eventParams[1]);
  XCTAssertEqual([self.animator valueForKey:@"iterationTimerTarget"], target);
  XCTAssertEqualWithAccuracy([[self.animator valueForKey:@"iterationTimerDeadline"] doubleValue],
                             [self.ui.view.layer convertTime:begin + 43 toLayer:nil], 0.000001);
  [self wakeAt:35];
  XCTAssertEqual(self.events.count, 3u);
}

- (void)testFiniteIterationsAndCompletionOrdering {
  for (NSNumber* count in @[ @1, @2.5, @3 ]) {
    for (NSNumber* deliverTimer in @[ @NO, @YES ]) {
      [self.animator cancel];
      self.info = [self.info copy];
      self.info.name = [NSString stringWithFormat:@"fade-%@-%@", count, deliverTimer];
      self.info.iterationCount = count.doubleValue;
      self.animator.testActiveTime = 0;
      [self applyAnimation];
      NSTimer* timer = [self timer];
      if (count.doubleValue == 1) {
        XCTAssertNil(timer);
      } else if (deliverTimer.boolValue) {
        [self wakeAt:count.doubleValue * self.info.duration];
        XCTAssertNil([self timer]);
      }
      [self completeAnimation];
      NSArray* expected = count.doubleValue == 1
                              ? @[ @"animationend" ]
                              : @[ @"animationiteration", @"animationiteration", @"animationend" ];
      XCTAssertEqualObjects(self.events, expected, @"count=%@, timer=%@", count, deliverTimer);
      XCTAssertFalse(timer.valid);
      XCTAssertNil([self timer]);
      [self setBackground:YES];
      [self setBackground:NO];
      XCTAssertEqual(self.animator.lifecycleNotificationCount, 0u);
    }
  }
}

- (void)testNegativeDelayAndAlternateCountEachLeg {
  self.info.delay = -15;
  self.info.direction = LynxAnimationDirectionAlternate;
  self.info.iterationCount = 4;
  [self applyAnimation];
  CAKeyframeAnimation* animation = [self.animator valueForKey:@"internalAnimators"][@"opacity"];
  XCTAssertTrue(animation.autoreverses);
  XCTAssertEqual(animation.repeatCount, 2);
  [self wakeAt:19.9];
  XCTAssertEqual(self.events.count, 0u);
  [self wakeAt:20];
  XCTAssertEqual(self.events.count, 1u);
  [self wakeAt:30];
  XCTAssertEqual(self.events.count, 2u);
  [self completeAnimation];
  XCTAssertEqualObjects(self.events.lastObject, @"animationend");
  XCTAssertEqual(self.events.count, 3u);
}

- (void)testPauseResumeDoesNotRepeatBoundary {
  [self applyAnimation];
  NSTimer* timer = [self timer];
  self.animator.testActiveTime = 10.001;
  LynxAnimationInfo* paused = [self.info copy];
  paused.playState = LynxAnimationPlayStatePaused;
  [self.animator apply:paused];
  XCTAssertFalse(timer.valid);
  XCTAssertNil([self timer]);
  XCTAssertEqual(self.events.count, 1u);
  [self setBackground:YES];
  [self setBackground:NO];
  XCTAssertEqual(self.animator.lifecycleNotificationCount, 0u);
  // Model a small difference between the sampling and synchronized pause clocks.
  self.animator.testActiveTime = 9.999;
  [self.animator apply:paused];
  [self.animator apply:self.info];
  [self wakeAt:10.001];
  XCTAssertEqual(self.events.count, 1u);
  [self wakeAt:20];
  XCTAssertEqual(self.events.count, 2u);
}

- (void)testBackgroundRecovery {
  // Cover notification-driven suspension, a timer seeing background first, and background creation.
  for (NSUInteger mode = 0; mode < 3; ++mode) {
    [self.animator cancel];
    self.animator.testActiveTime = 0;
    self.applicationState = mode == 2 ? UIApplicationStateBackground : UIApplicationStateActive;
    [self applyAnimation];
    [self.animator apply:self.info];
    if (mode == 0) {
      [self setBackground:YES];
    } else if (mode == 1) {
      self.applicationState = UIApplicationStateBackground;
      [self wakeAt:25];
    }
    XCTAssertNil([self timer]);
    self.animator.testActiveTime = 25;
    NSUInteger notifications = self.animator.lifecycleNotificationCount;
    [self setBackground:NO];
    XCTAssertEqual(self.animator.lifecycleNotificationCount, notifications + 1);
    XCTAssertEqual(self.events.count, 0u);
    [self wakeAt:30];
    XCTAssertEqual(self.events.count, 1u);
  }
  [self setBackground:YES];
  [self setIterationBound:NO];
  [self setBackground:NO];
  NSUInteger notifications = self.animator.lifecycleNotificationCount;
  [self setBackground:YES];
  [self setBackground:NO];
  XCTAssertEqual(self.animator.lifecycleNotificationCount, notifications);
  XCTAssertNil([self timer]);
}

- (void)testCancelAndDestroyRejectStaleCallbacks {
  for (NSNumber* destroy in @[ @NO, @YES ]) {
    [self applyAnimation];
    NSTimer* timer = [self timer];
    if (destroy.boolValue) {
      [self.animator destroy];
    } else {
      [self.animator cancel];
    }
    NSUInteger reads = self.animator.timeReadCount;
    [self.animator onIterationTimer:timer];
    [self setBackground:YES];
    [self setBackground:NO];
    XCTAssertFalse(timer.valid);
    XCTAssertNil([self timer]);
    XCTAssertEqual(self.animator.timeReadCount, reads);
    XCTAssertEqual(self.animator.lifecycleNotificationCount, 0u);
    XCTAssertEqual(self.events.count, 0u);
  }
}

- (void)testReplacementAnimationHasIndependentIterationProgress {
  [self applyAnimation];
  [self wakeAt:30];
  XCTAssertEqual(self.events.count, 3u);
  LynxAnimationInfo* replacement = [self.info copy];
  replacement.name = @"replacement";
  self.animator.testActiveTime = 0;
  [self.animator apply:replacement];
  [self.events removeAllObjects];
  [self.eventParams removeAllObjects];
  [self wakeAt:10];
  XCTAssertEqualObjects(self.events, (@[ @"animationiteration" ]));
  XCTAssertEqualObjects(self.eventParams.firstObject[@"animation_name"], @"replacement");
}

- (void)testLayerTimingChangesRecalculateDeadlineOnApply {
  CALayer* parent = [CALayer layer];
  [parent addSublayer:self.ui.view.layer];
  [self applyAnimation];
  NSTimer* oldTimer = [self timer];
  parent.speed = 2;
  self.ui.view.layer.speed = 0.5;
  self.ui.view.layer.timeOffset = 3;
  [self.animator apply:self.info];
  CFTimeInterval begin = [[self.animator valueForKey:@"keyframeStartTime"] doubleValue];
  XCTAssertEqualWithAccuracy([[self.animator valueForKey:@"iterationTimerDeadline"] doubleValue],
                             [self.ui.view.layer convertTime:begin + 10 toLayer:nil], 0.000001);
  XCTAssertFalse(oldTimer.valid);
  parent.speed = 0;
  [self.animator apply:self.info];
  XCTAssertNil([self timer]);
  parent.speed = 1;
  [self.animator apply:self.info];
  XCTAssertNotNil([self timer]);
  [self.ui.view.layer removeFromSuperlayer];
}

- (void)testShortIterationsHaveMinimumWakeInterval {
  self.info.duration = 0.001;
  self.info.iterationCount = 1E9;
  NSDate* before = [NSDate date];
  [self applyAnimation];
  XCTAssertGreaterThanOrEqual([[self timer].fireDate timeIntervalSinceDate:before], 1.0 / 60.0);
  [self wakeAt:0.01];
  XCTAssertEqual(self.events.count, 10u);
  XCTAssertNotNil([self timer]);
}

- (void)testCancelFromHandlerStopsCatchUpAndEnd {
  self.info.iterationCount = 4;
  [self applyAnimation];
  __weak LynxKeyframeAnimator* animator = self.animator;
  self.onIteration = ^{
    [animator cancel];
  };
  [self completeAnimation];
  XCTAssertEqualObjects(self.events, (@[ @"animationiteration" ]));
  XCTAssertNil([self timer]);
}

- (void)testPendingTimerDoesNotRetainAnimator {
  [self applyAnimation];
  NSTimer* timer = [self timer];
  __weak LynxKeyframeAnimator* animator = self.animator;
  self.animator = nil;
  XCTAssertNil(animator);
  XCTAssertFalse(timer.valid);
}

- (void)testRunLoopDeliversOneShotIteration {
  // Install only scheduling state here so CA completion cannot satisfy the expectation instead.
  self.info.duration = 0.05;
  self.info.iterationCount = 2;
  self.animator.useRealTime = YES;
  [self.animator setValue:self.info forKey:@"info"];
  [self.animator setValue:@(LynxKFAnimatorStateRunning) forKey:@"state"];
  [self.animator
      setValue:[@{@"opacity" : [CAKeyframeAnimation animationWithKeyPath:@"opacity"]} mutableCopy]
        forKey:@"internalAnimators"];
  [self.animator setValue:@([self.ui.view.layer convertTime:CACurrentMediaTime() + 0.1
                                                  fromLayer:nil])
                   forKey:@"keyframeStartTime"];
  XCTestExpectation* iteration = [self expectationWithDescription:@"scheduled iteration"];
  self.onIteration = ^{
    [iteration fulfill];
  };
  [self.animator apply:self.info];
  NSTimer* timer = [self timer];
  XCTAssertNotNil(timer);
  [self waitForExpectationsWithTimeout:2 handler:nil];
  XCTAssertEqualObjects(self.events, (@[ @"animationiteration" ]));
  XCTAssertFalse(timer.valid);
  XCTAssertNil([self timer]);
}

@end
