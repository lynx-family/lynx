// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionDataSource.h"
#import "LynxCollectionInvalidationContext.h"
#import "LynxCollectionScroll.h"
#import "LynxCollectionViewCell.h"
#import "LynxListAppearEventEmitter.h"
#import "LynxUICollection+Delegate.h"
#import "LynxUICollection+Internal.h"
#import "LynxUICollection.h"
#import "LynxUIListLoader.h"

@interface LynxCollectionDataSource ()
@property(nonatomic, weak, readonly) LynxUICollection *collection;
@property(nonatomic, assign) NSUInteger operationIDCount;
@property(nonatomic) NSInteger itemCount;
// mark whether `applyFirstTime` has been called to avoid the
// `layoutDidFinished` --> `applyFirstTime` --> `performBatchUpdate` --> `cellForItemAtIndexPath:`
// --> `layoutDidFinished` cycle.
@property(nonatomic) BOOL firstTime;
@end

@implementation LynxCollectionDataSource

- (instancetype)initWithLynxUICollection:(LynxUICollection *)collection {
  self = [super init];
  if (self) {
    _collection = collection;
    _firstTime = YES;
  }
  return self;
}

// this function will only be called in `layoutDidFinished` during the first screen of the
// LynxUICollection We could not call `applyFirstTime` in `propsDidUpdate`. This is because, during
// the first screen, `propsDidUpdate` is called before the layout of LynxUICollection is done.
// Without the layout of the LynxUICollection, the UICollectionView inside will have a default zero
// `frame.size`, which prevents it from loading cells. Cells that are not loaded in this layout
// cycle will be loaded in the call of the next `performBatchUpdate`. They are loaded in an
// `_updateVisibleCellsNow:` method that is called before the invocation of the block you provided
// as param. However, at this time, the ListComponentInfo vector in C++ has already been updated to
// a new state. Therefore, we notify the iOS only after we have already prepared the layout of the
// UICollectionView. Make sure that cells of the first screen are loaded and states are sync'ed.
- (BOOL)applyFirstTime {
  if (_firstTime) {
    _firstTime = NO;
    [self registerAllReuseIdentifiers];
    [self performBatchUpdatesFirstTime];
    return YES;
  } else {
    return NO;
  }
}

// call this method to get an invalidation context that could invalidate everthing in the layout.
// this is used on the first `setData` or before `reloadData`
- (LynxCollectionInvalidationContext *)invalidationContextForForcedItemCountChanging {
  NSUInteger numberOfItems = _collection.count;
  LynxCollectionInvalidationContext *context = [[LynxCollectionInvalidationContext alloc]
      initWithNumberOfItemsChanging:numberOfItems
                      fullSpanItems:_collection.fullSpanItems
                     stickyTopItems:_collection.stickyTopItems
                  stickyBottomItems:_collection.stickyBottomItems
                   estimatedHeights:_collection.estimatedHeights];

  return context;
}

// this function will be called in `propsDidUpdate` triggerred by `setData`
- (void)apply {
  if (_firstTime && !_collection.isDiffable) {
    return;
  }

  [self registerAllReuseIdentifiers];

  // LynxUICollection might be updated while it is not in any `window`
  // While the UICollectionView does not have a `window`, performBatchUpdates will not do all the
  // updates until it has a `window` again. This behaviour depends on recording all the intermediate
  // states. Use reloadData instead.
  LynxCollectionInvalidationContext *context;

  if (_collection.elementTypeUpdate && [self isEmptyDataUpdates]) {
    context = [[LynxCollectionInvalidationContext alloc]
        initWithFullSpanItems:_collection.fullSpanItems
               stickyTopItems:_collection.stickyTopItems
            stickyBottomItems:_collection.stickyBottomItems
             estimatedHeights:_collection.estimatedHeights];
  } else {
    context = [[LynxCollectionInvalidationContext alloc]
         initWithRemovals:_collection.diffResult.removePaths
               insertions:_collection.diffResult.insertPaths
                 moveFrom:_collection.diffResult.moveFromPaths
                   moveTo:_collection.diffResult.moveToPaths
            fullSpanItems:_collection.fullSpanItems
           stickyTopItems:_collection.stickyTopItems
        stickyBottomItems:_collection.stickyBottomItems
         estimatedHeights:_collection.estimatedHeights];
  }
  if (self.collection.reloadAll) {
    self.collection.reloadAll = NO;
    [self updateItemCount];
    [_collection.view reloadData];
    [_collection.view.collectionViewLayout
        invalidateLayoutWithContext:[self invalidationContextForForcedItemCountChanging]];
  } else if (!_collection.view.window) {
    [self updateItemCount];
    [_collection.view.collectionViewLayout invalidateLayoutWithContext:context];
    LYNX_LIST_DEBUG_LOG(@"reload with diffResult: %@", _collection.diffResult);
    [_collection.view reloadData];
    if (_collection.forceReloadData) {
      NSMutableArray<NSIndexPath *> *indexPathsToBeReload = [[NSMutableArray alloc] init];
      for (NSIndexPath *path in [_collection.view indexPathsForVisibleItems]) {
        if (path.item < _itemCount) {
          [indexPathsToBeReload addObject:path];
        }
      }
      [_collection.view reloadItemsAtIndexPaths:indexPathsToBeReload];
    }
  } else {
    self.ignoreLoadCell = YES;
    [self performBatchUpdatesWithInvaildationContext:context];
    self.ignoreLoadCell = NO;
  }
}

