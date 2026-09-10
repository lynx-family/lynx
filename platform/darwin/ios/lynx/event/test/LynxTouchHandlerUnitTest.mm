// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <XCTest/XCTest.h>

#include <deque>

#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxTouchHandler+Internal.h>
#import <Lynx/LynxTouchHandler.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxWeakProxy.h>
#import "LynxTouchHandlerUnitTest.h"

@interface LynxTouchHandler ()

- (void)onTouchesMoveWithTarget:(id<LynxEventTarget>)target;

@end

@interface MockEventTarget : NSObject <LynxEventTarget>

@property(nonatomic, readwrite) NSInteger count;

@end

@implementation MockEventTarget

- (NSInteger)signature {
  return self.count;
}

- (nullable id<LynxEventTarget>)parentTarget {
  return nil;
}

- (nullable id<LynxEventTargetBase>)parentResponder {
  return nil;
}

- (nullable NSDictionary*)getDataset {
  return nil;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  return nil;
}

- (BOOL)containsPoint:(CGPoint)point {
  return NO;
}

- (nullable NSDictionary<NSString*, LynxEventSpec*>*)eventSet {
  return nil;
}

- (nullable NSDictionary<NSNumber*, LynxGestureDetectorDarwin*>*)gestureMap {
  return nil;
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent*)event {
  return NO;
}

- (BOOL)consumeSlideEvent:(CGFloat)angle {
  return NO;
}

- (BOOL)ignoreFocus {
  return NO;
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer*)gestureRecognizer {
  return NO;
}

- (BOOL)eventThrough:(CGPoint)point {
  return NO;
}

- (enum LynxPointerEventsValue)pointerEvents {
  return kLynxPointerEventsValueAuto;
}

- (BOOL)enableTouchPseudoPropagation {
  return YES;
}

- (void)onPseudoStatusFrom:(int32_t)preStatus changedTo:(int32_t)currentStatus {
}

- (BOOL)dispatchTouch:(NSString* const)touchType
              touches:(NSSet<UITouch*>*)touches
            withEvent:(UIEvent*)event {
  return NO;
}

- (void)onResponseChain {
}

- (void)offResponseChain {
}

- (BOOL)isOnResponseChain {
  return NO;
}

@end

@interface LynxTouchHandlerTestView : UIView
@end

@implementation LynxTouchHandlerTestView
- (BOOL)isChildLynxPage {
  return YES;
}
@end

@interface LynxTouchHandlerTestRootUI : LynxRootUI
@end

@implementation LynxTouchHandlerTestRootUI
- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  return self;
}
@end

@interface LynxTouchHandlerTestTouch : UITouch
@property(nonatomic) CGPoint testPoint;
@property(nonatomic) UITouchPhase testPhase;
@end

@implementation LynxTouchHandlerTestTouch
- (CGPoint)locationInView:(UIView*)view {
  return self.testPoint;
}
- (UITouchPhase)phase {
  return self.testPhase;
}
@end

@implementation LynxTouchHandlerUnitTest {
  LynxTouchHandler* _handler;
  NSArray<UIView*>* _pageViews;
  LynxRootUI* _parentPage;
  LynxRootUI* _childPage;
  LynxUIContext* _childContext;
  LynxEventHandler* _parentEventHandler;
  LynxEventHandler* _childEventHandler;
  LynxEventEmitter* _childEmitter;
  NSMutableArray<LynxTouchEvent*>* _childTouchEvents;
  UIEvent* _touchEvent;
}

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  _handler = [[LynxTouchHandler alloc] init];
  // will call real method if not stubbed
  _handler = OCMPartialMock(_handler);
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  _handler = NULL;
}

