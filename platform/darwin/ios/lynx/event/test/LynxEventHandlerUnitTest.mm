// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventHandlerUnitTest.h"
#import <Lynx/LynxCustomGestureRecognizer.h>
#import <Lynx/LynxEventHandler+Internal.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxTouchHandler+Internal.h>
#import <Lynx/LynxTouchHandler.h>
#import <Lynx/LynxTransferWrapperView.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUITransfer.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxWeakProxy.h>

@interface LynxUI ()
@property(nonatomic, weak) LynxUIContext* context;
- (Class)iosPanInterceptViewClass;
- (Class)iosPanInterceptGestureClass;
- (NSInteger)iosPanInterceptViewTag;
- (BOOL)hasIosPanInterceptViewTag;
- (BOOL)hasResponseChainEvent:(NSString*)eventName;
@end

@interface LynxUIContext ()
@property(nonatomic, weak) LynxEventHandler* eventHandler;
@end

@interface LynxEventHandler (ConsumeBoundaryTest)
- (BOOL)hasConsumeSlideEvent;
- (BOOL)consumeSlideEvents:(CGFloat)angle;
@end

@interface LynxUITransfer (EventRoutingTest)
@property(nonatomic, strong, readonly) LynxEventHandler* transferEventHandler;
@end

@interface LynxTouchHandler (ExternalCoordinatesTest)
- (void)setEnableCurrentTargetTouchPosition:(BOOL)enable;
- (NSDictionary<NSNumber*, NSValue*>*)currentTargetPointMapForTarget:(id<LynxEventTarget>)target
                                                           pagePoint:(CGPoint)pagePoint
                                                           eventName:(NSString*)eventName;
@end

@interface LynxPanInterceptUnitTestGesture : UIGestureRecognizer
@property(nonatomic, assign) UIGestureRecognizerState testState;
@end

@implementation LynxPanInterceptUnitTestGesture
- (UIGestureRecognizerState)state {
  return self.testState;
}
- (void)setState:(UIGestureRecognizerState)state {
  self.testState = state;
}
@end

@interface LynxPanInterceptUnitTestView : UIView
@end

@implementation LynxPanInterceptUnitTestView
@end

static const NSInteger kLynxPanInterceptUnitTestViewTag = 1001;

@implementation LynxEventHanderUnitTest {
  LynxEventHandler* _handler;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  _handler = OCMPartialMock([[LynxEventHandler alloc] initWithRootView:rootUI.view
                                                            withRootUI:rootUI
                                                               andFlag:NO]);
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _handler = NULL;
}

- (void)testOnGestureRecognizedByEventTargetAsync {
  LynxUIView* mockUI1 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  LynxUIView* mockUI2 = OCMPartialMock([[LynxUIView alloc] initWithView:[UIView new]]);
  [mockUI1 insertChild:mockUI2 atIndex:0];

  id mockCtx = OCMPartialMock([[LynxUIContext alloc] init]);
  ((LynxUIContext*)mockCtx).eventHandler = _handler;
  OCMStub([mockCtx isTouchMoving]).andReturn(YES);
  NSArray* transformRaw = @[ @[
    @(LynxTransformTypeTranslateX), @10, @(LynxPlatformLengthUnitNumber), @0,
    @(LynxPlatformLengthUnitNumber), @0, @(LynxPlatformLengthUnitNumber)
  ] ];

  dispatch_queue_t concurrentQueue1 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_queue_t concurrentQueue2 = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  NSMutableArray* uiArray = [NSMutableArray new];
  for (size_t i = 0; i < 2000; ++i) {
    LynxUI* ui = [[LynxUI alloc] init];
    ui.context = mockCtx;
    ui.sign = i;
    [uiArray addObject:ui];
  }
  dispatch_async(concurrentQueue1, ^{
    for (size_t i = 0; i < 1000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });
  dispatch_async(concurrentQueue2, ^{
    for (size_t i = 1000; i < 2000; ++i) {
      [LynxPropsProcessor updateProp:transformRaw withKey:@"transform" forUI:uiArray[i]];
    }
  });

  NSDate* timeoutDate = [NSDate dateWithTimeIntervalSinceNow:6];
  while ([timeoutDate timeIntervalSinceNow] > 0) {
    [[NSRunLoop mainRunLoop] runMode:NSDefaultRunLoopMode
                          beforeDate:[NSDate dateWithTimeIntervalSinceNow:1]];
  }
  [uiArray enumerateObjectsUsingBlock:^(id _Nonnull ui, NSUInteger idx, BOOL* _Nonnull stop) {
    [(LynxUI*)ui performSelector:@selector(onNodeReadyForUIOwner)];
  }];
  XCTAssertEqual([(NSMutableSet*)[_handler valueForKey:@"_setOfPropsChanged"] count], 2000);
}

- (void)testIsFragmentLayerRenderOn {
  UIView* mockView = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  // Test 1: Flag is YES
  // When flag is YES, tap, LynxTouchHandler, and long press recognizers should be added.

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];
  OCMVerify([mockView addGestureRecognizer:[OCMArg isKindOfClass:[UITapGestureRecognizer class]]]);
  OCMVerify([mockView addGestureRecognizer:[OCMArg isKindOfClass:[LynxTouchHandler class]]]);
  OCMVerify(
      [mockView addGestureRecognizer:[OCMArg isKindOfClass:[UILongPressGestureRecognizer class]]]);
  XCTAssertTrue([[handlerOn valueForKey:@"_isFragmentLayerRendererOn"] boolValue]);

  // Test 2: Flag is NO
  // When flag is NO, tap and long press recognizers SHOULD be added.
  id mockView2 = OCMClassMock([UIView class]);
  LynxEventHandler* handlerOff = [[LynxEventHandler alloc] initWithRootView:mockView2
                                                                 withRootUI:mockRootUI
                                                                    andFlag:NO];
  OCMVerify([mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UITapGestureRecognizer class]]]);
  OCMVerify(
      [mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UILongPressGestureRecognizer class]]]);
  XCTAssertFalse([[handlerOff valueForKey:@"_isFragmentLayerRendererOn"] boolValue]);
}

