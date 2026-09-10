// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "LynxListItemHelper.h"
#import "LynxListScrollHelper.h"

@interface LynxListScrollHelperTestView : UIScrollView <LynxListScrollHelperView>
@property(nonatomic, assign) BOOL scrollToLower;
@property(nonatomic, assign) CGFloat scrollEstimatedOffset;
@property(nonatomic, assign, setter=setLynxListAdjustingContentOffset:,
          getter=isLynxListAdjustingContentOffset) BOOL adjustingContentOffsetInternally;
@property(nonatomic, assign) NSInteger adjustingContentOffsetCount;
@end

@implementation LynxListScrollHelperTestView
- (void)setLynxListAdjustingContentOffset:(BOOL)adjustingContentOffsetInternally {
  _adjustingContentOffsetInternally = adjustingContentOffsetInternally;
  ++_adjustingContentOffsetCount;
}
@end

@interface LynxListScrollHelperTestWrapper : UIView <LynxListItemWrapper>
@property(nonatomic, weak, nullable) LynxUIComponent *holdingUI;
@end

@implementation LynxListScrollHelperTestWrapper
@end

@interface LynxListScrollHelperTestOwner : NSObject <LynxListScrollHelperOwner>
@property(nonatomic, strong) LynxListScrollHelperTestView *scrollView;
@property(nonatomic, assign, getter=isVertical) BOOL vertical;
@property(nonatomic, assign, getter=isRTL) BOOL RTL;
@property(nonatomic, assign) CGRect listFrame;
@property(nonatomic, assign) UIEdgeInsets listPadding;
@property(nonatomic, assign) BOOL deferCompletion;
@property(nonatomic, strong, nullable) id completionToken;
@property(nonatomic, assign) BOOL finishTimeoutForPendingScroll;
@property(nonatomic, assign) NSInteger startAnimationCount;
@property(nonatomic, assign) NSInteger stopCount;
@property(nonatomic, assign) NSInteger finishCount;
@property(nonatomic, assign) LynxListProgrammaticScrollCompletionReason lastCompletionReason;
@property(nonatomic, copy, nullable) void (^finishHandler)(void);
@end

@implementation LynxListScrollHelperTestOwner

- (UIScrollView<LynxListScrollHelperView> *)scrollViewForListScrollHelper {
  return self.scrollView;
}

- (BOOL)isVerticalForListScrollHelper {
  return self.isVertical;
}

- (BOOL)isRTLForListScrollHelper {
  return self.isRTL;
}

- (CGRect)listFrameForListScrollHelper {
  return self.listFrame;
}

- (UIEdgeInsets)listPaddingForListScrollHelper {
  return self.listPadding;
}

- (BOOL)listScrollHelperShouldDeferCompletionForSmoothScroll:(BOOL)smooth
                                              needsAnimation:(BOOL)needsAnimation {
  return self.deferCompletion;
}

- (id)listScrollHelperCompletionToken {
  return self.completionToken;
}

- (BOOL)listScrollHelperShouldFinishTimeoutForPendingScroll {
  return self.finishTimeoutForPendingScroll;
}

- (void)listScrollHelperWillStartScrollAnimation {
  ++self.startAnimationCount;
}

- (void)listScrollHelperDidStopProgrammaticScroll {
  ++self.stopCount;
}

- (void)listScrollHelperDidFinishProgrammaticScrollWithReason:
    (LynxListProgrammaticScrollCompletionReason)reason {
  ++self.finishCount;
  self.lastCompletionReason = reason;
  self.completionToken = nil;
  if (self.finishHandler) {
    self.finishHandler();
  }
}

@end

@interface LynxListScrollHelperUnitTest : XCTestCase
@property(nonatomic, strong) LynxListScrollHelperTestOwner *owner;
@property(nonatomic, strong) LynxListScrollHelper *helper;
@end

@implementation LynxListScrollHelperUnitTest

