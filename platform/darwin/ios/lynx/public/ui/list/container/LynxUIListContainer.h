// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIScroller.h>

@class LynxUIComponent;

typedef void (^RestoreNativeStateBlock)(void);

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

- (void)updateScrollInfoWithEstimatedOffset:(CGFloat)estimatedOffset
                                     smooth:(BOOL)smooth
                                  scrolling:(BOOL)scrolling;
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
