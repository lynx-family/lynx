// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIListContainer.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxVersion.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxListItemHelper.h"
#import "LynxListScrollHelper.h"
#import "LynxListStickyManager.h"
#import "LynxUI+Gesture.h"

// Test-only implementation for verifying scale and translation. Production lists do not create a
// Carry transformer by default.
/** Carry-mode transformer for list-item scaling and translation. */
@interface LynxListItemCarryTransformer : NSObject <LynxListItemTransformer>

@property(nonatomic, assign) CGFloat normTranslationFactor;
@property(nonatomic, assign) CGFloat minScaleX;
@property(nonatomic, assign) CGFloat maxScaleX;
@property(nonatomic, assign) CGFloat minScaleY;
@property(nonatomic, assign) CGFloat maxScaleY;

@end

// Record protocol calls and their order to verify lifecycle behavior and arguments independently of
// the Carry algorithm.
@interface LynxRecordingListItemTransformer : NSObject <LynxListItemTransformer>
@property(nonatomic, strong) NSMutableArray<NSString *> *events;
@property(nonatomic, copy) NSString *name;
@property(nonatomic, assign) NSUInteger transformCount;
@property(nonatomic, assign) NSUInteger resetCount;
@property(nonatomic, assign) CGFloat offset;
@property(nonatomic, assign) BOOL vertical;
@property(nonatomic, assign) BOOL rtl;
@property(nonatomic, assign) CGFloat scale;
@end

@implementation LynxRecordingListItemTransformer
- (instancetype)init {
  if (self = [super init]) {
    _events = [NSMutableArray array];
    _name = @"transformer";
    _scale = 0.75;
  }
  return self;
}

- (void)transformItemInListContainer:(UIScrollView *)listContainerView
                            itemView:(UIView *)itemView
                          isVertical:(BOOL)isVertical
                               isRTL:(BOOL)isRTL
                      mainAxisOffset:(CGFloat)mainAxisOffset {
  self.transformCount++;
  self.offset = mainAxisOffset;
  self.vertical = isVertical;
  self.rtl = isRTL;
  [self.events addObject:[self.name stringByAppendingString:@":transform"]];
  itemView.layer.sublayerTransform = CATransform3DMakeScale(self.scale, self.scale, 1);
}

- (void)resetItem:(UIView *)itemView {
  self.resetCount++;
  [self.events addObject:[self.name stringByAppendingString:@":reset"]];
  itemView.layer.sublayerTransform = CATransform3DIdentity;
}
@end

@implementation LynxListItemCarryTransformer

- (instancetype)init {
  self = [super init];
  if (self) {
    _normTranslationFactor = 0;
    _minScaleX = 0.8;
    _maxScaleX = 1;
    _minScaleY = 0.8;
    _maxScaleY = 1;
  }
  return self;
}

