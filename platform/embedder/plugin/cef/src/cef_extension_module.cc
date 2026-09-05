// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/plugin/cef/src/cef_webview.h"

#include <lynx/registration.h>

LYNX_REGISTER_ELEMENT(
    "CEFWebviewElementModule",
    "x-webview",
    cef_webview_create_view,
    false,
    nullptr)

extern "C" void cef_extension_module_force_link_registration() {}
