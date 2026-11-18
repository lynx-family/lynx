// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxViewEnum.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUITransfer;

@interface LynxTransferWrapperView : UIView

- (instancetype)initWithTransfer:(LynxUITransfer*)transfer;

/**
 * Uses the same measurement modes as LynxView. Configure these properties, then call
 * triggerLayout. Exact uses preferredLayout*, Max uses preferredMaxLayout*, and
 * Undefined measures content without an external constraint on that axis.
 * A nonzero frame on first attachment initializes Exact constraints unless modes
 * were already configured. Subsequent frame changes follow enableAutoLayout.
 *
 * For a 300-point-wide, content-height view:
 *   view.layoutWidthMode = LynxViewSizeModeExact;
 *   view.preferredLayoutWidth = 300;
 *   view.layoutHeightMode = LynxViewSizeModeUndefined;
 *   [view triggerLayout];
 */
@property(nonatomic, assign) LynxViewSizeMode layoutWidthMode;
@property(nonatomic, assign) LynxViewSizeMode layoutHeightMode;
@property(nonatomic, assign) CGFloat preferredLayoutWidth;
@property(nonatomic, assign) CGFloat preferredLayoutHeight;
@property(nonatomic, assign) CGFloat preferredMaxLayoutWidth;
@property(nonatomic, assign) CGFloat preferredMaxLayoutHeight;

/**
 * Like LynxView, enabling this makes layoutSubviews use bounds as Exact constraints
 * on both axes. Leave disabled for content-driven dimensions.
 */
@property(nonatomic, assign) BOOL enableAutoLayout;

/**
 * Requests layout with the configured constraints. The result is available through
 * intrinsicContentSize after layout; the owning transfer listener receives changes.
 * The host owns this view's frame. Fitting queries do not change the constraints.
 * A listener using Auto Layout should call invalidateIntrinsicContentSize when
 * consuming a size change; a frame-based listener should assign the desired frame.
 */
- (void)triggerLayout;

@end

NS_ASSUME_NONNULL_END
