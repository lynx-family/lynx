// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <XElement/LynxUIScrollCoordinatorAutoRegistry.h>

@implementation LynxUIScrollCoordinatorAutoRegistry
LYNX_LAZY_REGISTER_UI("scroll-coordinator")
@end

@implementation LynxUIScrollCoordinatorHeaderAutoRegistry
LYNX_LAZY_REGISTER_UI("scroll-coordinator-header")
@end

@implementation LynxUIScrollCoordinatorSlotAutoRegistry
LYNX_LAZY_REGISTER_UI("scroll-coordinator-slot")
@end

@implementation LynxUIScrollCoordinatorSlotDragAutoRegistry
LYNX_LAZY_REGISTER_UI("scroll-coordinator-slot-drag")
@end

@implementation LynxUIScrollCoordinatorToolbarAutoRegistry
LYNX_LAZY_REGISTER_UI("scroll-coordinator-toolbar")
@end
