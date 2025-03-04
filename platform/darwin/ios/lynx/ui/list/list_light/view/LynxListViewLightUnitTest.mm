// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#import <Lynx/LynxListAnchorManager.h>
#import <Lynx/LynxListCachedCellManager.h>
#import <Lynx/LynxListHorizontalLayoutManager.h>
#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListReusePool.h>
#import <Lynx/LynxListVerticalLayoutManager.h>
#import <Lynx/LynxListViewCellLight.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUIListCellContentProducer.h>
#import <Lynx/LynxUIListDataSource.h>
#import <Lynx/LynxUIListScrollManager.h>

@interface mockLynxUIListCellContentProducer : LynxUIListCellContentProducer
@property NSArray<NSValue *> *layoutModel;
@end

@implementation mockLynxUIListCellContentProducer

- (__kindof id<LynxListCell>)listView:(LynxListViewLight *)listView
                   cellForItemAtIndex:(NSInteger)index {
  id<LynxListCell> cell = [[LynxListViewCellLight alloc] init];
  cell.updateToPath = index;
  CGSize size = _layoutModel[index].CGSizeValue;
  UIView *childView = [[UIView alloc] initWithFrame:(CGRect){{-1, -1}, size}];
  childView.layer.borderColor = UIColor.blackColor.CGColor;
  childView.layer.borderWidth = 2;
  [cell.contentView addSubview:childView];
  cell.contentView.frame = childView.frame;
  cell.frame = childView.frame;
  [listView addSubview:(UIView *)cell];
  cell.itemKey = [[NSString alloc] initWithFormat:@"%ld", index];
  return cell;
}

@end

@interface mockLynxUIListDataSource : LynxUIListDataSource
@property(nonatomic) mockLynxUIListCellContentProducer *mockInternalDataSourceLight;
- (id<LynxListCell> _Nullable)listView:(LynxListViewLight *)listView
                    cellForItemAtIndex:(NSInteger)index;
@end

@implementation mockLynxUIListDataSource

- (instancetype)init {
  self = [super init];
  if (self) {
    _mockInternalDataSourceLight = [[mockLynxUIListCellContentProducer alloc] init];
  }
  return self;
}

- (id<LynxListCell> _Nullable)listView:(LynxListViewLight *)listView
                    cellForItemAtIndex:(NSInteger)index {
  return [_mockInternalDataSourceLight listView:listView cellForItemAtIndex:index];
}

@end

@interface mockLynxListLayoutManager : LynxListLayoutManager

@end

@implementation mockLynxListLayoutManager

@end

@interface LynxListViewLight (Test)
@property(nonatomic, strong) LynxListLayoutManager *innerLayout;
@property(nonatomic, weak) id<LynxListLayoutProtocol> customizedLayout;
@property(nonatomic, strong) LynxListAnchorManager *anchorManager;
@property(nonatomic, strong) LynxUIListScrollManager *scrollManager;
@property(nonatomic, weak) LynxUIListDataSource *dataSource;
@property(nonatomic, strong) LynxListCachedCellManager *cachedCells;
@property(nonatomic, strong) LynxListReusePool *reusePool;
- (id<LynxListLayoutProtocol>)layout;
- (id<LynxListCell>)findAnchorCellForRemoval;
- (id<LynxListCell>)getAnchorCellInIndex:(NSInteger)anchorIndex;
- (LynxAnchorPolicies)makeAnchorPolicies;
@end

@interface LynxListViewLightUnitTest : XCTestCase

@property(nonatomic, assign) CGSize defaultSize;

@end

@implementation LynxListViewLightUnitTest
- (void)setUp {
  self.defaultSize = CGSizeMake(390, 844);
}

- (void)testInit {
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  XCTAssertNotNil(view.anchorManager);
  XCTAssertNotNil(view.cachedCells);
  XCTAssertNotNil(view.reusePool);
  XCTAssertNotNil(view.scrollManager);
}

- (LynxListViewLight *)createView {
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  view.frame = (CGRect){{0, 0}, self.defaultSize};
  view.preloadBufferCount = 3;
  view.anchorPriorityFromBegin = YES;
  view.insertAnchorModeInside = NO;

  return view;
}

- (void)testCustomLayout {
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  mockLynxListLayoutManager *layout = [[mockLynxListLayoutManager alloc] init];
  [view setLayout:layout];
  XCTAssertNotNil([view layout]);
  XCTAssertNil(view.innerLayout);
  XCTAssertTrue([view.layout isKindOfClass:mockLynxListLayoutManager.class]);
}

