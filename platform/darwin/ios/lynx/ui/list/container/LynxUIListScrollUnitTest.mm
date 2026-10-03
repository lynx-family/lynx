// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxScrollEventManager.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIOwner.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#import "LynxListStickyManager.h"
#import "LynxUIListScroll.h"

@interface LynxUIListScroll (Testing) <LynxBaseScrollViewDelegate>
@property(nonatomic, strong) LynxListItemHelper *itemHelper;
@property(nonatomic, strong) LynxListScrollHelper *scrollHelper;
@property(nonatomic, strong) LynxListStickyManager *stickyManager;
@property(nonatomic, strong) LynxScrollEventManager *scrollEventManager;
@property(nonatomic, assign) LynxBaseScrollViewScrollState currentScrollState;
- (void)insertChild:(nullable LynxUI *)child atIndex:(NSInteger)index;
@end

@interface LynxUIOwner (ListScrollTesting)
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, LynxUI *> *uiHolder;
@end

@interface LynxListScrollLifecycleRecorder : LynxListStickyManager <LynxUIListScrollDelegate>
@property(nonatomic, weak) LynxUIListScroll *list;
@property(nonatomic, strong) NSMutableArray<NSString *> *events;
@property(nonatomic, strong) UIView *lastWrapper;
@end

@implementation LynxListScrollLifecycleRecorder
- (void)insertListComponent:(LynxUIComponent *)component
                    wrapper:(LynxListScrollComponentWrapper *)wrapper {
  XCTAssertEqual(wrapper.holdingUI, component);
  XCTAssertEqual(wrapper.superview, self.list.view);
  XCTAssertEqual(component.view.superview, wrapper);
  XCTAssertEqual(wrapper.layer.zPosition, component.zIndex);
  self.lastWrapper = wrapper;
  [self.events addObject:@"delegate-insert"];
}
- (void)removeListComponent:(LynxUIComponent *)component {
  XCTAssertNil(component.view.superview);
  XCTAssertNil(self.lastWrapper.superview);
  [self.events addObject:@"delegate-remove"];
}
- (void)didAttachComponent:(LynxUIComponent *)component {
  [self.events addObject:@"sticky-attach"];
  [super didAttachComponent:component];
}
- (void)willDetachComponent:(LynxUIComponent *)component {
  if (self.lastWrapper.superview == self.list.view) {
    XCTAssertEqual(component.view.superview, self.lastWrapper);
  }
  [self.events addObject:@"sticky-detach"];
  [super willDetachComponent:component];
}
- (void)didLayoutComponent:(LynxUIComponent *)component {
  XCTAssertTrue(CGRectEqualToRect(self.lastWrapper.frame, component.frame));
  XCTAssertEqual(self.lastWrapper.layer.zPosition, component.zIndex);
  [self.events addObject:@"sticky-layout"];
  [super didLayoutComponent:component];
}
@end

@interface LynxListScrollStickyEventRecorder : LynxScrollEventManager
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *events;
@end

@implementation LynxListScrollStickyEventRecorder
- (void)sendScrollEvent:(NSString *)name
             scrollView:(UIScrollView *)scrollView
                 detail:(NSDictionary *)detail {
  [self.events addObject:@{@"name" : name, @"detail" : detail}];
}
@end

@interface LynxUIListScrollUnitTest : XCTestCase
@end

@implementation LynxUIListScrollUnitTest

