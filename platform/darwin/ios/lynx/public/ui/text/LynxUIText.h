// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxTextOverflowLayer.h"
#import "LynxTextRenderer.h"
#import "LynxTextView.h"
#import "LynxUI+Internal.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIText : LynxUI <LynxTextView *>

@property(nonatomic, readonly, nullable) LynxTextRenderer *renderer;
@property(nonatomic, readonly) CGPoint overflowLayerOffset;

- (CALayer *)getOverflowLayer;

@end

NS_ASSUME_NONNULL_END