- (void)transformItemInListContainer:(UIScrollView *)listContainerView
                            itemView:(UIView *)itemView
                          isVertical:(BOOL)isVertical
                               isRTL:(BOOL)isRTL
                      mainAxisOffset:(CGFloat)mainAxisOffset {
  CGFloat mainAxisSize =
      isVertical ? CGRectGetHeight(itemView.bounds) : CGRectGetWidth(itemView.bounds);
  if (mainAxisSize <= 0) {
    mainAxisSize = isVertical ? CGRectGetHeight(listContainerView.bounds)
                              : CGRectGetWidth(listContainerView.bounds);
  }
  if (mainAxisSize <= 0 || self.maxScaleX < self.minScaleX || self.maxScaleY < self.minScaleY) {
    [self resetItem:itemView];
    return;
  }

  CGFloat distance = fabs(mainAxisOffset);
  CGFloat scaleX = self.maxScaleX - distance * (self.maxScaleX - self.minScaleX) / mainAxisSize;
  CGFloat scaleY = self.maxScaleY - distance * (self.maxScaleY - self.minScaleY) / mainAxisSize;
  scaleX = MAX(self.minScaleX, MIN(self.maxScaleX, scaleX));
  scaleY = MAX(self.minScaleY, MIN(self.maxScaleY, scaleY));

  CGFloat translation = 0;
  if (self.normTranslationFactor > 0) {
    CGFloat maxScale = isVertical ? self.maxScaleY : self.maxScaleX;
    CGFloat minScale = isVertical ? self.minScaleY : self.minScaleX;
    CGFloat interval = self.normTranslationFactor * mainAxisSize * (2 - maxScale - minScale) / 2;
    distance = MIN(distance, mainAxisSize);
    CGFloat halfSize = mainAxisSize / 2;
    CGFloat normFactor = 1 - fabs(distance - halfSize) / halfSize;
    if (mainAxisOffset > 0) {
      translation = distance >= halfSize ? -interval + 0.5 * normFactor * interval
                                         : -0.5 * normFactor * interval;
    } else {
      translation = distance <= halfSize ? 0.5 * normFactor * interval
                                         : interval - 0.5 * normFactor * interval;
    }
  }

  // Convert translation along the logical main axis to physical view coordinates.
  CGFloat translationX = isVertical ? 0 : (isRTL ? -translation : translation);
  CGFloat translationY = isVertical ? translation : 0;
  CGAffineTransform transform =
      CGAffineTransformMake(scaleX, 0, 0, scaleY, translationX, translationY);
  // Transform visual sublayers while preserving the wrapper's layout frame.
  itemView.layer.sublayerTransform = CATransform3DMakeAffineTransform(transform);
}

- (void)resetItem:(UIView *)itemView {
  itemView.layer.sublayerTransform = CATransform3DIdentity;
}

@end

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

@interface LynxListItemTransformerUnitTest : XCTestCase
@property(nonatomic, strong) LynxUIContext *context;
@property(nonatomic, strong) LynxUIListContainer *list;
@end

@implementation LynxListItemTransformerUnitTest

