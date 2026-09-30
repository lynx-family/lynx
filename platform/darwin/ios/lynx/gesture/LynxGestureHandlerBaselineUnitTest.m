// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxUIContext.h>
#import <OCMock/OCMock.h>
#import <QuartzCore/QuartzCore.h>
#import <XCTest/XCTest.h>
#import "LynxGestureArenaManager.h"
#import "LynxGestureDetectorManager.h"
#import "LynxGestureExtraBundle.h"
#import "LynxGestureHandlerTrigger.h"

@interface LynxGestureHandlerTrigger (BaselineTesting)
- (LynxGestureHandlerState)getCurrentMemberState:(id<LynxGestureArenaMember>)member;
- (id<LynxGestureArenaMember>)reCompeteByGestures:(NSArray<id<LynxGestureArenaMember>> *)candidates
                                          current:(id<LynxGestureArenaMember>)current;
@end

@interface LynxGestureHandlerBaselineUnitTest : XCTestCase
@property(nonatomic, strong) id member;
@property(nonatomic, strong) id context;
@property(nonatomic, strong) id emitter;
@property(nonatomic, strong) NSMutableArray<LynxCustomEvent *> *events;
@property(nonatomic, strong) NSMutableArray<LynxBaseGestureHandler *> *handlers;
@end

@implementation LynxGestureHandlerBaselineUnitTest

- (void)setUp {
  [super setUp];
  self.events = [NSMutableArray array];
  self.handlers = [NSMutableArray array];
  self.member = OCMProtocolMock(@protocol(LynxGestureArenaMember));
  OCMStub([self.member getGestureArenaMemberId]).andReturn(10);
  self.context = OCMClassMock([LynxUIContext class]);
  self.emitter = OCMClassMock([LynxEventEmitter class]);
  OCMStub([self.context eventEmitter]).andReturn(self.emitter);
  OCMStub([self.member canConsumeGesture:CGPointZero]).ignoringNonObjectArgs().andReturn(YES);
  OCMStub([self.member getScrollContainerDirection]).andReturn(DIRECTION_VERTICAL);
  __weak typeof(self) weakSelf = self;
  OCMStub([self.emitter dispatchGestureEvent:1 event:[OCMArg any]])
      .ignoringNonObjectArgs()
      .andDo(^(NSInvocation *invocation) {
        __unsafe_unretained LynxCustomEvent *event;
        [invocation getArgument:&event atIndex:3];
        [weakSelf.events addObject:event];
      });
}

