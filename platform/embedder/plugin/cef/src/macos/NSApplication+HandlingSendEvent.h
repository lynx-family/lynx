// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_NSAPPLICATION_HANDLINGSENDEVENT_H_
#define PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_NSAPPLICATION_HANDLINGSENDEVENT_H_

#import <Cocoa/Cocoa.h>
#include "include/cef_application_mac.h"

// It's required by cef.
@interface NSApplication (HandlingSendEvent) <CefAppProtocol>

- (BOOL)isHandlingSendEvent;
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;

// TODO(chenyouhui): Handle event and terminate state.

@end

#endif  // PLATFORM_EMBEDDER_PLUGIN_CEF_SRC_MACOS_NSAPPLICATION_HANDLINGSENDEVENT_H_
