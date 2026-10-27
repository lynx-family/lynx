// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUI;
@class LynxUIComponent;
@protocol LynxEventTarget;

@protocol LynxListItemWrapper <NSObject>
@property(nonatomic, readonly, nullable) LynxUIComponent *holdingUI;
@end

@protocol LynxListItemHelperOwner <NSObject>
- (UIScrollView *)scrollViewForListItemHelper;
- (BOOL)isVerticalForListItemHelper;
- (nullable NSArray<NSString *> *)itemKeysForListItemHelper;
@end

// Owns reusable list-item view operations and viewport queries independently from a concrete list
// UI or scroll-view subclass.
@interface LynxListItemHelper : NSObject

+ (BOOL)isCUIListItem:(nullable LynxUI *)ui;
+ (BOOL)isCUIListItemView:(UIView *)itemView;
+ (BOOL)isListItemWrapperView:(UIView *)itemView;
+ (nullable LynxUIComponent *)getComponentForListItemView:(UIView *)itemView;
+ (void)applyLayoutForComponent:(LynxUIComponent *)component frame:(CGRect)frame;
+ (void)setZPosition:(CGFloat)zPosition forComponent:(LynxUIComponent *)component;
+ (void)bringComponentToFront:(LynxUIComponent *)component inScrollView:(UIScrollView *)scrollView;

- (instancetype)initWithOwner:(id<LynxListItemHelperOwner>)owner NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
- (void)attachComponent:(LynxUIComponent *)component
              toWrapper:(UIView<LynxListItemWrapper> *)wrapper;
- (void)updateLayoutForComponent:(LynxUIComponent *)component
                       inWrapper:(UIView<LynxListItemWrapper> *)wrapper;
- (void)attachCUIComponent:(LynxUIComponent *)component
           orderedChildren:(NSArray<LynxUI *> *)children
           stickyStartItem:(nullable LynxUIComponent *)stickyStartItem
             stickyEndItem:(nullable LynxUIComponent *)stickyEndItem;
- (void)detachCUIComponent:(LynxUIComponent *)component;
- (NSInteger)indexForItemKey:(nullable NSString *)itemKey;
- (NSArray<UIView *> *)visibleItemViews;
- (NSArray<NSDictionary *> *)visibleItemInfo;
- (nullable id<LynxEventTarget>)findHitTargetAtPoint:(CGPoint)point
                                           withEvent:(nullable UIEvent *)event;
// Compatibility query for list-container's legacy event path. It intentionally returns the first
// matching scroll-view subview, which is not necessarily a list-item wrapper.
- (nullable UIView *)firstSubviewAtPoint:(CGPoint)point;

@end

NS_ASSUME_NONNULL_END