- (void)testInsertion {
  LynxListVerticalLayoutManager *verticalLayoutManger =
      [[LynxListVerticalLayoutManager alloc] init];
  LynxListViewLight *view = [[LynxListViewLight alloc] init];
  view.numberOfColumns = 2;
  view.frame = (CGRect){{0, 0}, self.defaultSize};
  [view setLayout:verticalLayoutManger];
  mockLynxUIListDataSource *dataSource = [[mockLynxUIListDataSource alloc] init];
  [view setDataSource:dataSource];

  NSMutableArray<NSValue *> *layoutModel = [NSMutableArray array];
  for (int i = 0; i < 9; i++) {
    [layoutModel
        addObject:[NSValue
                      valueWithCGSize:CGSizeMake(UIScreen.mainScreen.bounds.size.width / 2, 400)]];
  }
  [dataSource.mockInternalDataSourceLight setLayoutModel:layoutModel];

  LynxUIListInvalidationContext *generalPropsContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  generalPropsContext.numberOfColumns = 2;
  generalPropsContext.layoutType = LynxListLayoutWaterfall;
  [view dispatchInvalidationContext:generalPropsContext];

  LynxUIListInvalidationContext *dataUpdate = [[LynxUIListInvalidationContext alloc] init];
  dataUpdate.insertions = @[ @(0), @(1), @(2), @(3), @(4), @(5), @(6), @(7), @(8) ];
  [view updateReuseIdentifiers:@[
    @"testCell", @"testCell", @"testCell", @"testCell", @"testCell", @"testCell", @"testCell",
    @"testCell", @"testCell"
  ]];
  dataUpdate.listUpdateType = LynxListUpdateTypeDataUpdate;
  [view dispatchInvalidationContext:dataUpdate];
  view.contentOffset = CGPointMake(0, 801);
  XCTAssertTrue([view.cachedCells firstVisibleCell].updateToPath == 4);
  XCTAssertTrue([view.cachedCells lastVisibleCell].updateToPath == 8);
  XCTAssertTrue(view.cachedCells.displayingCells.count == 5);
}

- (void)testRemoval {
  LynxListViewLight *view = [self createView];
  mockLynxUIListDataSource *dataSource = [[mockLynxUIListDataSource alloc] init];
  [view setDataSource:dataSource];
  LynxListVerticalLayoutManager *verticalLayoutManger =
      [[LynxListVerticalLayoutManager alloc] init];
  [view setLayout:verticalLayoutManger];
  view.numberOfColumns = 2;

  [self insertInitialObjects:view];

  view.contentOffset = CGPointMake(0, 360);
  [self checkCachedCells:view];
  id<LynxListCell> anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 3);
  view.deleteRegressPolicyToTop = YES;
  NSArray<NSNumber *> *removals = @[ @3 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 4);
  view.deleteRegressPolicyToTop = NO;
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 4);

  LynxUIListInvalidationContext *updateContext = [[LynxUIListInvalidationContext alloc] init];
  updateContext.removals = removals.copy;
  updateContext.listUpdateType = LynxListUpdateTypeDataUpdate;
  [view dispatchInvalidationContext:updateContext];
  [self checkCachedCells:view];

  // anchorPriorityFromEnd
  view.anchorPriorityFromBegin = NO;
  view.deleteRegressPolicyToTop = YES;
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 15);
  removals = @[ @15 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 14);
  view.deleteRegressPolicyToTop = NO;
  removals = @[ @14 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 12);

  view.contentOffset = CGPointMake(0, 20);
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 11);
  removals = @[ @11, @12, @13, @14, @15, @16, @17, @18 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 10);
}

- (void)testZeroPreloadBuffer {
  LynxListViewLight *view = [self createView];
  mockLynxUIListDataSource *dataSource = [[mockLynxUIListDataSource alloc] init];
  [view setDataSource:dataSource];
  LynxListVerticalLayoutManager *verticalLayoutManger =
      [[LynxListVerticalLayoutManager alloc] init];
  [view setLayout:verticalLayoutManger];
  view.numberOfColumns = 2;
  view.preloadBufferCount = 0;

  [self insertInitialObjects:view];

  [self checkCachedCells:view];
  view.insertAnchorModeInside = NO;
  id<LynxListCell> anchor = [self findAnchorCell:view];
  XCTAssertNil(anchor);
  view.insertAnchorModeInside = YES;
  anchor = [self findAnchorCell:view];
  XCTAssertEqual(anchor.updateToPath, 0);

  view.contentOffset = CGPointMake(0, 350);
  view.deleteRegressPolicyToTop = YES;
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 3);
  NSArray<NSNumber *> *removals = @[ @3 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 4);

  view.anchorPriorityFromBegin = NO;
  view.deleteRegressPolicyToTop = NO;
  removals = @[ @15 ];
  [removals
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [view.cachedCells markRemoveCellAtIndex:obj.integerValue];
      }];
  anchor = [self findAnchorCellForRemoval:view];
  XCTAssertEqual(anchor.updateToPath, 13);
}

