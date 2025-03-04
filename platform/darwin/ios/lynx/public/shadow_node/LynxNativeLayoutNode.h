// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxCustomMeasureDelegate.h"
#import "LynxShadowNode.h"

NS_ASSUME_NONNULL_BEGIN
@interface LynxNativeLayoutNode : LynxShadowNode
- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(MeasureContext *)context;
- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)context;

@property(nonatomic) NSString *idSelector;
@end
NS_ASSUME_NONNULL_END