- (void)testSetupPlatformGestureWithFragmentLayerRenderOn {
  UIView* mockView = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];

  // When flag is YES, setUpPlatformGesture should do nothing
  [handlerOn setEnablePlatformGesture:YES];

  // Verify that LynxCustomGestureRecognizer was NOT added
  OCMVerify(
      never(),
      [mockView addGestureRecognizer:[OCMArg isKindOfClass:NSClassFromString(
                                                               @"LynxCustomGestureRecognizer")]]);
}

- (void)testPlatformGestureBlocksOtherRecognizersOnlyAfterActive {
  [_handler setEnablePlatformGesture:YES];
  id<UIGestureRecognizerDelegate> delegate = _handler.customPlatformGesture.delegate;
  XCTAssertNotNil(delegate);

  UIView* otherView = [UIView new];
  UIPanGestureRecognizer* otherGesture = [[UIPanGestureRecognizer alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  XCTAssertFalse([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldBeRequiredToFailByGestureRecognizer:otherGesture]);
  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGesture]);

  [_handler onPlatformGestureStatusChanged:LynxGestureHandlerStateBegin];
  XCTAssertFalse([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldBeRequiredToFailByGestureRecognizer:otherGesture]);
  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGesture]);

  [_handler onPlatformGestureStatusChanged:LynxGestureHandlerStateActive];
  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldBeRequiredToFailByGestureRecognizer:otherGesture]);
  XCTAssertFalse([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGesture]);
}

- (void)testPlatformGestureRecordedRecognizersClearedOnBegin {
  [_handler setEnablePlatformGesture:YES];
  id<UIGestureRecognizerDelegate> delegate = _handler.customPlatformGesture.delegate;
  XCTAssertNotNil(delegate);

  UIView* otherView = [UIView new];
  UIPanGestureRecognizer* otherGesture = [[UIPanGestureRecognizer alloc] init];
  [otherView addGestureRecognizer:otherGesture];
  NSMutableDictionary* innerGestures = [(NSObject*)delegate valueForKey:@"innerGestures"];

  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGesture]);
  XCTAssertEqual(innerGestures.count, 1);

  [_handler onPlatformGestureStatusChanged:LynxGestureHandlerStateBegin];
  XCTAssertEqual(innerGestures.count, 0);
}