- (void)tearDown {
  for (LynxBaseGestureHandler *handler in self.handlers) {
    [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
  }
  [self.member stopMocking];
  [self.context stopMocking];
  [self.emitter stopMocking];
  [super tearDown];
}

- (LynxBaseGestureHandler *)handler:(LynxGestureTypeDarwin)type
                             config:(NSDictionary *)config
                          callbacks:(NSArray<NSString *> *)callbacks {
  LynxGestureDetectorDarwin *detector;
  if (config) {
    detector = [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                                        gestureType:type
                                               gestureCallbackNames:callbacks
                                                        relationMap:@{}
                                                          configMap:[config mutableCopy]];
  } else {
    detector = [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                                        gestureType:type
                                               gestureCallbackNames:callbacks
                                                        relationMap:@{}];
  }
  LynxBaseGestureHandler *handler =
      [LynxBaseGestureHandler convertToGestureHandler:10
                                              context:self.context
                                               member:self.member
                                     gestureDetectors:@{@1 : detector}]
          .allValues.firstObject;
  XCTAssertNotNil(handler);
  [self.handlers addObject:handler];
  return handler;
}

- (LynxBaseGestureHandler *)handler:(LynxGestureTypeDarwin)type config:(NSDictionary *)config {
  return [self handler:type config:config callbacks:@[ ON_BEGIN, ON_START, ON_UPDATE, ON_END ]];
}

- (void)send:(NSString *)type
       point:(CGPoint)point
          to:(LynxBaseGestureHandler *)handler
      bundle:(LynxGestureExtraBundle *)bundle {
  id touch = OCMClassMock([UITouch class]);
  OCMStub([touch locationInView:nil]).andReturn(point);
  LynxTouchEvent *event = [[LynxTouchEvent alloc] initWithName:type
                                                     targetTag:10
                                                   clientPoint:CGPointMake(100.25, -200.5)
                                                     pagePoint:CGPointMake(30.5, -40.25)
                                                     viewPoint:point];
  [handler onHandle:type
                   touches:[NSSet setWithObject:touch]
                     event:[[UIEvent alloc] init]
                touchEvent:event
                flingPoint:CGPointZero
      handleBySimultaneous:NO
               extraBundle:bundle];
  [touch stopMocking];
}

- (void)frame:(CGPoint)delta to:(LynxBaseGestureHandler *)handler {
  [handler onHandle:LynxEventTouchMove
                   touches:nil
                     event:nil
                touchEvent:nil
                flingPoint:delta
      handleBySimultaneous:NO
               extraBundle:nil];
}

- (NSArray<NSString *> *)names {
  return [self.events valueForKey:@"eventName"];
}

- (void)drainMainQueue {
  XCTestExpectation *drained = [self expectationWithDescription:@"Queued gesture timer"];
  dispatch_async(dispatch_get_main_queue(), ^{
    [drained fulfill];
  });
  [self waitForExpectations:@[ drained ] timeout:2];
}

- (void)testPanAndNativeReleasePreserveCallbackOrder {
  for (NSNumber *type in @[ @(LynxGestureTypePan), @(LynxGestureTypeNative) ]) {
    [self.events removeAllObjects];
    LynxBaseGestureHandler *handler = [self handler:type.unsignedIntegerValue config:nil];
    [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
    [self send:LynxEventTouchMove point:CGPointMake(20, 0) to:handler bundle:nil];
    [self send:LynxEventTouchMove point:CGPointMake(30, 0) to:handler bundle:nil];
    [self send:LynxEventTouchEnd point:CGPointMake(30, 0) to:handler bundle:nil];
    XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_START, ON_UPDATE, ON_UPDATE, ON_END ]));
    XCTAssertEqual(handler.status, LynxGestureHandlerStateCancel);
  }
}

- (void)testPanThresholdIsStrictAxisBasedAndFractional {
  for (NSNumber *sign in @[ @(-1), @1 ]) {
    LynxBaseGestureHandler *handler = [self handler:LynxGestureTypePan
                                             config:@{@"minDistance" : @10.5}];
    CGFloat boundary = sign.intValue * 10.5;
    [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
    [self send:LynxEventTouchMove point:CGPointMake(boundary, boundary) to:handler bundle:nil];
    XCTAssertEqual(handler.status, LynxGestureHandlerStateBegin);
    [self send:LynxEventTouchMove
         point:CGPointMake(boundary + sign.intValue * 0.25, 0)
            to:handler
        bundle:nil];
    XCTAssertEqual(handler.status, LynxGestureHandlerStateActive);
  }
}

- (void)testPanWithoutBeginSubscriptionStillStartsAndEnds {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypePan
                                           config:nil
                                        callbacks:@[ ON_START, ON_END ]];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchMove point:CGPointMake(1, 0) to:handler bundle:nil];
  [self send:LynxEventTouchEnd point:CGPointMake(1, 0) to:handler bundle:nil];
  XCTAssertEqualObjects([self names], (@[ ON_START, ON_END ]));
}

- (void)testPanCancelBeforeActivationEndsOnce {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypePan config:nil];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchCancel point:CGPointZero to:handler bundle:nil];
  [handler fail];
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END ]));
  XCTAssertEqual(handler.status, LynxGestureHandlerStateCancel);
}

- (void)testExplicitEndSurvivesFailAndResetStartsAnotherSequence {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypePan config:nil];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [handler end];
  [handler fail];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateEnd);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END ]));
  [handler reset];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchMove point:CGPointMake(1, 0) to:handler bundle:nil];
  [self send:LynxEventTouchEnd point:CGPointMake(1, 0) to:handler bundle:nil];
  XCTAssertEqualObjects([self names],
                        (@[ ON_BEGIN, ON_END, ON_BEGIN, ON_START, ON_UPDATE, ON_END ]));
}

