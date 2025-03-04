// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LUIBodyView;

@interface LynxRootUI : LynxUI <UIView <LUIBodyView>*>

- (instancetype)init NS_UNAVAILABLE;

- (instancetype)initWithView:(nullable UIView*)view NS_UNAVAILABLE;

- (instancetype)initWithLynxView:(UIView<LUIBodyView>*)lynxView;

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with;

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
    withLayoutAnimation:(BOOL)with;

/// An adapter to prevent breaking change under Lynx pipeline. It is recommended to access the root
/// view of Lynx UI tree by rootView. The lynxView adapter could be removed in next few patches.
@property(nonatomic, readonly, weak) LynxView* lynxView;
@property(nonatomic, readonly, weak) UIView<LUIBodyView>* rootView;
@property(nonatomic) BOOL layoutAnimationRunning;

@end

NS_ASSUME_NONNULL_END
