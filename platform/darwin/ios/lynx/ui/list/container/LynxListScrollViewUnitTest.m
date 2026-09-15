// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxUIScroller.h>
#import <Lynx/UIScrollView+Lynx.h>
#import <Lynx/UIScrollView+LynxGesture.h>
#import <Lynx/UIScrollView+Nested.h>
#import <XCTest/XCTest.h>

#import "LynxListScrollView.h"

@interface LynxBaseScrollView (ListViewTesting)
- (void)stopSameDirectionParentFlingWithTargetContentOffset:(CGPoint)offset
                                                   velocity:(CGPoint)velocity;
@end

@interface LynxListScrollView (ListViewTesting)
- (void)handlePanGesture:(UIPanGestureRecognizer *)recognizer;
@end

@interface LynxListViewTestOwner : NSObject <LynxListScrollViewOwner>
@property(nonatomic, strong) NSMutableArray<NSString *> *events;
@property(nonatomic, assign) CGSize oldSize;
@property(nonatomic, assign) CGSize newSize;
@property(nonatomic, assign) CGPoint velocity;
@property(nonatomic, assign) CGPoint dragTarget;
@property(nonatomic, assign) BOOL allowScrollToTop;
@property(nonatomic, assign) NSInteger stopCount;
@end

@implementation LynxListViewTestOwner
- (instancetype)init {
  if (self = [super init]) {
    _events = [NSMutableArray new];
  }
  return self;
}
- (NSArray<UIScrollView *> *)getHitTestChainForNestedScrollViews {
  return @[];
}
- (void)scrollStateChangedFrom:(LynxBaseScrollViewScrollState)from
                            to:(LynxBaseScrollViewScrollState)to {
  [self.events addObject:[NSString stringWithFormat:@"state:%ld:%ld", (long)from, (long)to]];
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  [self.events addObject:@"scroll"];
}
- (void)listScrollView:(UIScrollView *)scrollView
    contentSizeChangedFrom:(CGSize)oldSize
                        to:(CGSize)newSize {
  self.oldSize = oldSize;
  self.newSize = newSize;
  [self.events addObject:@"size"];
}
- (void)listScrollViewWillEndDragging:(UIScrollView *)scrollView
                         withVelocity:(CGPoint)velocity
                  targetContentOffset:(inout CGPoint *)targetContentOffset {
  self.velocity = velocity;
  *targetContentOffset = self.dragTarget;
  [self.events addObject:@"list-release"];
}
- (BOOL)listScrollViewShouldScrollToTop:(UIScrollView *)scrollView {
  return self.allowScrollToTop;
}
- (void)listScrollViewAutoScrollDidStop {
  ++self.stopCount;
  [self.events addObject:@"stop"];
}
- (void)detachedFromWindow {
  [self.events addObject:@"detach"];
}
- (void)updateContentSize {
  [self.events addObject:@"update-size"];
}
@end

@interface LynxListViewTestProbe : LynxListScrollView
@property(nonatomic, assign) BOOL testDragging;
@property(nonatomic, assign) BOOL testDecelerating;
@property(nonatomic, assign) CGPoint baseReleaseTarget;
@end

@implementation LynxListViewTestProbe
- (BOOL)isDragging {
  return self.testDragging;
}
- (BOOL)isDecelerating {
  return self.testDecelerating;
}
- (void)stopSameDirectionParentFlingWithTargetContentOffset:(CGPoint)offset
                                                   velocity:(CGPoint)velocity {
  self.baseReleaseTarget = offset;
  [((LynxListViewTestOwner *)self.ui).events addObject:@"base-release"];
  [super stopSameDirectionParentFlingWithTargetContentOffset:offset velocity:velocity];
}
@end

@interface LynxListViewTestRecognizer : UIPanGestureRecognizer
@property(nonatomic, assign) UIGestureRecognizerState recordedState;
@end

@implementation LynxListViewTestRecognizer
- (UIGestureRecognizerState)state {
  return self.recordedState;
}
- (void)setState:(UIGestureRecognizerState)state {
  self.recordedState = state;
}
@end

@interface LynxListScrollViewUnitTest : XCTestCase
@property(nonatomic, strong) LynxListViewTestOwner *owner;
@property(nonatomic, strong) LynxListViewTestProbe *scrollView;
@end

