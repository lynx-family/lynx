// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Receives views from {@code <transfer-view>}. Returning YES from onCreate:view:dataset:
 * grants ownership; only the registered owner receives dataset updates and removal callbacks.
 */
@protocol LynxTransferListener <NSObject>

/**
 * Called when a transfer view is ready to be mounted by the host.
 * @param transferId The {@code transfer-id} declared on the {@code <transfer-view>} node.
 * @param view The detached wrapper view containing the transfer subtree.
 * @param dataset The dataset declared on the {@code <transfer-view>} node.
 * @return YES to take ownership; NO to leave the view available to another listener.
 */
- (BOOL)onCreate:(NSString *)transferId view:(UIView *)view dataset:(NSDictionary *)dataset;

/**
 * Receives the latest dataset for the owned transfer identified by transferId after a props update.
 */
- (void)onDatasetUpdate:(NSString *)transferId dataset:(NSDictionary *)dataset;

/**
 * Called when an owned transfer is removed or the host {@code LynxView} is destroyed.
 * @param transferId The {@code transfer-id} associated with the removed transfer.
 * @param view The wrapper view that will be detached from its current parent.
 */
- (void)onRemove:(NSString *)transferId view:(UIView *)view;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_UI_LYNXTRANSFERLISTENER_H_
