// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxUIListDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LynxUIListScrollEvent <NSObject>
- (void)addListDelegate:(id<LynxUIListDelegate>)delegate;
- (void)removeListDelegate:(id<LynxUIListDelegate>)delegate;
@end

NS_ASSUME_NONNULL_END