- (LynxUIListScroll *)list {
  LynxUIListScroll *list = [LynxUIListScroll new];
  [list updateFrame:CGRectMake(0, 0, 100, 100)
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  list.view.contentSize = CGSizeMake(400, 400);
  return list;
}

- (LynxUIComponent *)component:(NSString *)key frame:(CGRect)frame {
  LynxUIComponent *component = [LynxUIComponent new];
  component.itemKey = key;
  [component updateFrame:frame
              withPadding:UIEdgeInsetsZero
                   border:UIEdgeInsetsZero
      withLayoutAnimation:NO];
  return component;
}

- (void)configureSticky:(LynxUIListScroll *)list
                   keys:(NSArray<NSString *> *)keys
                 starts:(NSArray<NSNumber *> *)starts
                   ends:(NSArray<NSNumber *> *)ends {
  [LynxPropsProcessor updateProp:@YES withKey:@"sticky" forUI:list];
  [LynxPropsProcessor updateProp:@{@"itemkeys" : keys, @"stickyStart" : starts, @"stickyEnd" : ends}
                         withKey:@"list-container-info"
                           forUI:list];
  [list propsDidUpdate];
}

- (void)testIndependentBackendAndCapability {
  LynxUIListScroll *list = [self list];
  XCTAssertEqual(LynxUIListScroll.superclass, LynxUI.class);
  XCTAssertTrue([list.view isKindOfClass:LynxBaseScrollView.class]);
  XCTAssertTrue(LynxIsListContainerUI(list));
  XCTAssertTrue(list.isScrollContainer);
  XCTAssertEqual((id)list.view.delegate, list.view);
  XCTAssertEqual(list.view.scrollDelegate, list);
  XCTAssertTrue(list.view.verticalOrientation);
  XCTAssertFalse(list.view.bounces);
  XCTAssertFalse(list.view.showsVerticalScrollIndicator);
  XCTAssertNotNil(list.itemHelper);
  XCTAssertNotNil(list.scrollHelper);
  XCTAssertNotNil(list.stickyManager);
  XCTAssertEqualObjects([list getHitTestChainForNestedScrollViews], @[]);
  [list.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateDragging];
  XCTAssertEqual(list.currentScrollState, LynxBaseScrollViewScrollStateDragging);
  [list.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  XCTAssertEqual(list.currentScrollState, LynxBaseScrollViewScrollStateIdle);
}

- (void)testLogicalChildrenWaitForPlatformInsertion {
  LynxUIListScroll *list = [self list];
  LynxUIComponent *first = [self component:@"first" frame:CGRectMake(10, 20, 80, 40)];
  LynxUIComponent *second = [self component:@"second" frame:CGRectMake(0, 60, 100, 40)];
  [list insertChild:first atIndex:5];
  [list insertChild:second atIndex:0];
  [list insertChild:nil atIndex:0];
  XCTAssertEqualObjects(list.children, (@[ second, first ]));
  XCTAssertEqual(first.parent, list);
  XCTAssertEqual(first.layoutObserver, list);
  XCTAssertNil(first.view.superview);
  [first asyncListItemRenderFinished:1];
  XCTAssertEqual(first.view.superview.superview, list.view);
  XCTAssertTrue(CGRectEqualToRect([first getHitTestFrameWithFrame:first.frame], first.view.bounds));
}

- (void)testItemLifecyclePreservesStickyAndDelegateOrdering {
  LynxUIListScroll *list = [self list];
  LynxListScrollLifecycleRecorder *recorder =
      [[LynxListScrollLifecycleRecorder alloc] initWithOwner:(id<LynxListStickyManagerOwner>)list];
  recorder.list = list;
  recorder.events = [NSMutableArray array];
  list.stickyManager = recorder;
  list.delegate = recorder;
  LynxUIComponent *component = [self component:@"item" frame:CGRectMake(10, 20, 80, 40)];
  component.zIndex = 3;
  component.view.alpha = 0.4;
  UIView *previousParent = [UIView new];
  [previousParent addSubview:component.view];

  [list insertListComponent:component];
  UIView *firstWrapper = recorder.lastWrapper;
  XCTAssertEqual(previousParent.subviews.count, 0U);
  XCTAssertEqualObjects(recorder.events, (@[ @"delegate-insert", @"sticky-attach" ]));
  [recorder.events removeAllObjects];
  [list insertListComponent:component];
  XCTAssertEqual(component.view.superview, firstWrapper);
  XCTAssertEqualObjects(recorder.events, (@[ @"sticky-attach" ]));

  [recorder.events removeAllObjects];
  component.frame = CGRectMake(15, 30, 60, 25);
  component.zIndex = 8;
  [list onComponentLayoutUpdated:component];
  XCTAssertTrue(CGRectEqualToRect(component.view.frame, CGRectMake(0, 0, 60, 25)));
  XCTAssertEqualObjects(recorder.events, (@[ @"sticky-layout" ]));
  [recorder.events removeAllObjects];
  [list removeListComponent:component];
  XCTAssertEqualObjects(recorder.events, (@[ @"sticky-detach", @"delegate-remove" ]));
  [recorder.events removeAllObjects];
  [list removeListComponent:component];
  XCTAssertEqualObjects(recorder.events, (@[ @"sticky-detach" ]));
  [recorder.events removeAllObjects];
  [list insertListComponent:component];
  XCTAssertNotEqual(recorder.lastWrapper, firstWrapper);
  XCTAssertEqualObjects(recorder.events, (@[ @"delegate-insert", @"sticky-attach" ]));
  XCTAssertEqualWithAccuracy(component.view.alpha, 0.4, 0.001);
}

- (void)testRemovalChecksParentOwnershipNotWrapperClass {
  LynxUIListScroll *list = [self list];
  LynxUIComponent *component = [LynxUIComponent new];
  UIView *wrapper = [UIView new];
  UIScrollView *foreign = [UIScrollView new];
  [wrapper addSubview:component.view];
  [foreign addSubview:wrapper];
  [list onComponentLayoutUpdated:component];
  [list removeListComponent:component];
  XCTAssertEqual(component.view.superview, wrapper);
  XCTAssertEqual(wrapper.superview, foreign);
  [list.view addSubview:wrapper];
  [list removeListComponent:component];
  XCTAssertNil(wrapper.superview);
  XCTAssertNil(component.view.superview);
  [list.view addSubview:component.view];
  [list removeListComponent:component];
  XCTAssertEqual(component.view.superview, list.view);
}

- (void)testFadeInUsesDefaultAndConfiguredDurationAfterAttachment {
  LynxUIListScroll *list = [self list];
  [LynxPropsProcessor updateProp:@YES withKey:@"enable-fade-in-animation" forUI:list];
  for (NSNumber *durationValue in @[ @0.1, @0.125, @0 ]) {
    NSTimeInterval duration = durationValue.doubleValue;
    if (duration != 0.1) {
      [LynxPropsProcessor updateProp:@(duration * 1000)
                             withKey:@"update-animation-fade-in-duration"
                               forUI:list];
    }
    LynxUIComponent *component = [LynxUIComponent new];
    __block BOOL animated = NO;
    id viewMock = OCMClassMock(UIView.class);
    @try {
      OCMStub([viewMock animateWithDuration:duration
                                      delay:0
                                    options:UIViewAnimationOptionAllowUserInteraction
                                 animations:[OCMArg any]
                                 completion:[OCMArg any]])
          .andDo(^(NSInvocation *invocation) {
            XCTAssertEqual(component.view.alpha, 0);
            XCTAssertEqual(component.view.superview.superview, list.view);
            __unsafe_unretained void (^animations)(void);
            [invocation getArgument:&animations atIndex:5];
            animations();
            animated = YES;
          });
      [list insertListComponent:component];
      XCTAssertTrue(animated);
      XCTAssertEqual(component.view.alpha, 1);
    } @finally {
      [viewMock stopMocking];
    }
    [list removeListComponent:component];
  }
  [LynxPropsProcessor updateProp:nil withKey:@"enable-fade-in-animation" forUI:list];
  LynxUIComponent *component = [LynxUIComponent new];
  component.view.alpha = 0.3;
  [list insertListComponent:component];
  XCTAssertEqualWithAccuracy(component.view.alpha, 0.3, 0.001);
}

- (void)testAsyncInsertionHonorsBothFlagsAndResets {
  for (NSNumber *batch in @[ @NO, @YES ]) {
    for (NSNumber *platform in @[ @NO, @YES ]) {
      LynxUIListScroll *list = [self list];
      [LynxPropsProcessor updateProp:batch
                             withKey:@"experimental-batch-render-strategy"
                               forUI:list];
      [LynxPropsProcessor updateProp:platform
                             withKey:@"enable-insert-platform-view-operation"
                               forUI:list];
      LynxUIComponent *component = [LynxUIComponent new];
      [list onAsyncComponentLayoutUpdated:component operationID:1];
      XCTAssertEqual(component.view.superview.superview == list.view,
                     !batch.boolValue && !platform.boolValue);
      [list removeListComponent:component];
      [LynxPropsProcessor updateProp:nil withKey:@"experimental-batch-render-strategy" forUI:list];
      [LynxPropsProcessor updateProp:nil
                             withKey:@"enable-insert-platform-view-operation"
                               forUI:list];
      [list onAsyncComponentLayoutUpdated:component operationID:2];
      XCTAssertEqual(component.view.superview.superview, list.view);
    }
  }
}

- (void)testNativeStateCachePreservesIdentityAndSupportsReplacement {
  LynxUIListScroll *list = [self list];
  LynxUI *child = [LynxUI new];
  NSMutableDictionary *cache = list.listNativeStateCache;
  NSObject *state = [NSObject new];
  [child storeKeyToNativeStorage:list key:@"item" value:state];
  XCTAssertEqual([child getNativeStorageFromList:list], cache);
  XCTAssertEqual(cache[@"item"], state);
  [child removeKeyFromNativeStorage:list key:@"item"];
  XCTAssertEqual(cache.count, 0U);
  XCTAssertEqual(list.listNativeStateCache, cache);
  list.listNativeStateCache = [NSMutableDictionary dictionary];
  XCTAssertNotEqual(list.listNativeStateCache, cache);
  [child storeKeyToNativeStorage:list key:@"new" value:state];
  XCTAssertEqual(list.listNativeStateCache[@"new"], state);
  list.listNativeStateCache = nil;
  XCTAssertNoThrow([child storeKeyToNativeStorage:list key:@"absent" value:state]);
  XCTAssertNil([child getNativeStorageFromList:list]);
}

- (void)testRestoreQueueIdentityDrainAndReentrantEnqueue {
  LynxUIListScroll *list = [self list];
  LynxUI *child = [LynxUI new];
  NSMutableArray *queue = [child getRestoreNativeStateBlockArrayFromList:list];
  __weak NSMutableArray *weakQueue = queue;
  XCTAssertEqual(queue, list.restoreNativeStateBlockArray);
  NSMutableArray *events = [NSMutableArray array];
  __weak LynxUIListScroll *weakList = list;
  [queue addObject:^{
    [events addObject:@1];
    XCTAssertNotEqual(weakList.restoreNativeStateBlockArray, weakQueue);
    [weakList.restoreNativeStateBlockArray addObject:^{
      [events addObject:@3];
    }];
  }];
  [queue addObject:^{
    [events addObject:@2];
  }];
  [list onNodeReady];
  XCTAssertEqualObjects(events, (@[ @1, @2 ]));
  [list onNodeReady];
  XCTAssertEqualObjects(events, (@[ @1, @2, @3 ]));
  [list onNodeReady];
  XCTAssertEqual(events.count, 3U);
}

- (void)testEventManagerIsCreatedOnceAndSharedWithSticky {
  LynxUIListScroll *list = [self list];
  [list eventDidSet];
  LynxScrollEventManager *manager = list.scrollEventManager;
  XCTAssertNotNil(manager);
  [list eventDidSet];
  XCTAssertEqual(list.scrollEventManager, manager);
  XCTAssertEqual([(id<LynxListStickyManagerOwner>)list eventManagerForListStickyManager], manager);
}

- (void)testFlushedPropsAreIsolatedByItemAndPropKey {
  LynxUIListScroll *list = [self list];
  XCTAssertFalse([list initialPropsFlushed:@"offset" cacheKey:@"first"]);
  [list setInitialPropsHasFlushed:@"offset" cacheKey:@"first"];
  [list setInitialPropsHasFlushed:@"offset" cacheKey:@"first"];
  XCTAssertTrue([list initialPropsFlushed:@"offset" cacheKey:@"first"]);
  XCTAssertFalse([list initialPropsFlushed:@"index" cacheKey:@"first"]);
  XCTAssertFalse([list initialPropsFlushed:@"offset" cacheKey:@"second"]);
  [list setInitialPropsHasFlushed:@"index" cacheKey:@"second"];
  [list onNodeReady];
  XCTAssertTrue([list initialPropsFlushed:@"offset" cacheKey:@"first"]);
  XCTAssertTrue([list initialPropsFlushed:@"index" cacheKey:@"second"]);
}

- (void)testStickyStartUsesBaseScrollCallbacksAndCompatibleEvents {
  LynxUIListScroll *list = [self list];
  LynxListScrollStickyEventRecorder *events = [LynxListScrollStickyEventRecorder new];
  events.events = [NSMutableArray array];
  list.scrollEventManager = events;
  [self configureSticky:list keys:@[ @"first", @"next" ] starts:@[ @0, @1 ] ends:@[]];
  [LynxPropsProcessor updateProp:@10 withKey:@"sticky-offset" forUI:list];
  LynxUIComponent *first = [self component:@"first" frame:CGRectMake(0, 0, 100, 40)];
  LynxUIComponent *next = [self component:@"next" frame:CGRectMake(0, 70, 100, 40)];
  [list insertChild:first atIndex:0];
  [list insertChild:next atIndex:1];
  [list insertListComponent:first];
  [list insertListComponent:next];
  list.view.contentOffset = CGPointMake(0, 20);
  [list.view.delegate scrollViewDidScroll:list.view];
  XCTAssertEqualWithAccuracy(first.view.superview.frame.origin.y, 30, 0.001);
  XCTAssertTrue(first.view.superview.layer.zPosition > first.zIndex);
  [list.view.delegate scrollViewDidScroll:list.view];
  XCTAssertEqualObjects([events.events valueForKey:@"name"],
                        (@[ LynxEventStickyTop, LynxEventStickyStart ]));
  XCTAssertEqualObjects(events.events[0][@"detail"], (@{@"top" : @"first"}));
  XCTAssertEqualObjects(events.events[1][@"detail"], (@{@"start" : @"first"}));
  XCTAssertEqual([list hitTest:CGPointMake(20, 35) withEvent:nil], first);
  list.view.contentOffset = CGPointMake(0, 50);
  [list.view.delegate scrollViewDidScroll:list.view];
  XCTAssertEqualWithAccuracy(first.view.superview.frame.origin.y, 30, 0.001);
}

- (void)testStickyEndLayoutAndDetachRestoreFrameAndZIndex {
  LynxUIListScroll *list = [self list];
  [self configureSticky:list keys:@[ @"last" ] starts:@[] ends:@[ @0 ]];
  LynxUIComponent *last = [self component:@"last" frame:CGRectMake(0, 230, 100, 30)];
  last.zIndex = 5;
  [list insertChild:last atIndex:0];
  [list insertListComponent:last];
  list.view.contentOffset = CGPointMake(0, 100);
  [list onNodeReady];
  XCTAssertEqualWithAccuracy(last.view.superview.frame.origin.y, 170, 0.001);
  last.frame = CGRectMake(0, 250, 100, 40);
  [last onNodeReady];
  XCTAssertEqualWithAccuracy(last.view.superview.frame.origin.y, 160, 0.001);
  UIView *wrapper = last.view.superview;
  [list removeListComponent:last];
  XCTAssertTrue(CGRectEqualToRect(wrapper.frame, last.frame));
  XCTAssertEqual(wrapper.layer.zPosition, 5);
  XCTAssertNil(wrapper.superview);
}

- (void)testStickyRemapAndReusedItemKey {
  LynxUIListScroll *list = [self list];
  [self configureSticky:list keys:@[ @"first", @"second" ] starts:@[ @0 ] ends:@[]];
  LynxUIComponent *component = [self component:@"first" frame:CGRectMake(0, 0, 100, 20)];
  [list insertListComponent:component];
  list.view.contentOffset = CGPointMake(0, 30);
  [list onNodeReady];
  XCTAssertEqualWithAccuracy(component.view.superview.frame.origin.y, 30, 0.001);
  [self configureSticky:list keys:@[ @"second", @"first" ] starts:@[ @0 ] ends:@[]];
  XCTAssertEqualWithAccuracy(component.view.superview.frame.origin.y, 0, 0.001);
  component.itemKey = @"second";
  // The renderer reattaches the reused component, even when its wrapper is already present.
  [list insertListComponent:component];
  [list onComponentLayoutUpdated:component];
  XCTAssertEqualWithAccuracy(component.view.superview.frame.origin.y, 30, 0.001);
  [LynxPropsProcessor updateProp:nil withKey:@"list-container-info" forUI:list];
  [list propsDidUpdate];
  XCTAssertEqualWithAccuracy(component.view.superview.frame.origin.y, 0, 0.001);
}

- (void)testHorizontalViewportAndOrientationProperties {
  LynxUIListScroll *list = [self list];
  [LynxPropsProcessor updateProp:@NO withKey:@"vertical-orientation" forUI:list];
  XCTAssertFalse(list.view.verticalOrientation);
  [self configureSticky:list keys:@[ @"first", @"second" ] starts:@[ @0 ] ends:@[]];
  LynxUIComponent *first = [self component:@"first" frame:CGRectMake(0, 0, 40, 100)];
  LynxUIComponent *second = [self component:@"second" frame:CGRectMake(60, 0, 40, 100)];
  [list insertChild:first atIndex:0];
  [list insertChild:second atIndex:1];
  [list insertListComponent:second];
  [list insertListComponent:first];
  list.view.contentOffset = CGPointMake(10, 0);
  [list onNodeReady];
  // The shared sticky manager supports horizontal start/end as well as vertical top/bottom.
  XCTAssertEqualWithAccuracy(first.view.superview.frame.origin.x, 10, 0.001);
  XCTAssertEqualObjects([list.visibleCells valueForKeyPath:@"holdingUI.itemKey"],
                        (@[ @"first", @"second" ]));
  XCTAssertEqual(list.visibleCellsInfo.count, 2U);
  XCTAssertEqual([list getIndexFromItemKey:@"second"], 1);
  XCTAssertEqual([list hitTest:CGPointMake(75, 20) withEvent:nil], second);
  XCTAssertEqual([list hitTest:CGPointMake(200, 200) withEvent:nil], list);
  [LynxPropsProcessor updateProp:@"vertical" withKey:@"scroll-orientation" forUI:list];
  XCTAssertTrue(list.view.verticalOrientation);
  [LynxPropsProcessor updateProp:@"horizontal" withKey:@"scroll-orientation" forUI:list];
  XCTAssertFalse(list.view.verticalOrientation);
  [LynxPropsProcessor updateProp:nil withKey:@"scroll-orientation" forUI:list];
  XCTAssertTrue(list.view.verticalOrientation);
}

- (void)testRendererDispatchAndContentSizeUseExistingHelpers {
  LynxUIListScroll *list = [self list];
  LynxUIComponent *component = [self component:@"item" frame:CGRectMake(0, 0, 100, 40)];
  LynxUIOwner *owner = [LynxUIOwner new];
  owner.uiHolder = [@{@1 : list, @2 : component} mutableCopy];
  [owner insertListComponent:1 componentSign:2];
  XCTAssertEqual(component.view.superview.superview, list.view);
  [owner removeListComponent:1 componentSign:2];
  XCTAssertNil(component.view.superview);
  [owner updateContentOffsetForListContainer:1 contentSize:600 deltaX:0 deltaY:10];
  XCTAssertTrue(list.needAdjustContentOffset);
  [list onNodeReady];
  XCTAssertFalse(list.needAdjustContentOffset);
  XCTAssertEqualWithAccuracy(list.view.contentSize.height, 600, 0.001);
  XCTAssertTrue(CGPointEqualToPoint(list.targetDelta, CGPointZero));
  [owner updateScrollInfo:1 estimatedOffset:80 smooth:NO scrolling:NO];
  XCTAssertEqualWithAccuracy(list.view.contentOffset.y, 80, 0.001);
  XCTAssertEqual(list.view.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
  [owner updateScrollInfo:1 estimatedOffset:100 smooth:YES scrolling:YES];
  XCTAssertEqualWithAccuracy(list.view.contentOffset.y, 80, 0.001);
  XCTAssertEqual(list.view.scrollEstimatedOffset, 100);
  [LynxPropsProcessor updateProp:@"horizontal" withKey:@"scroll-orientation" forUI:list];
  [owner updateScrollInfo:1 estimatedOffset:60 smooth:NO scrolling:NO];
  XCTAssertEqualWithAccuracy(list.view.contentOffset.x, 60, 0.001);
}

- (void)testProgrammaticStateCompletionAndNoOpClearEstimatedOffset {
  LynxUIListScroll *list = [self list];
  [list updateScrollInfoWithEstimatedOffset:0 smooth:YES scrolling:NO];
  XCTAssertEqual(list.view.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
  [list updateScrollInfoWithEstimatedOffset:80 smooth:YES scrolling:NO];
  [list.view tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  XCTAssertEqual(list.currentScrollState, LynxBaseScrollViewScrollStateIdle);
  XCTAssertEqual(list.view.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
}

- (void)testRestoreTasksAndHelpersDoNotRetainTheList {
  __weak LynxUIListScroll *weakList;
  __weak NSObject *weakState;
  LynxListScrollComponentWrapper *wrapper = [LynxListScrollComponentWrapper new];
  @autoreleasepool {
    LynxUIListScroll *list = [self list];
    weakList = list;
    NSObject *state = [NSObject new];
    weakState = state;
    [list.restoreNativeStateBlockArray addObject:^{
      XCTAssertNotNil(state);
    }];
    [list onNodeReady];
    LynxUIComponent *component = [LynxUIComponent new];
    wrapper.holdingUI = component;
  }
  XCTAssertNil(weakList);
  XCTAssertNil(weakState);
  XCTAssertNil(wrapper.holdingUI);
}

- (void)testPendingAnimationUsesSharedTimeoutWithoutStateReentry {
  LynxUIListScroll *list = [self list];
  id viewMock = OCMPartialMock(list.view);
  @try {
    // Model UIKit accepting the request without delivering an animation-end callback.
    OCMStub([viewMock setContentOffset:CGPointMake(0, 60) animated:YES]);
    [list updateScrollInfoWithEstimatedOffset:60 smooth:YES scrolling:NO];
    XCTAssertEqual(list.currentScrollState, LynxBaseScrollViewScrollStateAnimating);
    XCTAssertEqual(list.view.scrollEstimatedOffset, 60);
    XCTestExpectation *finished = [self expectationWithDescription:@"shared timeout"];
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, 800 * NSEC_PER_MSEC), dispatch_get_main_queue(), ^{
          XCTAssertEqual(list.currentScrollState, LynxBaseScrollViewScrollStateIdle);
          XCTAssertEqual(list.view.scrollEstimatedOffset, LynxListScrollInvalidEstimatedOffset);
          [finished fulfill];
        });
    [self waitForExpectations:@[ finished ] timeout:2];
  } @finally {
    [viewMock stopMocking];
  }
}

@end
