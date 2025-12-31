// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/builtin_dialect.h"

namespace lynx {
namespace lepus {
namespace ir {

uint64_t BuiltinDialect::id = 0;

void* BuiltinDialect::GetId() { return &BuiltinDialect::id; }

std::string BuiltinDialect::GetName() const { return "builtin"; }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
