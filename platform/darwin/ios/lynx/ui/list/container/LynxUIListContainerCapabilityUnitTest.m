// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIListContainer.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxUIView.h>
#import <XCTest/XCTest.h>

#import "LynxUIListContainer+Internal.h"

@interface LynxUIOwner (ListCapabilityTesting)
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, LynxUI *> *uiHolder;
@end

@interface LynxUI (ListCapabilityTesting)
- (LynxUI *)getStickyScroller;
@end

// Deliberately does not inherit from the legacy list or scroller implementation.
@interface LynxListCapabilityTestUI : LynxUI <LynxListContainerInternal>
@property(nonatomic, assign) BOOL needAdjustContentOffset;
@property(nonatomic, assign) CGPoint targetDelta;
@property(nonatomic, assign) CGFloat targetContentSize;
@property(nonatomic, strong) NSMutableDictionary *listNativeStateCache;
@property(nonatomic, strong) NSMutableArray *restoreNativeStateBlockArray;
@property(nonatomic, strong) NSMutableDictionary<NSString *, NSMutableSet *> *flushedProps;
@property(nonatomic, assign) CGFloat estimatedOffset;
@property(nonatomic, assign) BOOL smooth;
@property(nonatomic, assign) BOOL scrolling;
@property(nonatomic, strong) LynxUIComponent *insertedComponent;
@property(nonatomic, strong) LynxUIComponent *removedComponent;
@end

@implementation LynxListCapabilityTestUI

@synthesize listNativeStateCache = _listNativeStateCache;

- (instancetype)init {
  self = [super init];
  if (self) {
    _listNativeStateCache = [NSMutableDictionary new];
    _restoreNativeStateBlockArray = [NSMutableArray new];
    _flushedProps = [NSMutableDictionary new];
  }
  return self;
}

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling {
  self.estimatedOffset = estimatedOffset;
  self.smooth = smooth;
  self.scrolling = scrolling;
}

- (void)insertListComponent:(LynxUIComponent *)component {
  self.insertedComponent = component;
}

- (void)removeListComponent:(LynxUIComponent *)component {
  self.removedComponent = component;
}

- (BOOL)initialPropsFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey {
  return [self.flushedProps[cacheKey] containsObject:initialPropKey];
}

- (void)setInitialPropsHasFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey {
  NSMutableSet *props = self.flushedProps[cacheKey] ?: [NSMutableSet new];
  [props addObject:initialPropKey];
  self.flushedProps[cacheKey] = props;
}

@end

@interface LynxListCapabilityRecordingComponent : LynxUIComponent
@property(nonatomic, strong) NSMutableArray<NSString *> *lifecycleEvents;
@property(nonatomic, strong) LynxUI *lastList;
@property(nonatomic, copy) NSString *lastItemKey;
@property(nonatomic, assign) BOOL lastExists;
@end

@implementation LynxListCapabilityRecordingComponent

- (instancetype)init {
  self = [super init];
  if (self) {
    _lifecycleEvents = [NSMutableArray new];
  }
  return self;
}

- (void)onListCellPrepareForReuse:(NSString *)itemKey withList:(LynxUI *)list {
  [self.lifecycleEvents addObject:@"reuse"];
  self.lastList = list;
  self.lastItemKey = itemKey;
}

- (void)onListCellAppear:(NSString *)itemKey withList:(LynxUI *)list {
  [self.lifecycleEvents addObject:@"appear"];
  self.lastList = list;
  self.lastItemKey = itemKey;
}

- (void)onListCellDisappear:(NSString *)itemKey exist:(BOOL)isExist withList:(LynxUI *)list {
  [self.lifecycleEvents addObject:@"disappear"];
  self.lastList = list;
  self.lastItemKey = itemKey;
  self.lastExists = isExist;
}

@end

@interface LynxUIListContainerCapabilityUnitTest : XCTestCase
@end

@implementation LynxUIListContainerCapabilityUnitTest

- (LynxUIOwner *)ownerWithList:(LynxUI *)list component:(LynxUIComponent *)component {
  LynxUIOwner *owner = [LynxUIOwner new];
  owner.uiHolder = [@{@1 : list, @2 : component, @3 : [LynxUIView new]} mutableCopy];
  return owner;
}

- (void)testCapabilityRecognitionDoesNotRequireLegacySuperclass {
  LynxUI *standalone = [LynxListCapabilityTestUI new];
  XCTAssertTrue(LynxIsListContainerUI(standalone));
  XCTAssertFalse([standalone isKindOfClass:LynxUIScroller.class]);
  XCTAssertTrue(LynxIsListContainerUI([LynxUIListContainer new]));
  XCTAssertFalse(LynxIsListContainerUI([LynxUIView new]));
  XCTAssertFalse(LynxIsListContainerUI([NSObject new]));
  XCTAssertFalse(LynxIsListContainerUI(nil));
}

