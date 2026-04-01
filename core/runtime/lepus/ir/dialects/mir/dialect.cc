// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialects/mir/dialect.h"

namespace lynx {
namespace lepus {
namespace ir {

uint64_t MIRDialect::id_ = 0;

void* MIRDialect::GetId() { return &MIRDialect::id_; }

std::string MIRDialect::GetName() const { return "mir"; }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
