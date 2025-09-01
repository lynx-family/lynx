// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Vertical.h>
#import <Lynx/UIScrollView+Lynx.h>
#import <XCTest/XCTest.h>

@interface LynxBaseScrollViewVerticalUnitTest : XCTestCase
@property(nonatomic, strong) LynxBaseScrollView *scrollView;
@end

@implementation LynxBaseScrollViewVerticalUnitTest

- (void)setUp {
  _scrollView = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  _scrollView.vertical = YES;
  _scrollView.contentSize = CGSizeMake(100, 200);
}

- (void)tearDown {
  _scrollView = nil;
}

- (void)testScrollToVertically {
  [_scrollView scrollToVertically:50];
  XCTAssertEqual(_scrollView.contentOffset.y, 50);
}

- (void)testScrollToVerticallyWithBounces {
  [_scrollView scrollToVertically:150];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.y, 100, 0.001);
}

- (void)testScrollToUnlimitedVertically {
  [_scrollView scrollToUnlimitedVertically:150];
  XCTAssertEqual(_scrollView.contentOffset.y, 150);
}

- (void)testScrollByVertically {
  [_scrollView scrollByVertically:50];
  XCTAssertEqual(_scrollView.contentOffset.y, 50);
}

- (void)testScrollByVerticallyWithBounces {
  [_scrollView scrollByVertically:150];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.y, 100, 0.001);
}

- (void)testScrollByUnlimitedVertically {
  [_scrollView scrollByUnlimitedVertically:150];
  XCTAssertEqual(_scrollView.contentOffset.y, 150);
}

- (void)testGetScrollOffsetVertically {
  _scrollView.contentOffset = CGPointMake(0, 50);
  XCTAssertEqual([_scrollView getScrollOffsetVertically], 50);
}

- (void)testSetScrollContentSizeVertically {
  [_scrollView setScrollContentSizeVertically:300];
  XCTAssertEqual(_scrollView.contentSize.height, 300);
}

- (void)testGetScrollRangeVertically {
  CGFloat range[2];
  [_scrollView getScrollRangeVertically:&range];
  XCTAssertEqual(range[0], 0);
  XCTAssertEqual(range[1], 100);
}

- (void)testCanScrollForwardsVertically {
  XCTAssertTrue([_scrollView canScrollForwardsVertically]);
  _scrollView.contentOffset = CGPointMake(0, 100);
  XCTAssertFalse([_scrollView canScrollForwardsVertically]);
}

- (void)testCanScrollBackwardsVertically {
  XCTAssertFalse([_scrollView canScrollBackwardsVertically]);
  _scrollView.contentOffset = CGPointMake(0, 50);
  XCTAssertTrue([_scrollView canScrollBackwardsVertically]);
}

@end