- (void)testRendererScrollAndLayoutCallbacksUseCapabilities {
  LynxListCapabilityTestUI *list = [LynxListCapabilityTestUI new];
  LynxUIOwner *owner = [self ownerWithList:list component:[LynxUIComponent new]];

  [owner updateScrollInfo:1 estimatedOffset:25.5 smooth:YES scrolling:NO];
  XCTAssertEqualWithAccuracy(list.estimatedOffset, 25.5, 0.001);
  XCTAssertTrue(list.smooth);
  XCTAssertFalse(list.scrolling);

  [owner updateContentOffsetForListContainer:1 contentSize:500 deltaX:2 deltaY:3];
  [owner updateContentOffsetForListContainer:1 contentSize:600 deltaX:4 deltaY:-1];
  XCTAssertTrue(list.needAdjustContentOffset);
  XCTAssertEqualWithAccuracy(list.targetContentSize, 600, 0.001);
  XCTAssertTrue(CGPointEqualToPoint(list.targetDelta, CGPointMake(6, 2)));

  // A regular UI has no list selectors; incorrect dispatch would raise an exception.
  XCTAssertNoThrow([owner updateScrollInfo:3 estimatedOffset:50 smooth:NO scrolling:YES]);
  XCTAssertNoThrow([owner updateContentOffsetForListContainer:3 contentSize:100 deltaX:1 deltaY:1]);
  XCTAssertNoThrow([owner updateScrollInfo:99 estimatedOffset:50 smooth:NO scrolling:YES]);
}

- (void)testRendererComponentCallbacksKeepChildTypeChecks {
  LynxListCapabilityTestUI *list = [LynxListCapabilityTestUI new];
  LynxUIComponent *component = [LynxUIComponent new];
  LynxUIOwner *owner = [self ownerWithList:list component:component];

  [owner insertListComponent:1 componentSign:3];
  [owner removeListComponent:1 componentSign:3];
  XCTAssertNil(list.insertedComponent);
  XCTAssertNil(list.removedComponent);
  [owner insertListComponent:1 componentSign:2];
  [owner removeListComponent:1 componentSign:2];
  XCTAssertEqual(list.insertedComponent, component);
  XCTAssertEqual(list.removedComponent, component);
  XCTAssertNoThrow([owner insertListComponent:3 componentSign:2]);
  XCTAssertNoThrow([owner removeListComponent:3 componentSign:2]);
}

- (void)testItemLifecycleRecognizesBothListImplementations {
  for (LynxUI *list in @[ [LynxUIListContainer new], [LynxListCapabilityTestUI new] ]) {
    LynxListCapabilityRecordingComponent *component = [LynxListCapabilityRecordingComponent new];
    component.parent = list;
    LynxUIOwner *owner = [self ownerWithList:list component:component];

    [owner listWillReuseNode:2 withItemKey:@"first"];
    [owner listCellWillAppear:2 withItemKey:@"second"];
    [owner ListCellDisappear:2 exist:YES withItemKey:@"second"];
    XCTAssertEqualObjects(component.lifecycleEvents, (@[ @"reuse", @"appear", @"disappear" ]));
    XCTAssertEqual(component.lastList, list);
    XCTAssertEqualObjects(component.lastItemKey, @"second");
    XCTAssertTrue(component.lastExists);

    component.parent = owner.uiHolder[@3];
    [owner listWillReuseNode:2 withItemKey:@"ignored"];
    [owner listCellWillAppear:2 withItemKey:@"ignored"];
    [owner ListCellDisappear:2 exist:NO withItemKey:@"ignored"];
    XCTAssertEqual(component.lifecycleEvents.count, 3U);
  }
}

- (void)testNativeStoragePreservesCacheAndRestoreQueueIdentity {
  LynxUI *child = [LynxUIView new];
  for (LynxUI<LynxListContainerInternal> *list in
       @[ [LynxUIListContainer new], [LynxListCapabilityTestUI new] ]) {
    NSMutableDictionary *cache = list.listNativeStateCache;
    [child storeKeyToNativeStorage:list key:@"state" value:@42];
    XCTAssertEqualObjects(cache[@"state"], @42);
    XCTAssertEqual(list.listNativeStateCache, cache);

    NSMutableArray *queue = [child getRestoreNativeStateBlockArrayFromList:list];
    XCTAssertEqual(queue, list.restoreNativeStateBlockArray);
    __block BOOL restored = NO;
    [queue addObject:^{
      restored = YES;
    }];
    void (^restore)(void) = list.restoreNativeStateBlockArray.lastObject;
    restore();
    XCTAssertTrue(restored);
  }
  XCTAssertNil([child getRestoreNativeStateBlockArrayFromList:[LynxUIView new]]);
}

- (void)testListItemsUseLocalHitTestBounds {
  LynxUIComponent *component = [LynxUIComponent new];
  CGRect frame = CGRectMake(10, 20, 100, 40);
  [component updateFrame:frame
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  for (LynxUI *list in @[ [LynxUIListContainer new], [LynxListCapabilityTestUI new] ]) {
    component.parent = list;
    XCTAssertTrue(
        CGRectEqualToRect([component getHitTestFrameWithFrame:frame], component.view.bounds));
  }
  LynxUI *ordinaryParent = [LynxUIView new];
  component.parent = ordinaryParent;
  XCTAssertTrue(CGRectEqualToRect([component getHitTestFrameWithFrame:frame], frame));
}

- (void)testStickyScrollerSearchSkipsListContainers {
  LynxUIScroller *outerScroller = [LynxUIScroller new];
  LynxUI *child = [LynxUIView new];
  for (LynxUI *list in @[ [LynxUIListContainer new], [LynxListCapabilityTestUI new] ]) {
    list.parent = outerScroller;
    child.parent = list;
    XCTAssertEqual([child getStickyScroller], outerScroller);
  }
}

@end