- (void)testOnTouchesMoveWithTarget {
  NSMutableArray<id<LynxEventTarget>>* pre = [[NSMutableArray alloc] init];
  ;
  for (int i = 0; i < 10; ++i) {
    MockEventTarget* target = [[MockEventTarget alloc] init];
    target.count = i;
    [pre addObject:target];
  }

  NSMutableArray<LynxWeakProxy*>* preTouchDeque = [[NSMutableArray alloc] init];
  for (id<LynxEventTarget> target : pre) {
    [preTouchDeque addObject:[LynxWeakProxy proxyWithTarget:target]];
  }

  _handler.touchDeque = preTouchDeque;

  MockEventTarget* target = [[MockEventTarget alloc] init];
  target.count = 11;
  [_handler onTouchesMoveWithTarget:target];

  XCTAssert([_handler.touchDeque count] == 0);
}

- (void)testEventThrough {
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  LynxUIView* parentUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [rootUI insertChild:parentUI atIndex:0];
  LynxUIView* childUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [parentUI insertChild:childUI atIndex:0];
  XCTAssertFalse([childUI eventThrough:CGPointZero]);
  [LynxPropsProcessor updateProp:@YES withKey:@"event-through" forUI:parentUI];
  XCTAssertTrue([childUI eventThrough:CGPointZero]);
}

- (void)testEventThroughInheritsFromPageOnlyWhenEnabled {
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  LynxUIView* childUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [rootUI insertChild:childUI atIndex:0];
  [LynxPropsProcessor updateProp:@YES withKey:@"event-through" forUI:rootUI];

  XCTAssertFalse([childUI eventThrough:CGPointZero]);

  LynxUIContext* mockContext = OCMClassMock([LynxUIContext class]);
  OCMStub([mockContext enableEventThroughInheritFromPage]).andReturn(YES);
  childUI.context = mockContext;
  XCTAssertTrue([childUI eventThrough:CGPointZero]);

  [LynxPropsProcessor updateProp:@NO withKey:@"event-through" forUI:childUI];
  XCTAssertFalse([childUI eventThrough:CGPointZero]);
}

- (void)testEventThroughActiveRegions {
  LynxRootUI* rootUI = [[LynxRootUI alloc]
      initWithLynxView:(LynxView*)[[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)]];
  LynxUIView* parentUI =
      [[LynxUIView alloc] initWithView:[[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)]];
  [rootUI insertChild:parentUI atIndex:0];
  LynxUIView* childUI =
      [[LynxUIView alloc] initWithView:[[UIView alloc] initWithFrame:CGRectMake(0, 0, 300, 300)]];
  [parentUI insertChild:childUI atIndex:0];

  XCTAssertFalse([childUI eventThrough:CGPointZero]);
  [LynxPropsProcessor updateProp:@[ @[ @"0px", @"0px", @"300px", @"150px" ] ]
                         withKey:@"event-through-active-regions"
                           forUI:childUI];
  XCTAssertFalse([childUI eventThrough:CGPointMake(150, 50)]);
  XCTAssertTrue([childUI eventThrough:CGPointMake(150, 200)]);

  [LynxPropsProcessor updateProp:@YES withKey:@"event-through" forUI:parentUI];
  XCTAssertTrue([childUI eventThrough:CGPointMake(150, 50)]);
  XCTAssertFalse([childUI eventThrough:CGPointMake(150, 200)]);

  [LynxPropsProcessor updateProp:@[ @[ @"0px", @"150px", @"300px", @"150px" ] ]
                         withKey:@"event-through-active-regions"
                           forUI:parentUI];
  XCTAssertFalse([childUI eventThrough:CGPointMake(150, 50)]);
  XCTAssertFalse([childUI eventThrough:CGPointMake(150, 200)]);
}

- (void)testIgnoreFocus {
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  LynxUIView* parentUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [rootUI insertChild:parentUI atIndex:0];
  LynxUIView* childUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [parentUI insertChild:childUI atIndex:0];
  XCTAssertFalse([childUI ignoreFocus]);
  [LynxPropsProcessor updateProp:@YES withKey:@"ignore-focus" forUI:parentUI];
  XCTAssertTrue([childUI ignoreFocus]);
}

