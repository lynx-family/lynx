// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionScroll.h"
#import "LynxListDebug.h"
#import "LynxUICollection.h"
#import "UIScrollView+LynxGesture.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxListAppearEventEmitter;
@class LynxUIListDelegate;
@class LynxCollectionViewLayout;
@class LynxCollectionDataSource;
@class LynxListScrollEventEmitter;
@protocol LynxUIListDelegate;

static const CGFloat kInvalidSnapFactor = -1;

typedef NS_ENUM(NSUInteger, LynxCollectionCellUpdateAnimationType) {
  // The size of the size will be updated without animation, but other cells still have animation.
  // In this case, the cell may collides with others while animated.
  LynxCollectionCellUpdateAnimationTypeNone,
  // While updating the size of the cell, the animation of the collectionView is disabled.
  LynxCollectionCellUpdateAnimationTypeDisable,
  // The updated cell will first disappear and then fade in the screen.
  LynxCollectionCellUpdateAnimationTypeFadeIn
};

typedef NS_ENUM(NSInteger, LynxBounceForbiddenDirection) {
  LynxForbiddenNone,
  LynxForbiddenUpper,
  LynxForbiddenLower,
};

@interface LynxUICollection ()
@property(nonatomic) NSInteger scrollUpperThresholdItemCount;
@property(nonatomic) NSInteger scrollLowerThresholdItemCount;
@property(nonatomic) BOOL enableUpdateAnimation;
@property(nonatomic) LynxCollectionCellUpdateAnimationType cellUpdateAnimationType;
@property(nonatomic) BOOL needsVisibleCells;
@property(nonatomic) BOOL needsLayoutCompleteEvent;
@property(nonatomic, assign) BOOL enableListDebugInfoEvent;
@property(nonatomic) NSInteger numberOfColumns;
@property(nonatomic) CGFloat mainAxisGap;
@property(nonatomic) CGFloat crossAxisGap;
@property(nonatomic) NSInteger initialScrollIndex;
@property(nonatomic) NSInteger updatedScrollIndex;
@property(nonatomic) CGFloat pagingAlignFactor;
@property(nonatomic) CGFloat pagingAlignOffset;

@property(nonatomic, copy) NSMutableArray *listDelegates;
@property(nonatomic, assign) BOOL fixedContentOffset;
@property(nonatomic, assign) BOOL reloadAll;
@property(nonatomic) LynxBounceForbiddenDirection bounceForbiddenDirection;

@property(nonatomic) LynxListAppearEventEmitter *appearEventCourier;
@property(nonatomic) LynxListScrollEventEmitter *scrollEventEmitter;
@property(nonatomic) LynxCollectionScroll *scroll;
@property(nonatomic) LynxCollectionViewLayout *layout;
@property(nonatomic) LynxCollectionDataSource *dataSource;

@property(nonatomic, strong) NSDictionary<NSString *, NSArray *> *curComponents;
@property(nonatomic, strong) NSDictionary *_Nullable diffResultFromTasm;
@property(nonatomic, strong) NSDictionary *_Nullable listNoDiffInfo;
@property(nonatomic, assign) LynxListDebugInfoLevel debugInfoLevel;
@property(nonatomic, strong) LynxGestureConsumer *gestureConsumer;

- (void)performBatchUpdates:(void (^)(void))updates
                 completion:(void (^_Nullable)(BOOL))completion
                   animated:(BOOL)animated;
- (BOOL)isInOfflineMode;
- (void)sendListDebugInfoEvent:(NSString *)info;

- (void)setIncreaseFrequencyWithGesture:(BOOL)enable;

@end

NS_ASSUME_NONNULL_END
