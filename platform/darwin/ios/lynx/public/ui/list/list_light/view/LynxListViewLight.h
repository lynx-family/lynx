// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIListInvalidationContext.h>
#import <Lynx/LynxUIListProtocol.h>

@class LynxListLayoutManager;
@class LynxUIListDataSource;
@class LynxUIContext;
@class LynxUIComponent;
@class LynxEventEmitter;
@protocol LynxEventTarget;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxAnchorVisibility) {
  LynxAnchorVisibilityNoAdjustment,
  LynxAnchorVisibilityShow,
  LynxAnchorVisibilityHide,
};

@interface LynxListViewLight : UIScrollView <LynxListEventsProtocol>
@property(nonatomic, assign)
    NSInteger numberOfColumns;  // numberOfColumns must be initialized to a non-zero value.
@property(nonatomic, assign) BOOL verticalOrientation;  // Horizontal / Vertical Layout
@property(nonatomic, assign) NSInteger preloadBufferCount;
/**
 Anchor related properties.
 */
@property(nonatomic, assign) BOOL anchorPriorityFromBegin;
@property(nonatomic, assign) BOOL deleteRegressPolicyToTop;
@property(nonatomic, assign) BOOL insertAnchorModeInside;

/**
 The properties to consider when adjusting contentOffset based on an anchor.
 */
@property(nonatomic, assign) LynxAnchorVisibility anchorVisibility;
@property(nonatomic, assign) BOOL anchorAlignToBottom;
@property(nonatomic, assign) BOOL isAsync;

/**
 Animation related properties.
 */
@property(nonatomic, assign) BOOL enableFadeInAnimation;
@property(nonatomic, assign) CGFloat updateAnimationFadeInDuration;

/**
 Set the layout, dataSource and UISign that needs to render the list.
 */
- (void)setLayout:(LynxListLayoutManager *_Nullable)layout;
- (void)setDataSource:(LynxUIListDataSource *)dataSource;
- (void)setSign:(NSInteger)sign;
- (void)setUIContext:(LynxUIContext *)context;
- (void)setEventEmitter:(LynxEventEmitter *)eventEmitter;
- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with;

/**
 Update scrollThresholds for internal scrollManager.
 @param scrollThreSholds - settings from propSetter.
 */
- (void)updateScrollThresholds:(LynxUIListScrollThresholds *)scrollThreSholds;

/**
 Dispatch different types of invalidationContext to different process. The main entry for every
 updates to list.
 @param context - The update infomation.
 */
- (void)dispatchInvalidationContext:(LynxUIListInvalidationContext *)context;

/**
 @return All cells in visible area. Not including those in the preloaded cache.
 */
- (NSArray<id<LynxListCell>> *)visibleCells;

/*
 Replace the old reuseIdentifiers to new ones.
 */
- (void)updateReuseIdentifiers:(NSArray<NSString *> *)reuseIdentifiers;

/**
 Register reuseIdentifiers with cellClass to create a <reuseIdentifier, cellClass> map in listView
 @param cellClass - The cell class that can be reused in a listView and conform to the
 <LynxListCell> protocol
 @param reuseIdentifiers - All reuseIdentifiers from this diff.
 */
- (void)registerCellClass:(Class)cellClass reuseIdentifiers:(NSArray<NSString *> *)reuseIdentifiers;

/**
 Update the layout of all visible cells with an updateToPath value greater than the given value.
 @param index - invalidation start index.
 */
- (void)invalidLayoutFromIndex:(NSInteger)index;

/**
 Dequeue a reusable cell at certain position from reusePool with reuseIdentifier.
 @param index - Cell position. Will be translated to reuseIdentifier inside.
 @return An empty, reusable cell needs to fill with.
 */
- (id<LynxListCell>)dequeueReusableCellForIndex:(NSInteger)index;

- (void)onAsyncComponentLayoutUpdated:(nonnull LynxUIComponent *)component
                          operationID:(int64_t)operationID;
- (void)onComponentLayoutUpdated:(LynxUIComponent *)component;
- (id<LynxListCell>)visibleCellAtPoint:(CGPoint)point;

- (id<LynxEventTarget>)findHitTestTarget:(CGPoint)point withEvent:(UIEvent *)event;
@end

NS_ASSUME_NONNULL_END