- (void)testPointerEvents {
  LynxRootUI* rootUI = [[LynxRootUI alloc] initWithLynxView:(LynxView*)[UIView new]];
  LynxUIView* parentUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [rootUI insertChild:parentUI atIndex:0];
  LynxUIView* childUI = [[LynxUIView alloc] initWithView:[UIView new]];
  [parentUI insertChild:childUI atIndex:0];
  XCTAssertTrue([childUI pointerEvents] == kLynxPointerEventsValueAuto);
  [LynxPropsProcessor updateProp:@1 withKey:@"pointer-events" forUI:parentUI];
  XCTAssertTrue([childUI pointerEvents] == kLynxPointerEventsValueNone);
}

- (void)testDispatchPlatformUIEvent {
  // 1. Mock dependencies
  LynxEventHandler* mockEventHandler = OCMClassMock([LynxEventHandler class]);
  LynxUIOwner* mockUIOwner = OCMClassMock([LynxUIOwner class]);
  LynxUIContext* mockUIContext = OCMClassMock([LynxUIContext class]);
  LynxContext* mockLynxContext = OCMClassMock([LynxContext class]);
  LynxView* mockRootView = OCMClassMock([LynxView class]);
  LynxTemplateRender* mockTemplateRender = OCMClassMock([LynxTemplateRender class]);
  LynxTouchHandler* touchHandler =
      OCMPartialMock([[LynxTouchHandler alloc] initWithEventHandler:mockEventHandler]);

  // 2. Setup mock chain
  OCMStub([mockEventHandler uiOwner]).andReturn(mockUIOwner);
  OCMStub([mockUIOwner uiContext]).andReturn(mockUIContext);
  OCMStub([mockUIContext lynxContext]).andReturn(mockLynxContext);
  OCMStub([mockUIContext rootView]).andReturn(mockRootView);
  OCMStub([mockRootView templateRender]).andReturn(mockTemplateRender);

  // 3. Test Case: isFragmentLayerRenderOn is YES
  __block BOOL stubReturnValue = YES;
  OCMStub([mockLynxContext isFragmentLayerRenderOn]).andDo(^(NSInvocation* invocation) {
    BOOL result = stubReturnValue;
    [invocation setReturnValue:&result];
  });

  // Mock touch and event
  UITouch* mockTouch = OCMClassMock([UITouch class]);
  OCMStub([mockTouch type]).andReturn(UITouchTypeDirect);
  OCMStub([mockTouch locationInView:OCMArg.any]).andReturn(CGPointMake(10, 20));
  NSSet* touches = [NSSet setWithObject:mockTouch];
  UIEvent* event = OCMClassMock([UIEvent class]);

  // Stub inner methods to verify they are NOT called
  OCMStub([touchHandler touchesBeganInner:OCMArg.any withEvent:OCMArg.any]);
  OCMStub([touchHandler touchesMovedInner:OCMArg.any withEvent:OCMArg.any]);
  OCMStub([touchHandler touchesEndedInner:OCMArg.any withEvent:OCMArg.any]);
  OCMStub([touchHandler touchesCancelledInner:OCMArg.any withEvent:OCMArg.any]);

  // Verify touchesBegan
  [touchHandler touchesBegan:touches withEvent:event];
  OCMVerify([mockTemplateRender DispatchPlatformInputEvent:[OCMArg any] withData:[OCMArg any]]);
  OCMVerify(never(), [touchHandler touchesBeganInner:OCMArg.any withEvent:OCMArg.any]);

  // Verify touchesMoved
  [touchHandler touchesMoved:touches withEvent:event];
  OCMVerify([mockTemplateRender DispatchPlatformInputEvent:[OCMArg any] withData:[OCMArg any]]);
  OCMVerify(never(), [touchHandler touchesMovedInner:OCMArg.any withEvent:OCMArg.any]);

  // Verify touchesEnded
  [touchHandler touchesEnded:touches withEvent:event];
  OCMVerify([mockTemplateRender DispatchPlatformInputEvent:[OCMArg any] withData:[OCMArg any]]);
  OCMVerify(never(), [touchHandler touchesEndedInner:OCMArg.any withEvent:OCMArg.any]);

  // Verify touchesCancelled
  [touchHandler touchesBegan:touches withEvent:event];
  [touchHandler touchesCancelled:touches withEvent:event];
  OCMVerify([mockTemplateRender DispatchPlatformInputEvent:[OCMArg any] withData:[OCMArg any]]);
  OCMVerify(never(), [touchHandler touchesCancelledInner:OCMArg.any withEvent:OCMArg.any]);

  // 4. Test Case: isFragmentLayerRenderOn is NO
  stubReturnValue = NO;

  // Verify touchesBegan
  [touchHandler touchesBegan:touches withEvent:event];
  OCMVerify([touchHandler touchesBeganInner:touches withEvent:event]);
}

