// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxTouchEvent.h>
#import <Lynx/LynxUIText.h>
#import <XCTest/XCTest.h>
#import "LynxGestureArenaManager.h"
#import "LynxGestureDetectorManager.h"
#import "LynxGestureFlingTrigger.h"
#import "LynxGestureHandlerTrigger.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxGestureHandlerTrigger (UnitTest)

@property(nonatomic, weak) LynxGestureVelocityTracker *velocityTracker;
@property(nonatomic, strong) NSPointerArray *velocityTrackers;

@property(nonatomic, strong) LynxGestureFlingTrigger *flinger;

@property(nonatomic, weak) LynxGestureDetectorManager *gestureDetectorManager;
@property(nonatomic, weak, readonly) LynxGestureArenaManager *gestureArenaManager;
@property(nonatomic, weak) id<LynxGestureArenaMember> winner;
@property(nonatomic, weak) id<LynxGestureArenaMember> lastWinner;
@property(nonatomic, strong) NSArray<id<LynxGestureArenaMember>> *simultaneousWinners;

@property(nonatomic, assign) CGPoint lastPoint;
@property(nonatomic, assign) CGPoint lastFlingPoint;
@property(nonatomic, assign) NSInteger lastFlingTargetId;

@end

@interface LynxGestureArenaMemberMock : NSObject <LynxGestureArenaMember>
@property(nonatomic, strong) NSDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlers;
@property(nonatomic, strong) NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *gestureMap;
@property(nonatomic, assign) NSInteger memberId;
@end

@implementation LynxGestureArenaMemberMock

- (instancetype)init {
  if (self = [super init]) {
    self.memberId = 1;
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

- (void)setGestureMap:(NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureMap {
  _gestureMap = gestureMap;
}

- (void)setGestureArenaMemberId:(NSInteger)memberId {
  _memberId = memberId;
}

- (BOOL)canConsumeGesture:(CGPoint)delta {
  return YES;
}

- (NSInteger)getGestureArenaMemberId {
  return _memberId;
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

@end

@interface LynxGestureHandlerTriggerUnitTest : XCTestCase

@property(nonatomic, strong) LynxGestureHandlerTrigger *trigger;
@property(nonatomic, strong) LynxGestureArenaManager *manager;
@property(nonatomic, strong) LynxGestureDetectorManager *detectorManager;

@end

@implementation LynxGestureHandlerTriggerUnitTest

- (void)setUp {
  self.manager = [[LynxGestureArenaManager alloc] init];
  self.detectorManager = [[LynxGestureDetectorManager alloc] initWithArenaManager:self.manager];
  self.trigger = [[LynxGestureHandlerTrigger alloc] initWithDetectorManager:self.detectorManager
                                                               arenaManager:self.manager];
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testInvocation {
  id<LynxGestureArenaMember> mock = [[LynxGestureArenaMemberMock alloc] init];
  [self.trigger setCurrentWinnerWhenDown:mock];

  id<LynxGestureArenaMember> mock2 = [[LynxGestureArenaMemberMock alloc] init];
  id<LynxGestureArenaMember> mock3 = [[LynxGestureArenaMemberMock alloc] init];

  XCTAssertTrue(self.trigger.winner == mock);

  [self.trigger resolveTouchEvent:LynxEventTouchStart
                          touches:[NSSet set]
                            event:[[UIEvent alloc] init]
                       touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchStart
                                                             targetTag:1
                                                            touchPoint:CGPointZero]
        completionChainCandidates:@[ mock, mock2, mock3 ]
                 bubbleCandidates:@[ mock, mock2, mock3 ]];

  [self.trigger dispatchBubble:LynxEventTouchStart
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchStart
                                                          targetTag:1
                                                         touchPoint:CGPointZero]
               bubbleCandidate:@[ mock, mock2, mock3 ]
                        winner:mock];

  [self.trigger resolveTouchEvent:LynxEventTouchMove
                          touches:[NSSet set]
                            event:[[UIEvent alloc] init]
                       touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchMove
                                                             targetTag:1
                                                            touchPoint:CGPointZero]
        completionChainCandidates:@[ mock, mock2, mock3 ]
                 bubbleCandidates:@[ mock, mock2, mock3 ]];

  [self.trigger dispatchBubble:LynxEventTouchMove
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchMove
                                                          targetTag:1
                                                         touchPoint:CGPointZero]
               bubbleCandidate:@[ mock, mock2, mock3 ]
                        winner:mock];

  [self.trigger resolveTouchEvent:LynxEventTouchEnd
                          touches:[NSSet set]
                            event:[[UIEvent alloc] init]
                       touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchEnd
                                                             targetTag:1
                                                            touchPoint:CGPointZero]
        completionChainCandidates:@[ mock, mock2, mock3 ]
                 bubbleCandidates:@[ mock, mock2, mock3 ]];

  [self.trigger dispatchBubble:LynxEventTouchEnd
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchEnd
                                                          targetTag:1
                                                         touchPoint:CGPointZero]
               bubbleCandidate:@[ mock, mock2, mock3 ]
                        winner:mock];

  [self.trigger resolveTouchEvent:LynxEventTouchCancel
                          touches:[NSSet set]
                            event:[[UIEvent alloc] init]
                       touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchCancel
                                                             targetTag:1
                                                            touchPoint:CGPointZero]
        completionChainCandidates:@[ mock, mock2, mock3 ]
                 bubbleCandidates:@[ mock, mock2, mock3 ]];

  [self.trigger dispatchBubble:LynxEventTouchCancel
                    touchEvent:[[LynxTouchEvent alloc] initWithName:LynxEventTouchCancel
                                                          targetTag:1
                                                         touchPoint:CGPointZero]
               bubbleCandidate:@[ mock, mock2, mock3 ]
                        winner:mock];

  [self.trigger handleGestureDetectorState:mock gestureId:1 state:LynxGestureStateActive];

  LynxGestureVelocityTracker *tracker =
      [[LynxGestureVelocityTracker alloc] initWithRootView:[[UIView alloc] init]];

  [self.trigger addVelocityTracker:tracker];

  LynxGestureVelocityTracker *velocityTracker = [self.trigger.velocityTrackers pointerAtIndex:0];

  XCTAssertTrue(velocityTracker == tracker);
}