- (void)testPlatformGestureBeginResetsLeakedActiveState {
  [_handler setEnablePlatformGesture:YES];
  id<UIGestureRecognizerDelegate> delegate = _handler.customPlatformGesture.delegate;
  XCTAssertNotNil(delegate);

  UIView* otherView = [UIView new];
  UIPanGestureRecognizer* otherGesture = [[UIPanGestureRecognizer alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler onPlatformGestureStatusChanged:LynxGestureHandlerStateActive];
  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldBeRequiredToFailByGestureRecognizer:otherGesture]);
  XCTAssertFalse([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:otherGesture]);

  UIView* nextOtherView = [UIView new];
  UIPanGestureRecognizer* nextOtherGesture = [[UIPanGestureRecognizer alloc] init];
  [nextOtherView addGestureRecognizer:nextOtherGesture];

  [_handler onPlatformGestureStatusChanged:LynxGestureHandlerStateBegin];
  XCTAssertFalse([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldBeRequiredToFailByGestureRecognizer:nextOtherGesture]);
  XCTAssertTrue([delegate gestureRecognizer:_handler.customPlatformGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:nextOtherGesture]);
}

- (void)testAttachContainerViewWithFragmentLayerRenderOn {
  UIView* mockView1 = OCMClassMock([UIView class]);
  LynxRootUI* mockRootUI = OCMClassMock([LynxRootUI class]);

  LynxEventHandler* handlerOn = [[LynxEventHandler alloc] initWithRootView:mockView1
                                                                withRootUI:mockRootUI
                                                                   andFlag:YES];

  UIView* mockView2 = OCMClassMock([UIView class]);
  // When flag is YES, attachContainerView should add tap and long press recognizers.

  [handlerOn attachContainerView:mockView2];
  OCMVerify([mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UITapGestureRecognizer class]]]);
  OCMVerify([mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[LynxTouchHandler class]]]);
  OCMVerify(
      [mockView2 addGestureRecognizer:[OCMArg isKindOfClass:[UILongPressGestureRecognizer class]]]);
}

- (LynxUIView*)configuredPanInterceptTargetUIWithViewTag:(BOOL)hasViewTag {
  LynxUIView* targetUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [LynxPropsProcessor updateProp:@(kLynxPanInterceptDirectionHorizontal)
                         withKey:@"pan-intercept-direction"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:@(kLynxPanInterceptScopeAll)
                         withKey:@"pan-intercept-scope"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:NSStringFromClass([LynxPanInterceptUnitTestGesture class])
                         withKey:@"ios-pan-intercept-gesture-class"
                           forUI:targetUI];
  [LynxPropsProcessor updateProp:NSStringFromClass([LynxPanInterceptUnitTestView class])
                         withKey:@"ios-pan-intercept-view-class"
                           forUI:targetUI];
  if (hasViewTag) {
    [LynxPropsProcessor updateProp:@(kLynxPanInterceptUnitTestViewTag)
                           withKey:@"ios-pan-intercept-view-tag"
                             forUI:targetUI];
  }
  XCTAssertEqual(targetUI.iosPanInterceptGestureClass, [LynxPanInterceptUnitTestGesture class]);
  XCTAssertEqual(targetUI.iosPanInterceptViewClass, [LynxPanInterceptUnitTestView class]);
  XCTAssertEqual(targetUI.hasIosPanInterceptViewTag, hasViewTag);
  if (hasViewTag) {
    XCTAssertEqual(targetUI.iosPanInterceptViewTag, kLynxPanInterceptUnitTestViewTag);
  }
  [_handler setValue:targetUI forKey:@"_firstPanInterceptDirectionTarget"];
  return targetUI;
}

- (LynxUIView*)configuredPanInterceptTargetUI {
  return [self configuredPanInterceptTargetUIWithViewTag:YES];
}

- (void)testInterceptPanGestureWithConfiguredGestureViewClassAndTag {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStateFailed);
}

