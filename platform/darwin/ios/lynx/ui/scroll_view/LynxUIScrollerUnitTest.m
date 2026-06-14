// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBounceView.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIScroller.h>
#import <Lynx/LynxUIView.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>
#import "LynxUI+Gesture.h"
#import "LynxUIScrollerUnitTestUtils.h"

@interface LynxUIContext (NewStickyScrollerUnitTest)
- (void)setEnableNewSticky:(BOOL)enable;
@end

@interface LynxUIScroller (NewStickyUnitTest)
@property(nonatomic, strong) NSMutableArray<NSNumber *> *stickyChildSignArray;
@end

@interface LynxUIScrollerUnitTest : XCTestCase
@end

@implementation LynxUIScrollerUnitTest

- (void)testScrollX {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil(mockContext.mockUI.view);
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-x" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:NO
                                       size:CGSizeMake(100, 100)];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentSize.width, 1000.0f);
  XCTAssertEqual(scrollView.contentSize.height, 100.0f);
}

- (void)testScrollY {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil(mockContext.mockUI.view);
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-y" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:YES
                                       size:CGSizeMake(100, 100)];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentSize.width, 428.0f);
  XCTAssertEqual(scrollView.contentSize.height, 1000.0f);
}

- (void)testDoubleSideBounceViewScrollX {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil((UIScrollView *)mockContext.mockUI.view);

  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-x" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];

  [self subTestDoubleSideBounceViewScrollX:mockContext];
}

- (void)testDoubleSideBounceViewScrollXRTL {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil((UIScrollView *)mockContext.mockUI.view);
  mockContext.mockUI.directionType = LynxDirectionRtl;

  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-x" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];

  [self subTestDoubleSideBounceViewScrollX:mockContext];
}

- (void)subTestDoubleSideBounceViewScrollX:(LynxUIMockContext *)mockContext {
  NSInteger childCount = 10;
  LynxUIScroller *scroller = (LynxUIScroller *)mockContext.mockUI;
  [LynxUIScrollerUnitTestUtils mockChildren:childCount
                                    context:mockContext
                                    scrollY:NO
                                       size:CGSizeMake(100.0f, 100.0f)];
  [LynxUIScrollerUnitTestUtils mockBounceView:mockContext
                                    direction:@"left"
                        triggerBounceDistance:0.0f
                                         size:CGSizeMake(100.0f, 100.0f)];
  [LynxUIScrollerUnitTestUtils mockBounceView:mockContext
                                    direction:@"right"
                        triggerBounceDistance:0.0f
                                         size:CGSizeMake(100.0f, 100.0f)];
  if (scroller.lowerBounceUI.direction == LynxBounceViewDirectionRight) {
    XCTAssertEqual(scroller.lowerBounceUI.view.frame.origin.x, 1000.0f);
  }
  if (scroller.upperBounceUI.direction == LynxBounceViewDirectionLeft) {
    XCTAssertEqual(scroller.upperBounceUI.view.frame.origin.x, -100.0f);
  }
}

- (void)testDoubleSideBounceViewScrollY {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil(mockContext.mockUI.view);
  NSInteger childCount = 10;
  LynxUIScroller *scroller = (LynxUIScroller *)mockContext.mockUI;
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-y" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];

  [LynxUIScrollerUnitTestUtils mockChildren:childCount
                                    context:mockContext
                                    scrollY:YES
                                       size:CGSizeMake(100.0f, 100.0f)];
  [LynxUIScrollerUnitTestUtils mockBounceView:mockContext
                                    direction:@"top"
                        triggerBounceDistance:0.0f
                                         size:CGSizeMake(100.0f, 100.0f)];
  [LynxUIScrollerUnitTestUtils mockBounceView:mockContext
                                    direction:@"bottom"
                        triggerBounceDistance:0.0f
                                         size:CGSizeMake(100.0f, 100.0f)];

  if (scroller.lowerBounceUI.direction == LynxBounceViewDirectionBottom) {
    XCTAssertEqual(scroller.lowerBounceUI.view.frame.origin.y, 1000.0f);
  }
  if (scroller.upperBounceUI.direction == LynxBounceViewDirectionTop) {
    XCTAssertEqual(scroller.upperBounceUI.view.frame.origin.y, -100.0f);
  }
}