- (void)testTouchPayloadPreservesFractionsAndUsesMonotonicMilliseconds {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypePan config:nil];
  double before = CACurrentMediaTime() * 1000;
  [self send:LynxEventTouchStart point:CGPointMake(-1.25, 2.75) to:handler bundle:nil];
  NSDictionary *params = self.events.lastObject.params;
  XCTAssertEqualObjects(params[@"x"], @(-1.25));
  XCTAssertEqualObjects(params[@"y"], @2.75);
  XCTAssertEqualObjects(params[@"pageX"], @30.5);
  XCTAssertEqualObjects(params[@"pageY"], @(-40.25));
  XCTAssertEqualObjects(params[@"clientX"], @100.25);
  XCTAssertEqualObjects(params[@"clientY"], @(-200.5));
  XCTAssertEqualObjects(params[@"type"], LynxEventTouchStart);
  XCTAssertGreaterThanOrEqual([params[@"timestamp"] doubleValue], before);
  XCTAssertLessThanOrEqual([params[@"timestamp"] doubleValue], CACurrentMediaTime() * 1000);
  XCTAssertEqual(self.events.lastObject.targetSign, 10);
}

- (void)testTapDistanceBoundaryAndBeyond {
  for (NSNumber *distance in @[ @10, @(-10), @10.25, @(-10.25) ]) {
    [self.events removeAllObjects];
    LynxBaseGestureHandler *handler =
        [self handler:LynxGestureTypeTap config:@{@"maxDistance" : @10, @"maxDuration" : @60000}];
    [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
    [self send:LynxEventTouchMove
         point:CGPointMake(distance.doubleValue, distance.doubleValue)
            to:handler
        bundle:nil];
    [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
    XCTAssertEqualObjects([self names], fabs(distance.doubleValue) <= 10
                                            ? (@[ ON_BEGIN, ON_START, ON_END ])
                                            : (@[ ON_BEGIN, ON_END ]));
  }
}

- (void)testTapCancelCurrentlyRecognizesTap {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeTap config:nil];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchCancel point:CGPointZero to:handler bundle:nil];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateActive);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_START, ON_END ]));
}

- (void)testTapPartialConfigClearsMissingDistance {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeTap
                                           config:@{@"maxDuration" : @60000}];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchMove point:CGPointMake(0.25, 0) to:handler bundle:nil];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateCancel);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END ]));
}

- (void)testTapTimeoutPreventsSubsequentClick {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeTap
                                           config:@{@"maxDuration" : @0, @"maxDistance" : @10}];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self drainMainQueue];
  [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateCancel);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END ]));
}

- (void)testDiscreteHandlersCurrentlyRepeatEndOnRepeatedFail {
  for (NSNumber *type in @[ @(LynxGestureTypeTap), @(LynxGestureTypeLongPress) ]) {
    [self.events removeAllObjects];
    LynxBaseGestureHandler *handler = [self handler:type.unsignedIntegerValue config:nil];
    [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
    [handler fail];
    [handler fail];
    [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
    XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END, ON_END ]));
  }
}

- (void)testLongPressRemainsActiveBeyondDistanceAfterActivation {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeLongPress
                                           config:@{@"minDuration" : @0, @"maxDistance" : @10}];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self drainMainQueue];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateActive);
  [self send:LynxEventTouchMove point:CGPointMake(100, 100) to:handler bundle:nil];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateActive);
  [self send:LynxEventTouchEnd point:CGPointMake(100, 100) to:handler bundle:nil];
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_START, ON_END ]));
}

- (void)testLongPressReleaseCancelsPendingActivation {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeLongPress
                                           config:@{@"minDuration" : @0, @"maxDistance" : @10}];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
  [self drainMainQueue];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateCancel);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_END ]));
}

