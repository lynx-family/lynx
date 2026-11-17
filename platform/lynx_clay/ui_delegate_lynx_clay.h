// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_LYNX_CLAY_UI_DELEGATE_LYNX_CLAY_H_
#define PLATFORM_LYNX_CLAY_UI_DELEGATE_LYNX_CLAY_H_

#include <memory>

#include "clay/lynx_adaptor/ui_delegate_clay.h"

namespace lynx::tasm {

std::unique_ptr<UIDelegateClay> CreateLynxClayUIDelegate(
    clay::ViewContext* view_context,
    std::unique_ptr<runtime::NativeModuleFactory> module_factory);

}  // namespace lynx::tasm

#endif  // PLATFORM_LYNX_CLAY_UI_DELEGATE_LYNX_CLAY_H_