- (void)setUpParentChildPagesWithMultiTouch:(BOOL)enableMultiTouch {
  _pageViews = @[ [UIView new], [LynxTouchHandlerTestView new] ];
  _parentPage = [[LynxTouchHandlerTestRootUI alloc] initWithLynxView:(LynxView*)_pageViews[0]];
  _childPage = [[LynxTouchHandlerTestRootUI alloc] initWithLynxView:(LynxView*)_pageViews[1]];
  _parentPage.sign = 1;
  _childPage.sign = 2;
  _parentEventHandler = [[LynxEventHandler alloc] initWithRootView:_parentPage.view
                                                        withRootUI:_parentPage];
  _childEventHandler = [[LynxEventHandler alloc] initWithRootView:_childPage.view
                                                       withRootUI:_childPage];
  // Child pages receive touches from the parent instead of an attached recognizer.
  [_childPage.view removeGestureRecognizer:_childEventHandler.touchRecognizer];
  _childContext = OCMClassMock([LynxUIContext class]);
  OCMStub([_childContext eventHandler]).andReturn(_childEventHandler);
  _childPage.context = _childContext;
  [_parentPage setChildrenLynxPageUI:[@{[NSString stringWithFormat:@"%p", _parentPage] : _childPage}
                                         mutableCopy]];
  [_parentEventHandler.touchRecognizer setEnableMultiTouch:enableMultiTouch];
  [_childEventHandler.touchRecognizer setEnableMultiTouch:enableMultiTouch];

  _childEmitter = OCMClassMock([LynxEventEmitter class]);
  _childTouchEvents = [NSMutableArray new];
  NSMutableArray<LynxTouchEvent*>* events = _childTouchEvents;
  void (^recordEvent)(NSInvocation*) = ^(NSInvocation* invocation) {
    __unsafe_unretained LynxTouchEvent* event;
    [invocation getArgument:&event atIndex:2];
    [events addObject:event];
  };
  OCMStub([_childEmitter dispatchTouchEvent:OCMArg.any]).andDo(recordEvent);
  OCMStub([_childEmitter dispatchMultiTouchEvent:OCMArg.any]).andDo(recordEvent);
  [_childEventHandler updateUiOwner:nil eventEmitter:_childEmitter];
  _touchEvent = OCMClassMock([UIEvent class]);
  [_parentEventHandler hitTest:CGPointZero withEvent:_touchEvent];
}

- (LynxTouchHandlerTestTouch*)touchAtPoint:(CGPoint)point {
  LynxTouchHandlerTestTouch* touch = [LynxTouchHandlerTestTouch new];
  touch.testPoint = point;
  touch.testPhase = UITouchPhaseBegan;
  return touch;
}

