// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxError.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN
/**
 * The interface provides internal behavior for LynxView and can
 * be accessed by internal framework classes.
 */
@interface LynxView ()

- (void)setIntrinsicContentSize:(CGSize)size;
- (void)dispatchError:(LynxError *)error;

@end

NS_ASSUME_NONNULL_END