- (void)setUp {
  [super setUp];
  self.owner = [[LynxListScrollHelperTestOwner alloc] init];
  self.owner.scrollView =
      [[LynxListScrollHelperTestView alloc] initWithFrame:CGRectMake(10, 20, 100, 80)];
  self.owner.scrollView.contentSize = CGSizeMake(300, 400);
  self.owner.scrollView.scrollEstimatedOffset = LynxListScrollInvalidEstimatedOffset;
  self.owner.vertical = YES;
  self.owner.listFrame = self.owner.scrollView.frame;
  self.helper = [[LynxListScrollHelper alloc] initWithOwner:self.owner];
}

- (void)testVerticalGeometryAndClamping {
  self.owner.scrollView.contentOffset = CGPointMake(12, 350);

  XCTAssertEqualWithAccuracy(self.helper.normalizedContentOffset.x, 12, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.normalizedContentOffset.y, 350, 0.001);
  XCTAssertEqualWithAccuracy([self.helper clampedContentOffsetForVerticalAxis:YES], 320, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationMaxScrollableDistance, 320, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationSize, 100, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationContentSize, 400, 0.001);
  XCTAssertEqualWithAccuracy([self.helper clampContentOffset:15 lower:20 size:100 viewportSize:30],
                             20, 0.001);
  XCTAssertEqualWithAccuracy([self.helper clampContentOffset:90 lower:20 size:100 viewportSize:30],
                             70, 0.001);
}

- (void)testContentOffsetIsConstrainedToEstimatedOffset {
  CGPoint offset = [LynxListScrollHelper contentOffset:CGPointMake(10, 130)
                          constrainedToEstimatedOffset:100
                                              vertical:YES
                                         scrollToLower:YES];
  XCTAssertTrue(CGPointEqualToPoint(offset, CGPointMake(10, 100)));

  offset = [LynxListScrollHelper contentOffset:CGPointMake(40, 20)
                  constrainedToEstimatedOffset:60
                                      vertical:NO
                                 scrollToLower:NO];
  XCTAssertTrue(CGPointEqualToPoint(offset, CGPointMake(60, 20)));

  offset = [LynxListScrollHelper contentOffset:CGPointMake(10, 130)
                  constrainedToEstimatedOffset:LynxListScrollInvalidEstimatedOffset
                                      vertical:YES
                                 scrollToLower:YES];
  XCTAssertTrue(CGPointEqualToPoint(offset, CGPointMake(10, 130)));
}

- (void)testApplyVerticalContentSizeAndOffsetAdjustment {
  self.owner.listFrame = CGRectMake(0, 0, 120, 80);
  self.owner.listPadding = UIEdgeInsetsMake(0, 10, 0, 20);

  CGPoint result = [self.helper applyContentSize:500
                                     offsetDelta:CGPointMake(0, 30)
                           previousContentOffset:CGPointMake(5, 100)
                          disableScrollFiltering:NO];

  XCTAssertTrue(CGSizeEqualToSize(self.owner.scrollView.contentSize, CGSizeMake(90, 500)));
  XCTAssertTrue(CGPointEqualToPoint(result, CGPointMake(5, 130)));
  XCTAssertTrue(CGPointEqualToPoint(self.owner.scrollView.contentOffset, CGPointMake(5, 130)));
  XCTAssertEqual(self.owner.scrollView.adjustingContentOffsetCount, 2);
  XCTAssertFalse(self.owner.scrollView.isLynxListAdjustingContentOffset);
}

- (void)testApplyHorizontalRTLContentSizeAndOffsetAdjustment {
  self.owner.vertical = NO;
  self.owner.RTL = YES;
  self.owner.listFrame = CGRectMake(0, 0, 100, 90);
  self.owner.listPadding = UIEdgeInsetsMake(10, 0, 15, 0);

  CGPoint result = [self.helper applyContentSize:400
                                     offsetDelta:CGPointMake(40, 0)
                           previousContentOffset:CGPointMake(50, 7)
                          disableScrollFiltering:NO];

  XCTAssertTrue(CGSizeEqualToSize(self.owner.scrollView.contentSize, CGSizeMake(400, 65)));
  XCTAssertTrue(CGPointEqualToPoint(result, CGPointMake(90, 7)));
  XCTAssertTrue(CGPointEqualToPoint(self.owner.scrollView.contentOffset, CGPointMake(210, 7)));
  XCTAssertEqual(self.owner.scrollView.adjustingContentOffsetCount, 2);
}

