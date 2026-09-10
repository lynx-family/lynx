// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUIListContainer.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxVersion.h>
#import <XCTest/XCTest.h>
#import "LynxListItemHelper.h"
#import "LynxListScrollHelper.h"
#import "LynxListStickyManager.h"
#import "LynxUI+Gesture.h"

@interface LynxUIListContainer (Testing)
@property(nonatomic, assign) CGFloat pagingAlignFactor;
@property(nonatomic, assign) CGFloat pagingAlignOffset;
@property(nonatomic, strong) LynxListItemHelper *itemHelper;
@property(nonatomic, strong) LynxListScrollHelper *scrollHelper;
@property(nonatomic, strong) LynxListStickyManager *stickyManager;
- (CGFloat)clampToValidScrollEdge:(BOOL)isVertical;
- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
@end

@interface LynxListUIContainerUnitTest : XCTestCase

@end
@implementation LynxListUIContainerUnitTest

- (void)setUp {
}

- (LynxUIListContainer *)setUpList {
  LynxUIListContainer *list = [[LynxUIListContainer alloc] init];
  [list updateFrame:UIScreen.mainScreen.bounds
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [list.view setContentSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width,
                                       UIScreen.mainScreen.bounds.size.height * 5)];
  return list;
}

- (void)testSnap {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.view);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(0),
    @"offset" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
  XCTAssertTrue(list.pagingAlignFactor == 0);
  XCTAssertTrue(list.pagingAlignOffset == 20);
  [LynxPropsProcessor updateProp:@{
    @"factor" : @(-1),
    @"align" : @(20),
  }
                         withKey:@"item-snap"
                           forUI:list];
}

- (void)testStickyPropertiesAreForwardedToManager {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.stickyManager);

  [LynxPropsProcessor updateProp:@YES withKey:@"sticky" forUI:list];
  [LynxPropsProcessor updateProp:@12.5 withKey:@"sticky-offset" forUI:list];

  XCTAssertTrue(list.stickyManager.enabled);
  XCTAssertEqualWithAccuracy(list.stickyManager.offset, 12.5, 0.001);
}

- (void)testItemOperationsAreForwardedToHelper {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.itemHelper);
  [LynxPropsProcessor updateProp:@{@"itemkeys" : @[ @"first", @"second" ]}
                         withKey:@"list-container-info"
                           forUI:list];

  LynxUIComponent *component = [[LynxUIComponent alloc] init];
  component.itemKey = @"second";
  [component updateFrame:CGRectMake(0, 20, 100, 40)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [list insertListComponent:component];

  XCTAssertEqual([list getIndexFromItemKey:@"second"], 1);
  XCTAssertEqualObjects([list.visibleCells valueForKeyPath:@"holdingUI.itemKey"], (@[ @"second" ]));
  XCTAssertTrue(CGRectEqualToRect(component.view.frame, CGRectMake(0, 0, 100, 40)));

  [list removeListComponent:component];
  XCTAssertEqual(list.visibleCells.count, 0U);
}

- (void)testScrollOperationsAreForwardedToHelper {
  LynxUIListContainer *list = [self setUpList];
  XCTAssertNotNil(list.scrollHelper);
  list.view.contentOffset = CGPointMake(0, UIScreen.mainScreen.bounds.size.height * 10);

  XCTAssertEqualWithAccuracy([list clampToValidScrollEdge:YES],
                             UIScreen.mainScreen.bounds.size.height * 4, 0.001);

  [list updateScrollInfoWithEstimatedOffset:100 smooth:NO scrolling:NO];

  XCTAssertEqualWithAccuracy(list.view.contentOffset.y, 100, 0.001);
}

@end
