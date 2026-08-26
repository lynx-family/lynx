// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxScrollEventManager;
@class LynxUIComponent;
@protocol LynxEventTarget;

@protocol LynxListStickyManagerOwner <NSObject>
- (UIScrollView *)scrollViewForListStickyManager;
- (BOOL)isVerticalForListStickyManager;
- (nullable NSArray<NSString *> *)itemKeysForListStickyManager;
- (nullable LynxScrollEventManager *)eventManagerForListStickyManager;
@end

// Owns list sticky state and behavior independently from a concrete list UI or scroll-view
// subclass.
@interface LynxListStickyManager : NSObject

@property(nonatomic, assign, getter=isEnabled) BOOL enabled;
@property(nonatomic, assign) CGFloat offset;

- (instancetype)initWithOwner:(id<LynxListStickyManagerOwner>)owner NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

- (void)setStickyStartIndexes:(nullable NSArray<NSNumber *> *)startIndexes
                   endIndexes:(nullable NSArray<NSNumber *> *)endIndexes;

- (void)propsDidUpdate;
- (void)didLayoutComponent:(LynxUIComponent *)component;
- (void)didAttachComponent:(LynxUIComponent *)component;
- (void)willDetachComponent:(LynxUIComponent *)component;
- (void)updateStickyItems;

- (nullable id<LynxEventTarget>)findHitTargetAtPoint:(CGPoint)point
                                           withEvent:(nullable UIEvent *)event;

@end

NS_ASSUME_NONNULL_END