- (void)setUp {
  [super setUp];
  self.context = [[LynxUIContext alloc] init];
  self.context.fetcher = OCMProtocolMock(@protocol(ListNodeInfoFetcherProtocol));
  self.list = [[LynxUIListContainer alloc] init];
  self.list.context = self.context;
  [self.list updateFrame:CGRectMake(0, 0, 300, 300)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  self.list.view.contentSize = CGSizeMake(1200, 1200);
}

- (LynxUIComponent *)addItemWithFrame:(CGRect)frame {
  LynxUIComponent *component = [[LynxUIComponent alloc] init];
  [component updateFrame:frame
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [self.list insertChild:component atIndex:self.list.children.count];
  [self.list insertListComponent:component];
  return component;
}

- (void)testNoTransformerPreservesVisualPropertiesAndNormalLayout {
  // Without a transformer, the frame callback leaves the wrapper unchanged. The normal layout
  // callback still updates its position while preserving existing visual effects.
  XCTAssertNil(self.list.listItemTransformer);
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(20, 40, 100, 100)];
  UIView *wrapper = component.view.superview;
  CATransform3D visual = CATransform3DMakeScale(0.6, 0.7, 1);
  wrapper.layer.sublayerTransform = visual;
  [component updateFrame:CGRectMake(20, 80, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertEqualWithAccuracy(wrapper.frame.origin.y, 40, 0.001);
  [self.list onComponentLayoutUpdated:component];
  XCTAssertTrue(CGRectEqualToRect(wrapper.frame, CGRectMake(20, 80, 100, 100)));
  self.list.view.contentOffset = CGPointMake(0, 30);
  [self.list requestListItemTransform];
  XCTAssertTrue(CATransform3DEqualToTransform(wrapper.layer.sublayerTransform, visual));
  [self.list removeListComponent:component];
  XCTAssertNil(wrapper.superview);
  XCTAssertNil(component.view.superview);
  XCTAssertTrue(CATransform3DEqualToTransform(wrapper.layer.sublayerTransform, visual));
}

- (void)testTransformerAppliesToExistingAndNewItemsOnly {
  // Transform existing and newly attached wrappers immediately, excluding ordinary subviews such as
  // scroll indicators.
  LynxUIComponent *existing = [self addItemWithFrame:CGRectMake(0, 40, 100, 100)];
  UIView *decoration = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 10, 10)];
  [self.list.view addSubview:decoration];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  XCTAssertEqual(transformer.transformCount, 1U);
  XCTAssertEqualWithAccuracy(existing.view.superview.layer.sublayerTransform.m11, 0.75, 0.001);
  LynxUIComponent *added = [self addItemWithFrame:CGRectMake(0, 160, 100, 100)];
  XCTAssertEqual(transformer.transformCount, 2U);
  XCTAssertEqualWithAccuracy(transformer.offset, 160, 0.001);
  XCTAssertEqualWithAccuracy(added.view.superview.layer.sublayerTransform.m11, 0.75, 0.001);
  XCTAssertTrue(CATransform3DIsIdentity(decoration.layer.sublayerTransform));
}

- (void)testFrameCallbackUsesUpdatedWrapperFrame {
  // With a transformer installed, a component frame change must update the wrapper before
  // calculating the offset from its new position.
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(0, 40, 100, 100)];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  [component updateFrame:CGRectMake(0, 80, 100, 120)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertTrue(transformer.transformCount > 1U);
  XCTAssertEqualWithAccuracy(transformer.offset, 80, 0.001);
  XCTAssertTrue(CGRectEqualToRect(component.view.superview.frame, CGRectMake(0, 80, 100, 120)));
  XCTAssertTrue(CGRectEqualToRect(component.view.frame, CGRectMake(0, 0, 100, 120)));
}

- (void)testScrollCallbackRefreshesCurrentOffset {
  // Invoke scrollViewDidScroll after changing the scroll position and verify that the transformer
  // receives the latest item top minus contentOffset.
  [self addItemWithFrame:CGRectMake(0, 80, 100, 100)];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  self.list.view.delegate = nil;
  self.list.view.contentOffset = CGPointMake(0, 30);
  [self.list scrollViewDidScroll:self.list.view];
  XCTAssertEqual(transformer.transformCount, 2U);
  XCTAssertEqualWithAccuracy(transformer.offset, 50, 0.001);
}

- (void)testHorizontalRtlUsesLogicalRightEdge {
  // Horizontal RTL begins at the right edge: subtract the item's right edge from the viewport's
  // right edge instead of using the LTR left-edge calculation.
  [LynxPropsProcessor updateProp:@"horizontal" withKey:@"scroll-orientation" forUI:self.list];
  [LynxPropsProcessor updateProp:@(LynxDirectionRtl) withKey:@"direction" forUI:self.list];
  [self addItemWithFrame:CGRectMake(150, 0, 100, 100)];
  self.list.view.contentOffset = CGPointMake(20, 0);
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  XCTAssertTrue(transformer.rtl);
  XCTAssertFalse(transformer.vertical);
  XCTAssertEqualWithAccuracy(transformer.offset, 70, 0.001);
  self.list.view.bounds = CGRectMake(20, 0, 400, 300);
  [self.list requestListItemTransform];
  XCTAssertEqualWithAccuracy(transformer.offset, 170, 0.001);
}

- (void)testHorizontalLtrAndVerticalRtlOffsets {
  // Horizontal LTR uses the left edge. Vertical RTL still uses the top edge without reversing the
  // vertical offset.
  [LynxPropsProcessor updateProp:@"horizontal" withKey:@"scroll-orientation" forUI:self.list];
  [self addItemWithFrame:CGRectMake(150, 80, 100, 100)];
  self.list.view.contentOffset = CGPointMake(20, 0);
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  XCTAssertFalse(transformer.rtl);
  XCTAssertEqualWithAccuracy(transformer.offset, 130, 0.001);
  [LynxPropsProcessor updateProp:@"vertical" withKey:@"scroll-orientation" forUI:self.list];
  [LynxPropsProcessor updateProp:@(LynxDirectionRtl) withKey:@"direction" forUI:self.list];
  self.list.view.contentOffset = CGPointMake(20, 30);
  [self.list requestListItemTransform];
  XCTAssertTrue(transformer.vertical);
  XCTAssertTrue(transformer.rtl);
  XCTAssertEqualWithAccuracy(transformer.offset, 50, 0.001);
}

- (void)testSameInstanceAndExplicitRefresh {
  // Assigning the same transformer does not refresh its effects. An explicit request applies
  // updated parameters.
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  transformer.scale = 0.5;
  self.list.listItemTransformer = transformer;
  XCTAssertEqual(transformer.transformCount, 1U);
  XCTAssertEqual(transformer.resetCount, 0U);
  [self.list requestListItemTransform];
  XCTAssertEqual(transformer.transformCount, 2U);
  XCTAssertEqualWithAccuracy(component.view.superview.layer.sublayerTransform.m11, 0.5, 0.001);
}

- (void)testReplacementResetsOldTransformerBeforeApplyingNew {
  // Reset the old transformer before applying the new one so that the reset cannot overwrite the
  // new visual effects.
  [self addItemWithFrame:CGRectMake(0, 0, 100, 100)];
  NSMutableArray<NSString *> *events = [NSMutableArray array];
  LynxRecordingListItemTransformer *oldTransformer =
      [[LynxRecordingListItemTransformer alloc] init];
  oldTransformer.name = @"old";
  oldTransformer.events = events;
  LynxRecordingListItemTransformer *newTransformer =
      [[LynxRecordingListItemTransformer alloc] init];
  newTransformer.name = @"new";
  newTransformer.events = events;
  self.list.listItemTransformer = oldTransformer;
  [events removeAllObjects];
  self.list.listItemTransformer = newTransformer;
  XCTAssertEqualObjects(events, (@[ @"old:reset", @"new:transform" ]));
}

- (void)testClearStopsTransformCallbacks {
  // Clearing the transformer restores existing wrappers. Later layout, insertion, and explicit
  // refresh requests must not call the old transformer.
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  self.list.listItemTransformer = nil;
  XCTAssertEqual(transformer.resetCount, 1U);
  XCTAssertTrue(CATransform3DIsIdentity(component.view.superview.layer.sublayerTransform));
  [component updateFrame:CGRectMake(0, 40, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  [self.list onComponentLayoutUpdated:component];
  [self addItemWithFrame:CGRectMake(0, 160, 100, 100)];
  [self.list requestListItemTransform];
  XCTAssertEqual(transformer.transformCount, 1U);
}

- (void)testDetachResetsAndReuseTransformsNewWrapper {
  // Detaching resets the old wrapper, and detached frame updates do not transform it. Reusing the
  // component transforms its new wrapper.
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(0, 0, 100, 100)];
  UIView *oldWrapper = component.view.superview;
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  self.list.listItemTransformer = transformer;
  [self.list removeListComponent:component];
  XCTAssertEqual(transformer.resetCount, 1U);
  XCTAssertTrue(CATransform3DIsIdentity(oldWrapper.layer.sublayerTransform));
  [component updateFrame:CGRectMake(0, 80, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  XCTAssertEqual(transformer.transformCount, 1U);
  [self.list insertListComponent:component];
  XCTAssertEqual(transformer.transformCount, 2U);
  XCTAssertNotEqual(component.view.superview, oldWrapper);
  XCTAssertEqualWithAccuracy(transformer.offset, 80, 0.001);
}

- (void)testStickyAndTransformerAreMutuallyExclusive {
  // Reject transformers while sticky positioning is enabled. Enabling sticky positioning after
  // installation must reset and clear the transformer.
  LynxUIComponent *component = [self addItemWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxRecordingListItemTransformer *transformer = [[LynxRecordingListItemTransformer alloc] init];
  [LynxPropsProcessor updateProp:@YES withKey:@"sticky" forUI:self.list];
  self.list.listItemTransformer = transformer;
  XCTAssertNil(self.list.listItemTransformer);
  XCTAssertEqual(transformer.transformCount, 0U);
  [LynxPropsProcessor updateProp:@NO withKey:@"sticky" forUI:self.list];
  self.list.listItemTransformer = transformer;
  [LynxPropsProcessor updateProp:@YES withKey:@"sticky" forUI:self.list];
  XCTAssertNil(self.list.listItemTransformer);
  XCTAssertEqual(transformer.resetCount, 1U);
  XCTAssertTrue(CATransform3DIsIdentity(component.view.superview.layer.sublayerTransform));
}

- (void)testCarryMirrorsRtlTranslationWithoutChangingFrame {
  // At half an item length, scale is 0.9 and horizontal translation is -5, or +5 in RTL. The frame
  // stays unchanged, and reset restores the identity matrix.
  UIView *item = [[UIView alloc] initWithFrame:CGRectMake(20, 40, 100, 100)];
  LynxListItemCarryTransformer *transformer = [[LynxListItemCarryTransformer alloc] init];
  transformer.normTranslationFactor = 1;
  [transformer transformItemInListContainer:self.list.view
                                   itemView:item
                                 isVertical:NO
                                      isRTL:NO
                             mainAxisOffset:50];
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m11, 0.9, 0.001);
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m41, -5, 0.001);
  [transformer transformItemInListContainer:self.list.view
                                   itemView:item
                                 isVertical:NO
                                      isRTL:YES
                             mainAxisOffset:50];
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m41, 5, 0.001);
  XCTAssertTrue(CGRectEqualToRect(item.frame, CGRectMake(20, 40, 100, 100)));
  [transformer resetItem:item];
  XCTAssertTrue(CATransform3DIsIdentity(item.layer.sublayerTransform));
}

- (void)testCarryClampsScaleAndResetsInvalidConfiguration {
  // Clamp distant items to the minimum scale. Negative vertical offsets produce positive
  // translation, and invalid scale ranges restore the identity matrix.
  UIView *item = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 100, 100)];
  LynxListItemCarryTransformer *transformer = [[LynxListItemCarryTransformer alloc] init];
  transformer.normTranslationFactor = 1;
  [transformer transformItemInListContainer:self.list.view
                                   itemView:item
                                 isVertical:YES
                                      isRTL:YES
                             mainAxisOffset:-1000];
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m22, 0.8, 0.001);
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m42, 10, 0.001);
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m41, 0, 0.001);
  transformer.minScaleX = 2;
  [transformer transformItemInListContainer:self.list.view
                                   itemView:item
                                 isVertical:YES
                                      isRTL:NO
                             mainAxisOffset:0];
  XCTAssertTrue(CATransform3DIsIdentity(item.layer.sublayerTransform));
}

- (void)testCarryFallsBackToViewportForUnlaidOutItem {
  // Fall back to the viewport size for zero-sized items. If the viewport is also zero-sized,
  // restore the identity matrix without dividing by zero.
  UIView *item = [[UIView alloc] init];
  LynxListItemCarryTransformer *transformer = [[LynxListItemCarryTransformer alloc] init];
  transformer.normTranslationFactor = 1;
  [transformer transformItemInListContainer:self.list.view
                                   itemView:item
                                 isVertical:YES
                                      isRTL:NO
                             mainAxisOffset:150];
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m22, 0.9, 0.001);
  XCTAssertEqualWithAccuracy(item.layer.sublayerTransform.m42, -15, 0.001);
  UIScrollView *emptyViewport = [[UIScrollView alloc] init];
  [transformer transformItemInListContainer:emptyViewport
                                   itemView:item
                                 isVertical:YES
                                      isRTL:NO
                             mainAxisOffset:150];
  XCTAssertTrue(CATransform3DIsIdentity(item.layer.sublayerTransform));
}

@end
