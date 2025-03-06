// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include <deque>

#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxWeakProxy.h>
#import "LynxTouchHandler+Internal.h"
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

- (BOOL)eventThrough {
  return NO;
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
  XCTAssertFalse([childUI eventThrough]);
  [LynxPropsProcessor updateProp:@YES withKey:@"event-through" forUI:parentUI];
  XCTAssertTrue([childUI eventThrough]);
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

- (NSInteger)getGestureArenaMemberId {
  return 0;
}

@end