- (void)testConfiguredPanInterceptRequiresViewClassMatch {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  UIView* otherView = [[UIView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

- (void)testConfiguredPanInterceptRequiresViewTagMatch {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUI];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = kLynxPanInterceptUnitTestViewTag + 1;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

- (void)testConfiguredPanInterceptRequiresViewTagProperty {
  LynxUIView* targetUI = [self configuredPanInterceptTargetUIWithViewTag:NO];
  XCTAssertNotNil(targetUI);

  LynxPanInterceptUnitTestView* otherView = [[LynxPanInterceptUnitTestView alloc] init];
  otherView.tag = 0;
  LynxPanInterceptUnitTestGesture* otherGesture = [[LynxPanInterceptUnitTestGesture alloc] init];
  [otherView addGestureRecognizer:otherGesture];

  [_handler interceptPanGestures:@[ [LynxWeakProxy proxyWithTarget:otherGesture] ]
             withPlatformGesture:nil];

  XCTAssertEqual(otherGesture.state, UIGestureRecognizerStatePossible);
}

- (void)testExternalConsumeSlideEventStopsAtEventRoot {
  for (NSString* scope in @[ @"outside", @"nonmatching-root", @"root", @"child", @"ordinary" ]) {
    LynxUIView* outer = [[LynxUIView alloc] initWithView:[UIView new]];
    LynxUIView* root = [[LynxUIView alloc] initWithView:[UIView new]];
    LynxUIView* child = [[LynxUIView alloc] initWithView:[UIView new]];
    [outer insertChild:root atIndex:0];
    [root insertChild:child atIndex:0];
    [LynxPropsProcessor updateProp:@[ @[ @(-180), @180 ] ]
                           withKey:@"consume-slide-event"
                             forUI:outer];
    if ([scope isEqualToString:@"nonmatching-root"]) {
      [LynxPropsProcessor updateProp:@[ @[ @80, @100 ] ] withKey:@"consume-slide-event" forUI:root];
    } else if ([scope isEqualToString:@"root"] || [scope isEqualToString:@"child"]) {
      [LynxPropsProcessor updateProp:@[ @[ @(-180), @180 ] ]
                             withKey:@"consume-slide-event"
                               forUI:[scope isEqualToString:@"root"] ? root : child];
    }
    UIView* source = [UIView new];
    LynxEventHandler* handler = [[LynxEventHandler alloc] initWithRootView:source
                                                                withRootUI:root
                                                                   andFlag:NO];
    if (![scope isEqualToString:@"ordinary"]) {
      [handler setTouchEventSource:source payloadRootView:outer.view];
    }
    [handler setValue:child forKey:@"touchTarget"];
    XCTAssertEqual([handler hasConsumeSlideEvent], ![scope isEqualToString:@"outside"], @"%@",
                   scope);
    BOOL shouldConsume = [scope isEqualToString:@"root"] || [scope isEqualToString:@"child"] ||
                         [scope isEqualToString:@"ordinary"];
    XCTAssertEqual([handler consumeSlideEvents:0], shouldConsume, @"%@", scope);
  }
}

- (void)testTouchEventSourceCoordinateConversion {
  UIView* payloadRootView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)];
  UIView* touchSourceView = [[UIView alloc] initWithFrame:CGRectMake(30, 40, 120, 120)];
  UIView* targetView = [[UIView alloc] initWithFrame:CGRectMake(10, 15, 80, 80)];
  [payloadRootView addSubview:touchSourceView];
  [payloadRootView addSubview:targetView];

  LynxUIView* targetUI = [[LynxUIView alloc] initWithView:targetView];
  [_handler setTouchEventSource:touchSourceView payloadRootView:payloadRootView];

  UITouch* mockTouch = OCMClassMock([UITouch class]);
  OCMStub([mockTouch locationInView:touchSourceView]).andReturn(CGPointMake(5, 6));

  CGPoint pagePoint = [_handler pagePointFromTouch:mockTouch];
  CGPoint viewPoint = [_handler viewPointFromTouch:mockTouch target:targetUI];

  XCTAssertTrue(CGPointEqualToPoint(pagePoint, CGPointMake(35, 46)));
  XCTAssertTrue(CGPointEqualToPoint(viewPoint, CGPointMake(25, 31)));
}

- (void)testTransferRootHitReturnsWrapper {
  id transfer = OCMClassMock([LynxUITransfer class]);
  id handler = OCMClassMock([LynxEventHandler class]);
  UIView* hiddenHost = [UIView new];
  hiddenHost.hidden = YES;
  OCMStub([transfer view]).andReturn(hiddenHost);
  OCMStub([transfer transferEventHandler]).andReturn(handler);
  CGPoint point = CGPointMake(20, 30);
  OCMStub([handler hitTest:point withEvent:nil]).andReturn(transfer);
  OCMStub([transfer eventThrough:point]).andReturn(NO);
  LynxTransferWrapperView* wrapper = [[LynxTransferWrapperView alloc] initWithTransfer:transfer];
  wrapper.frame = CGRectMake(0, 0, 100, 100);
  XCTAssertEqual([wrapper hitTest:point withEvent:nil], wrapper);
  OCMVerify([transfer eventThrough:point]);
}

- (void)testExternalCurrentTargetCoordinatesUsePayloadRoot {
  UIView* page = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 500, 500)];
  UIView* wrapper = [[UIView alloc] initWithFrame:CGRectMake(100, 200, 200, 200)];
  [page addSubview:wrapper];
  UIView* childView = [[UIView alloc] initWithFrame:CGRectMake(10, 20, 100, 100)];
  [wrapper addSubview:childView];
  LynxUIView* root = [[LynxUIView alloc] initWithView:[UIView new]];
  LynxUIView* child = [[LynxUIView alloc] initWithView:childView];
  root.sign = 1;
  child.sign = 2;
  [root insertChild:child atIndex:0];
  [wrapper addSubview:childView];
  XCTAssertEqual(child.parentTarget, root);
  id rootMock = OCMPartialMock(root);
  id childMock = OCMPartialMock(child);
  OCMStub([rootMock hasResponseChainEvent:[OCMArg any]]).andReturn(YES);
  OCMStub([childMock hasResponseChainEvent:[OCMArg any]]).andReturn(YES);
  LynxEventHandler* handler = [[LynxEventHandler alloc] initWithRootView:wrapper
                                                              withRootUI:root
                                                                 andFlag:NO];
  [handler setTouchEventSource:wrapper payloadRootView:page];
  [_handler.touchRecognizer setEnableCurrentTargetTouchPosition:YES];
  [handler syncConfigWith:_handler];
  for (NSString* name in @[ @"touchstart", @"tap", @"click" ]) {
    NSDictionary<NSNumber*, NSValue*>* points =
        [handler.touchRecognizer currentTargetPointMapForTarget:child
                                                      pagePoint:CGPointMake(125, 245)
                                                      eventName:name];
    XCTAssertNotNil(points[@1]);
    XCTAssertNotNil(points[@2]);
    XCTAssertTrue(CGPointEqualToPoint(points[@1].CGPointValue, CGPointMake(25, 45)));
    XCTAssertTrue(CGPointEqualToPoint(points[@2].CGPointValue, CGPointMake(15, 25)));
  }
}

