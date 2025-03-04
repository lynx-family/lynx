// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxTransformOriginRaw : NSObject
@property(nonatomic, assign) float p0;
@property(nonatomic, assign) float p1;

- (bool)isValid;
- (bool)isP0Valid;
- (bool)isP1Valid;
- (bool)isP0Percent;
- (bool)isP1Percent;
- (bool)hasPercent;
- (bool)isDefault;

+ (LynxTransformOriginRaw*)convertToLynxTransformOriginRaw:(id)arr;
@end

NS_ASSUME_NONNULL_END
