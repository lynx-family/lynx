// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

@protocol LynxTransferListener;

NS_ASSUME_NONNULL_BEGIN

@interface LynxTransferManager : NSObject

- (void)registerTransferListener:(id<LynxTransferListener>)listener;
- (void)unregisterTransferListener:(id<LynxTransferListener>)listener;
- (void)dispatchTransferCreate:(NSString*)transferId
                          view:(UIView*)view
                       dataset:(NSDictionary*)dataset;
- (void)dispatchTransferDatasetUpdate:(UIView*)view dataset:(NSDictionary*)dataset;
- (void)dispatchTransferRemove:(NSString*)transferId view:(UIView*)view;
- (void)clearTransfers;

@end

NS_ASSUME_NONNULL_END