- (void)testChildTouchEventsSurviveRepeatedHitTest {
  for (NSNumber* cancelled in @[ @NO, @YES ]) {
    [self setUpParentChildPagesWithMultiTouch:NO];
    LynxTouchHandlerTestTouch* touch = [self touchAtPoint:CGPointMake(10, 20)];
    NSSet<UITouch*>* touches = [NSSet setWithObject:touch];
    [_parentEventHandler.touchRecognizer touchesBeganInner:touches withEvent:_touchEvent];

    [_parentEventHandler hitTest:touch.testPoint withEvent:_touchEvent];
    [_parentEventHandler hitTest:touch.testPoint withEvent:_touchEvent];
    touch.testPoint = CGPointMake(12, 20);
    touch.testPhase = UITouchPhaseMoved;
    [_parentEventHandler.touchRecognizer touchesMovedInner:touches withEvent:_touchEvent];

    // A terminal UITouch phase does not mean the child's terminal callback has run.
    touch.testPhase = cancelled.boolValue ? UITouchPhaseCancelled : UITouchPhaseEnded;
    [_parentEventHandler hitTest:touch.testPoint withEvent:_touchEvent];
    if (cancelled.boolValue) {
      [_parentEventHandler.touchRecognizer touchesCancelledInner:touches withEvent:_touchEvent];
    } else {
      [_parentEventHandler.touchRecognizer touchesEndedInner:touches withEvent:_touchEvent];
    }

    NSString* terminalEvent = cancelled.boolValue ? LynxEventTouchCancel : LynxEventTouchEnd;
    XCTAssertEqualObjects([_childTouchEvents valueForKey:@"eventName"],
                          (@[ LynxEventTouchStart, LynxEventTouchMove, terminalEvent ]));
    for (LynxTouchEvent* event in _childTouchEvents) {
      XCTAssertEqual(event.eventTarget, _childPage);
      XCTAssertEqual(event.targetSign, _childPage.sign);
    }
    XCTAssertNil(_childEventHandler.touchRecognizer.target);
  }
}

- (void)testAdditionalChildTouchPreservesExistingIdentifiers {
  [self setUpParentChildPagesWithMultiTouch:YES];
  LynxTouchHandlerTestTouch* first = [self touchAtPoint:CGPointMake(10, 20)];
  LynxTouchHandlerTestTouch* second = [self touchAtPoint:CGPointMake(20, 30)];
  NSSet<UITouch*>* firstTouches = [NSSet setWithObject:first];
  NSSet<UITouch*>* secondTouches = [NSSet setWithObject:second];
  [_parentEventHandler.touchRecognizer touchesBeganInner:firstTouches withEvent:_touchEvent];
  [_parentEventHandler hitTest:second.testPoint withEvent:_touchEvent];
  [_parentEventHandler.touchRecognizer touchesBeganInner:secondTouches withEvent:_touchEvent];

  first.testPoint = CGPointMake(12, 20);
  first.testPhase = UITouchPhaseMoved;
  [_parentEventHandler.touchRecognizer touchesMovedInner:firstTouches withEvent:_touchEvent];
  first.testPhase = UITouchPhaseEnded;
  [_parentEventHandler.touchRecognizer touchesEndedInner:firstTouches withEvent:_touchEvent];
  XCTAssertEqual(_childEventHandler.touchRecognizer.target, _childPage);

  second.testPoint = CGPointMake(22, 30);
  second.testPhase = UITouchPhaseMoved;
  [_parentEventHandler.touchRecognizer touchesMovedInner:secondTouches withEvent:_touchEvent];
  second.testPhase = UITouchPhaseCancelled;
  [_parentEventHandler.touchRecognizer touchesCancelledInner:secondTouches withEvent:_touchEvent];

  XCTAssertEqualObjects([_childTouchEvents valueForKey:@"eventName"], (@[
                          LynxEventTouchStart, LynxEventTouchStart, LynxEventTouchMove,
                          LynxEventTouchEnd, LynxEventTouchMove, LynxEventTouchCancel
                        ]));
  NSMutableArray<NSNumber*>* identifiers = [NSMutableArray new];
  for (LynxTouchEvent* event in _childTouchEvents) {
    NSArray<NSArray*>* touchData = event.uiTouchMap[@"2"];
    XCTAssertEqual(touchData.count, 1u);
    [identifiers addObject:touchData.firstObject.firstObject];
  }
  XCTAssertEqualObjects(identifiers, (@[ @0, @1, @0, @0, @1, @1 ]));
  XCTAssertNil(_childEventHandler.touchRecognizer.target);
}

