// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollListener.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxFluencyMonitor : NSObject

@property(nonatomic, readonly) BOOL shouldSendAllScrollEvent;

- (void)startWithScrollInfo:(LynxScrollInfo*)info;

- (void)stopWithScrollInfo:(LynxScrollInfo*)info;

- (void)setFluencyPageconfigProbability:(CGFloat)probability;
@end

NS_ASSUME_NONNULL_END