- (void)testScrollToBounces {
  // testcases:
  //@[
  //   direction,
  //   scrollToPosition,
  //   triggerBounceDistance,
  //   correct @"bounceDistance" result
  //]
  NSArray<NSArray *> *testCases = @[
    @[
      @"left", NSStringFromCGPoint(CGPointMake(-10, 0)), [NSNumber numberWithFloat:0.0f],
      [NSNumber numberWithFloat:-10.0f]
    ],
    @[
      @"right", NSStringFromCGPoint(CGPointMake(10 * 100.0f - 428.0 + 10, 0)),
      [NSNumber numberWithFloat:0.0f], [NSNumber numberWithFloat:10.0f]
    ],
    @[
      @"top", NSStringFromCGPoint(CGPointMake(0, -10.0)), [NSNumber numberWithFloat:0.0f],
      [NSNumber numberWithFloat:-10.0f]
    ],
    @[
      @"bottom", NSStringFromCGPoint(CGPointMake(0, 10 * 100.0f - 100.0f + 10)),
      [NSNumber numberWithFloat:0.0f], [NSNumber numberWithFloat:10.0f]
    ],
  ];

  for (NSInteger i = 0; i < (NSInteger)testCases.count; i++) {
    NSString *direction = [[testCases objectAtIndex:i] objectAtIndex:0];
    CGPoint position = CGPointFromString([[testCases objectAtIndex:i] objectAtIndex:1]);
    CGFloat distance = ((NSNumber *)[[testCases objectAtIndex:i] objectAtIndex:2]).floatValue;
    CGFloat correctDistance =
        ((NSNumber *)[[testCases objectAtIndex:i] objectAtIndex:3]).floatValue;

    [self scrollToBouncesSubTests:direction
                   scrollPosition:position
            triggerBounceDistance:distance
                  correctDistance:correctDistance];
  }
}