- (void)testNewChildTouchRecoversStaleSequence {
  [self setUpParentChildPagesWithMultiTouch:NO];
  [_parentEventHandler.touchRecognizer setEnableMultiTouch:YES];
  // Another parent touch remains active outside this child page.
  NSMutableDictionary* childPages = _parentPage.childrenLynxPageUI;
  [_parentPage setChildrenLynxPageUI:[NSMutableDictionary new]];
  LynxTouchHandlerTestTouch* parentTouch = [self touchAtPoint:CGPointZero];
  [_parentEventHandler.touchRecognizer touchesBeganInner:[NSSet setWithObject:parentTouch]
                                               withEvent:_touchEvent];
  [_parentPage setChildrenLynxPageUI:childPages];
  LynxTouchHandlerTestTouch* staleTouch = [self touchAtPoint:CGPointMake(10, 20)];
  // Simulate a child sequence whose terminal event was never forwarded by the parent.
  [_childEventHandler.touchRecognizer touchesBeganInner:[NSSet setWithObject:staleTouch]
                                              withEvent:_touchEvent];
  LynxTouchHandlerTestTouch* touch = [self touchAtPoint:CGPointMake(20, 30)];
  NSSet<UITouch*>* touches = [NSSet setWithObject:touch];
  [_parentEventHandler hitTest:touch.testPoint withEvent:_touchEvent];
  [_parentEventHandler.touchRecognizer touchesBeganInner:touches withEvent:_touchEvent];
  touch.testPhase = UITouchPhaseEnded;
  [_parentEventHandler.touchRecognizer touchesEndedInner:touches withEvent:_touchEvent];

  XCTAssertEqualObjects([_childTouchEvents valueForKey:@"eventName"],
                        (@[ LynxEventTouchStart, LynxEventTouchStart, LynxEventTouchEnd ]));
  XCTAssertTrue(CGPointEqualToPoint(_childTouchEvents.lastObject.pagePoint, touch.testPoint));
  XCTAssertNil(_childEventHandler.touchRecognizer.target);
}

- (void)testSingleTouchModeStillIgnoresAdditionalTouch {
  [self setUpParentChildPagesWithMultiTouch:NO];
  LynxTouchHandlerTestTouch* first = [self touchAtPoint:CGPointMake(10, 20)];
  LynxTouchHandlerTestTouch* second = [self touchAtPoint:CGPointMake(20, 30)];
  NSSet<UITouch*>* touches = [NSSet setWithObject:first];
  [_parentEventHandler.touchRecognizer touchesBeganInner:touches withEvent:_touchEvent];
  [_parentEventHandler hitTest:second.testPoint withEvent:_touchEvent];
  [_parentEventHandler.touchRecognizer touchesBeganInner:[NSSet setWithObject:second]
                                               withEvent:_touchEvent];
  first.testPhase = UITouchPhaseEnded;
  [_parentEventHandler.touchRecognizer touchesEndedInner:touches withEvent:_touchEvent];

  XCTAssertEqualObjects([_childTouchEvents valueForKey:@"eventName"],
                        (@[ LynxEventTouchStart, LynxEventTouchEnd ]));
  XCTAssertTrue(CGPointEqualToPoint(_childTouchEvents.lastObject.pagePoint, first.testPoint));
  XCTAssertNil(_childEventHandler.touchRecognizer.target);
}

