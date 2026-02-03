// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxUIOwner.h>
#import <XElement/LynxUIRefreshAutoRegistry.h>

@implementation LynxUIRefreshShadowNodeAutoRegistry

LYNX_LAZY_REGISTER_SHADOW_NODE("refresh")

@end

@implementation LynxUIRefreshAutoRegistry

LYNX_LAZY_REGISTER_UI("refresh")

@end

@implementation LynxUIRefreshHeaderAutoRegistry

LYNX_LAZY_REGISTER_UI("refresh-header")

@end
