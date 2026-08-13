// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include <deque>

#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTouchHandler+Internal.h>
#import <Lynx/LynxTouchHandler.h>
#import <Lynx/LynxUIRenderer.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxWeakProxy.h>
#import "LynxTouchHandlerUnitTest.h"

@interface LynxTouchHandler ()

- (void)onTouchesMoveWithTarget:(id<LynxEventTarget>)target;
- (BOOL)dispatchPlatformUIEvent:(NSSet<UITouch*>*)touches
                      withEvent:(UIEvent*)event
                        forType:(NSInteger)actionType;
- (BOOL)isDescendantOfLynxView:(UIGestureRecognizer*)gesture;
- (BOOL)blockNativeEvent:(UIGestureRecognizer*)gestureRecognizer;
- (BOOL)enableSimultaneousTouch;
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer;

@end

@interface LynxUIRenderer (LynxPlatformInputEventTest)

- (NSArray*)expandLegacyPointerEventData:(NSArray*)eventData
                                  action:(NSInteger)action
                             pointerType:(NSInteger)pointerType
                            pointerCount:(NSInteger)pointerCount;

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

@implementation LynxTouchHandlerUnitTest {
  LynxTouchHandler* _handler;
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
  __block NSArray* capturedIEventData = nil;
  __block NSArray* capturedFEventData = nil;
  OCMStub([mockTemplateRender DispatchPlatformInputEvent:[OCMArg any] withData:[OCMArg any]])
      .andDo(^(NSInvocation* invocation) {
        __unsafe_unretained NSArray* iEventData = nil;
        __unsafe_unretained NSArray* fEventData = nil;
        [invocation getArgument:&iEventData atIndex:2];
        [invocation getArgument:&fEventData atIndex:3];
        capturedIEventData = [iEventData copy];
        capturedFEventData = [fEventData copy];
        BOOL consumed = NO;
        [invocation setReturnValue:&consumed];
      });

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
  XCTAssertEqualObjects(capturedIEventData, (@[ @0, @0, @0, @1, @0 ]));
  XCTAssertEqualObjects(capturedFEventData, (@[ @0, @10, @20, @0, @YES, @0, @1 ]));
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

- (void)testFragmentGestureConflictCancelsPlatformTouches {
  LynxEventHandler* eventHandler = OCMClassMock([LynxEventHandler class]);
  LynxTouchHandler* touchHandler =
      OCMPartialMock([[LynxTouchHandler alloc] initWithEventHandler:eventHandler]);
  UITouch* platformTouch = OCMClassMock([UITouch class]);
  UITouch* legacyTouch = OCMClassMock([UITouch class]);
  NSSet<UITouch*>* platformTouches = [NSSet setWithObject:platformTouch];
  [touchHandler setValue:[platformTouches mutableCopy] forKey:@"platformUITouches"];
  [touchHandler setValue:[NSMutableSet setWithObject:legacyTouch] forKey:@"touches"];
  [touchHandler setValue:@YES forKey:@"touchBegin"];
  [touchHandler setValue:@NO forKey:@"touchEndOrCancel"];

  UIEvent* event = OCMClassMock([UIEvent class]);
  [touchHandler setValue:event forKey:@"event"];
  UIGestureRecognizer* lynxGesture = OCMClassMock([UIGestureRecognizer class]);
  UIGestureRecognizer* externalGesture = OCMClassMock([UIGestureRecognizer class]);
  OCMStub([touchHandler isDescendantOfLynxView:externalGesture]).andReturn(NO);
  OCMStub([touchHandler blockNativeEvent:lynxGesture]).andReturn(NO);
  OCMStub([touchHandler enableSimultaneousTouch]).andReturn(NO);

  __block NSSet<UITouch*>* cancelledTouches = nil;
  OCMStub([touchHandler dispatchPlatformUIEvent:[OCMArg any] withEvent:event forType:3])
      .andDo(^(NSInvocation* invocation) {
        __unsafe_unretained NSSet<UITouch*>* touches = nil;
        [invocation getArgument:&touches atIndex:2];
        cancelledTouches = [touches copy];
        BOOL consumed = YES;
        [invocation setReturnValue:&consumed];
      });

  [touchHandler gestureRecognizer:lynxGesture
      shouldRecognizeSimultaneouslyWithGestureRecognizer:externalGesture];

  XCTAssertEqualObjects(cancelledTouches, platformTouches);
  XCTAssertFalse([cancelledTouches containsObject:legacyTouch]);
}

- (void)testLegacyPointerPrimaryRemainsReservedUntilTypeEnds {
  LynxUIRenderer* renderer = [LynxUIRenderer new];
  NSArray* primaryDown = [renderer expandLegacyPointerEventData:@[ @10, @0, @0 ]
                                                         action:0
                                                    pointerType:0
                                                   pointerCount:1];
  NSArray* secondaryDown = [renderer expandLegacyPointerEventData:@[ @11, @0, @0 ]
                                                           action:0
                                                      pointerType:0
                                                     pointerCount:1];
  NSArray* primaryUp = [renderer expandLegacyPointerEventData:@[ @10, @0, @0 ]
                                                       action:1
                                                  pointerType:0
                                                 pointerCount:1];
  NSArray* thirdDown = [renderer expandLegacyPointerEventData:@[ @12, @0, @0 ]
                                                       action:0
                                                  pointerType:0
                                                 pointerCount:1];

  XCTAssertEqualObjects(primaryDown[4], @YES);
  XCTAssertEqualObjects(secondaryDown[4], @NO);
  XCTAssertEqualObjects(primaryUp[4], @YES);
  XCTAssertEqualObjects(thirdDown[4], @NO);

  [renderer expandLegacyPointerEventData:@[ @11, @0, @0 ] action:1 pointerType:0 pointerCount:1];
  [renderer expandLegacyPointerEventData:@[ @12, @0, @0 ] action:1 pointerType:0 pointerCount:1];
  NSArray* nextPrimaryDown = [renderer expandLegacyPointerEventData:@[ @13, @0, @0 ]
                                                             action:0
                                                        pointerType:0
                                                       pointerCount:1];
  XCTAssertEqualObjects(nextPrimaryDown[4], @YES);
}

- (NSInteger)getGestureArenaMemberId {
  return 0;
}

@end
