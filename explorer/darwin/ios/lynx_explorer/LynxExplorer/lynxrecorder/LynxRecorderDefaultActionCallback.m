// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxRecorderDefaultActionCallback.h"
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxRawTextShadowNode.h>
#import <Lynx/LynxTextShadowNode.h>
#import <Lynx/LynxUIText.h>
#import <Lynx/LynxUIView.h>
#import <Lynx/LynxView.h>
#import "ExplorerModule.h"
#import "ExplorerTestModuleRegistrar.h"

static const int kVirtual = 1 << 2;

@interface LynxRecorderDefaultActionCallback ()
@end

@implementation LynxRecorderDefaultActionCallback

- (void)onLynxViewWillBuild:(LynxRecorderActionManager *)manager
                    builder:(LynxViewBuilder *)builder {
  [builder.config registerModule:[ExplorerModule class]];
  [ExplorerTestModuleRegistrar registerModulesInConfig:builder.config];
}
- (void)onLynxViewDidBuild:(LynxView *)lynxView {
}
@end
