// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <XCTest/XCTest.h>

@interface LynxBaseScrollViewPublicUnitTest : XCTestCase
@property(nonatomic, strong) LynxBaseScrollView *scrollView;
@end

@implementation LynxBaseScrollViewPublicUnitTest

- (void)setUp {
  _scrollView = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
}

- (void)tearDown {
  _scrollView = nil;
}

- (void)testEnableScroll {
  [_scrollView enableScroll:NO];
  XCTAssertFalse(_scrollView.scrollEnabled);
  [_scrollView enableScroll:YES];
  XCTAssertTrue(_scrollView.scrollEnabled);
}

- (void)testStopScrolling {
  _scrollView.contentSize = CGSizeMake(200, 200);
  _scrollView.contentOffset = CGPointMake(50, 50);
  [_scrollView stopScrolling];
  XCTAssertEqual(_scrollView.contentOffset.x, 50);
  XCTAssertEqual(_scrollView.contentOffset.y, 50);
  XCTAssertEqual([_scrollView currentScrollState], LynxBaseScrollViewScrollStateIdle);
}

- (void)testCurrentScrollState {
  _scrollView.scrollState = LynxBaseScrollViewScrollStateDragging;
  XCTAssertEqual([_scrollView currentScrollState], LynxBaseScrollViewScrollStateDragging);
}

@end
