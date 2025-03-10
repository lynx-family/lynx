// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxNativeLayoutNode.h"
#import "LynxCustomMeasureDelegate+Internal.h"
#import "LynxPropsProcessor.h"
#include "core/public/layout_node_manager.h"
#include "core/public/layout_node_value.h"

using namespace lynx::tasm;

@implementation LynxNativeLayoutNode

- (MeasureResult)measureWithMeasureParam:(MeasureParam *)param
                          MeasureContext:(MeasureContext *)ctx {
  return [self.layoutNodeManager measureWithSign:self.sign MeasureParam:param MeasureContext:ctx];
}
- (void)alignWithAlignParam:(AlignParam *)param AlignContext:(AlignContext *)context {
  [self.layoutNodeManager alignWithSign:self.sign AlignParam:param AlignContext:context];
}

- (BOOL)supportInlineView {
  return YES;
}

LYNX_PROP_SETTER("vertical-align", setVerticalAlign, NSArray *) {
  [self setVerticalAlignOnShadowNode:requestReset value:value];
}

LYNX_PROP_SETTER("idSelector", setIdSelector, NSString *) { self.idSelector = value; }

@end
