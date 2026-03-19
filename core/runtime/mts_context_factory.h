// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_MTS_CONTEXT_FACTORY_H_
#define CORE_RUNTIME_MTS_CONTEXT_FACTORY_H_

#include <memory>

#include "core/public/page_options.h"
#include "core/runtime/mts_context.h"

namespace lynx {
namespace runtime {

// Factory for creating MTSContext and its corresponding ContextBundle.
//
// Declaration is OSS-only (under lynx/). Implementations are split between
// OSS/non-OSS directories to isolate runtime-specific implementations.
class MTSContextFactory {
 public:
  static std::unique_ptr<MTSContext> Create(
      ContextType type, const std::shared_ptr<MTSContextDelegate>& delegate,
      bool disable_tracing_gc, int runtime_mode,
      const tasm::PageOptions& page_options);
};

class ContextBundleFactory {
 public:
  static std::unique_ptr<ContextBundle> Create(ContextType context_type);
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_MTS_CONTEXT_FACTORY_H_
