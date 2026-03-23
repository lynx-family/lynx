// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/dialects/mir/mir_dumper.h"

#include <string>

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/Hashing.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

namespace {
inline void Indent(std::ostream& os, unsigned count) {
  for (unsigned i = 0; i < count; ++i) {
    os << ' ';
  }
}
}  // namespace

bool MIRPrinter::VisitBlock(const Block& block) {
  auto* mutable_bb = const_cast<Block*>(&block);
  Indent(os_, indent_);
  if (mutable_bb->GetName().empty()) {
    os_ << "%BB" << namer_.GetBBNumber(mutable_bb) << ":\n";
  } else {
    os_ << "%BB." << mutable_bb->GetName() << "."
        << namer_.GetBBNumber(mutable_bb) << ":\n";
  }
  indent_ += 2;

  // Use IRVisitor dispatch to visit the instructions.
  for (auto& inst : block) {
    Visit(inst);
  }

  indent_ -= 2;
  return true;
}

bool MIRPrinter::VisitInstruction(const Instruction& inst) {
  auto* mutable_inst = const_cast<Instruction*>(&inst);

  if (false) {
    Indent(os_, indent_);
    os_ << "; ";
    PrintSourceLocation(mutable_inst->GetLocation());
    os_ << "\n";
  }

  Indent(os_, indent_);
  PrintInstruction(mutable_inst);
  os_ << "\n";
  return true;
}

bool MIRPrinter::VisitFuncOp(const FuncOp& func) {
  auto* mutable_func = const_cast<FuncOp*>(&func);
  llvh::SmallVector<Block*, 8> order{};
  for (auto& bb : *mutable_func) {
    order.push_back(&bb);
  }
  VisitFuncOp(func, order);
  return true;
}

void MIRPrinter::VisitFuncOp(const FuncOp& func, llvh::ArrayRef<Block*> order) {
  auto* mutable_func = const_cast<FuncOp*>(&func);
  Indent(os_, indent_);
  namer_.NewFunction(&func);
  // Number all instructions sequentially.
  for (auto* bb : order) {
    for (auto* inst : bb->InstRange()) {
      namer_.GetInstNumber(inst);
    }
  }

  PrintMethodHeader(mutable_func);
  os_ << "\n";

  for (auto* bb : order) {
    Visit(*bb);
  }

  Indent(os_, indent_);
  os_ << "function_end"
      << "\n";
  os_ << "\n";
}

bool MIRPrinter::VisitModuleOp(const ModuleOp& mod) {
  llvh::for_each(const_cast<ModuleOp*>(&mod)->FuncRange(),
                 [&](FuncOp* func_op) { VisitFuncOp(*func_op); });
  return true;
}

void MIRPrinter::PrintBlock(const Block& block) {
  if (block.GetName().empty()) {
    os_ << "%BB" << namer_.GetBBNumber(const_cast<Block*>(&block));
  } else {
    os_ << "%BB." << block.GetName() << "."
        << namer_.GetBBNumber(const_cast<Block*>(&block));
  }
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
