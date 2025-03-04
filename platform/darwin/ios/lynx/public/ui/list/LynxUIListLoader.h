// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListDebug.h"
#import "LynxUI.h"
#import "LynxUIComponent.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIListDiffResult : NSObject

/// The indexPaths that will be removed after updates. The `removePaths` are the positions as they
/// were before the update.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *removePaths;

/// The indexPaths that will be inserted after updates. The `insertPaths` are the positions as they
/// are after the update. an insertion indicates that you could either update a lynxUI that has the
/// same reuseIdentifier to the insertionPaths or render a new one.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *insertPaths;

/// The indexPaths that wil be updated from. The `updateFromPaths` are the positions as they were
/// before the update. It must have the same count as that of `updateToPaths`, the corresponding
/// element in `updateToPaths` which has the same index indicates where this element will be updated
/// to. an update indicates that you must call `updateLynxUI:toIndexPath:` to update the ui.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *updateFromPaths;
/// The indexPaths that wil be updated to. The `updateToPaths` are the positions as they are after
/// the update.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *updateToPaths;

/// The indexPaths that wil be moved from. The `moveFromPaths` are the positions as they were before
/// the update. It must have the same count as that of  `moveToPaths`, the corresponding element in
/// `moveToPaths` which has the same index indicates where this element will be moved to. a move
/// indicates that you do not need to call `updateLynxUI:toIndexPath:` to update the ui.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *moveFromPaths;
/// The indexPaths that wil be moved to. The `moveToPaths` are the positions as they are after the
/// update.
@property(nonatomic, readonly) NSArray<NSIndexPath *> *moveToPaths;

/// The `LynxUIListDiffResult` is empty when all its `____paths` are empty.
@property(nonatomic, readonly, getter=isEmpty) BOOL empty;

#if defined(LYNX_LIST_DEBUG)
- (NSString *)description;
#endif

@end

@interface LynxUIListLoader<__covariant V : UIView *> : LynxUI <V> <LynxUIComponentLayoutObserver>

/**
 * The number of elements in the list. This property is updated after `propsDidUpdate`.
 * The value of this property is always the same the count of reuseIdentifiers.
 */
@property(nonatomic, readonly) NSUInteger count;

/**
 * Whether the list is diffable or not.
 *
 * The list is always NOT diffable on its first `propsDidUpdate`.
 * The list is diffable unless it is set to be not diffable in its props.
 */
@property(nonatomic, readonly, getter=isDiffable) BOOL diffable;

/**
 * If the list is diffable, then the the property `diffResult` contains the diff result that could
 * transform the list before updates to the list after updates. Otherwise, it is `nil`.
 */
@property(nonatomic, nullable, readonly) LynxUIListDiffResult *diffResult;

/**
 * Index for elements in the list that is under the `<header>`, `<footer>`, or `<row>` tags.
 */
@property(nonatomic, nullable, readonly) NSMutableArray<NSIndexPath *> *fullSpanItems;

/**
 * Index for elements in the list with 'sticky-top'.
 */
@property(nonatomic, nullable, readonly) NSMutableArray<NSIndexPath *> *stickyTopItems;

/**
 * Index for elements in the list with 'sticky-bottom'.
 */
@property(nonatomic, nullable, readonly) NSMutableArray<NSIndexPath *> *stickyBottomItems;

/**
 * Full-span item or sticky item changed.
 */
@property(nonatomic, readonly) BOOL elementTypeUpdate;

/**
 * Index for fiber  elements in the list that is under the `<header>`, `<footer>`, or `<row>` tags.
 */
@property(nonatomic, nullable, readonly) NSMutableArray *fiberFullSpanItems;

/**
 * Index for fiber elements in the list with 'sticky-top'.
 */
@property(nonatomic, nullable, readonly) NSMutableArray *fiberStickyTopItems;

/**
 * Index for fiber elements in the list with 'sticky-bottom'.
 */
@property(nonatomic, nullable, readonly) NSMutableArray *fiberStickyBottomItems;

