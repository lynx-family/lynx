// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxListLayoutUpdateType) {
  LynxListUpdateTypeNoneUpdate,
  LynxListUpdateTypeDataUpdate,
  LynxListUpdateTypeScrollBoundsUpdate,        // scrolling or internal offset change
  LynxListUpdateTypeScrollThresholdsUpdate,    // any thresholds change
  LynxListUpdateTypeInitialScrollIndexUpdate,  // initial-scroll-index
  LynxListUpdateTypeScrollToPositionUpdate,    // scrollToPosition
  LynxListUpdateTypeLayoutGeneralPropsUpdate,  // columns, axisGap, etc. will effect overall layout
  LynxListUpdateTypeLayoutSelfSizing,          // default size change to real size
};

typedef NS_ENUM(NSUInteger, LynxListLayoutType) {
  LynxListLayoutNone,
  LynxListLayoutWaterfall,
  LynxListLayoutFlow
};

@interface LynxUIListScrollThresholds : NSObject
/*
 Use the existence check to verify if the propSetter changed the value.
 */
@property(nonatomic, strong) NSNumber* scrollToStartOffset;     // upper-threshold
@property(nonatomic, strong) NSNumber* scrollToEndOffset;       // lower-threshold
@property(nonatomic, strong) NSNumber* scrollToStartItemCount;  // upper-threshold-item-count
@property(nonatomic, strong) NSNumber* scrollToEndItemCount;    // lower-threshold-item-count
@property(nonatomic, assign) NSNumber* throttle;                // scroll-event-throttle
@end

@interface LynxUIListInvalidationContext : NSObject

#pragma mark Diff Info
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* removals;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* insertions;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* moveFrom;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* moveTo;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* updateTo;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* updateFrom;
@property(nonatomic, strong, nullable) NSDictionary<NSNumber*, NSValue*>* updates;
@property(nonatomic, strong, nullable) NSDictionary<NSNumber*, NSNumber*>* estimatedHeights;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* fullSpanItems;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* stickyTopItems;
@property(nonatomic, strong, nullable) NSArray<NSNumber*>* stickyBottomItems;

#pragma mark Layout Info
@property(nonatomic, assign) UIEdgeInsets insets;
@property(nonatomic, assign) NSInteger numberOfColumns;
@property(nonatomic, assign) CGFloat mainAxisGap;
@property(nonatomic, assign) CGFloat crossAxisGap;
@property(nonatomic, assign) LynxListLayoutType layoutType;

@property(nonatomic, assign) BOOL needsInternalCellAppearNotification;
@property(nonatomic, assign) BOOL needsInternalCellDisappearNotification;
@property(nonatomic, assign) BOOL needsInternalCellPrepareForReuseNotification;

@property(nonatomic, assign) LynxListLayoutUpdateType listUpdateType;

// Use the existence to identify whether the value is set.
@property(nonatomic, strong) NSNumber* needsVisibleCells;

/*
 scrollToPosition properties.
 */
@property(nonatomic, assign) NSInteger scrollToPosition;
@property(nonatomic, assign) BOOL smooth;
@property(nonatomic, strong) NSString* alignTo;
@property(nonatomic, assign) CGFloat offset;

@property(nonatomic, assign) NSInteger initialScrollIndex;  // initial-scroll-index

@property(nonatomic, strong) LynxUIListScrollThresholds* scrollThresholds;

#pragma mark initialization
/** @return A valid invalidationContext with different updateType
 */
/*
  Init with scrolling or internal offset change.
*/
- (instancetype)initWithBoundsChange;

/*
  Init with scroll thresholds change.
*/
- (instancetype)initWithScrollThresholdsUpdate:(LynxUIListScrollThresholds*)scrollThresholds;

/*
 Init with model updates, including selfSizing or layout update.
 */
- (instancetype)initWithModelUpdates:(NSDictionary*)updates;

/*
 A simple init with updateType generalPropsUpdate.
 */
- (instancetype)initWithGeneralPropsUpdate;

/*
 Init with initial-scroll-index.
 */
- (instancetype)initWithInitialScrollIndex:(NSInteger)index;

/*
 Init with scrollToPosition.
 */
- (instancetype)initWithScrollToInfo:(NSInteger)position
                              offset:(CGFloat)offset
                             alignTo:(NSString*)alignTo
                              smooth:(BOOL)smooth;

@end

NS_ASSUME_NONNULL_END
