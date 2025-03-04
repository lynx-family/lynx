// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxBaseGestureHandler.h"
#import "LynxGestureArenaManager.h"
#import "LynxGestureArenaMember.h"
#import "LynxGestureDetectorManager.h"
#import "LynxGestureFlingTrigger.h"
#import "LynxGestureHandlerTrigger.h"
#import "LynxTouchEvent.h"
#import "LynxUIText.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxGestureArenaMemberMockForArena : NSObject <LynxGestureArenaMember, LynxEventTarget>
@property(nonatomic, strong) NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers;
@property(nonatomic, strong) NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *gestureMap;
@end

@implementation LynxGestureArenaMemberMockForArena

- (instancetype)init {
  if (self = [super init]) {
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

    self.gestureMap =
        @{@0 : detectorDarwinFling, @1 : detectorDarwinPan, @2 : detectorDarwinDefault};

    self.gestureHandlers = [LynxBaseGestureHandler convertToGestureHandler:1
                                                                   context:(LynxUIContext *)context
                                                                    member:self
                                                          gestureDetectors:self.gestureMap];
  }
  return self;
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return YES;
}

- (NSInteger)getGestureArenaMemberId {
  return 1;
}

- (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)getGestureDetectorMap {
  return _gestureMap;
}

- (NSDictionary<NSNumber *, LynxBaseGestureHandler *> *)getGestureHandlers {
  return _gestureHandlers;
}

- (CGFloat)getMemberScrollX {
  return 1.0;
}

- (CGFloat)getMemberScrollY {
  return 1.0;
}

- (void)onGestureScrollBy:(CGPoint)delta {
}

- (NSInteger)signature {
  return 1;
}

- (int32_t)pseudoStatus {
  return 0;
}

- (nullable id<LynxEventTarget>)parentTarget {
  return nil;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return self;
}

- (BOOL)containsPoint:(CGPoint)point {
  return YES;
}

- (nullable NSDictionary<NSString *, LynxEventSpec *> *)eventSet {
  return nil;
}

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent *)event {
  return NO;
}

- (BOOL)ignoreFocus {
  return NO;
}

- (BOOL)consumeSlideEvent:(CGFloat)angle {
  return NO;
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer *)gestureRecognizer {
  return NO;
}

- (BOOL)eventThrough {
  return NO;
}

- (BOOL)enableTouchPseudoPropagation {
  return NO;
}

- (void)onPseudoStatusFrom:(int32_t)preStatus changedTo:(int32_t)currentStatus {
}

// only include touches and event, don't care Lynx frontend event
- (BOOL)dispatchTouch:(NSString *const)touchType
              touches:(NSSet<UITouch *> *)touches
            withEvent:(UIEvent *)event {
  return NO;
}

// include target point and Lynx frontend event
- (BOOL)dispatchEvent:(LynxEventDetail *)event {
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

@interface LynxGestureArenaManagerUnitTest : XCTestCase

@property(nonatomic, strong) LynxGestureArenaManager *manager;

@end

@implementation LynxGestureArenaManagerUnitTest

- (void)setUp {
  self.manager = [[LynxGestureArenaManager alloc] init];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testInvocation {
  id<LynxGestureArenaMember, LynxEventTarget> mock =
      [[LynxGestureArenaMemberMockForArena alloc] init];

  NSInteger gestureId = [self.manager addMember:mock];
  [self.manager registerGestureDetectors:gestureId detectorMap:[mock getGestureDetectorMap]];

  [self.manager setActiveUIToArena:mock];

  [self.manager dispatchTouchToArena:LynxEventTouchStart
                             touches:[NSSet set]
                               event:[[UIEvent alloc] init]
                          touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchStart
                                                                targetTag:1
                                                               touchPoint:CGPointZero]];

  [self.manager dispatchBubble:LynxEventTouchStart
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchStart
                                                          targetTag:1
                                                         touchPoint:CGPointZero]];

  [self.manager dispatchTouchToArena:LynxEventTouchMove
                             touches:[NSSet set]
                               event:[[UIEvent alloc] init]
                          touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchMove
                                                                targetTag:1
                                                               touchPoint:CGPointZero]];

  [self.manager dispatchBubble:LynxEventTouchMove
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchMove
                                                          targetTag:1
                                                         touchPoint:CGPointZero]];

  [self.manager dispatchTouchToArena:LynxEventTouchEnd
                             touches:[NSSet set]
                               event:[[UIEvent alloc] init]
                          touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchEnd
                                                                targetTag:1
                                                               touchPoint:CGPointZero]];

  [self.manager dispatchBubble:LynxEventTouchEnd
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchEnd
                                                          targetTag:1
                                                         touchPoint:CGPointZero]];

  [self.manager dispatchTouchToArena:LynxEventTouchCancel
                             touches:[NSSet set]
                               event:[[UIEvent alloc] init]
                          touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchCancel
                                                                targetTag:1
                                                               touchPoint:CGPointZero]];

  [self.manager dispatchBubble:LynxEventTouchCancel
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchCancel
                                                          targetTag:1
                                                         touchPoint:CGPointZero]];

  [self.manager removeMember:mock detectorMap:[mock getGestureDetectorMap]];
  [self.manager unregisterGestureDetectors:gestureId detectorMap:[mock getGestureDetectorMap]];
}

@end
