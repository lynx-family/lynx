// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxBaseGestureHandler.h"
#import "LynxDefaultGestureHandler.h"
#import "LynxFlingGestureHandler.h"
#import "LynxGestureArenaMember.h"
#import "LynxGestureDetectorDarwin.h"
#import "LynxGestureHandlerTrigger.h"
#import "LynxPanGestureHandler.h"
#import "LynxUIText.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxFlingGestureHandlerUnitTest : XCTestCase

@end

@interface LynxFlingGestureHandlerUnitTest () <LynxGestureArenaMember>

@end

@implementation LynxFlingGestureHandlerUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testMask {
  LynxFlingGestureHandler *handler = [[LynxFlingGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc]
                          initWithGestureID:0
                                gestureType:LynxGestureTypeFling
                       gestureCallbackNames:@[
                         ON_TOUCHES_DOWN, ON_TOUCHES_MOVE, ON_TOUCHES_UP, ON_TOUCHES_CANCEL,
                         ON_BEGIN, ON_UPDATE, ON_END
                       ]
                                relationMap:@{}]];
  XCTAssertTrue([handler isGestureTypeMatched:LynxGestureHandlerOptionFling]);
  XCTAssertFalse(
      [handler isGestureTypeMatched:LynxGestureHandlerOptionPan & LynxGestureHandlerOptionDefault]);
}

- (void)testInvocation {
  LynxFlingGestureHandler *handler = [[LynxFlingGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc]
                          initWithGestureID:0
                                gestureType:LynxGestureTypeFling
                       gestureCallbackNames:@[
                         ON_TOUCHES_DOWN, ON_TOUCHES_MOVE, ON_TOUCHES_UP, ON_TOUCHES_CANCEL,
                         ON_BEGIN, ON_UPDATE, ON_END
                       ]
                                relationMap:@{}]];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
  [handler onTouchesDown:nil];
  [handler onTouchesMove:nil];
  [handler onTouchesUp:nil];
  [handler onTouchesCancel:nil];
  [handler begin:LynxGestureHandlerOptionFling
           point:CGPointZero
         touches:nil
           event:nil
      touchEvent:nil];
  [handler update:LynxGestureHandlerOptionFling
            point:CGPointZero
          touches:nil
            event:nil
       touchEvent:nil];
  [handler end:LynxGestureHandlerOptionFling
           point:CGPointZero
         touches:nil
           event:nil
      touchEvent:nil];

  NSSet<UITouch *> *touches = [NSSet setWithObject:[[UITouch alloc] init]];
  UIEvent *event = [[UIEvent alloc] init];
  LynxTouchEvent *touchEvent = [[LynxTouchEvent alloc] init];
  CGPoint flingPoint = CGPointMake(100.0, 200.0);
  [handler onHandle:LynxEventTouchStart
            touches:touches
              event:event
         touchEvent:touchEvent
         flingPoint:CGPointZero];
  XCTAssertEqual(handler.status, LYNX_STATE_UNDETERMINED);
  [handler reset];

  [handler onHandle:LynxEventTouchMove
            touches:nil
              event:nil
         touchEvent:nil
         flingPoint:CGPointMake(100.0, 200.0)];
  XCTAssertEqual(handler.status, LYNX_STATE_ACTIVE);

#pragma clang diagnostic pop
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return YES;
}

- (NSInteger)getGestureArenaMemberId {
  return 0;
}

- (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)getGestureDetectorMap {
  return nil;
}

- (NSDictionary<NSNumber *, LynxBaseGestureHandler *> *)getGestureHandlers {
  return nil;
}

- (CGFloat)getMemberScrollX {
  return 0;
}

- (CGFloat)getMemberScrollY {
  return 0;
}

- (void)onGestureScrollBy:(CGPoint)delta {
}

- (BOOL)getGestureBorder:(BOOL)start {
  return NO;
}

@end
