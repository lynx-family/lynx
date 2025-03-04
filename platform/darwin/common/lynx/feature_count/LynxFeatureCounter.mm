// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFeatureCounter.h"
#include "core/services/feature_count/global_feature_counter.h"

@interface LynxFeatureCounter ()

@end

@implementation LynxFeatureCounter

#pragma mark - Public Methods
+ (void)count:(LynxFeature)feature instanceId:(int32_t)instanceId {
  lynx::tasm::report::GlobalFeatureCounter::Count((lynx::tasm::report::LynxFeature)feature,
                                                  instanceId);
}

@end
