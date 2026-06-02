// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>
#import <XElement/LynxUIMarkdownAutoRegistry.h>

@implementation LynxUIMarkdownShadowNodeAutoRegistry

LYNX_LAZY_REGISTER_SHADOW_NODE("markdown")

@end

@implementation LynxUIMarkdownAutoRegistry

LYNX_LAZY_REGISTER_UI("markdown")

@end
