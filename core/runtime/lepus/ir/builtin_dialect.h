// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_BUILTIN_DIALECT_H_
#define CORE_RUNTIME_LEPUS_IR_BUILTIN_DIALECT_H_

#include <string>

#include "core/runtime/lepus/ir/dialect.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class BuiltinDialect : public Dialect {
  NON_COPYABLE(BuiltinDialect);
  BuiltinDialect() = default;

 public:
  static BuiltinDialect* SharedInstance() {
    static auto dialect = new BuiltinDialect();
    if (!dialect) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BuiltinDialect::SharedInstance returned nullptr");
    }
    dialect->RegisterOperation();
    return dialect;
  }
  void* GetId() override;
  std::string GetName() const override;

 private:
  void RegisterOperation() override {
    AddOperations<
#define ENABLE_MIR_INSTR 0

#define DEF_VALUE(CLASS, PARENT) ValueKind::CLASS##Kind,
#define DEF_TAG(CLASS, PARENT) ValueKind::CLASS##Kind,

#include "core/runtime/lepus/ir/value_kinds.def"

#undef DEF_VALUE
#undef DEF_TAG

#undef ENABLE_MIR_INSTR
        ValueKind::Last_ValueKind>();
  }

 public:
  static uint64_t id;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_BUILTIN_DIALECT_H_