- (void)scrollToBouncesSubTests:(NSString *)direction
                 scrollPosition:(CGPoint)position
          triggerBounceDistance:(CGFloat)distance
                correctDistance:(CGFloat)correctDistance {
  NSInteger childCount = 10;
  LynxUIMockContext *mockContext =
      [LynxUIScrollerUnitTestUtils updateUIMockContext:nil
                                                  sign:0
                                                   tag:@"scroll-view"
                                              eventSet:[NSSet setWithArray:@[ @"scrolltobounce" ]]
                                         lepusEventSet:[NSSet set]
                                                 props:[NSDictionary dictionary]];
  LynxUIScroller *scroller = (LynxUIScroller *)[mockContext.UIOwner findUIBySign:0];
  [scroller updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertNotNil(mockContext.mockUI.view);

  if ([direction isEqualToString:@"left"] || [direction isEqualToString:@"right"]) {
    [LynxPropsProcessor updateProp:@1 withKey:@"scroll-x" forUI:mockContext.mockUI];
    [mockContext.mockUI propsDidUpdate];
    [LynxUIScrollerUnitTestUtils mockChildren:childCount
                                      context:mockContext
                                      scrollY:NO
                                         size:CGSizeMake(100.0f, 100.0f)];
  } else {
    [LynxPropsProcessor updateProp:@1 withKey:@"scroll-y" forUI:mockContext.mockUI];
    [mockContext.mockUI propsDidUpdate];
    [LynxUIScrollerUnitTestUtils mockChildren:childCount
                                      context:mockContext
                                      scrollY:YES
                                         size:CGSizeMake(100.0f, 100.0f)];
  }

  [LynxUIScrollerUnitTestUtils mockBounceView:mockContext
                                    direction:direction
                        triggerBounceDistance:distance
                                         size:CGSizeMake(100.0f, 100.0f)];
  [mockContext.mockUI.view setContentOffset:position];

  XCTAssertNotNil(
      ((LynxEventEmitterUnitTestHelper *)mockContext.mockUI.context.eventEmitter).event);
  LynxCustomEvent *event =
      ((LynxEventEmitterUnitTestHelper *)mockContext.mockUI.context.eventEmitter).event;
  NSDictionary *detail = event.params;

  XCTAssert(
      [detail[@"bounceDistance"] isEqualToNumber:[NSNumber numberWithInteger:correctDistance]]);
  XCTAssert([detail[@"triggerDistance"] isEqualToNumber:[NSNumber numberWithInteger:distance]]);
  XCTAssert([detail[@"direction"] isEqualToString:direction]);
}

- (void)testGestureInterfaces {
  LynxUIMockContext *mockContext =
      [LynxUIUnitTestUtils initUIMockContextWithUI:[[LynxUIScroller alloc] init]];
  [mockContext.mockUI updateFrame:CGRectMake(0, 0, 428.0f, 100.0f)
                      withPadding:UIEdgeInsetsZero
                           border:UIEdgeInsetsZero
              withLayoutAnimation:NO];
  XCTAssertNotNil(mockContext.mockUI.view);
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-y" forUI:mockContext.mockUI];
  [mockContext.mockUI propsDidUpdate];
  [LynxUIScrollerUnitTestUtils mockChildren:10
                                    context:mockContext
                                    scrollY:YES
                                       size:CGSizeMake(100, 100)];
  UIScrollView *scrollView = (UIScrollView *)mockContext.mockUI.view;
  XCTAssertEqual(scrollView.contentSize.width, 428.0f);
  XCTAssertEqual(scrollView.contentSize.height, 1000.0f);
  XCTAssertTrue(scrollView.contentOffset.y == 0.0f);

  XCTAssertFalse([mockContext.mockUI canConsumeGesture:CGPointMake(10, -1)]);
  [mockContext.mockUI onGestureScrollBy:CGPointMake(10, 10)];
  XCTAssertTrue(scrollView.contentOffset.y == 10.f);
  XCTAssertTrue(scrollView.contentOffset.x == 0);
  XCTAssertTrue([mockContext.mockUI canConsumeGesture:CGPointMake(10, 10)]);
  XCTAssertTrue([mockContext.mockUI getMemberScrollX] == 0.0f);
  XCTAssertTrue([mockContext.mockUI getMemberScrollY] == 10.0f);

  [mockContext.mockUI onGestureScrollBy:CGPointMake(10, 10000)];
  XCTAssertFalse([mockContext.mockUI canConsumeGesture:CGPointMake(10, 1)]);
}

// Focus: Direct add/remove operations deduplicate sticky signs and keep enableSticky in sync with
// the scroll-view's sticky sign array.
- (void)testNewStickyChildSignRegistrationIsDeduplicated {
  LynxUIScroller *scroller = [[LynxUIScroller alloc] init];

  [scroller addStickyChildSign:7];
  [scroller addStickyChildSign:7];

  [self assertStickyChildSigns:scroller expectedSigns:@[ @7 ]];

  [scroller removeStickyChildSign:7];

  [self assertStickyChildSigns:scroller expectedSigns:@[]];
}

// Focus: A valid new-sticky payload registers the sticky node sign on the nearest scroll-view, and
// refreshStickyChildren updates that node by looking it up from the recorded sign array.
- (void)testNewStickyRegistersWithScrollerAndRefreshesBySign {
  LynxUIMockContext *mockContext = nil;
  LynxUIScroller *scroller = [self createScrollerWithSign:0 mockContext:&mockContext];
  LynxUI *sticky = [self createStickyChildWithSign:1 mockContext:mockContext];
  [scroller insertChild:sticky atIndex:0];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:10]];
  [self assertStickyChildSigns:scroller expectedSigns:@[ @(sticky.sign) ]];

  UIScrollView *scrollView = (UIScrollView *)scroller.view;
  scrollView.contentOffset = CGPointMake(0, 150);
  [scroller refreshStickyChildren];

  XCTAssertEqualWithAccuracy(sticky.backgroundManager.postTranslate.y, 60.f, 0.001f);
}

// Focus: Updating sticky info keeps one recorded sign for the same node, while invalid or nil
// payloads clear the node from the scroll-view's sticky sign array.
- (void)testNewStickyUpdatesStickyChildSignsWhenInfoChanges {
  LynxUIMockContext *mockContext = nil;
  LynxUIScroller *scroller = [self createScrollerWithSign:0 mockContext:&mockContext];
  LynxUI *sticky = [self createStickyChildWithSign:1 mockContext:mockContext];
  [scroller insertChild:sticky atIndex:0];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:10]];
  [self assertStickyChildSigns:scroller expectedSigns:@[ @(sticky.sign) ]];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:20]];
  [self assertStickyChildSigns:scroller expectedSigns:@[ @(sticky.sign) ]];

  [sticky updateSticky:@[ @0, @20, @0, @0 ]];
  [self assertStickyChildSigns:scroller expectedSigns:@[]];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:30]];
  [self assertStickyChildSigns:scroller expectedSigns:@[ @(sticky.sign) ]];

  [sticky updateSticky:nil];
  [self assertStickyChildSigns:scroller expectedSigns:@[]];
}

