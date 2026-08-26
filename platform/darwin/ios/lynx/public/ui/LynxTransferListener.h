// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Receives transfer views created by {@code <transfer-view>} nodes.
 *
 * A listener becomes the owner of a transfer only when {@code onCreate:view:dataset:} returns YES.
 * Only that owner listener will receive the matching dataset updates and {@code onRemove:view:}
 * callback when the transfer is removed.
 */
@protocol LynxTransferListener <NSObject>

/**
 * Called when a transfer view is ready to be mounted by the host.
 *
 * @param transferId The {@code transfer-id} declared on the {@code <transfer-view>} node.
 * @param view The detached wrapper view containing the transfer subtree.
 * @param dataset The dataset declared on the {@code <transfer-view>} node.
 * @return YES if this listener takes ownership of the view; NO to leave it pending for another
 *     listener.
 */
- (BOOL)onCreate:(NSString *)transferId view:(UIView *)view dataset:(NSDictionary *)dataset;

/**
 * Called after the properties of an owned transfer have been updated.
 *
 * @param transferId The {@code transfer-id} associated with the updated transfer.
 * @param dataset The latest dataset declared on the {@code <transfer-view>} node.
 */
- (void)onDatasetUpdate:(NSString *)transferId dataset:(NSDictionary *)dataset;

/**
 * Called when an owned transfer is removed or the host {@code LynxView} is destroyed.
 *
 * This callback is sent only to the listener that previously returned YES from
 * {@code onCreate:view:dataset:} for the same view.
 *
 * @param transferId The {@code transfer-id} associated with the removed transfer.
 * @param view The wrapper view that will be detached from its current parent.
 */
- (void)onRemove:(NSString *)transferId view:(UIView *)view;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_