- (void)testConvertResponseChainToCompeteChain1 {
  LynxGestureArenaMemberMock *mock1 = [[LynxGestureArenaMemberMock alloc] init];
  LynxGestureArenaMemberMock *mock2 = [[LynxGestureArenaMemberMock alloc] init];
  LynxGestureArenaMemberMock *mock3 = [[LynxGestureArenaMemberMock alloc] init];

  [self.trigger setCurrentWinnerWhenDown:mock1];

  LynxGestureDetectorDarwin *detectorDarwinPan =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockPan" ]
                                               relationMap:@{@"waitFor" : @[ @2, @3 ]}];

  LynxGestureDetectorDarwin *detectorDarwinFling2 =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockFling" ]
                                               relationMap:@{}];

  LynxGestureDetectorDarwin *detectorDarwinFling3 =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:3
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockFling" ]
                                               relationMap:@{}];

  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap = @{@1 : detectorDarwinPan};
  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap2 =
      @{@2 : detectorDarwinFling2};
  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap3 =
      @{@3 : detectorDarwinFling3};

  [mock1 setGestureMap:newGestureMap];

  [mock2 setGestureMap:newGestureMap2];
  [mock2 setMemberId:2];

  [mock3 setGestureMap:newGestureMap3];
  [mock3 setMemberId:3];

  [[mock1 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock1.memberId detector:obj];
      }];
  [[mock2 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock2.memberId detector:obj];
      }];
  [[mock3 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock3.memberId detector:obj];
      }];

  NSArray<id<LynxGestureArenaMember>> *responseList = @[ mock1, mock2, mock3 ];

  NSMutableArray<id<LynxGestureArenaMember>> *competeChain =
      [self.detectorManager convertResponseChainToCompeteChain:responseList];

  XCTAssertEqualObjects(competeChain[0], mock2, @"First object in competeChain should be mock2");
  XCTAssertEqualObjects(competeChain[1], mock3, @"Second object in competeChain should be mock3");
  XCTAssertEqualObjects(competeChain[2], mock1, @"Third object in competeChain should be mock1");
}

- (void)testConvertResponseChainToCompeteChain2 {
  LynxGestureArenaMemberMock *mock1 = [[LynxGestureArenaMemberMock alloc] init];
  LynxGestureArenaMemberMock *mock2 = [[LynxGestureArenaMemberMock alloc] init];
  LynxGestureArenaMemberMock *mock3 = [[LynxGestureArenaMemberMock alloc] init];

  [self.trigger setCurrentWinnerWhenDown:mock1];

  LynxGestureDetectorDarwin *detectorDarwinPan =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockPan" ]
                                               relationMap:@{@"waitFor" : @[ @3, @2 ]}];

  LynxGestureDetectorDarwin *detectorDarwinFling2 =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:2
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockFling" ]
                                               relationMap:@{}];

  LynxGestureDetectorDarwin *detectorDarwinFling3 =
      [[LynxGestureDetectorDarwin alloc] initWithGestureID:3
                                               gestureType:LynxGestureTypeFling
                                      gestureCallbackNames:@[ @"mockFling" ]
                                               relationMap:@{}];

  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap = @{@1 : detectorDarwinPan};
  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap2 =
      @{@2 : detectorDarwinFling2};
  NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *newGestureMap3 =
      @{@3 : detectorDarwinFling3};

  [mock1 setGestureMap:newGestureMap];

  [mock2 setGestureMap:newGestureMap2];
  [mock2 setMemberId:2];

  [mock3 setGestureMap:newGestureMap3];
  [mock3 setMemberId:3];

  [[mock1 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock1.memberId detector:obj];
      }];
  [[mock2 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock2.memberId detector:obj];
      }];
  [[mock3 getGestureDetectorMap]
      enumerateKeysAndObjectsUsingBlock:^(
          NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull obj, BOOL *_Nonnull stop) {
        [self.detectorManager registerGestureDetector:mock3.memberId detector:obj];
      }];

  NSArray<id<LynxGestureArenaMember>> *responseList = @[ mock1, mock2, mock3 ];

  NSMutableArray<id<LynxGestureArenaMember>> *competeChain =
      [self.detectorManager convertResponseChainToCompeteChain:responseList];

  XCTAssertEqualObjects(competeChain[0], mock3, @"First object in competeChain should be mock3");
  XCTAssertEqualObjects(competeChain[1], mock2, @"Second object in competeChain should be mock2");
  XCTAssertEqualObjects(competeChain[2], mock1, @"Third object in competeChain should be mock1");
}

@end
