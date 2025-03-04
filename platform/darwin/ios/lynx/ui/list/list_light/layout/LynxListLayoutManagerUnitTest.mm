// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <Lynx/LynxListHorizontalLayoutManager.h>
#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListVerticalLayoutManager.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUIListProtocol.h>

@interface LynxListLayoutManagerUnitTest : XCTestCase

@property(nonatomic, assign) CGSize defaultSize;

@end

@implementation LynxListLayoutManagerUnitTest

- (void)setUp {
  self.defaultSize = UIScreen.mainScreen.bounds.size;
}

- (void)testVerticalLayout {
  LynxListVerticalLayoutManager *verticalLayoutManger =
      [[LynxListVerticalLayoutManager alloc] init];
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  view.frame = CGRectMake(0, 0, self.defaultSize.width, self.defaultSize.height);
  [view setLayout:verticalLayoutManger];
  XCTAssertTrue([verticalLayoutManger isVerticalLayout]);

  LynxUIListInvalidationContext *generalPropsContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  generalPropsContext.numberOfColumns = 2;
  generalPropsContext.layoutType = LynxListLayoutWaterfall;
  [view dispatchInvalidationContext:generalPropsContext];

  LynxUIListInvalidationContext *dataUpdate = [[LynxUIListInvalidationContext alloc] init];
  dataUpdate.insertions = @[ @(0), @(1), @(2), @(3), @(4) ];
  dataUpdate.listUpdateType = LynxListUpdateTypeDataUpdate;
  [view dispatchInvalidationContext:dataUpdate];

  // =============== test waterfall ===================
  [verticalLayoutManger layoutFrom:0 to:5];
  LynxListLayoutModelLight *model3 = [verticalLayoutManger attributesFromIndex:3];
  NSLog(@"[testVerticalLayout] layout %@", verticalLayoutManger.models);
  XCTAssertTrue(CGRectEqualToRect(model3.frame,
                                  CGRectMake(self.defaultSize.width / 2, self.defaultSize.height,
                                             self.defaultSize.width / 2, self.defaultSize.height)));

  // =============== test fullspan ===================
  LynxUIListInvalidationContext *fullSpanItemUpdateContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  fullSpanItemUpdateContext.fullSpanItems = @[ @(0) ];
  [view dispatchInvalidationContext:fullSpanItemUpdateContext];
  [verticalLayoutManger layoutFrom:0 to:5];
  LynxListLayoutModelLight *model0 = [verticalLayoutManger attributesFromIndex:0];
  XCTAssertTrue(CGRectEqualToRect(
      model0.frame, CGRectMake(0, 0, self.defaultSize.width, self.defaultSize.height)));
  LynxListLayoutModelLight *model1 = [verticalLayoutManger attributesFromIndex:1];
  XCTAssertTrue(CGRectEqualToRect(
      model1.frame,
      CGRectMake(0, self.defaultSize.height, self.defaultSize.width / 2, self.defaultSize.height)));

  // ================ test top cells ==================
  NSDictionary<NSNumber *, NSNumber *> *topCells =
      [verticalLayoutManger findWhichItemToDisplayOnTop];
  NSLog(@"topCells: %@, layout.columnInfo: %@, contentOffset: %@", topCells,
        verticalLayoutManger.layoutColumnInfo, NSStringFromCGPoint(view.contentOffset));
  XCTAssertEqual((NSInteger)topCells.count, 1);
  XCTAssertEqual(topCells[@(0)].integerValue, 0);
}

- (void)testLayoutSuperClass {
  LynxListLayoutManager *layoutManager = [[LynxListLayoutManager alloc] init];
  XCTAssertTrue([layoutManager isVerticalLayout]);
  XCTAssertTrue(CGSizeEqualToSize([layoutManager getContentSize], CGSizeZero));
}

- (void)testHorizontalLayout {
  LynxListHorizontalLayoutManager *horizontalLayoutManager =
      [[LynxListHorizontalLayoutManager alloc] init];
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  view.frame = CGRectMake(0, 0, self.defaultSize.width, self.defaultSize.height);
  [view setLayout:horizontalLayoutManager];
  XCTAssertFalse([horizontalLayoutManager isVerticalLayout]);

  LynxUIListInvalidationContext *generalPropsContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  generalPropsContext.numberOfColumns = 2;
  generalPropsContext.layoutType = LynxListLayoutWaterfall;
  [view dispatchInvalidationContext:generalPropsContext];

  LynxUIListInvalidationContext *dataUpdate = [[LynxUIListInvalidationContext alloc] init];
  dataUpdate.insertions = @[ @(0), @(1), @(2), @(3), @(4) ];
  dataUpdate.listUpdateType = LynxListUpdateTypeDataUpdate;
  [view dispatchInvalidationContext:dataUpdate];
  [horizontalLayoutManager layoutFrom:0 to:5];

  // ================ test waterfall ===============
  LynxListLayoutModelLight *model3 = [horizontalLayoutManager attributesFromIndex:3];
  XCTAssertTrue(CGRectEqualToRect(model3.frame,
                                  CGRectMake(self.defaultSize.width, self.defaultSize.height / 2,
                                             self.defaultSize.width, self.defaultSize.height / 2)));

  // =============== test fullspan ===================
  LynxUIListInvalidationContext *fullSpanItemUpdateContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  fullSpanItemUpdateContext.fullSpanItems = @[ @(0) ];
  [view dispatchInvalidationContext:fullSpanItemUpdateContext];
  [horizontalLayoutManager layoutFrom:0 to:5];
  LynxListLayoutModelLight *model0 = [horizontalLayoutManager attributesFromIndex:0];
  XCTAssertTrue(CGRectEqualToRect(
      model0.frame, CGRectMake(0, 0, self.defaultSize.width, self.defaultSize.height)));
  LynxListLayoutModelLight *model1 = [horizontalLayoutManager attributesFromIndex:1];
  XCTAssertTrue(CGRectEqualToRect(
      model1.frame,
      CGRectMake(self.defaultSize.width, 0, self.defaultSize.width, self.defaultSize.height / 2)));

  // ================ test top cells ==================
  NSDictionary<NSNumber *, NSNumber *> *topCells =
      [horizontalLayoutManager findWhichItemToDisplayOnTop];
  NSLog(@"topCells: %@, layout.columnInfo: %@, contentOffset: %@", topCells,
        horizontalLayoutManager.layoutColumnInfo, NSStringFromCGPoint(view.contentOffset));
  XCTAssertEqual((NSInteger)topCells.count, 1);
  XCTAssertEqual(topCells[@(0)].integerValue, 0);
}

@end
