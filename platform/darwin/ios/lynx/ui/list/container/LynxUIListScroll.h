// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIComponent.h>

#import "LynxListItemHelper.h"
#import "LynxListScrollView.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxListScrollComponentWrapper : UIView <LynxListItemWrapper>
@property(nonatomic, weak, nullable) LynxUIComponent *holdingUI;
@end

@protocol LynxUIListScrollDelegate <NSObject>
- (void)insertListComponent:(LynxUIComponent *)component
                    wrapper:(LynxListScrollComponentWrapper *)wrapper;
- (void)removeListComponent:(LynxUIComponent *)component;
@end

// Private, unregistered list foundation. Event and command integration precedes tag registration.
@interface LynxUIListScroll : LynxUI <LynxUIComponentLayoutObserver, LynxListContainerInternal>
@property(nonatomic, weak, nullable) id<LynxUIListScrollDelegate> delegate;
@property(nonatomic, assign) BOOL needAdjustContentOffset;
@property(nonatomic, assign) CGPoint targetDelta;
@property(nonatomic, assign) CGFloat targetContentSize;
@property(nonatomic, strong, nullable) NSMutableDictionary *listNativeStateCache;
@property(nonatomic, strong, readonly) NSMutableArray<void (^)(void)> *restoreNativeStateBlockArray;

- (LynxListScrollView *)view;
- (void)insertListComponent:(LynxUIComponent *)component;
- (void)removeListComponent:(LynxUIComponent *)component;
- (NSInteger)getIndexFromItemKey:(NSString *)itemKey;
- (NSArray<UIView<LynxListItemWrapper> *> *)visibleCells;
- (NSArray<NSDictionary *> *)visibleCellsInfo;
@end

NS_ASSUME_NONNULL_END
