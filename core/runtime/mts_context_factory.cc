// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/mts_context_factory.h"

#include <cassert>

#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace runtime {

std::unique_ptr<MTSContext> MTSContextFactory::Create(
    ContextType type, MTSRuntime* runtime_private, bool disable_tracing_gc,
    int runtime_mode, const tasm::PageOptions& page_options) {
  switch (type) {
    case ContextType::LepusNGContextType:
      return std::make_unique<lepus::QuickContext>(
          runtime_private, disable_tracing_gc, runtime_mode, page_options);

    case ContextType::VMContextType:
#if !ENABLE_JUST_LEPUSNG
      return std::make_unique<lepus::VMContext>(runtime_private);
#else
      LOGE("lepusng sdk do not support vm context");
      assert(false);
      return nullptr;
#endif

    case ContextType::RTSContextType:
    case ContextType::RTSNativeContextType:
      // RTS/NativeContext are non-open-source implementation.
      LOGE("RTS/NativeContext is not available in OSS build");
      return nullptr;

    default:
      LOGE("Unknown ContextType.");
      return nullptr;
  }
}

std::unique_ptr<ContextBundle> ContextBundleFactory::Create(
    ContextType context_type) {
  switch (context_type) {
    case ContextType::LepusNGContextType:
      return std::make_unique<lepus::QuickContextBundle>();

    case ContextType::VMContextType:
#if !ENABLE_JUST_LEPUSNG
      return std::make_unique<lepus::VMContextBundle>();
#else
      return nullptr;
#endif

    case ContextType::RTSContextType:
    case ContextType::RTSNativeContextType:
      // RTS bundles are non-open-source implementation.
      return nullptr;

    default:
      return nullptr;
  }
}

}  // namespace runtime
}  // namespace lynx
