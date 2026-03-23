// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialect.h"

#include "core/runtime/lepus/ir/builtin_dialect.h"
#include "core/runtime/lepus/ir/dialects/mir/dialect.h"
namespace lynx {
namespace lepus {
namespace ir {
DialectRegistry::DialectRegistry() {
  {
    auto dialect = BuiltinDialect::SharedInstance();
    id_to_dialect_.insert(std::make_pair(dialect->GetId(), dialect));
  }

  {
    auto dialect = MIRDialect::SharedInstance();
    id_to_dialect_.insert(std::make_pair(dialect->GetId(), dialect));
  }
}

bool Dialect::IsMIRDialect() const {
  return this == MIRDialect::SharedInstance();
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