/**
 * A Dictionary for reuseIdentifier for the element at some indexPath.
 * The reuseidentifier is the same as the name of component.
 */
@property(nonatomic, nullable, readonly) NSMutableArray<NSString *> *reuseIdentifiers;

/**
 * A Set for all itemKey in list.
 */
@property(nonatomic, nullable, readonly) NSMutableArray<NSString *> *currentItemKeys;

/**
 * A Dictionary for estimated heights for the element at some indexPath.
 * estimated heights are pass by key: `estimated-height` in components' props
 * if not specified, it will be assign a negative number
 */
@property(nonatomic, nullable, readonly)
    NSMutableDictionary<NSIndexPath *, NSNumber *> *estimatedHeights;

/**
 * @abstract Render the LynxUI at the given indexPath.
 *
 * @param indexPath a indexPath. The section of the `indexPath` must be 0. The row of the
 * `indexPath` indicates its index in the list.
 * @return the `LynxUI` for the indexPath.
 * @throw If the indexPath not smaller than the `count`, an exception will be thrown.
 */
- (LynxUI *)renderLynxUIAtIndexPath:(NSIndexPath *)indexPath;

/**
 * @abstract Update an LynxUI to the given indexPath.
 *
 * @param lynxUI  a `LynxUI` returned by `renderLynxUIAtIndexPath:` of this `LynxUIListLoader`.
 * @param indexPath an `NSIndexPath` that you want the `lynxUI` to be updated to.
 * The section of the `indexPath` must be 0. The row of the `indexPath` indicates its index in the
 * list. The reuseIdentifier of the indexPath must be the same as that of the provided `lynxUI` was
 * rendered before.
 * @throw If the indexPath not smaller than the `count`.
 * @throw If the LynxUI is not returned by `renderLynxUIAtIndexPath:` of this `LynxUIListLoader`.
 */
- (void)updateLynxUI:(LynxUI *)lynxUI toIndexPath:(NSIndexPath *)indexPath;

#pragma mark - List New Arch APIs

/**
 *  @abstract flag indicates whether the list uses the new archtecture. the new arch calls
 * `uiAtIndexPath:` in `collectionView:cellForIndexAtPath:` to get the LynxUI and calls
 * `recycleLynxUI:` while the cell is no longer needed, e.g. `collectionView:didEndDisplayCell:`
 */

@property(nonatomic, readonly, getter=isNewArch) BOOL newArch;

@property(nonatomic) BOOL needsInternalCellAppearNotification;
@property(nonatomic) BOOL needsInternalCellDisappearNotification;
@property(nonatomic) BOOL needsInternalCellPrepareForReuseNotification;
/**
 * @abstract get the LynxUI at specified `indexPath`, the `LynxUI` returned might be created,
 * reused, or updated.
 *
 * @param indexPath the index for required `LynxUI`
 * @return the `LynxUI` at given indexPath
 */
- (LynxUI *)uiAtIndexPath:(NSIndexPath *)indexPath;
- (void)asyncUIAtIndexPath:(NSIndexPath *)indexPath operationID:(int64_t)operationID;
/**
 * @abstract Recycle the LynxUI
 *
 * @param ui the `LynxUI` that will be marked as recycled, its element might be reused on subsequent
 * calls to `uiAtIndexPath`
 */
- (void)recycleLynxUI:(LynxUI *)ui;
- (void)asyncRecycleLynxUI:(LynxUI *)ui;
/**
 * @abstract apply diff result and component infos
 *
 * @param diffResult the diff info dispatched from tasm
 * @param components all component info dispatched from tasm
 */
- (void)loadListInfo:(NSDictionary *)diffResult
          components:(NSDictionary<NSString *, NSArray *> *)components;

/**
 * @abstract update list nodiff  action info
 *
 * @param noDiffResult  no diff info dispatched from tasm

 */
- (void)updateListActionInfo:(NSDictionary *)noDiffResult;

- (void)markIsNewArch;

- (BOOL)isAsync;

- (BOOL)isPartOnLayout;
@end

NS_ASSUME_NONNULL_END
