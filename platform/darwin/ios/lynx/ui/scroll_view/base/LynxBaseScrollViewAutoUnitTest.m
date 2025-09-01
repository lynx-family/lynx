// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <Lynx/LynxBaseScrollView+Vertical.h>
#import <XCTest/XCTest.h>

@interface LynxBaseScrollViewAutoUnitTest : XCTestCase
@property(nonatomic, strong) LynxBaseScrollView *scrollView;
@end

@implementation LynxBaseScrollViewAutoUnitTest

- (void)setUp {
  _scrollView = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  [_scrollView setVertical:YES];
  [_scrollView setScrollContentSize:CGSizeMake(200, 200)];
}

- (void)tearDown {
  _scrollView = nil;
}

- (void)testScrollTo {
  [_scrollView scrollTo:CGPointMake(50, 50)];
  XCTAssertEqual(_scrollView.contentOffset.x, 0);
  XCTAssertEqual(_scrollView.contentOffset.y, 50);
}

- (void)testScrollToWithBounces {
  [_scrollView scrollTo:CGPointMake(150, 150)];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.x, 0, 0.001);
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.y, 100, 0.001);
}

- (void)testScrollToUnlimited {
  [_scrollView scrollToUnlimited:CGPointMake(150, 150)];
  XCTAssertEqual(_scrollView.contentOffset.x, 0);
  XCTAssertEqual(_scrollView.contentOffset.y, 150);
}

- (void)testScrollBy {
  [_scrollView scrollBy:CGPointMake(50, 50)];
  XCTAssertEqual(_scrollView.contentOffset.x, 0);
  XCTAssertEqual(_scrollView.contentOffset.y, 50);
}

- (void)testScrollByWithBounces {
  [_scrollView scrollBy:CGPointMake(150, 150)];
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.x, 0, 0.001);
  XCTAssertEqualWithAccuracy(_scrollView.contentOffset.y, 100, 0.001);
}

- (void)testScrollByUnlimited {
  [_scrollView scrollByUnlimited:CGPointMake(150, 150)];
  XCTAssertEqual(_scrollView.contentOffset.x, 0);
  XCTAssertEqual(_scrollView.contentOffset.y, 150);
}

- (void)testGetScrollOffset {
  [_scrollView scrollTo:CGPointMake(50, 50)];
  CGPoint offset = [_scrollView getScrollOffset];
  XCTAssertEqual(offset.x, 0);
  XCTAssertEqual(offset.y, 50);
}

- (void)testSetScrollContentSize {
  [_scrollView setScrollContentSize:CGSizeMake(300, 300)];
  XCTAssertEqual(_scrollView.contentSize.width, 100);
  XCTAssertEqual(_scrollView.contentSize.height, 300);
}

- (void)testGetScrollRange {
  CGFloat range[4];
  [_scrollView getScrollRange:&range];
  XCTAssertEqual(range[0], 0);
  XCTAssertEqual(range[1], 0);
  XCTAssertEqual(range[2], 0);
  XCTAssertEqual(range[3], 100);
}

- (void)testCanScrollForwards {
  _scrollView.vertical = YES;
  XCTAssertTrue([_scrollView canScrollForwards]);
  _scrollView.contentOffset = CGPointMake(0, 100);
  XCTAssertFalse([_scrollView canScrollForwards]);
}

- (void)testCanScrollBackwards {
  _scrollView.vertical = YES;
  XCTAssertFalse([_scrollView canScrollBackwards]);
  _scrollView.contentOffset = CGPointMake(0, 50);
  XCTAssertTrue([_scrollView canScrollBackwards]);
}

@end
