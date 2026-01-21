// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "core/renderer/dom/fragment/display_list.h"

@protocol LynxRendererHost;

@interface LynxDisplayListApplier : NSObject

- (instancetype)initWithView:(UIView<LynxRendererHost> *)view;

- (void)applyDisplayList:(lynx::tasm::DisplayList *)list;

- (void)reset;

@end
