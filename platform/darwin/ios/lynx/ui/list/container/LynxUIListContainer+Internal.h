// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import "LynxListScrollHelper.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxUIComponent;

static const CGFloat kInvalidSnapFactor = -1;

typedef NS_ENUM(NSInteger, LynxListContainerScrollState) {
  LynxListScrollStateIdle = 1,
  LynxListScrollStateDragging = 2,
  LynxListScrollStateFling = 3,
  LynxListScrollStateScrollAnimation = 4,
};

@protocol LynxListContainerScrollView <LynxListScrollHelperView>
@property(nonatomic, assign) BOOL verticalOrientation;
@end

// Shared list capabilities, independent of the concrete UI and its scroll-view superclass.
@protocol LynxListContainerInternal <NSObject>
@property(nonatomic, assign) BOOL needAdjustContentOffset;
@property(nonatomic, assign) CGPoint targetDelta;
@property(nonatomic, assign) CGFloat targetContentSize;
@property(nonatomic, strong, nullable) NSMutableDictionary *listNativeStateCache;
@property(nonatomic, strong, readonly) NSMutableArray *restoreNativeStateBlockArray;
- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
- (void)insertListComponent:(LynxUIComponent *)component;
- (void)removeListComponent:(LynxUIComponent *)component;
- (BOOL)initialPropsFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey;
- (void)setInitialPropsHasFlushed:(NSString *)initialPropKey cacheKey:(NSString *)cacheKey;
@end

NS_INLINE BOOL LynxIsListContainerUI(id _Nullable object) {
  return [object conformsToProtocol:@protocol(LynxListContainerInternal)];
}

NS_ASSUME_NONNULL_END
