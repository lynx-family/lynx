// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/UIScrollView+Lynx.h>

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxView+Internal.h>
#import <XCTest/XCTest.h>

@interface LynxBaseScrollViewUnitTest : XCTestCase
@property(nonatomic, strong) LynxBaseScrollView *scrollView;
@end

@implementation LynxBaseScrollViewUnitTest

- (void)setUp {
  _scrollView = [[LynxBaseScrollView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
}

- (void)tearDown {
  _scrollView = nil;
}

- (void)testInitialization {
  XCTAssertNotNil(_scrollView);
  XCTAssertEqual(_scrollView.vertical, NO);
  XCTAssertEqual(_scrollView.layoutFromEnd, NO);
  XCTAssertEqual(_scrollView.scrollState, LynxBaseScrollViewScrollStateIdle);
}

- (void)testInitializationWithVertical {
  LynxBaseScrollView *scrollView = [[LynxBaseScrollView alloc] initWithVertical:YES
                                                                  layoutFromEnd:NO];
  XCTAssertTrue(scrollView.vertical);
}

- (void)testTryToUpdateScrollState {
  [_scrollView tryToUpdateScrollState:LynxBaseScrollViewScrollStateDragging];
  XCTAssertEqual(_scrollView.scrollState, LynxBaseScrollViewScrollStateDragging);
}

- (void)testAutoScroll {
  XCTestExpectation *expectation = [self expectationWithDescription:@"Auto scroll completes"];
  _scrollView.contentSize = CGSizeMake(200, 200);
  [_scrollView autoScrollWithRate:100
                         interval:0.1
                         complete:^(BOOL completed) {
                           XCTAssertTrue(completed);
                           [expectation fulfill];
                         }];
  [self waitForExpectationsWithTimeout:1 handler:nil];
}

@end