- (void)registerAllReuseIdentifiers {
  [_collection.reuseIdentifiers
      enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        [self.collection.view registerClass:[LynxCollectionViewCell class]
                 forCellWithReuseIdentifier:obj];
      }];
}

- (void)performBatchUpdatesFirstTime {
  LYNX_LIST_DEBUG_LOG(@"count: %@", @(_collection.count));
  [self performBatchUpdatesWithInvaildationContext:
            (LynxCollectionInvalidationContext *)
                [self invalidationContextForForcedItemCountChanging]];
}

- (void)performBatchUpdatesWithInvaildationContext:(LynxCollectionInvalidationContext *)context {
  if ([_collection.diffResult isEmpty]) {
    return;
  }
  LYNX_LIST_DEBUG_LOG(@"diffResult: %@", _collection.diffResult);
  if (_collection.debugInfoLevel >= LynxListDebugInfoLevelInfo &&
      [_collection shouldGenerateDebugInfo]) {
    [_collection sendListDebugInfoEvent:[NSString stringWithFormat:@"diffResult: %@",
                                                                   _collection.diffResult]];
  }

  void (^updates)(void) = ^{
    LYNX_LIST_DEBUG_LOG(@"update block did invoked");
    self.ignoreLoadCell = NO;
    [self updateItemCount];
    [self updateWithDiffResultApplyInvalidationContext:context];
  };

  typeof(self) __weak weakSelf = self;
  void (^sendLayoutComplete)(BOOL) = ^(BOOL completed) {
    typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf == nil) {
      return;
    }
    [strongSelf.collection sendLayoutCompleteEvent];
  };

  [_collection performBatchUpdates:updates
                        completion:sendLayoutComplete
                          animated:_collection.enableUpdateAnimation];

  // `updateFrame:withPadding:border:margin:withLayoutAnimation:` might be called during
  // `performBatchUpdates` However, iOS somehow ignores this frame change and set the bounds of the
  // collections using the stale one we have to set the frame after `performBatchUpdates` to avoid
  // this.
  if (CATransform3DIsIdentity(_collection.view.layer.transform)) {
    [_collection.view setFrame:_collection.frame];
  } else {
    CGRect bounds = _collection.frame;
    bounds.origin = _collection.view.bounds.origin;
    _collection.view.bounds = bounds;
    _collection.view.center =
        CGPointMake(_collection.frame.origin.x + _collection.frame.size.width / 2,
                    _collection.frame.origin.y + _collection.frame.size.height / 2);
  }
  LynxCollectionInvalidationContext *boundsContext =
      [[LynxCollectionInvalidationContext alloc] initWithBoundsChanging:_collection.frame];
  [_collection.view.collectionViewLayout invalidateLayoutWithContext:boundsContext];
}

- (void)updateWithDiffResultApplyInvalidationContext:(LynxCollectionInvalidationContext *)context {
  LynxUIListDiffResult *diffResult = _collection.diffResult;
  UICollectionView *collectionView = _collection.view;

  // invalidate the layout using diff result
  if (context) {
    [collectionView.collectionViewLayout invalidateLayoutWithContext:context];
  }

  // try to scroll to the indexPath given by `initial-scroll-index`
  [self.collection.scroll initialScrollCollectionViewIfNeeded:self.collection.layout];

  // first delete
  [collectionView deleteItemsAtIndexPaths:diffResult.removePaths];
  // second insert
  [collectionView insertItemsAtIndexPaths:diffResult.insertPaths];
  // Third Move
  for (size_t i = 0; i < [diffResult.moveFromPaths count]; ++i) {
    [collectionView moveItemAtIndexPath:[diffResult.moveFromPaths objectAtIndex:i]
                            toIndexPath:[diffResult.moveToPaths objectAtIndex:i]];
  }
  // Finally Update
  // not using reloadItemsAtIndexPaths because UICollectionView does not treat the cells
  // updated as reusable during batchUpdate and thus a new component will be created.
  // notice that we do not trigger __nodeappear__ or __nodedisappear__ events when updating
  // We circurvent iOS by get the cell from visibleCell and calling UpdateComponent by
  // ourselves instead of calling dequeueReusableCellWithReuseIdentifier:forIndexPath:
  for (size_t i = 0; i < [diffResult.updateFromPaths count]; ++i) {
    NSIndexPath *fromPath = [diffResult.updateFromPaths objectAtIndex:i];
    NSIndexPath *toPath = [diffResult.updateToPaths objectAtIndex:i];
    // A cell that needs update must be a visibleCell
    // if the cell becomes __VISIBILE__ later, the collectionView:cellForItemAtIndexPath:
    // will be called and the cell will thus be updated during the call.
    // The cell object at the corresponding index path is nil if the cell is not visible
    // or indexPath is out of range.
    UICollectionViewCell *uiCell = [collectionView cellForItemAtIndexPath:fromPath];
    if (uiCell == nil || uiCell.hidden) {
      continue;
    }

    LynxCollectionViewCell *cell = (LynxCollectionViewCell *)uiCell;
    cell.updateToPath = toPath;
    cell.loading = YES;
    [_collection.appearEventCourier onCellDisappear:cell.ui atIndexPath:fromPath];
    [self loadCell:cell forIndexPath:toPath];
    [_collection.appearEventCourier onCellAppear:cell.ui atIndexPath:toPath];
    cell.updateToPath = nil;
    cell.loading = NO;
  }
}

