// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "LynxFeature.h"
NS_ASSUME_NONNULL_BEGIN

/// Report feature.
/// All public methods can call on any thread.
@interface LynxFeatureCounter : NSObject

/// Count feature and upload later.
/// Can call on any thread.
/// - Parameter feature: LynxFeature
/// - Parameter instanceId: int32_t
+ (void)count:(LynxFeature)feature instanceId:(int32_t)instanceId;

@end

NS_ASSUME_NONNULL_END