// Focus: Removing a sticky node unregisters it from the scroll-view sticky sign array, so later
// sticky refreshes do not process the removed node.
- (void)testNewStickyUnregistersFromScrollerOnNodeRemoved {
  LynxUIMockContext *mockContext = nil;
  LynxUIScroller *scroller = [self createScrollerWithSign:0 mockContext:&mockContext];
  LynxUI *sticky = [self createStickyChildWithSign:1 mockContext:mockContext];
  [scroller insertChild:sticky atIndex:0];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:10]];
  [self assertStickyChildSigns:scroller expectedSigns:@[ @(sticky.sign) ]];

  UIScrollView *scrollView = (UIScrollView *)scroller.view;
  scrollView.contentOffset = CGPointMake(0, 150);
  [scroller refreshStickyChildren];
  XCTAssertEqualWithAccuracy(sticky.backgroundManager.postTranslate.y, 60.f, 0.001f);

  [sticky onNodeRemoved];
  [scroller removeChild:sticky atIndex:0];
  [self assertStickyChildSigns:scroller expectedSigns:@[]];

  scrollView.contentOffset = CGPointMake(0, 200);
  [scroller refreshStickyChildren];
  XCTAssertTrue(CGPointEqualToPoint(sticky.backgroundManager.postTranslate, CGPointZero));
}

// Focus: Moving a sticky node between two scroll-views removes its sign from the old scroll-view
// and records it on the new scroll-view after sticky info is updated.
- (void)testNewStickyMovesBetweenScrollersWhenStickyInfoUpdates {
  LynxUIMockContext *mockContext = nil;
  LynxUIScroller *firstScroller = [self createScrollerWithSign:1 mockContext:&mockContext];
  LynxUIScroller *secondScroller = [self createScrollerWithSign:3 mockContext:&mockContext];
  LynxUI *sticky = [self createStickyChildWithSign:2 mockContext:mockContext];
  [firstScroller insertChild:sticky atIndex:0];

  [sticky updateSticky:[self createVerticalStickyInfoWithTop:10]];
  [self assertStickyChildSigns:firstScroller expectedSigns:@[ @(sticky.sign) ]];
  [self assertStickyChildSigns:secondScroller expectedSigns:@[]];

  [firstScroller removeChild:sticky atIndex:0];
  [secondScroller insertChild:sticky atIndex:0];
  [sticky updateSticky:[self createVerticalStickyInfoWithTop:10]];

  [self assertStickyChildSigns:firstScroller expectedSigns:@[]];
  [self assertStickyChildSigns:secondScroller expectedSigns:@[ @(sticky.sign) ]];
}

- (LynxUIScroller *)createScrollerWithSign:(NSInteger)sign
                               mockContext:(LynxUIMockContext **)mockContext {
  *mockContext = [LynxUIUnitTestUtils updateUIMockContext:*mockContext
                                                     sign:sign
                                                      tag:@"scroll-view"
                                                 eventSet:[NSSet set]
                                            lepusEventSet:[NSSet set]
                                                    props:[NSDictionary dictionary]];
  LynxUIScroller *scroller = (LynxUIScroller *)[(*mockContext).UIOwner findUIBySign:sign];
  [scroller.context setEnableNewSticky:YES];
  [scroller updateFrame:CGRectMake(0, 0, 200, 200)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [LynxPropsProcessor updateProp:@1 withKey:@"scroll-y" forUI:scroller];
  [scroller propsDidUpdate];
  UIScrollView *scrollView = (UIScrollView *)scroller.view;
  scrollView.contentSize = CGSizeMake(200, 500);
  return scroller;
}

- (LynxUI *)createStickyChildWithSign:(NSInteger)sign mockContext:(LynxUIMockContext *)mockContext {
  [LynxUIUnitTestUtils updateUIMockContext:mockContext
                                      sign:sign
                                       tag:@"view"
                                  eventSet:[NSSet set]
                             lepusEventSet:[NSSet set]
                                     props:[NSDictionary dictionary]];
  LynxUI *sticky = [mockContext.UIOwner findUIBySign:sign];
  [sticky updateFrame:CGRectMake(0, 100, 50, 40)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  return sticky;
}

- (NSArray *)createVerticalStickyInfoWithTop:(CGFloat)top {
  return @[ @0, @(top), @0, @0, @(-1), @(-1), @0, @100, @0, @0 ];
}

- (void)assertStickyChildSigns:(LynxUIScroller *)scroller
                 expectedSigns:(NSArray<NSNumber *> *)expectedSigns {
  NSArray<NSNumber *> *stickyChildSigns = scroller.stickyChildSignArray ?: @[];
  XCTAssertEqualObjects(stickyChildSigns, expectedSigns);
  XCTAssertEqual(scroller.enableSticky, expectedSigns.count != 0);
}

@end