- (void)testFlingAfterReleaseUpdatesWithoutStartCallback {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeFling config:nil];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:nil];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateUndetermined);
  [self send:LynxEventTouchEnd point:CGPointZero to:handler bundle:nil];
  [self frame:CGPointMake(4.25, -7.5) to:handler];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateBegin);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_UPDATE ]));
  XCTAssertEqualObjects(self.events.lastObject.params[@"deltaX"], @4.25);
  XCTAssertEqualObjects(self.events.lastObject.params[@"deltaY"], @(-7.5));
  XCTAssertNil(self.events.lastObject.params[@"timestamp"]);
  XCTAssertNil(self.events.lastObject.params[@"pageX"]);
  [self frame:CGPointMake(FLT_EPSILON, FLT_EPSILON) to:handler];
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_UPDATE, ON_END ]));
}

- (void)testFlingWithoutReleaseConsumesFirstFrameAsActivation {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeFling config:nil];
  [self frame:CGPointMake(4, -7) to:handler];
  XCTAssertEqual(handler.status, LynxGestureHandlerStateActive);
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_START ]));
  [self frame:CGPointMake(3, -6) to:handler];
  XCTAssertEqualObjects([self names], (@[ ON_BEGIN, ON_START, ON_UPDATE ]));
}

- (void)testFailedSimultaneousDefaultStillConsumesSharedDelta {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeDefault config:nil];
  [handler fail];
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];
  bundle.isNeedConsumedSimultaneousGesture = YES;
  bundle.simultaneousDeltaX = 4.25;
  bundle.simultaneousDeltaY = -7.5;
  [handler onHandle:LynxEventTouchMove
                   touches:nil
                     event:nil
                touchEvent:nil
                flingPoint:CGPointZero
      handleBySimultaneous:YES
               extraBundle:bundle];
  OCMVerify([self.member onGestureScrollBy:CGPointMake(4.25, -7.5)]);
  XCTAssertEqual(self.events.count, 0);
}

- (void)testDefaultLocksDirectionAndPublishesConsumedDelta {
  LynxBaseGestureHandler *handler = [self handler:LynxGestureTypeDefault config:nil];
  LynxGestureExtraBundle *bundle = [[LynxGestureExtraBundle alloc] init];
  [self send:LynxEventTouchStart point:CGPointZero to:handler bundle:bundle];
  [self send:LynxEventTouchMove point:CGPointMake(4, 4) to:handler bundle:bundle];
  OCMVerify([self.member onGestureScrollBy:CGPointMake(0, -4)]);
  XCTAssertEqual(bundle.gestureDirection, DIRECTION_VERTICAL);
  XCTAssertTrue(bundle.isNeedConsumedSimultaneousGesture);
  XCTAssertEqual(bundle.simultaneousDeltaX, 0);
  XCTAssertEqual(bundle.simultaneousDeltaY, -4);
  [bundle resetSimultaneousDelta];
  XCTAssertEqual(bundle.gestureDirection, DIRECTION_VERTICAL);
  XCTAssertFalse(bundle.isNeedConsumedSimultaneousGesture);
}

- (void)testFactoryCollapsesDuplicateTypesAndIgnoresUnsupportedTypes {
  LynxGestureDetectorDarwin *first =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                               gestureType:LynxGestureTypePan
                                      gestureCallbackNames:@[]
                                               relationMap:@{}];
  LynxGestureDetectorDarwin *second =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypePan
                                      gestureCallbackNames:@[]
                                               relationMap:@{}];
  LynxGestureDetectorDarwin *pinch =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:3
                                               gestureType:LynxGestureTypePinch
                                      gestureCallbackNames:@[]
                                               relationMap:@{}];
  NSDictionary *handlers =
      [LynxBaseGestureHandler convertToGestureHandler:10
                                              context:self.context
                                               member:self.member
                                     gestureDetectors:@{@1 : first, @2 : second, @3 : pinch}];
  XCTAssertEqual(handlers.count, 1);
  XCTAssertNotNil(handlers[@(LynxGestureHandlerOptionPan)]);
}

