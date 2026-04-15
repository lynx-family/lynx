// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_DIALECT_H_
#define CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_DIALECT_H_

#include <string>

#include "core/runtime/lepus/ir/dialect.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class MIRDialect : public Dialect {
  MIRDialect() = default;

 public:
  static MIRDialect* SharedInstance() {
    static auto dialect = new MIRDialect();
    if (LEPUS_UNLIKELY(!dialect)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: MIRDialect::SharedInstance failed to create "
          "singleton");
    }
    dialect->RegisterOperation();
    return dialect;
  }
  void* GetId() override;
  std::string GetName() const override;

 private:
  void RegisterOperation() override {
    AddOperations<
#define ENABLE_MIR_INSTR 1

#define DEF_VALUE(CLASS, PARENT) ValueKind::CLASS##Kind,
#define DEF_TAG(CLASS, PARENT) ValueKind::CLASS##Kind,

#include "core/runtime/lepus/ir/instrs.def"

#undef DEF_VALUE
#undef DEF_TAG

#undef ENABLE_MIR_INSTR
        Dialect::DIALECT_GUARD_OP>();
  }

 private:
  static uint64_t id_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_DIALECT_H_
