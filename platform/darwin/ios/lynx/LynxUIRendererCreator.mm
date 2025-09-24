// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIRendererCreator.h"
#import "LynxUIRenderer.h"

@implementation LynxUIRendererCreator

- (id<LynxUIRendererProtocol>)createUIRendererWithContext:(LynxContext *)context
                                            containerView:(UIView<LUIBodyView> *)containerView
                                                  builder:(LynxViewBuilder *)builder
                                         providerRegistry:(LynxProviderRegistry *)providerRegistry {
  return [[LynxUIRenderer alloc] initWithLynxContext:context
                                       containerView:containerView
                                             builder:builder
                                    providerRegistry:providerRegistry];
}

@end