- (void)testApplyContentSizeFiltersUnchangedOffset {
  self.owner.scrollView.contentSize = CGSizeMake(100, 400);
  self.owner.scrollView.contentOffset = CGPointMake(0, 20);

  CGPoint result = [self.helper applyContentSize:400
                                     offsetDelta:CGPointZero
                           previousContentOffset:CGPointMake(30, 40)
                          disableScrollFiltering:NO];

  XCTAssertTrue(CGPointEqualToPoint(result, CGPointMake(30, 40)));
  XCTAssertTrue(CGPointEqualToPoint(self.owner.scrollView.contentOffset, CGPointMake(0, 20)));
  XCTAssertEqual(self.owner.scrollView.adjustingContentOffsetCount, 0);
}

- (void)testHorizontalRTLGeometry {
  self.owner.vertical = NO;
  self.owner.RTL = YES;
  self.owner.scrollView.contentOffset = CGPointMake(50, 7);

  XCTAssertEqualWithAccuracy(self.helper.normalizedContentOffset.x, 150, 0.001);
  XCTAssertEqualWithAccuracy([self.helper contentOffsetXForRTL:50], 150, 0.001);
  XCTAssertEqualWithAccuracy([self.helper clampedContentOffsetForVerticalAxis:NO], 150, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationMaxScrollableDistance, 200, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationSize, 110, 0.001);
  XCTAssertEqualWithAccuracy(self.helper.orientationContentSize, 300, 0.001);
}

- (void)testAvailableOffsetsOnlyUseListItemWrappers {
  self.owner.listPadding = UIEdgeInsetsMake(5, 0, 10, 0);
  [self.owner.scrollView addSubview:[[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 400)]];
  [self.owner.scrollView
      addSubview:[[LynxListScrollHelperTestWrapper alloc] initWithFrame:CGRectMake(0, 5, 100, 40)]];
  [self.owner.scrollView addSubview:[[LynxListScrollHelperTestWrapper alloc]
                                        initWithFrame:CGRectMake(0, 340, 100, 50)]];

  XCTAssertEqualWithAccuracy(
      [self.helper availableScrollOffsetFromSubviewsForward:YES currentOffset:30], 320, 0.001);
  XCTAssertEqualWithAccuracy(
      [self.helper availableScrollOffsetFromSubviewsForward:NO currentOffset:30], 0, 0.001);
}

- (void)testScrollingUpdateOnlyStoresEstimatedOffset {
  [self.helper updateScrollInfoWithEstimatedOffset:120 smooth:NO scrolling:YES];

  XCTAssertEqualWithAccuracy(self.owner.scrollView.scrollEstimatedOffset, 120, 0.001);
  XCTAssertTrue(CGPointEqualToPoint(self.owner.scrollView.contentOffset, CGPointZero));
  XCTAssertEqual(self.owner.finishCount, 0);
  XCTAssertTrue([self.helper stopProgrammaticScroll]);
  XCTAssertEqual(self.owner.stopCount, 1);
}

- (void)testImmediateProgrammaticScrollPreservesImmediateCompletionPath {
  self.owner.scrollView.contentOffset = CGPointMake(0, 20);

  [self.helper updateScrollInfoWithEstimatedOffset:100 smooth:NO scrolling:NO];

  XCTAssertTrue(self.owner.scrollView.scrollToLower);
  XCTAssertEqualWithAccuracy(self.owner.scrollView.contentOffset.y, 100, 0.001);
  XCTAssertEqualWithAccuracy(self.owner.scrollView.scrollEstimatedOffset,
                             LynxListScrollInvalidEstimatedOffset, 0.001);
  XCTAssertEqual(self.owner.startAnimationCount, 0);
  XCTAssertEqual(self.owner.stopCount, 0);
  XCTAssertEqual(self.owner.finishCount, 1);
  XCTAssertEqual(self.owner.lastCompletionReason,
                 LynxListProgrammaticScrollCompletionReasonImmediate);
}

