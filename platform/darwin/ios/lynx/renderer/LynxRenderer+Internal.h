// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRenderer.h>
#include "core/renderer/dom/fragment/display_list.h"

@interface LynxRenderer ()

- (void)updateDisplayList:(lynx::tasm::DisplayList *)list;

- (lynx::tasm::DisplayList *)getDisplayList;

@end
