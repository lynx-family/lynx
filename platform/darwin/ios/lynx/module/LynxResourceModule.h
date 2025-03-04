// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxContextModule.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxResourceModule : NSObject <LynxContextModule>

- (instancetype)initWithLynxContext:(LynxContext *)context;

@end

NS_ASSUME_NONNULL_END
