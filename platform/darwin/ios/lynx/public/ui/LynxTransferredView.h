// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
@class LynxTransferredView;

typedef NS_ENUM(NSInteger, LynxTransferReleaseReason) {
  LynxTransferReleaseReasonExplicitRelease,
  LynxTransferReleaseReasonOwnerDestroy,
  LynxTransferReleaseReasonLoadFailed,
};

@protocol LynxTransferClient <NSObject>
@optional
- (void)onLoadStart:(LynxTransferredView*)view;
- (void)onLoadSuccess:(LynxTransferredView*)view;
- (void)onFirstScreen:(LynxTransferredView*)view;
- (void)onReleased:(LynxTransferredView*)view reason:(LynxTransferReleaseReason)reason;
@end

/** Main-thread handle returned by LynxView. Removing it from its superview allows reuse. */
@interface LynxTransferredView : UIView
/** Replace props while preserving the frontend instance. */
- (void)updateProps:(NSDictionary*)props;
/** Replace props and reset frontend instance state. */
- (void)reload:(NSDictionary*)props;
/** Permanently dispose this transfer. Repeated calls have no effect. */
- (void)clearForDestroy;
@end
NS_ASSUME_NONNULL_END
