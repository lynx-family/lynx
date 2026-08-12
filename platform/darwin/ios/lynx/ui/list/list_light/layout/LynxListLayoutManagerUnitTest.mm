// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import <Lynx/LynxListHorizontalLayoutManager.h>
#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListVerticalLayoutManager.h>
#import <Lynx/LynxUIListProtocol.h>

@interface LynxListLayoutManagerUnitTest : XCTestCase

@property(nonatomic, assign) CGSize defaultSize;

@end

@implementation LynxListLayoutManagerUnitTest

- (void)setUp {
  self.defaultSize = CGSizeMake(390, 844);
}

- (void)prepareLayoutManager:(LynxListLayoutManager *)layoutManager {
  LynxUIListInvalidationContext *generalPropsContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  generalPropsContext.numberOfColumns = 2;
  generalPropsContext.layoutType = LynxListLayoutWaterfall;
  [layoutManager updateBasicInvalidationContext:generalPropsContext
                                         bounds:(CGRect){{0, 0}, self.defaultSize}];

  NSArray<NSNumber *> *insertions = @[ @(0), @(1), @(2), @(3), @(4) ];
  [layoutManager updateModelsWithInsertions:insertions];
  NSMutableDictionary<NSNumber *, NSValue *> *modelUpdates = [NSMutableDictionary dictionary];
  for (NSNumber *index in insertions) {
    modelUpdates[index] = [NSValue valueWithCGRect:(CGRect){{0, 0}, self.defaultSize}];
  }
  [layoutManager updateModels:modelUpdates];
}

- (void)applyFullSpanItemToLayoutManager:(LynxListLayoutManager *)layoutManager {
  LynxUIListInvalidationContext *fullSpanItemUpdateContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  fullSpanItemUpdateContext.fullSpanItems = @[ @(0) ];
  [layoutManager updateBasicInvalidationContext:fullSpanItemUpdateContext
                                         bounds:(CGRect){{0, 0}, self.defaultSize}];
}

- (void)testVerticalLayout {
  LynxListVerticalLayoutManager *verticalLayoutManger =
      [[LynxListVerticalLayoutManager alloc] init];
  XCTAssertTrue([verticalLayoutManger isVerticalLayout]);
  [self prepareLayoutManager:verticalLayoutManger];

  // =============== test waterfall ===================
  [verticalLayoutManger layoutFrom:0 to:5];
  LynxListLayoutModelLight *model3 = [verticalLayoutManger attributesFromIndex:3];
  XCTAssertTrue(CGRectEqualToRect(model3.frame,
                                  CGRectMake(self.defaultSize.width / 2, self.defaultSize.height,
                                             self.defaultSize.width / 2, self.defaultSize.height)));

  // =============== test fullspan ===================
  [self applyFullSpanItemToLayoutManager:verticalLayoutManger];
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
  XCTAssertFalse([horizontalLayoutManager isVerticalLayout]);
  [self prepareLayoutManager:horizontalLayoutManager];
  [horizontalLayoutManager layoutFrom:0 to:5];

  // ================ test waterfall ===============
  LynxListLayoutModelLight *model3 = [horizontalLayoutManager attributesFromIndex:3];
  XCTAssertTrue(CGRectEqualToRect(model3.frame,
                                  CGRectMake(self.defaultSize.width, self.defaultSize.height / 2,
                                             self.defaultSize.width, self.defaultSize.height / 2)));

  // =============== test fullspan ===================
  [self applyFullSpanItemToLayoutManager:horizontalLayoutManager];
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
  XCTAssertEqual((NSInteger)topCells.count, 1);
  XCTAssertEqual(topCells[@(0)].integerValue, 0);
}

@end
