// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUI.h>
#import <MJRefresh/MJRefresh.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxUIRefreshDelegate <NSObject>

- (void)refreshview:(MJRefreshComponent *)refreshview
    didUpdatePullingPrecent:(CGFloat)pullingPercent;
- (void)refreshview:(MJRefreshComponent *)refreshview
    didUpdateShowingPrecent:(CGFloat)showingPercent;

@end

@interface LynxUIRefresh<__covariant V : UIView *> : LynxUI <V> <LynxUIRefreshDelegate>

@property(nonatomic, strong, readonly) UIScrollView *scrollView;

- (nullable UIView *)findViewWithKind:(Class)clz
                             fromView:(UIView *)view
                         excludeViews:(nullable NSArray<UIView *> *)excludeViews;

@end

NS_ASSUME_NONNULL_END
