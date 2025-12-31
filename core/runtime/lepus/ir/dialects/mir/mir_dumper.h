
// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_DUMPER_H_
#define CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_DUMPER_H_

#include "core/runtime/lepus/ir/ir_dumper.h"

namespace lynx {
namespace lepus {
namespace ir {

class MIRPrinter : public IRWalker<MIRPrinter>, public IRPrinter {
  using SuperType = IRWalker<MIRPrinter>;

 public:
  explicit MIRPrinter(IRContext* ir_ctx, std::ostream& os, bool escape = false)
      : IRPrinter(ir_ctx, os, escape) {}
  ~MIRPrinter() override = default;

  // IRWalker/IRVisitor entrypoints.
  bool VisitInstruction(const Instruction& inst);
  bool VisitBlock(const Block& block);
  void VisitFuncOp(const FuncOp& m, llvh::ArrayRef<Block*> order);
  bool VisitFuncOp(const FuncOp& func);
  bool VisitModuleOp(const ModuleOp& mod);

  void PrintBlock(const Block& block) override;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_DUMPER_H_
