// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIScroller.h>

@class LynxUIComponent;

typedef void (^RestoreNativeStateBlock)(void);

/** Applies parent-owned visual transformations to attached list-item wrappers. */
@protocol LynxListItemTransformer <NSObject>

/**
 * Applies a visual transformation to one attached list item.
 *
 * @param listContainerView The scroll view presenting the item.
 * @param itemView The attached list-item wrapper to transform.
 * @param isVertical Whether the list's main scroll axis is vertical.
 * @param isRTL Whether the list uses right-to-left layout. The vertical main axis is unchanged.
 * @param mainAxisOffset The signed distance from the viewport's logical start to the item's
 *                       logical start, in points. Uses top edges for vertical lists, left edges
 *                       for horizontal LTR lists, and right edges for horizontal RTL lists.
 *                       Positive values follow the layout direction. Horizontal translations
 *                       calculated along this logical axis must be negated for RTL before
 *                       applying them to the view's physical X axis.
 */
- (void)transformItemInListContainer:(UIScrollView *_Nonnull)listContainerView
                            itemView:(UIView *_Nonnull)itemView
                          isVertical:(BOOL)isVertical
                               isRTL:(BOOL)isRTL
                      mainAxisOffset:(CGFloat)mainAxisOffset;

/** Clears visual properties previously written by this transformer. */
- (void)resetItem:(UIView *_Nonnull)itemView;

@end

@interface LynxListContainerComponentWrapper : UIView
@property(nonatomic, weak, nullable) LynxUIComponent *holdingUI;
@end

@protocol LynxUIListContainerDelegate <NSObject>

- (void)insertListComponent:(LynxUIComponent *_Nonnull)component
                    wrapper:(LynxListContainerComponentWrapper *_Nullable)wrapper;

- (void)removeListComponent:(LynxUIComponent *_Nonnull)component;

@end

@interface LynxUIListContainer : LynxUIScroller <LynxUIComponentLayoutObserver>
// Mark c++ has updated contentSize and contentOffset
@property(nonatomic, assign) BOOL needAdjustContentOffset;
// Target delta from c++
@property(nonatomic, assign) CGPoint targetDelta;
// Target contentSize from c++
@property(nonatomic, assign) CGFloat targetContentSize;
@property(nonatomic, strong, nullable) NSMutableDictionary *listNativeStateCache;
// <cacheKey, Set<initial- props>> Stores flushed initial- props for cacheKey
@property(nonatomic, strong, nullable)
    NSMutableDictionary<NSString *, NSMutableSet<NSString *> *> *initialFlushPropCache;
@property(nonatomic, assign) CGFloat pagingAlignFactor;
@property(nonatomic, assign) CGFloat pagingAlignOffset;
@property(nonatomic, strong, nullable)
    NSMutableArray<RestoreNativeStateBlock> *restoreNativeStateBlockArray;

@property(nonatomic, weak, nullable) id<LynxUIListContainerDelegate> delegate;
/** Replacing this property resets the old transformer before applying the new one. */
@property(nonatomic, strong, nullable) id<LynxListItemTransformer> listItemTransformer;

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
/** Reapplies the current transformer without requiring the list to scroll. */
- (void)requestListItemTransform;
- (void)insertListComponent:(LynxUIComponent *_Nonnull)component;
- (void)removeListComponent:(LynxUIComponent *_Nonnull)component;
- (void)detachedFromWindow;

- (NSInteger)getIndexFromItemKey:(NSString *_Nullable)itemKey;
- (void)scrollToPosition:(NSInteger)position
                  offset:(float)offset
                   align:(int)align
                  smooth:(BOOL)smooth;
- (NSArray<LynxListContainerComponentWrapper *> *_Nullable)visibleCells;

@end