// A system interaction (e.g. a dismissing UIMenu on iOS 26+) can take a recognizer's
// touches away without delivering touchesEnded / touchesCancelled — UIKit then only
// calls reset. The engine must treat that as the end of the in-flight sequence,
// otherwise _touches never empties and every later touch is dropped as an "extra
// finger" of the dead sequence.
- (void)testResetEndsStrandedTouchSequence {
  LynxEventHandler* mockEventHandler = OCMClassMock([LynxEventHandler class]);
  id<LynxEventTarget> mockTarget = OCMProtocolMock(@protocol(LynxEventTarget));
  OCMStub([mockEventHandler touchTarget]).andReturn(mockTarget);
  [[[[mockEventHandler stub] ignoringNonObjectArgs] andReturn:mockTarget]
      hitTestInner:CGPointZero
         withEvent:[OCMArg any]];
  LynxTouchHandler* handler = [[LynxTouchHandler alloc] initWithEventHandler:mockEventHandler];

  UITouch* mockTouch = OCMClassMock([UITouch class]);
  OCMStub([mockTouch locationInView:OCMArg.any]).andReturn(CGPointMake(10, 20));
  UIEvent* mockEvent = OCMClassMock([UIEvent class]);

  [handler touchesBegan:[NSSet setWithObject:mockTouch] withEvent:mockEvent];
  XCTAssertTrue(handler.target == mockTarget);

  // UIKit takes the touches away: no touchesEnded / touchesCancelled, only reset.
  [handler reset];

  OCMVerify([mockTarget dispatchTouch:LynxEventTouchCancel
                              touches:OCMArg.any
                            withEvent:OCMArg.any]);
  XCTAssertNil(handler.target);
  XCTAssertTrue(handler.preTarget == mockTarget);

  // The next touch must start a fresh sequence instead of being treated as an
  // extra finger of the stranded one.
  UITouch* nextTouch = OCMClassMock([UITouch class]);
  OCMStub([nextTouch locationInView:OCMArg.any]).andReturn(CGPointMake(30, 40));
  [handler touchesBegan:[NSSet setWithObject:nextTouch] withEvent:mockEvent];
  XCTAssertTrue(handler.target == mockTarget);
}

// Even if reset is never delivered, a touchesBegan that arrives while a dead touch is
// still tracked must end the stranded sequence first and then start a fresh one.
- (void)testTouchesBeganFlushesStrandedTouchSequence {
  LynxEventHandler* mockEventHandler = OCMClassMock([LynxEventHandler class]);
  id<LynxEventTarget> strandedTarget = OCMProtocolMock(@protocol(LynxEventTarget));
  OCMStub([mockEventHandler touchTarget]).andReturn(strandedTarget);
  [[[[mockEventHandler stub] ignoringNonObjectArgs] andReturn:strandedTarget]
      hitTestInner:CGPointZero
         withEvent:[OCMArg any]];
  LynxTouchHandler* handler = [[LynxTouchHandler alloc] initWithEventHandler:mockEventHandler];

  UITouch* strandedTouch = OCMClassMock([UITouch class]);
  OCMStub([strandedTouch locationInView:OCMArg.any]).andReturn(CGPointMake(10, 20));
  OCMStub([strandedTouch phase]).andReturn(UITouchPhaseEnded);
  UIEvent* strandedEvent = OCMClassMock([UIEvent class]);
  [handler touchesBegan:[NSSet setWithObject:strandedTouch] withEvent:strandedEvent];
  XCTAssertTrue(handler.target == strandedTarget);

  // A new touch arrives; the tracked touch is no longer alive in the new event.
  id<LynxEventTarget> freshTarget = OCMProtocolMock(@protocol(LynxEventTarget));
  OCMStub([mockEventHandler touchTarget]).andReturn(freshTarget);
  [[[[mockEventHandler stub] ignoringNonObjectArgs] andReturn:freshTarget]
      hitTestInner:CGPointZero
         withEvent:[OCMArg any]];
  UITouch* freshTouch = OCMClassMock([UITouch class]);
  OCMStub([freshTouch locationInView:OCMArg.any]).andReturn(CGPointMake(30, 40));
  OCMStub([freshTouch phase]).andReturn(UITouchPhaseBegan);
  UIEvent* freshEvent = OCMClassMock([UIEvent class]);
  OCMStub([freshEvent allTouches]).andReturn([NSSet setWithObject:freshTouch]);

  [handler touchesBegan:[NSSet setWithObject:freshTouch] withEvent:freshEvent];

  // The stranded sequence was closed with touchcancel…
  OCMVerify([strandedTarget dispatchTouch:LynxEventTouchCancel
                                  touches:OCMArg.any
                                withEvent:OCMArg.any]);
  // …and the new touch started a fresh sequence on the new target.
  OCMVerify([freshTarget dispatchTouch:LynxEventTouchStart
                               touches:OCMArg.any
                             withEvent:OCMArg.any]);
  XCTAssertTrue(handler.target == freshTarget);
}

- (NSInteger)getGestureArenaMemberId {
  return 0;
}

@end
