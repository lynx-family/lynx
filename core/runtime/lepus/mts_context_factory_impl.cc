// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cassert>

#include "core/renderer/utils/base/base_def.h"
#include "core/runtime/lepus/mts_context_factory.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/quick_context.h"

namespace lynx {
namespace lepus {

std::unique_ptr<MTSContext> MTSContextFactory::Create(
    ContextType type, const std::shared_ptr<MTSContextDelegate>& delegate,
    void* runtime, bool disable_tracing_gc, int runtime_mode,
    const tasm::PageOptions& page_options) {
  switch (type) {
    case ContextType::LepusNGContextType:
      return std::make_unique<QuickContext>(
          delegate, runtime, disable_tracing_gc, runtime_mode, page_options);

    case ContextType::VMContextType:
#if !ENABLE_JUST_LEPUSNG
      return std::make_unique<VMContext>(delegate, runtime);
#else
      LOGE("lepusng sdk do not support vm context");
      assert(false);
      return nullptr;
#endif

    default:
      LOGE("Unknown ContextType.");
      return nullptr;
  }
}

std::unique_ptr<ContextBundle> ContextBundleFactory::Create(
    ContextType context_type) {
  switch (context_type) {
    case ContextType::LepusNGContextType:
      return std::make_unique<QuickContextBundle>();

    case ContextType::VMContextType:
#if !ENABLE_JUST_LEPUSNG
      return std::make_unique<VMContextBundle>();
#else
      return nullptr;
#endif

    default:
      return nullptr;
  }
}

}  // namespace lepus
}  // namespace lynx