- (void)testDeferredProgrammaticScrollFinishesWithAnimationCallback {
  self.owner.deferCompletion = YES;
  self.owner.completionToken = [[NSObject alloc] init];
  self.owner.scrollView.contentOffset = CGPointMake(0, 20);

  [self.helper updateScrollInfoWithEstimatedOffset:100 smooth:YES scrolling:NO];

  XCTAssertEqual(self.owner.startAnimationCount, 1);
  XCTAssertEqualWithAccuracy(self.owner.scrollView.scrollEstimatedOffset, 100, 0.001);
  XCTAssertEqual(self.owner.finishCount, 0);

  [self.helper finishProgrammaticScrollRequest];

  XCTAssertEqual(self.owner.stopCount, 1);
  XCTAssertEqual(self.owner.finishCount, 1);
  XCTAssertEqual(self.owner.lastCompletionReason,
                 LynxListProgrammaticScrollCompletionReasonAnimationEnded);
}

- (void)testInvalidatedRequestDoesNotRunTimeoutFallback {
  self.owner.deferCompletion = YES;
  self.owner.completionToken = [[NSObject alloc] init];
  self.owner.scrollView.contentOffset = CGPointMake(0, 40);
  XCTestExpectation *expectation = [self expectationWithDescription:@"invalidated timeout"];
  expectation.inverted = YES;
  self.owner.finishHandler = ^{
    [expectation fulfill];
  };

  [self.helper updateScrollInfoWithEstimatedOffset:40 smooth:YES scrolling:NO];
  [self.helper invalidateProgrammaticScrollRequest];
  self.owner.completionToken = [[NSObject alloc] init];
  [self waitForExpectations:@[ expectation ] timeout:0.8];

  XCTAssertEqual(self.owner.stopCount, 0);
  XCTAssertEqual(self.owner.finishCount, 0);
  XCTAssertTrue([self.helper stopProgrammaticScroll]);
}

- (void)testDeferredNoOpRequestUsesTimeoutFallback {
  self.owner.deferCompletion = YES;
  self.owner.completionToken = [[NSObject alloc] init];
  self.owner.scrollView.contentOffset = CGPointMake(0, 40);
  XCTestExpectation *expectation = [self expectationWithDescription:@"programmatic scroll timeout"];
  self.owner.finishHandler = ^{
    [expectation fulfill];
  };

  [self.helper updateScrollInfoWithEstimatedOffset:40 smooth:YES scrolling:NO];
  [self waitForExpectations:@[ expectation ] timeout:1.5];

  XCTAssertEqual(self.owner.startAnimationCount, 0);
  XCTAssertEqual(self.owner.stopCount, 1);
  XCTAssertEqual(self.owner.finishCount, 1);
  XCTAssertEqual(self.owner.lastCompletionReason,
                 LynxListProgrammaticScrollCompletionReasonTimeout);
}

- (void)testTimeoutCanFinishPendingRequestWithoutCompletionToken {
  self.owner.deferCompletion = YES;
  self.owner.finishTimeoutForPendingScroll = YES;
  XCTestExpectation *expectation = [self expectationWithDescription:@"pending scroll timeout"];
  self.owner.finishHandler = ^{
    [expectation fulfill];
  };

  [self.helper updateScrollInfoWithEstimatedOffset:100 smooth:YES scrolling:NO];
  [self waitForExpectations:@[ expectation ] timeout:1.5];

  XCTAssertEqual(self.owner.startAnimationCount, 1);
  XCTAssertEqual(self.owner.stopCount, 1);
  XCTAssertEqual(self.owner.finishCount, 1);
  XCTAssertEqual(self.owner.lastCompletionReason,
                 LynxListProgrammaticScrollCompletionReasonTimeout);
}

@end
