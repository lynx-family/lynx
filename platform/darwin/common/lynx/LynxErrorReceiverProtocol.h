// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxError.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * base protocol to receive `LynxError` for `LynxTemplateRenderer` and `LynxBackgroundRuntime`
 */
@protocol LynxErrorReceiverProtocol <NSObject>

@required
/**
 * Dispatch LynxError
 */
- (void)onErrorOccurred:(LynxError *)error;

@end

NS_ASSUME_NONNULL_END