- (nonnull __kindof UICollectionViewCell *)collectionView:(nonnull UICollectionView *)collectionView
                                   cellForItemAtIndexPath:(nonnull NSIndexPath *)indexPath {
  NSString *reuseIdentifier = _collection.reuseIdentifiers[indexPath.item];
  LynxCollectionViewCell *cell = (LynxCollectionViewCell *)[collectionView
      dequeueReusableCellWithReuseIdentifier:reuseIdentifier
                                forIndexPath:indexPath];
  cell.isPartOnLayout = [self.collection isPartOnLayout];
  cell.contentView.transform = _collection.enableRtl && _collection.isRtl
                                   ? CGAffineTransformMakeScale(-1.0, 1)
                                   : CGAffineTransformMakeScale(1.0, 1.0);
  LYNX_LIST_DEBUG_LOG(@"row: %@", @(indexPath.row));
  [self loadCell:cell forIndexPath:indexPath];

  return cell;
}

- (int64_t)generateOperationId {
  return (((int64_t)(self.collection.sign)) << 32) + (int64_t)(self.operationIDCount++);
}

- (void)loadCell:(LynxCollectionViewCell *)cell forIndexPath:(NSIndexPath *)indexPath {
  cell.updateToPath = indexPath;
  cell.loading = YES;
  if (_collection.isNewArch) {
    // The reuse pool maintained by iOS `dequeueReusableCell` does not hold the lynxUI anymore.
    // cell.ui will be recycled on its disappearing. `dequeueReusableCell` exists for providing
    // `UICollectionViewCell` only.
    if ([self.collection isAsync]) {
      BOOL sameItemKey = indexPath.item < (NSInteger)self.collection.currentItemKeys.count &&
                         [cell.ui.itemKey
                             isEqualToString:self.collection.currentItemKeys[indexPath.item]];
      if (cell.ui && !sameItemKey) {
        LynxUI *lynxUI = [cell removeLynxUI];
        [self.collection asyncRecycleLynxUI:lynxUI];
      }
      int64_t operationID = [self generateOperationId];
      cell.operationID = operationID;
      [self.collection asyncUIAtIndexPath:indexPath operationID:operationID];
      cell.loading = NO;
    } else {
      LynxUI *lynxUI = [self.collection uiAtIndexPath:indexPath];
      if (cell.ui != lynxUI) {
        // recycle original ui if needed
        LynxUI *oriUI = cell.ui;
        if (oriUI) {
          [cell removeLynxUI];
          [self.collection recycleLynxUI:oriUI];
        }
        [cell addLynxUI:lynxUI];
      }
    }

  } else {
    if (cell.ui) {
      LYNX_LIST_DEBUG_LOG(@"update at %@", @(indexPath.row));
      if (self.collection.needsInternalCellPrepareForReuseNotification) {
        // item-key pass-through only supported in new-arch
        [cell.ui onListCellPrepareForReuse:nil withList:self.collection];
      }
      [_collection updateLynxUI:cell.ui toIndexPath:indexPath];
      [cell adjustComponentFrame];
      [cell restartAnimation];
    } else {
      LYNX_LIST_DEBUG_LOG(@"render at %@", @(indexPath.row));
      [cell addLynxUI:[_collection renderLynxUIAtIndexPath:indexPath]];
    }
  }
  [_collection.scroll updateWithCellLoadedAtIndexPath:indexPath];
  cell.updateToPath = nil;
  cell.loading = NO;
}

- (BOOL)isEmptyDataUpdates {
  if (_collection.diffResult.removePaths.count > 0) {
    return NO;
  }
  if (_collection.diffResult.insertPaths.count > 0) {
    return NO;
  }
  if (_collection.diffResult.moveToPaths.count > 0) {
    return NO;
  }
  if (_collection.diffResult.moveFromPaths.count > 0) {
    return NO;
  }
  return YES;
}

- (void)updateItemCount {
  _itemCount = _collection.count;
}

- (NSInteger)collectionView:(nonnull UICollectionView *)collectionView
     numberOfItemsInSection:(NSInteger)section {
  return _itemCount;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
  return 1;
}

@end
