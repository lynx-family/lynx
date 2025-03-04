// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LynxKeyboardEventDispatcher_h
#define LynxKeyboardEventDispatcher_h

#import "LynxContext.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxKeyboardEventDispatcher : NSObject

- (instancetype)initWithContext:(LynxContext *)context;

@end

NS_ASSUME_NONNULL_END

#endif /* LynxKeyboardEventDispatcher_h */
