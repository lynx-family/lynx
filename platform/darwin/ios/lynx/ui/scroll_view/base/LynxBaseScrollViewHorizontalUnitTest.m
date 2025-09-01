// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <XCTest/XCTest.h>

@interface LynxBaseScrollViewHorizontalUnitTest : XCTestCase
@property(nonatomic, strong) LynxBaseScrollView *scrollView;
@end

@implementation LynxBaseScrollViewHorizontalUnitTest

- (void)setUp {
  _scrollView = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  _scrollView.vertical = NO;
  _scrollView.contentSize = CGSizeMake(200, 100);
}

- (void)tearDown {
  _scrollView = nil;
}

- (void)testScrollToHorizontally {
  [_scrollView scrollToHorizontally:50];
  XCTAssertEqual(_scrollView.contentOffset.x, 50);
}

- (void)testScrollToHorizontallyWithBounces {
  [_scrollView scrollToHorizontally:150];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.x, 100, 0.001);
}

- (void)testScrollToUnlimitedHorizontally {
  [_scrollView scrollToUnlimitedHorizontally:150];
  XCTAssertEqual(_scrollView.contentOffset.x, 150);
}

- (void)testScrollByHorizontally {
  [_scrollView scrollByHorizontally:50];
  XCTAssertEqual(_scrollView.contentOffset.x, 50);
}

- (void)testScrollByHorizontallyWithBounces {
  [_scrollView scrollByHorizontally:150];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.x, 100, 0.001);
}

- (void)testScrollByUnlimitedHorizontally {
  [_scrollView scrollByUnlimitedHorizontally:150];
  XCTAssertEqual(_scrollView.contentOffset.x, 150);
}

- (void)testGetScrollOffsetHorizontally {
  _scrollView.contentOffset = CGPointMake(50, 0);
  XCTAssertEqual([_scrollView getScrollOffsetHorizontally], 50);
}

- (void)testSetScrollContentSizeHorizontally {
  [_scrollView setScrollContentSizeHorizontally:300];
  XCTAssertEqual(_scrollView.contentSize.width, 300);
}

- (void)testGetScrollRangeHorizontally {
  CGFloat range[2];
  [_scrollView getScrollRangeHorizontally:&range];
  XCTAssertEqual(range[0], 0);
  XCTAssertEqual(range[1], 100);
}

- (void)testCanScrollForwardsHorizontally {
  XCTAssertTrue([_scrollView canScrollForwardsHorizontally]);
  _scrollView.contentOffset = CGPointMake(100, 0);
  XCTAssertFalse([_scrollView canScrollForwardsHorizontally]);
}

- (void)testCanScrollBackwardsHorizontally {
  XCTAssertFalse([_scrollView canScrollBackwardsHorizontally]);
  _scrollView.contentOffset = CGPointMake(50, 0);
  XCTAssertTrue([_scrollView canScrollBackwardsHorizontally]);
}

@end