- (void)testSyncConfigWith {
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  LynxEventHandler* sourceHandler = [[LynxEventHandler alloc] initWithRootView:rootUI.view
                                                                    withRootUI:rootUI
                                                                       andFlag:NO];
  LynxEventHandler* targetHandler = [[LynxEventHandler alloc] initWithRootView:[UIView new]
                                                                    withRootUI:rootUI
                                                                       andFlag:NO];

  [sourceHandler setEnableViewReceiveTouch:YES];
  [sourceHandler setEnableSimultaneousTap:YES];
  [sourceHandler setDisableLongpressAfterScroll:YES];
  [sourceHandler setTapSlop:@"24px"];
  [sourceHandler setLongPressDuration:700];
  [sourceHandler setEnablePlatformGesture:YES];
  [sourceHandler.touchRecognizer setEnableEndGestureAtLastFingerUp:YES];
  [sourceHandler.touchRecognizer setEnableTouchPseudo:YES];
  [sourceHandler.touchRecognizer setEnableMultiTouch:YES];
  [sourceHandler.touchRecognizer setEnableCurrentTargetTouchPosition:YES];
  sourceHandler.touchRecognizer.enableNewGesture = YES;

  [targetHandler syncConfigWith:sourceHandler];

  XCTAssertTrue(sourceHandler.enableSimultaneousTap == targetHandler.enableSimultaneousTap);
  XCTAssertTrue(((UITapGestureRecognizer*)targetHandler.tapRecognizer).cancelsTouchesInView == NO);
  XCTAssertEqualWithAccuracy(
      ((UILongPressGestureRecognizer*)targetHandler.longPressRecognizer).minimumPressDuration, 0.7,
      0.0001);
  XCTAssertTrue(targetHandler.customPlatformGesture != nil);
  XCTAssertTrue(
      [[targetHandler.touchRecognizer valueForKey:@"enableEndGestureAtLastFingerUp"] boolValue]);
  XCTAssertTrue([[targetHandler.touchRecognizer valueForKey:@"enableTouchPseudo"] boolValue]);
  XCTAssertTrue([[targetHandler.touchRecognizer valueForKey:@"enableMultiTouch"] boolValue]);
  XCTAssertTrue(targetHandler.touchRecognizer.enableNewGesture);
  XCTAssertTrue(
      [[targetHandler.touchRecognizer valueForKey:@"enableCurrentTargetTouchPosition"] boolValue]);
}

@end
