// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@protocol LynxEventTargetBase <NSObject>

- (nullable id<LynxEventTargetBase>)parentResponder;

- (nullable NSDictionary*)getDataset;

@optional

// Returns whether this target's native event bindings contain eventName, excluding ancestors.
// For compatibility with existing targets, check respondsToSelector: before calling this method
// and use NO when it is unavailable.
- (BOOL)hasEventListener:(NSString*)eventName;

@end

NS_ASSUME_NONNULL_END
