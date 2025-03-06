// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxUIScroller.h>
#import <XCTest/XCTest.h>
#import "LynxUIScrollerUnitTestUtils.h"
#import "LynxUIUnitTestUtils.h"

@interface LynxUIScrollerScrollEventUnitTest : XCTestCase
@end

@implementation LynxUIScrollerScrollEventUnitTest
- (void)testBindScroll {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scroll" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @NO}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(100, 0);
  XCTAssertNotNil(mockContext.mockEventEmitter.event);
}

- (void)testBindScrollToUpperWithoutThreshold {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @NO}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(100, 0);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 0);
  XCTAssertNotNil(mockContext.mockEventEmitter.event);
}

- (void)testBindScrollToUpperThresholdNormal {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"upper-threshold" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(200, 0);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(150, 0);
  // Before reaching upperThreshold, scrolltoupper event should be sent
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the first time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(100, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
  // Trigger the second time
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(150, 0);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(90, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToUpperThresholdNegative {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"upper-threshold" : @-100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(100, 0);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 0);
  // Before reaching upperThreshold, scrolltoupper event should be sent
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the first time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(-100, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
  // Trigger the second time
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(150, 0);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(-160, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToUpperThresholdExtremelyLargeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"upper-threshold" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.width -
      scroller.view.frame.size.width;
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToLowerWithoutThreshold {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @NO}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  // Set offset to the end. childCount * childWidth - self.width
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.width -
      scroller.view.frame.size.width;
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdNormal {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"lower-threshold" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.width -
      scroller.view.frame.size.width;
  // Trigger the fist time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance - 100, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(200, 0);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the second time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance - 90, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdNegative {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"lower-threshold" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.width -
      scroller.view.frame.size.width;
  // Trigger the fist time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance + 100, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(200, 0);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the second time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(maxScrollDistance + 90, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdExtremelyLargeValue {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO, @"lower-threshold" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(1, 0);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToUpperWithoutThresholdScrollY {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 100);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 0);
  XCTAssertNotNil(mockContext.mockEventEmitter.event);
}

- (void)testBindScrollToUpperThresholdNormalScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"upper-threshold" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 200);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 150);
  // Before reaching upperThreshold, scrolltoupper event should be sent
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the first time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 100);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
  // Trigger the second time
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 150);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 90);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToUpperThresholdNegativeScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"upper-threshold" : @-100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 100);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 0);
  // Before reaching upperThreshold, scrolltoupper event should be sent
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the first time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, -100);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
  // Trigger the second time
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 150);
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, -160);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToUpperThresholdExtremelyLargeValueScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltoupper" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"upper-threshold" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  // Set offset to the end. childCount * childWidth - self.height
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 10 * 100.0f - 100.0f);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltoupper"]);
}

- (void)testBindScrollToLowerWithoutThresholdScrollY {
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:@{@"scroll-y" : @YES}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.height -
      scroller.view.frame.size.height;
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdNormalScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"lower-threshold" : @100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.height -
      scroller.view.frame.size.height;
  // Trigger the fist time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance - 100);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 200);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the second time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance - 90);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdNegativeScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"lower-threshold" : @-100}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.height -
      scroller.view.frame.size.height;
  // Trigger the fist time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance + 100);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
  [mockContext.mockEventEmitter clearEvent];
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, 200);
  XCTAssertNil(mockContext.mockEventEmitter.event);
  // Trigger the second time
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance + 90);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindScrollToLowerThresholdExtremelyLargeValueScrollY {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"scrolltolower" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @YES, @"lower-threshold" : @10000}];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [mockContext.mockEventEmitter clearEvent];
  CGFloat maxScrollDistance =
      scroller.children.count * scroller.children.lastObject.frame.size.height -
      scroller.view.frame.size.height;
  ((UIScrollView *)scroller.view).contentOffset = CGPointMake(0, maxScrollDistance);
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"scrolltolower"]);
}

- (void)testBindContentSizeChanged {
  LynxUIMockContext *mockContext = [LynxUIScrollerUnitTestUtils
      updateUIMockContext:nil
                     sign:0
                      tag:@"scroll-view"
                 eventSet:[NSSet setWithArray:@[ @"contentsizechanged" ]]
            lepusEventSet:[NSSet set]
                    props:@{@"scroll-y" : @NO}];
  XCTAssert(mockContext.mockEventEmitter.event &&
            [mockContext.mockEventEmitter.event.eventName isEqualToString:@"contentsizechanged"]);
}
@end
