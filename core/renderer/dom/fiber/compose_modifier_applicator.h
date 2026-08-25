// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_MODIFIER_APPLICATOR_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_MODIFIER_APPLICATOR_H_

#include <cstddef>
#include <string>

#include "base/include/value/base_value.h"

namespace lynx {
namespace runtime {
class MTSRuntime;
}  // namespace runtime
namespace tasm {

class ComposeElementHandle;

// Parses and validates a complete Compose Modifier IR and then replaces its
// physical ModifierElement topology as one fail-closed operation.
class ComposeModifierApplicator final {
 public:
  struct ApplyResult {
    bool success{false};
    size_t error_position{0};
    std::string error_message;
  };

  static ApplyResult Apply(ComposeElementHandle* handle,
                           const lepus::Value& modifier_tail,
                           runtime::MTSRuntime*);

  static bool ValidateTopology(ComposeElementHandle* handle,
                               std::string* error_message);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_MODIFIER_APPLICATOR_H_
