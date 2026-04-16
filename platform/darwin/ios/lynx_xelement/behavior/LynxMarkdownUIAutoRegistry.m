// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>
#import <XElement/LynxMarkdownUIAutoRegistry.h>

@implementation LynxMarkdownShadowNodeAutoRegistry

LYNX_LAZY_REGISTER_SHADOW_NODE("x-markdown")

@end

@implementation LynxMarkdownUIAutoRegistry

LYNX_LAZY_REGISTER_UI("x-markdown")

@end