#pragma mark utils

- (id<LynxListCell>)findAnchorCell:(LynxListViewLight *)view {
  NSInteger anchorIndex = [view.anchorManager findAnchorCell:view.cachedCells
                                              anchorPolicies:[view makeAnchorPolicies]
                                                  layoutInfo:view.layout.layoutColumnInfo];
  id<LynxListCell> cell = [view getAnchorCellInIndex:anchorIndex];
  return cell;
}

- (id<LynxListCell>)findAnchorCellForRemoval:(LynxListViewLight *)view {
  NSInteger anchorIndex = [view.anchorManager findAnchorCellForRemoval:view.cachedCells
                                                        anchorPolicies:[view makeAnchorPolicies]
                                                            layoutInfo:view.layout.layoutColumnInfo
                                                         deleteIndexes:nil];
  id<LynxListCell> cell = [view getAnchorCellInIndex:anchorIndex];
  return cell;
}

- (void)insertInitialObjects:(LynxListViewLight *)view {
  // Build special layoutModel set to test
  NSMutableArray<NSValue *> *layoutModel = [NSMutableArray array];
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 200)]];  // 0
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 100)]];  // 1
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 200)]];  // 2
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 200)]];  // 3
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 100)]];  // 4
  [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 200)]];  // 5
  for (int i = 3; i < 50; i++) {
    [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 100)]];
    [layoutModel addObject:[NSValue valueWithCGSize:CGSizeMake(100, 200)]];
    ;
  }
  [((mockLynxUIListDataSource *)view.dataSource).mockInternalDataSourceLight
      setLayoutModel:layoutModel];

  // adjust reuseIdentifiers
  NSMutableArray<NSString *> *reuseIdentifiers = [NSMutableArray array];
  [layoutModel
      enumerateObjectsUsingBlock:^(NSValue *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [reuseIdentifiers addObject:@(obj.CGSizeValue.height / 10).stringValue];
      }];
  [view updateReuseIdentifiers:reuseIdentifiers];
  [view registerCellClass:LynxListViewCellLightLynxUI.class reuseIdentifiers:reuseIdentifiers];
  // initialize list-items
  LynxUIListInvalidationContext *generalPropsContext =
      [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  generalPropsContext.numberOfColumns = 2;
  generalPropsContext.layoutType = LynxListLayoutWaterfall;
  [view dispatchInvalidationContext:generalPropsContext];

  LynxUIListInvalidationContext *initialInsertContext =
      [[LynxUIListInvalidationContext alloc] init];
  NSMutableArray<NSNumber *> *insertions = [NSMutableArray array];
  for (NSInteger i = 0; i < 100; i++) {
    [insertions addObject:@(i)];
  }
  initialInsertContext.insertions = insertions.copy;
  initialInsertContext.listUpdateType = LynxListUpdateTypeDataUpdate;
  [view dispatchInvalidationContext:initialInsertContext];
}

- (void)checkCachedCells:(LynxListViewLight *)listView {
  LynxListCachedCellManager *cachedCells = listView.cachedCells;
  XCTAssertLessThanOrEqual((NSInteger)cachedCells.lowerCachedCells.count,
                           listView.preloadBufferCount);
  XCTAssertLessThanOrEqual((NSInteger)cachedCells.upperCachedCells.count,
                           listView.preloadBufferCount);

  if ((NSInteger)cachedCells.lowerCachedCells.count < listView.preloadBufferCount) {
    __block NSInteger lastIndex = -1;
    [listView.cachedCells.allCachedCells
        enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          lastIndex = MAX(obj.updateToPath, lastIndex);
        }];
    XCTAssertEqual(lastIndex, listView.cachedCells.lastIndexInPathOrder);
    XCTAssertEqual((NSInteger)cachedCells.lowerCachedCells.count,
                   [listView layout].getCount - listView.cachedCells.lastIndexInPathOrder);
  }

  if ((NSInteger)cachedCells.upperCachedCells.count < listView.preloadBufferCount) {
    __block NSInteger firstIndex = NSIntegerMax;
    [listView.cachedCells.displayingCells
        enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          firstIndex = MIN(obj.updateToPath, firstIndex);
        }];
    XCTAssertEqual((NSInteger)cachedCells.upperCachedCells.count, MAX(firstIndex, 0));
  }
}
@end
