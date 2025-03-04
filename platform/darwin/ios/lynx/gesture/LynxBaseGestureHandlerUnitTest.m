// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxBaseGestureHandler.h"
#import "LynxDefaultGestureHandler.h"
#import "LynxFlingGestureHandler.h"
#import "LynxGestureArenaMember.h"
#import "LynxGestureDetectorDarwin.h"
#import "LynxPanGestureHandler.h"
#import "LynxUIText.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxBaseGestureHandler (UnitTest)

@property(nonatomic, assign) BOOL active;

@property(nonatomic, strong) NSMutableDictionary<const NSString *, NSNumber *> *enableFlags;

@end

@interface LynxBaseGestureHandlerUnitTest : XCTestCase

@end

@interface LynxBaseGestureHandlerUnitTest () <LynxGestureArenaMember>

@end

@implementation LynxBaseGestureHandlerUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testCreateGesture {
  LynxUIMockContext *context =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIText alloc] init]];

  LynxGestureDetectorDarwin *detectorDarwinFling =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:0
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockFling" ]
                                               relationMap:@{@"waitFor" : @[ @1 ]}];

  LynxGestureDetectorDarwin *detectorDarwinPan =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                               gestureType:LynxGestureTypePan
                                      gestureCallbackNames:@[ @"mockPan" ]
                                               relationMap:@{}];

  LynxGestureDetectorDarwin *detectorDarwinDefault =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypeDefault
                                      gestureCallbackNames:@[ @"mockDefault" ]
                                               relationMap:@{}];

  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers =
      [LynxBaseGestureHandler convertToGestureHandler:1
                                              context:(LynxUIContext *)context
                                               member:self
                                     gestureDetectors:@{
                                       @0 : detectorDarwinFling,
                                       @1 : detectorDarwinPan,
                                       @2 : detectorDarwinDefault
                                     }];

  XCTAssertTrue(
      [gestureHandlers[@(LynxGestureHandlerOptionPan)] isKindOfClass:LynxPanGestureHandler.class]);
  XCTAssertTrue([gestureHandlers[@(LynxGestureHandlerOptionFling)]
      isKindOfClass:LynxFlingGestureHandler.class]);
  XCTAssertTrue([gestureHandlers[@(LynxGestureHandlerOptionDefault)]
      isKindOfClass:LynxDefaultGestureHandler.class]);

  NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers2 = [LynxBaseGestureHandler
      convertToGestureHandler:1
                      context:(LynxUIContext *)context
                       member:self
             gestureDetectors:@{@0 : detectorDarwinFling, @1 : detectorDarwinPan}];

  XCTAssertFalse(gestureHandlers2[@(LynxGestureHandlerOptionDefault)]);
}

- (void)testEnableFlags {
  LynxBaseGestureHandler *handler = [[LynxBaseGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc] initWithGestureID:0
                                                            gestureType:LynxGestureTypeFling
                                                   gestureCallbackNames:@[]
                                                            relationMap:@{}]];

  XCTAssertFalse(handler.enableFlags[ON_TOUCHES_DOWN].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_TOUCHES_MOVE].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_TOUCHES_UP].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_TOUCHES_CANCEL].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_BEGIN].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_UPDATE].boolValue);
  XCTAssertFalse(handler.enableFlags[ON_END].boolValue);

  XCTAssertFalse([handler onBeginEnabled]);
  XCTAssertFalse([handler onEndEnabled]);
  XCTAssertFalse([handler onUpdateEnabled]);

  LynxBaseGestureHandler *handler2 = [[LynxBaseGestureHandler alloc]
      initWithSign:1
           context:(LynxUIContext *)[LynxUIUnitTestUtils
                       initUIMockContextWithUI:[[LynxUIText alloc] init]]
            member:self
          detector:[[LynxGestureDetectorDarwin alloc]
                          initWithGestureID:0
                                gestureType:LynxGestureTypeFling
                       gestureCallbackNames:@[ ON_TOUCHES_DOWN, ON_TOUCHES_UP, ON_BEGIN, ON_END ]
                                relationMap:@{}]];

  XCTAssertTrue(handler2.enableFlags[ON_TOUCHES_DOWN].boolValue);
  XCTAssertFalse(handler2.enableFlags[ON_TOUCHES_MOVE].boolValue);
  XCTAssertTrue(handler2.enableFlags[ON_TOUCHES_UP].boolValue);
  XCTAssertFalse(handler2.enableFlags[ON_TOUCHES_CANCEL].boolValue);
  XCTAssertTrue(handler2.enableFlags[ON_BEGIN].boolValue);
  XCTAssertFalse(handler2.enableFlags[ON_UPDATE].boolValue);
  XCTAssertTrue(handler2.enableFlags[ON_END].boolValue);

  XCTAssertTrue([handler2 onBeginEnabled]);
  XCTAssertTrue([handler2 onEndEnabled]);
  XCTAssertFalse([handler2 onUpdateEnabled]);
}

- (void)testInvocation {
  LynxBaseGestureHandler *handler = [[LynxBaseGestureHandler alloc]
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
  [handler begin:0 point:CGPointZero touches:nil event:nil touchEvent:nil];
  [handler update:0 point:CGPointZero touches:nil event:nil touchEvent:nil];
  [handler end:0 point:CGPointZero touches:nil event:nil touchEvent:nil];
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

@end
