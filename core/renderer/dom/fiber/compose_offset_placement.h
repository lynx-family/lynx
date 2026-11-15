// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_OFFSET_PLACEMENT_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_OFFSET_PLACEMENT_H_

#include <memory>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/renderer/starlight/types/layout_measurefunc.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}
namespace tasm {
class Element;

// The logical Compose handle owns this binding. The physical carrier retains
// only its placement context, just like an external custom Layout policy.
class ComposeOffsetPlacement final
    : public std::enable_shared_from_this<ComposeOffsetPlacement> {
 public:
  ComposeOffsetPlacement(Element& owner, runtime::MTSRuntime& runtime,
                         const lepus::Value& callback);
  void Attach();
  void Detach();
  void visitor(void* rt, void* func, uint64_t trace_tool) const;

 private:
  starlight::PlacementOffset Resolve();
  fml::RefPtr<Element> owner_;
  runtime::MTSRuntime& runtime_;
  lepus::Value callback_;
  bool attached_{false};
  bool resolving_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_OFFSET_PLACEMENT_H_
