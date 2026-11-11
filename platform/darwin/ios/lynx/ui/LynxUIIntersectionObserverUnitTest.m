// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIIntersectionObserverUnitTest.h"
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIOwner.h>

@interface LynxIntersectionObserverPayloadManager : LynxUIIntersectionObserverManager
@property(nonatomic, copy) NSDictionary *payload;
@property(nonatomic, assign) NSUInteger callbackCount;
@end

@implementation LynxIntersectionObserverPayloadManager
- (void)sendIntersectionObserverGlobalEventWithObserverId:(NSInteger)observerId
                                               callbackId:(NSInteger)callbackId
                                                  payload:(NSDictionary *)payload {
  self.payload = payload;
  self.callbackCount++;
}
@end

@interface LynxUIIntersectionObserver (InitialRatioTesting)
- (BOOL)shouldNotifyInitialRatio:(float)ratio;
- (BOOL)hasCrossedThreshold:(IntersectionObserverEntry *)oldEntry
                   newEntry:(IntersectionObserverEntry *)newEntry;
@end

@implementation LynxUIIntersectionObserverUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testObserve {
  LynxUIIntersectionObserverManager *observerManager =
      [[LynxUIIntersectionObserverManager alloc] init];
  LynxView *lynxView = [LynxView new];
  LynxUIOwner *uiOwner = [[LynxUIOwner alloc] initWithContainerView:lynxView
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  [uiOwner createUIWithSign:0
                    tagName:@"page"
                   eventSet:nil
              lepusEventSet:nil
                      props:nil
                  nodeIndex:0
         gestureDetectorSet:nil];
  [uiOwner.rootUI setIdSelector:@"box"];
  observerManager.uiOwner = uiOwner;
  LynxUIIntersectionObserver *mockObserver =
      OCMPartialMock([[LynxUIIntersectionObserver alloc] initWithManager:observerManager
                                                              observerId:0
                                                             componentId:@"0"
                                                                 options:nil]);
  NSMutableArray *array = OCMPartialMock([NSMutableArray new]);
  mockObserver.observationTargets = array;

  [mockObserver observe:@"#box" callbackId:0];
  XCTAssertTrue([array count] == 1);
  [mockObserver disconnect];
  [mockObserver observe:@"0" callbackId:0];
  XCTAssertTrue([array count] == 1);
}

- (void)testObserverRunLoopLifecycle {
  LynxUIIntersectionObserverManager *observerManager =
      OCMPartialMock([[LynxUIIntersectionObserverManager alloc] initWithLynxContext:nil]);
  OCMStub([observerManager addIntersectionObserverToRunLoop]);
  OCMStub([observerManager destroyIntersectionObserver]);

  LynxUIIntersectionObserver *firstObserver =
      [[LynxUIIntersectionObserver alloc] initWithManager:observerManager
                                               observerId:1
                                              componentId:@"0"
                                                  options:nil];
  LynxUIIntersectionObserver *secondObserver =
      [[LynxUIIntersectionObserver alloc] initWithManager:observerManager
                                               observerId:2
                                              componentId:@"0"
                                                  options:nil];

  [observerManager addIntersectionObserver:firstObserver];
  OCMVerify(times(1), [observerManager addIntersectionObserverToRunLoop]);

  [observerManager addIntersectionObserver:secondObserver];
  OCMVerify(times(1), [observerManager addIntersectionObserverToRunLoop]);

  [observerManager removeIntersectionObserver:1];
  OCMVerify(never(), [observerManager destroyIntersectionObserver]);

  [observerManager removeIntersectionObserver:2];
  OCMVerify(times(1), [observerManager destroyIntersectionObserver]);
}

- (void)assertInitialRatio:(float)ratio
              initialRatio:(NSNumber *)initialRatio
                thresholds:(NSArray<NSNumber *> *)thresholds
                compatible:(BOOL)compatible
                  expected:(BOOL)expected {
  LynxUIContext *context = OCMClassMock([LynxUIContext class]);
  OCMStub(context.enableIntersectionObserverInitialRatioCompatibility).andReturn(compatible);
  LynxUIOwner *owner = OCMClassMock([LynxUIOwner class]);
  OCMStub(owner.uiContext).andReturn(context);
  LynxUIIntersectionObserverManager *manager = [[LynxUIIntersectionObserverManager alloc] init];
  manager.uiOwner = owner;
  LynxUIIntersectionObserver *observer = [[LynxUIIntersectionObserver alloc]
      initWithManager:manager
           observerId:1
          componentId:@"0"
              options:@{@"initialRatio" : initialRatio, @"thresholds" : thresholds}];
  XCTAssertEqual([observer shouldNotifyInitialRatio:ratio], expected);
  [(OCMockObject *)owner stopMocking];
  [(OCMockObject *)context stopMocking];
}

- (void)testLegacyInitialRatioPreservesFractionAndRequiresStrictlyGreaterRatio {
  NSArray *thresholds = @[ @0.3, @0.5, @1 ];
  [self assertInitialRatio:0.3 initialRatio:@0.5 thresholds:thresholds compatible:NO expected:NO];
  [self assertInitialRatio:0.5 initialRatio:@0.5 thresholds:thresholds compatible:NO expected:NO];
  [self assertInitialRatio:1 initialRatio:@0.5 thresholds:thresholds compatible:NO expected:YES];
  [self assertInitialRatio:0.2 initialRatio:@0 thresholds:thresholds compatible:NO expected:YES];
}

- (void)testCompatibleInitialRatioRequiresDifferenceAndAnyReachedThreshold {
  NSArray *thresholds = @[ @1, @0.5, @0.3 ];
  [self assertInitialRatio:0.3 initialRatio:@0.5 thresholds:thresholds compatible:YES expected:YES];
  [self assertInitialRatio:0.5 initialRatio:@0.5 thresholds:thresholds compatible:YES expected:NO];
  [self assertInitialRatio:1 initialRatio:@0.5 thresholds:thresholds compatible:YES expected:YES];
  [self assertInitialRatio:0.2 initialRatio:@0 thresholds:thresholds compatible:YES expected:NO];
  [self assertInitialRatio:0.4 initialRatio:@0.5 thresholds:thresholds compatible:YES expected:YES];
}

- (void)testCompatibleInitialZeroRequiresDifferentInitialRatioAndZeroThreshold {
  [self assertInitialRatio:0 initialRatio:@1 thresholds:@[ @0 ] compatible:YES expected:YES];
  [self assertInitialRatio:0 initialRatio:@0 thresholds:@[ @0 ] compatible:YES expected:NO];
  [self assertInitialRatio:0 initialRatio:@1 thresholds:@[ @0.3 ] compatible:YES expected:NO];
  [self assertInitialRatio:0 initialRatio:@1 thresholds:@[ @0 ] compatible:NO expected:NO];
  [self assertInitialRatio:1 initialRatio:@1 thresholds:@[ @0 ] compatible:YES expected:NO];
  [self assertInitialRatio:1 initialRatio:@1 thresholds:@[ @0 ] compatible:NO expected:NO];
}

- (void)assertInitialScreenTargetRect:(CGRect)targetRect
                     intersectionRect:(CGRect)intersectionRect
                                ratio:(float)ratio {
  LynxUIContext *context = OCMClassMock([LynxUIContext class]);
  OCMStub(context.enableIntersectionObserverInitialRatioCompatibility).andReturn(YES);
  LynxUIOwner *owner = OCMClassMock([LynxUIOwner class]);
  OCMStub(owner.uiContext).andReturn(context);
  LynxRootUI *root = OCMClassMock([LynxRootUI class]);
  CGRect rootRect = CGRectInset([UIScreen mainScreen].bounds, -300, -300);
  OCMStub([root getBoundingClientRect]).andReturn(rootRect);
  OCMStub([root isVisible]).andReturn(YES);
  UIView *rootView = OCMClassMock([UIView class]);
  OCMStub(root.view).andReturn(rootView);
  OCMStub([rootView convertRect:targetRect toView:nil]).andReturn(targetRect);
  OCMStub(owner.rootUI).andReturn(root);
  LynxUI *target = OCMClassMock([LynxUI class]);
  OCMStub(target.parent).andReturn(root);
  OCMStub([target getBoundingClientRect]).andReturn(targetRect);
  OCMStub([target isVisible]).andReturn(YES);
  OCMStub([owner findUIBySign:1]).andReturn(target);

  LynxIntersectionObserverPayloadManager *manager =
      [[LynxIntersectionObserverPayloadManager alloc] init];
  manager.uiOwner = owner;
  LynxUIIntersectionObserver *observer =
      [[LynxUIIntersectionObserver alloc] initWithManager:manager
                                               observerId:1
                                              componentId:@"0"
                                                  options:@{
                                                    @"initialRatio" : @1,
                                                    @"thresholds" : @[ @0 ],
                                                    @"__enableGlobalEvent" : @YES
                                                  }];
  [observer relativeToScreenWithMargins:@{}];
  [observer observe:@"1" callbackId:7];

  XCTAssertEqual(manager.callbackCount, 1u);
  XCTAssertEqualWithAccuracy([manager.payload[@"intersectionRatio"] floatValue], ratio, 0.0001);
  XCTAssertEqual([manager.payload[@"isIntersecting"] boolValue], !CGRectIsEmpty(intersectionRect));
  NSDictionary *expectedRect = @{
    @"left" : @((int)CGRectGetMinX(intersectionRect)),
    @"top" : @((int)CGRectGetMinY(intersectionRect)),
    @"right" : @((int)CGRectGetMaxX(intersectionRect)),
    @"bottom" : @((int)CGRectGetMaxY(intersectionRect))
  };
  XCTAssertEqualObjects(manager.payload[@"intersectionRect"], expectedRect);
  [observer checkForIntersections];
  XCTAssertEqual(manager.callbackCount, 1u);

  [(OCMockObject *)target stopMocking];
  [(OCMockObject *)rootView stopMocking];
  [(OCMockObject *)root stopMocking];
  [(OCMockObject *)owner stopMocking];
  [(OCMockObject *)context stopMocking];
}

- (void)testCompatibleInitialScreenOffscreenPayload {
  CGRect screenRect = [UIScreen mainScreen].bounds;
  NSArray<NSValue *> *targetRects = @[
    [NSValue valueWithCGRect:CGRectMake(CGRectGetMaxX(screenRect) + 100, 0, 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(CGRectGetMinX(screenRect) - 200, 0, 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(0, CGRectGetMaxY(screenRect) + 100, 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(0, CGRectGetMinY(screenRect) - 200, 100, 100)]
  ];
  for (NSValue *targetRect in targetRects) {
    [self assertInitialScreenTargetRect:targetRect.CGRectValue intersectionRect:CGRectZero ratio:0];
  }
}

- (void)testCompatibleInitialScreenTouchingEdgePayload {
  CGRect screenRect = [UIScreen mainScreen].bounds;
  NSArray<NSValue *> *targetRects = @[
    [NSValue valueWithCGRect:CGRectMake(CGRectGetMaxX(screenRect), 0, 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(CGRectGetMinX(screenRect) - 100, 0, 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(0, CGRectGetMaxY(screenRect), 100, 100)],
    [NSValue valueWithCGRect:CGRectMake(0, CGRectGetMinY(screenRect) - 100, 100, 100)]
  ];
  for (NSValue *targetRect in targetRects) {
    [self assertInitialScreenTargetRect:targetRect.CGRectValue intersectionRect:CGRectZero ratio:0];
  }
}

- (void)testCompatibleInitialScreenPartialIntersectionPayload {
  CGRect screenRect = [UIScreen mainScreen].bounds;
  CGRect targetRect = CGRectMake(CGRectGetMaxX(screenRect) - 50, 25, 100, 50);
  CGRect intersectionRect = CGRectMake(CGRectGetMaxX(screenRect) - 50, 25, 50, 50);
  [self assertInitialScreenTargetRect:targetRect intersectionRect:intersectionRect ratio:0.5];
}

- (void)testLaterThresholdCrossingUsesObservedRatio {
  LynxUIIntersectionObserver *observer =
      [[LynxUIIntersectionObserver alloc] initWithManager:nil
                                               observerId:1
                                              componentId:@"0"
                                                  options:@{@"thresholds" : @[ @0.3, @0.5, @1 ]}];
  IntersectionObserverEntry *oldEntry = [IntersectionObserverEntry new];
  oldEntry.intersectionRect = CGRectMake(0, 0, 100, 100);
  oldEntry.intersectionRatio = 0.4;
  IntersectionObserverEntry *newEntry = [IntersectionObserverEntry new];
  newEntry.intersectionRect = oldEntry.intersectionRect;
  newEntry.intersectionRatio = 0.45;
  XCTAssertFalse([observer hasCrossedThreshold:oldEntry newEntry:newEntry]);
  newEntry.intersectionRatio = 0.6;
  XCTAssertTrue([observer hasCrossedThreshold:oldEntry newEntry:newEntry]);
}

@end
