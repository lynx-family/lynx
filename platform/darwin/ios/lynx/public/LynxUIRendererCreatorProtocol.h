// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

#import <Lynx/LUIBodyView.h>

@class LynxContext;
@class LynxViewBuilder;
@class LynxProviderRegistry;
@protocol LynxUIRendererProtocol;

@protocol LynxUIRendererCreatorProtocol <NSObject>

- (id<LynxUIRendererProtocol>)createUIRendererWithContext:(LynxContext *)context
                                            containerView:(UIView<LUIBodyView> *)containerView
                                                  builder:(LynxViewBuilder *)builder
                                         providerRegistry:(LynxProviderRegistry *)providerRegistry;

@end
