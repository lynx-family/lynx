// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxBaseGestureHandler.h"
#import "LynxDefaultGestureHandler.h"
#import "LynxFlingGestureHandler.h"
#import "LynxGestureArenaMember.h"
#import "LynxGestureDetectorDarwin.h"
#import "LynxGestureHandlerTrigger.h"
#import "LynxLongPressGestureHandler.h"
#import "LynxUIText.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxLongPressGestureHandlerUnitTest : XCTestCase

@end

@interface LynxLongPressGestureHandlerUnitTest () <LynxGestureArenaMember>

@end

@implementation LynxLongPressGestureHandlerUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testMask {
  LynxLongPressGestureHandler *handler = [[LynxLongPressGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc]
                          initWithGestureID:0
                                gestureType:LynxGestureTypeLongPress
                       gestureCallbackNames:@[
                         ON_TOUCHES_DOWN, ON_TOUCHES_MOVE, ON_TOUCHES_UP, ON_TOUCHES_CANCEL,
                         ON_BEGIN, ON_UPDATE, ON_END
                       ]
                                relationMap:@{}
                                  configMap:@{@"minDuration" : @300.f, @"maxDistance" : @200.f}]];

  XCTAssertTrue([handler isGestureTypeMatched:LynxGestureHandlerOptionLongPress]);
  XCTAssertFalse([handler
      isGestureTypeMatched:LynxGestureHandlerOptionFling & LynxGestureHandlerOptionDefault]);
}

- (void)testInvocation {
  LynxLongPressGestureHandler *handler = [[LynxLongPressGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc]
                          initWithGestureID:0
                                gestureType:LynxGestureTypeLongPress
                       gestureCallbackNames:@[
                         ON_TOUCHES_DOWN, ON_TOUCHES_MOVE, ON_TOUCHES_UP, ON_TOUCHES_CANCEL,
                         ON_BEGIN, ON_UPDATE, ON_END
                       ]
                                relationMap:@{}
                                  configMap:@{@"minDistance" : @300.f}]];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
  [handler onTouchesDown:nil];
  [handler onTouchesMove:nil];
  [handler onTouchesUp:nil];
  [handler onTouchesCancel:nil];
  [handler begin:LynxGestureHandlerOptionLongPress
           point:CGPointZero
         touches:nil
           event:nil
      touchEvent:nil];
  [handler update:LynxGestureHandlerOptionLongPress
            point:CGPointZero
          touches:nil
            event:nil
       touchEvent:nil];
  [handler end:LynxGestureHandlerOptionLongPress
           point:CGPointZero
         touches:nil
           event:nil
      touchEvent:nil];

  [handler onHandle:nil touches:nil event:nil touchEvent:nil flingPoint:CGPointZero];
  XCTAssertEqual(handler.status, LYNX_STATE_UNDETERMINED);
  [handler reset];
  XCTAssertEqual(handler.status, LYNX_STATE_INIT);
  NSSet<UITouch *> *touches = [NSSet setWithObject:[[UITouch alloc] init]];
  UIEvent *event = [[UIEvent alloc] init];
  LynxTouchEvent *touchEvent = [[LynxTouchEvent alloc] init];
  CGPoint flingPoint = CGPointMake(100.0, 200.0);
  [handler onHandle:LynxEventTouchStart
            touches:touches
              event:event
         touchEvent:touchEvent
         flingPoint:CGPointZero];
  XCTAssertEqual(handler.status, LYNX_STATE_BEGIN);
  [handler onHandle:LynxEventTouchEnd
            touches:touches
              event:event
         touchEvent:touchEvent
         flingPoint:CGPointZero];
  XCTAssertEqual(handler.status, LYNX_STATE_FAIL);

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
