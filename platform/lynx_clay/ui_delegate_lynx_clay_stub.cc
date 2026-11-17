// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "platform/lynx_clay/ui_delegate_lynx_clay.h"

namespace lynx::tasm {

std::unique_ptr<UIDelegateClay> CreateLynxClayUIDelegate(
    clay::ViewContext* view_context,
    std::unique_ptr<runtime::NativeModuleFactory> module_factory) {
  return std::make_unique<UIDelegateClay>(view_context,
                                          std::move(module_factory));
}

}  // namespace lynx::tasm
