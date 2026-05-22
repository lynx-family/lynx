// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/keyboard_bridge.h"

#include <utility>

#include "build/build_config.h"

namespace clay {
KeyboardBridge::KeyboardBridge(KeyboardClient* client) : client_(client) {}

void KeyboardBridge::RequestSoftKeyboard(KeyboardInputType type,
                                         KeyboardAction action,
                                         PageView* page_view) {
#if OS_ANDROID
  client_->PlatformShowSoftInput(static_cast<int>(type),
                                 static_cast<int>(action));
#endif
}  // namespace clay

void KeyboardBridge::HideSoftKeyboard() {
#if OS_ANDROID
  client_->PlatformHideSoftInput();
#endif
}

}  // namespace clay