@implementation LynxListScrollViewUnitTest
- (void)setUp {
  [super setUp];
  self.owner = [LynxListViewTestOwner new];
  self.scrollView = [LynxListViewTestProbe new];
  self.scrollView.frame = CGRectMake(0, 0, 100, 100);
  self.scrollView.contentSize = CGSizeMake(300, 400);
  self.scrollView.ui = self.owner;
}
- (void)tearDown {
  [self.scrollView stopScroll];
  self.scrollView = nil;
  self.owner = nil;
  [super tearDown];
}
- (NSDictionary *)nativeGestureMap {
  return @{
    @1 : [[LynxGestureDetectorDarwin alloc] initWithGestureID:1
                                                  gestureType:LynxGestureTypeNative
                                         gestureCallbackNames:@[]
                                                  relationMap:@{}]
  };
}
- (void)testInitialStateAndIndependentBackend {
  XCTAssertTrue([self.scrollView isKindOfClass:LynxBaseScrollView.class]);
  XCTAssertFalse([self.scrollView isKindOfClass:LynxScrollView.class]);
  XCTAssertTrue([self.scrollView conformsToProtocol:@protocol(LynxListContainerScrollView)]);
  XCTAssertTrue(self.scrollView.verticalOrientation);
  XCTAssertTrue(self.scrollView.vertical);
  XCTAssertTrue(self.scrollView.scrollY);
  XCTAssertFalse(self.scrollView.layoutFromEnd);
  XCTAssertEqual(self.scrollView.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
  XCTAssertFalse(self.scrollView.adjustingContentOffsetInternally);
  self.scrollView.adjustingContentOffsetInternally = YES;
  XCTAssertTrue([self.scrollView isLynxListAdjustingContentOffset]);
}
- (void)testDirectionSetterAndExplicitInitialization {
  self.scrollView.verticalOrientation = NO;
  XCTAssertFalse(self.scrollView.vertical);
  XCTAssertFalse(self.scrollView.scrollY);
  self.scrollView.verticalOrientation = YES;
  XCTAssertTrue(self.scrollView.vertical);
  XCTAssertTrue(self.scrollView.scrollY);
  LynxListScrollView *horizontal = [[LynxListScrollView alloc] initWithVertical:NO
                                                                  layoutFromEnd:YES];
  XCTAssertFalse(horizontal.verticalOrientation);
  XCTAssertFalse(horizontal.scrollY);
  XCTAssertTrue(horizontal.layoutFromEnd);
  XCTAssertEqual(horizontal.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
}
- (void)testOwnerDoesNotReplaceUIKitDelegateAndIsWeak {
  XCTAssertEqual(self.scrollView.delegate, self.scrollView);
  XCTAssertEqual(self.scrollView.scrollDelegate, self.owner);
  self.scrollView.ui = nil;
  XCTAssertNil(self.scrollView.scrollDelegate);
  @autoreleasepool {
    LynxListViewTestOwner *temporaryOwner = [LynxListViewTestOwner new];
    self.scrollView.ui = temporaryOwner;
    XCTAssertEqual(self.scrollView.scrollDelegate, temporaryOwner);
  }
  XCTAssertNil(self.scrollView.ui);
  XCTAssertNil(self.scrollView.scrollDelegate);
  XCTAssertEqual(self.scrollView.delegate, self.scrollView);
}
- (void)testEstimatedOffsetConstraintsAndRawBounceOffsets {
  for (NSNumber *vertical in @[ @YES, @NO ]) {
    self.scrollView.verticalOrientation = vertical.boolValue;
    for (NSNumber *lower in @[ @YES, @NO ]) {
      self.scrollView.scrollToLower = lower.boolValue;
      self.scrollView.scrollEstimatedOffset = 60;
      CGPoint proposal = lower.boolValue ? CGPointMake(80, 80) : CGPointMake(40, 40);
      CGPoint expected =
          vertical.boolValue ? CGPointMake(proposal.x, 60) : CGPointMake(60, proposal.y);
      self.scrollView.contentOffset = proposal;
      XCTAssertTrue(CGPointEqualToPoint(self.scrollView.contentOffset, expected));
      [self.scrollView setContentOffset:proposal animated:NO];
      XCTAssertTrue(CGPointEqualToPoint(self.scrollView.contentOffset, expected));
    }
  }
  self.scrollView.scrollEstimatedOffset = LynxListScrollInvalidEstimatedOffset;
  self.scrollView.contentOffset = CGPointMake(-12, -25);
  XCTAssertTrue(CGPointEqualToPoint(self.scrollView.contentOffset, CGPointMake(-12, -25)));
}
- (void)testAnimatedStateUsesConstrainedTargetAndIgnoresNoOp {
  self.scrollView.contentOffset = CGPointMake(0, 60);
  self.scrollView.scrollToLower = YES;
  self.scrollView.scrollEstimatedOffset = 60;
  [self.owner.events removeAllObjects];
  [self.scrollView setContentOffset:CGPointMake(0, 80) animated:YES];
  XCTAssertEqual(self.scrollView.scrollState, LynxBaseScrollViewScrollStateIdle);
  XCTAssertFalse([self.owner.events containsObject:@"state:0:2"]);
  [self.scrollView setContentOffset:CGPointMake(0, 40) animated:YES];
  // UIKit may finish an offscreen animation synchronously; assert the transition itself.
  XCTAssertTrue([self.owner.events containsObject:@"state:0:2"]);
  [self.scrollView.delegate scrollViewDidEndScrollingAnimation:self.scrollView];
  XCTAssertEqual(self.scrollView.scrollState, LynxBaseScrollViewScrollStateIdle);
}
- (void)testGestureFrequencyFiltersNativeOffsetsButAllowsGestureConsumption {
  self.scrollView.gestureEnabled = YES;
  self.scrollView.increaseFrequencyWithGesture = YES;
  self.scrollView.testDragging = YES;
  self.scrollView.contentOffset = CGPointMake(0, 50);
  [self.scrollView setContentOffset:CGPointMake(0, 50) animated:YES];
  XCTAssertTrue(CGPointEqualToPoint(self.scrollView.contentOffset, CGPointZero));
  XCTAssertEqual(self.scrollView.scrollState, LynxBaseScrollViewScrollStateIdle);
  self.scrollView.testDragging = NO;
  self.scrollView.testDecelerating = YES;
  self.scrollView.contentOffset = CGPointMake(0, 50);
  XCTAssertTrue(CGPointEqualToPoint(self.scrollView.contentOffset, CGPointZero));
  self.scrollView.duringGestureScroll = YES;
  self.scrollView.contentOffset = CGPointMake(0, 50);
  XCTAssertEqual(self.scrollView.contentOffset.y, 50);
  self.scrollView.duringGestureScroll = NO;
  self.scrollView.gestureEnabled = NO;
  self.scrollView.contentOffset = CGPointMake(0, 70);
  XCTAssertEqual(self.scrollView.contentOffset.y, 70);
}
- (void)testSizeDeduplicationAndWindowLifecycleOrdering {
  self.scrollView.contentSize = CGSizeMake(300, 400);
  XCTAssertEqual(self.owner.events.count, 0U);
  self.scrollView.contentSize = CGSizeMake(300, 500);
  XCTAssertTrue(CGSizeEqualToSize(self.owner.oldSize, CGSizeMake(300, 400)));
  XCTAssertTrue(CGSizeEqualToSize(self.owner.newSize, self.scrollView.contentSize));
  [self.scrollView updateContentSize];
  XCTAssertEqualObjects(self.owner.events, (@[ @"size", @"update-size" ]));
  [self.scrollView tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
  [self.owner.events removeAllObjects];
  [self.scrollView willMoveToWindow:nil];
  XCTAssertEqualObjects(self.owner.events, (@[ @"state:2:0", @"detach" ]));
  [self.owner.events removeAllObjects];
  [self.scrollView willMoveToWindow:[UIWindow new]];
  XCTAssertEqual(self.owner.events.count, 0U);
}
- (void)testReleaseTargetIsForwardedToBaseAfterOwnerAdjustment {
  self.owner.dragTarget = CGPointMake(0, 85);
  CGPoint target = CGPointMake(0, 120);
  [self.scrollView.delegate scrollViewWillEndDragging:self.scrollView
                                         withVelocity:CGPointMake(0, 2)
                                  targetContentOffset:&target];
  XCTAssertEqualObjects(self.owner.events, (@[ @"list-release", @"base-release" ]));
  XCTAssertTrue(CGPointEqualToPoint(target, self.owner.dragTarget));
  XCTAssertTrue(CGPointEqualToPoint(self.scrollView.baseReleaseTarget, target));
  XCTAssertEqual(self.owner.velocity.y, 2);
  XCTAssertFalse([self.scrollView.delegate scrollViewShouldScrollToTop:self.scrollView]);
  self.owner.allowScrollToTop = YES;
  XCTAssertTrue([self.scrollView.delegate scrollViewShouldScrollToTop:self.scrollView]);
}
- (void)testBaseScrollAndStateCallbacksStillReachOwner {
  self.scrollView.contentOffset = CGPointMake(0, 30);
  XCTAssertTrue([self.owner.events containsObject:@"scroll"]);
  [self.owner.events removeAllObjects];
  [self.scrollView.delegate scrollViewWillBeginDragging:self.scrollView];
  [self.scrollView.delegate scrollViewDidEndDragging:self.scrollView willDecelerate:YES];
  [self.scrollView.delegate scrollViewDidEndDecelerating:self.scrollView];
  XCTAssertEqualObjects(self.owner.events, (@[ @"state:0:1", @"state:1:3", @"state:3:0" ]));
}
- (void)testReplacingAutoScrollDoesNotStopNewGeneration {
  LynxListViewTestOwner *owner = self.owner;
  __block NSInteger cancellations = 0;
  [self.scrollView autoScrollWithRate:10
                             behavior:LynxScrollViewTouchBehaviorForbid
                             interval:0
                             autoStop:NO
                             vertical:YES
                             complete:^BOOL(BOOL enabled, BOOL completed) {
                               XCTAssertFalse(completed);
                               XCTAssertEqual(owner.stopCount, 0);
                               ++cancellations;
                               return enabled;
                             }];
  [self.scrollView
      autoScrollWithRate:20
                behavior:LynxScrollViewTouchBehaviorForbid
                interval:0
                autoStop:NO
                vertical:YES
                complete:^BOOL(BOOL enabled, BOOL completed) {
                  XCTAssertTrue(completed);  // Explicit stopScroll completes the request.
                  XCTAssertEqual(owner.stopCount, 1);
                  [owner.events addObject:@"callback"];
                  return enabled;
                }];
  XCTAssertEqual(cancellations, 1);
  XCTAssertEqual(owner.stopCount, 0);
  [self.scrollView stopScroll];
  [self.scrollView stopScroll];
  XCTAssertEqualObjects(owner.events, (@[ @"stop", @"callback" ]));
  XCTAssertTrue(self.scrollView.scrollEnabled);
}
- (void)testAutoScrollBoundaryCompletesOnceUsingRealDisplayLink {
  XCTestExpectation *done = [self expectationWithDescription:@"boundary completion"];
  LynxListViewTestOwner *owner = self.owner;
  [self.scrollView autoScrollWithRate:1000
                             behavior:LynxScrollViewTouchBehaviorForbid
                             interval:0
                             autoStop:YES
                             vertical:YES
                             complete:^BOOL(BOOL enabled, BOOL completed) {
                               XCTAssertTrue(completed);
                               XCTAssertEqual(owner.stopCount, 1);
                               [done fulfill];
                               return enabled;
                             }];
  [self waitForExpectationsWithTimeout:2 handler:nil];
  XCTAssertEqual(self.scrollView.contentOffset.y, 300);
  [self.scrollView stopScroll];
  XCTAssertEqual(owner.stopCount, 1);
  XCTAssertTrue(self.scrollView.scrollEnabled);
}
- (void)testNilAutoScrollCallbackRestoresOriginalScrollEnabled {
  for (NSNumber *enabled in @[ @YES, @NO ]) {
    self.scrollView.scrollEnabled = enabled.boolValue;
    [self.scrollView autoScrollWithRate:10
                               behavior:LynxScrollViewTouchBehaviorForbid
                               interval:0
                               autoStop:NO
                               vertical:YES
                               complete:nil];
    [self.scrollView stopScroll];
    XCTAssertEqual(self.scrollView.scrollEnabled, enabled.boolValue);
  }
  XCTAssertEqual(self.owner.stopCount, 2);
}
- (void)testNativeRecognizerInstallationRemovalAndDeallocation {
  [self.scrollView setupNativeGestureRecognizerIfNeeded:@{}];
  XCTAssertNil(self.scrollView.nativeGesturePanRecognizer);
  [self.scrollView setupNativeGestureRecognizerIfNeeded:[self nativeGestureMap]];
  UIPanGestureRecognizer *recognizer = self.scrollView.nativeGesturePanRecognizer;
  XCTAssertEqual(recognizer.delegate, self.scrollView);
  XCTAssertTrue([self.scrollView.gestureRecognizers containsObject:recognizer]);
  [self.scrollView setupNativeGestureRecognizerIfNeeded:[self nativeGestureMap]];
  XCTAssertEqual(self.scrollView.nativeGesturePanRecognizer, recognizer);
  [self.scrollView setupNativeGestureRecognizerIfNeeded:@{}];
  XCTAssertNil(recognizer.delegate);
  XCTAssertNil(recognizer.view);
  XCTAssertNil(self.scrollView.nativeGesturePanRecognizer);
  __weak LynxListScrollView *weakView;
  @autoreleasepool {
    LynxListScrollView *temporary = [LynxListScrollView new];
    [temporary setupNativeGestureRecognizerIfNeeded:[self nativeGestureMap]];
    recognizer = temporary.nativeGesturePanRecognizer;
    weakView = temporary;
  }
  XCTAssertNil(weakView);
  XCTAssertNil(recognizer.delegate);
  XCTAssertNil(recognizer.view);
}
- (void)testNativeInterceptionAndRecognizerPriority {
  [self.scrollView setupNativeGestureRecognizerIfNeeded:[self nativeGestureMap]];
  UIPanGestureRecognizer *native = self.scrollView.nativeGesturePanRecognizer;
  XCTAssertFalse([self.scrollView gestureRecognizerShouldBegin:native]);
  self.scrollView.gestureConsumer = [LynxGestureConsumer new];
  [self.scrollView.gestureConsumer interceptGesture:YES];
  XCTAssertTrue([self.scrollView gestureRecognizerShouldBegin:native]);
  XCTAssertTrue(
      [self.scrollView gestureRecognizerShouldBegin:self.scrollView.panGestureRecognizer]);
  UIPanGestureRecognizer *other = [UIPanGestureRecognizer new];
  UIView *otherView = [UIView new];
  [otherView addGestureRecognizer:other];
  XCTAssertFalse([self.scrollView gestureRecognizer:native
      shouldRecognizeSimultaneouslyWithGestureRecognizer:other]);
  self.scrollView.forceCanScroll = YES;
  self.scrollView.blockGestureClass = UIView.class;
  self.scrollView.recognizedViewTag = 42;
  XCTAssertFalse([self.scrollView gestureRecognizer:native
          shouldBeRequiredToFailByGestureRecognizer:other]);
  otherView.tag = 42;
  XCTAssertTrue([self.scrollView gestureRecognizer:native
         shouldBeRequiredToFailByGestureRecognizer:other]);
  self.scrollView.forceCanScroll = NO;
  XCTAssertFalse([self.scrollView gestureRecognizer:native
          shouldBeRequiredToFailByGestureRecognizer:other]);
  [self.scrollView.gestureConsumer interceptGesture:NO];
  XCTAssertFalse([self.scrollView gestureRecognizerShouldBegin:native]);
}
- (void)testSimultaneousRecognitionPreservesBothNestedBackends {
  LynxBaseScrollView *base = [[LynxBaseScrollView alloc] initWithVertical:YES layoutFromEnd:NO];
  XCTAssertTrue([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:base.panGestureRecognizer]);
  base.vertical = NO;
  XCTAssertFalse([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:base.panGestureRecognizer]);
  UIScrollView *legacyParent = [UIScrollView new];
  self.scrollView.parentScrollView = legacyParent;
  self.scrollView.enableNested = YES;
  XCTAssertTrue([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:legacyParent.panGestureRecognizer]);
}

- (void)testNativePanCancellationAndBlockedDescendants {
  LynxListViewTestRecognizer *pan = [LynxListViewTestRecognizer new];
  pan.recordedState = UIGestureRecognizerStateBegan;
  [self.scrollView handlePanGesture:pan];
  XCTAssertEqual(pan.state, UIGestureRecognizerStateCancelled);
  self.scrollView.gestureConsumer = [LynxGestureConsumer new];
  [self.scrollView.gestureConsumer interceptGesture:YES];
  pan.recordedState = UIGestureRecognizerStateBegan;
  [self.scrollView handlePanGesture:pan];
  XCTAssertEqual(pan.state, UIGestureRecognizerStateBegan);

  UIView *child = [UIView new];
  [self.scrollView addSubview:child];
  [child addGestureRecognizer:pan];
  [self.scrollView.gestureConsumer consumeGesture:NO];
  [self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:[UIPanGestureRecognizer new]];
  XCTAssertEqual(pan.state, UIGestureRecognizerStateFailed);
}

- (void)testHorizontalNavigationEdgeCompatibilityInBothDirections {
  UINavigationController *navigation =
      [[UINavigationController alloc] initWithRootViewController:[UIViewController new]];
  UIView *navigationView = navigation.view;
  UIPanGestureRecognizer *navigationPan = [UIPanGestureRecognizer new];
  [navigationView addGestureRecognizer:navigationPan];
  self.scrollView.verticalOrientation = NO;
  self.scrollView.bounces = NO;
  XCTAssertTrue([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:navigationPan]);
  self.scrollView.contentOffset = CGPointMake(50, 0);
  XCTAssertFalse([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:navigationPan]);
  self.scrollView.isRTL = YES;
  self.scrollView.contentOffset = CGPointMake(200, 0);
  XCTAssertTrue([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:navigationPan]);
  self.scrollView.bounces = YES;
  XCTAssertFalse([self.scrollView gestureRecognizer:self.scrollView.panGestureRecognizer
      shouldRecognizeSimultaneouslyWithGestureRecognizer:navigationPan]);
}
@end