- (void)testPanReleaseDoesNotDiscardSameNodeFling {
  LynxBaseGestureHandler *pan = [self handler:LynxGestureTypePan config:nil];
  LynxGestureDetectorDarwin *detector = [[LynxGestureDetectorDarwin alloc]
         initWithGestureID:2
               gestureType:LynxGestureTypeFling
      gestureCallbackNames:@[ ON_BEGIN, ON_START, ON_UPDATE, ON_END ]
               relationMap:@{}];
  LynxBaseGestureHandler *fling = [LynxBaseGestureHandler convertToGestureHandler:10
                                                                          context:self.context
                                                                           member:self.member
                                                                 gestureDetectors:@{@2 : detector}]
                                      .allValues.firstObject;
  OCMStub([self.member getGestureHandlers])
      .andReturn(
          (@{@(LynxGestureHandlerOptionPan) : pan, @(LynxGestureHandlerOptionFling) : fling}));
  id arena = OCMClassMock([LynxGestureArenaManager class]);
  LynxGestureDetectorManager *manager =
      [[LynxGestureDetectorManager alloc] initWithArenaManager:arena];
  LynxGestureHandlerTrigger *trigger =
      [[LynxGestureHandlerTrigger alloc] initWithDetectorManager:manager arenaManager:arena];
  [trigger setCurrentWinnerWhenDown:self.member];
  [self send:LynxEventTouchStart point:CGPointZero to:pan bundle:nil];
  [self send:LynxEventTouchStart point:CGPointZero to:fling bundle:nil];
  [self send:LynxEventTouchMove point:CGPointMake(20, 0) to:pan bundle:nil];
  [self send:LynxEventTouchEnd point:CGPointMake(20, 0) to:pan bundle:nil];
  [self send:LynxEventTouchEnd point:CGPointMake(20, 0) to:fling bundle:nil];
  XCTAssertEqual([trigger getCurrentMemberState:self.member], LynxGestureHandlerStateBegin);
  [self.events removeAllObjects];
  [self frame:CGPointMake(8, -9) to:fling];
  XCTAssertEqualObjects([self names], (@[ ON_UPDATE ]));
  [arena stopMocking];
}

- (void)testFailedParentCanReenterAfterChildFails {
  LynxBaseGestureHandler *parentPan = [self handler:LynxGestureTypePan config:nil];
  id child = OCMProtocolMock(@protocol(LynxGestureArenaMember));
  OCMStub([child getGestureArenaMemberId]).andReturn(20);
  LynxGestureDetectorDarwin *detector =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypePan
                                      gestureCallbackNames:@[]
                                               relationMap:@{}];
  LynxBaseGestureHandler *childPan =
      [LynxBaseGestureHandler convertToGestureHandler:20
                                              context:self.context
                                               member:child
                                     gestureDetectors:@{@2 : detector}]
          .allValues.firstObject;
  OCMStub([self.member getGestureHandlers]).andReturn(@{
    @(LynxGestureHandlerOptionPan) : parentPan
  });
  OCMStub([child getGestureHandlers]).andReturn(@{@(LynxGestureHandlerOptionPan) : childPan});
  id arena = OCMClassMock([LynxGestureArenaManager class]);
  LynxGestureDetectorManager *manager =
      [[LynxGestureDetectorManager alloc] initWithArenaManager:arena];
  LynxGestureHandlerTrigger *trigger =
      [[LynxGestureHandlerTrigger alloc] initWithDetectorManager:manager arenaManager:arena];
  [trigger setCurrentWinnerWhenDown:self.member];
  NSArray *chain = @[ self.member, child ];
  [parentPan fail];
  XCTAssertEqual([trigger reCompeteByGestures:chain current:self.member], child);
  [childPan fail];
  XCTAssertEqual([trigger reCompeteByGestures:chain current:child], self.member);
  XCTAssertEqual(parentPan.status, LynxGestureHandlerStateInit);
  [parentPan end];
  XCTAssertNil([trigger reCompeteByGestures:chain current:self.member]);
  [child stopMocking];
  [arena stopMocking];
}

@end
