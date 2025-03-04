// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxContextModule.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxTextInfoModule : NSObject <LynxContextModule>

- (NSDictionary *)getTextInfo:(NSString *)text params:(NSDictionary *)params;

@end

NS_ASSUME_NONNULL_END
