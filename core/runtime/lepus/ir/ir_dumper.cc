// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/ir_dumper.h"

#include <algorithm>
#include <utility>

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/ir_visitor.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/region_op.h"
#include "core/runtime/lepus/ir/type_op.h"

namespace lynx {
namespace lepus {
namespace ir {

inline void Indent(std::ostream& os, unsigned count) {
  for (unsigned i = 0; i < count; ++i) {
    os << ' ';
  }
}

IRPrinter::IRPrinter(IRContext* ir_ctx, bool use_persistent,
                     std::ostream& output_stream, bool escape)
    : ir_ctx_(ir_ctx),
      indent_(0),
      os_(output_stream),
      need_escape_(escape),
      temp_namer_(new Namer(use_persistent)),
      namer_(*temp_namer_) {}

unsigned ValueNamer::GetNumber(Value* value) {
  ValueKind kind = value->GetKind();
  auto [it, inserted] =
      map_.try_emplace(value, ValueT(kind, current_gen_, counter_));
  if (inserted) {
    ++counter_;
  } else {
    if (it->second.visited_gen_ != current_gen_) {
      it->second.visited_gen_ = current_gen_;
    }
    if (it->second.kind_ != kind) {
      // If the kind of the value has changed, reset the counter.
      it->second.kind_ = kind;
      it->second.number_ = counter_++;
    }
  }
  return it->second.number_;
}

bool IRPrinter::PrintInstructionDestination(Instruction* inst) {
  unsigned number = namer_.GetInstNumber(inst);
  if (inst->HasOutput()) {
    os_ << "%" << number;
    return true;
  } else {
    // Calculate the width of the printed number.
    unsigned width = 1;
    unsigned tmp_number = number;
    while (tmp_number > 9) {
      tmp_number /= 10;
      ++width;
    }
    Indent(os_, width);
    return false;
  }
}

void IRPrinter::PrintBlock(const Block& block) {
  if (block.GetName().empty()) {
    os_ << "block." << namer_.GetBBNumber(const_cast<Block*>(&block));
  } else {
    os_ << "block." << block.GetName() << "."
        << namer_.GetBBNumber(const_cast<Block*>(&block));
  }
}

void IRPrinter::PrintSourceLocation(int64_t loc) { os_ << ":" << loc; }

void IRPrinter::PrintValueLabel(Instruction* inst, Value* value,
                                unsigned op_index) {
  if (!value) {
    // if (llvh::llvh::isa<ReturnInst>(I) || llvh::llvh::isa<IfInst>(I)) return;
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRPrinter::PrintValueLabel got nullptr operand");
  }

  if (llvh::isa<Instruction>(value)) {
    os_ << "%" << namer_.GetInstNumber(llvh::cast<Instruction>(value));
  } else if (llvh::isa<LiteralNull>(value)) {
    os_ << "nil";
  } else if (const auto literal_int8 = llvh::dyn_cast<LiteralInt8>(value)) {
    os_ << std::to_string(literal_int8->GetValue());
  } else if (const auto literal_int32 = llvh::dyn_cast<LiteralInt32>(value)) {
    os_ << std::to_string(literal_int32->GetValue());
  } else if (const auto literal_uint8 = llvh::dyn_cast<LiteralUint8>(value)) {
    os_ << std::to_string(literal_uint8->GetValue());
  } else if (const auto literal_uint32 = llvh::dyn_cast<LiteralUint32>(value)) {
    os_ << std::to_string(literal_uint32->GetValue());
  } else if (auto literal_float64 = llvh::dyn_cast<LiteralFloat64>(value)) {
    const auto float_value = literal_float64->GetValue();
    if (float_value == 0 && std::signbit(float_value)) {
      // Ensure we output -0 correctly
      os_ << "-0";
    } else {
      os_ << std::to_string(float_value);
    }
  } else if (llvh::isa<Block>(value)) {
    auto block = llvh::cast<Block>(value);
    PrintBlock(*block);
  } else if (auto* method = llvh::dyn_cast<FuncOp>(value)) {
    os_ << "method " << method->GetNameForDump();
  } else if (auto literal_bool = llvh::dyn_cast<LiteralBool>(value)) {
    os_ << (literal_bool->GetValue() ? "true" : "false");
  } else if (auto parameter = llvh::dyn_cast<Parameter>(value)) {
    os_ << "param [" << parameter->GetParamIndex() << "]";
  } else {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: IRPrinter::PrintValueLabel encountered unsupported "
        "Value kind");
  }

  PrintTypeLabel(value);
}

void IRPrinter::PrintInstruction(Instruction* inst) {
  if (PrintInstructionDestination(inst)) {
    os_ << " = ";
  } else {
    os_ << "   ";
  }

  if (inst->GetName() == "<this>" && need_escape_) {
    os_ << "\\<this\\>";
  } else {
    os_ << inst->GetName();
  }

  // Don't print the type of instructions without output.
  if (inst->HasOutput()) {
    os_ << " (";
    os_ << ':';
    PrintType(inst->GetType(), os_);
    os_ << ")";
  }
  bool first = true;
  if (llvh::isa<LoadConstInst>(inst)) {
    os_ << " ";
    os_ << "table index";
  }
  for (int i = 0, e = inst->GetNumOperands(); i < e; i++) {
    os_ << (first ? " " : ", ");
    PrintValueLabel(inst, inst->GetOperand(i), i);
    first = false;
  }

  if (inst->GetClosureVarReg() != constants::kInvalidSignedValue) {
    os_ << " [attr: closure var reg]: " << inst->GetClosureVarReg();
  }

  if (inst->GetToplevelVarReg() != constants::kInvalidSignedValue) {
    os_ << " [attr: var reg]: " << inst->GetToplevelVarReg();
  }

  if (llvh::isa<CallInst>(inst) && !inst->GetBuiltinFuncName().empty()) {
    os_ << " [attr: builtin function name]: " << inst->GetBuiltinFuncName();
  }
}

void IRPrinter::PrintTypeLabel(Value* value) {
  if (llvh::isa<Block>(value)) {
    return;
  }
  os_ << ": ";
  PrintType(value->GetType(), os_);
}

std::string IRPrinter::QuoteStr(const std::string& name) {
  if (name.find(" ") != std::string::npos || name.empty()) {
    return GetQuoteSign() + name + GetQuoteSign();
  }
  return name;
}

void ValueNamer::NextGeneration() {
  for (auto it = map_.begin(), end = map_.end(); it != end;) {
    auto cur = it++;
    if (cur->second.visited_gen_ != current_gen_) {
      map_.erase(cur);
    }
  }
  current_gen_ ^= 1;
}

void ValueNamer::Clear() {
  current_gen_ = 0;
  counter_ = 0;
  map_.clear();
}

void IRPrinter::PrintMethodHeader(FuncOp* func, bool skipClass) {
  os_ << QuoteStr(func->GetNameForDump());

  uint32_t idx = 0u - 1;
  os_ << "(";
  for (auto* p : func->GetParams()) {
    ++idx;
    if (idx > 0) {
      os_ << ", ";
    }
    os_ << "param [" << idx << "]";
    PrintTypeLabel(p);
  }
  os_ << ") : ";
  PrintType(func->GetRetTy(), os_);
}

void IRPrinter::PrintType(TypeOp* type, std::ostream& output_stream) {
  output_stream << *type;
}

void IRDumper::VisitModule(const ModuleOp& mod) {}

void IRDumper::VisitInstruction(const Instruction& I) {
  auto* mutable_inst = const_cast<Instruction*>(&I);

  if (false) {
    Indent(os_, indent_);
    os_ << "; ";
    PrintSourceLocation(mutable_inst->GetLocation());
    os_ << "\n";
  }

  Indent(os_, indent_);
  PrintInstruction(mutable_inst);
  os_ << "\n";
}

void IRDumper::VisitMethod(const FuncOp& M) {
  auto* mutable_func = const_cast<FuncOp*>(&M);
  llvh::SmallVector<Block*, 8> order{};
  for (auto& bb : *mutable_func) {
    order.push_back(&bb);
  }
  VisitMethod(M, order);
}

void IRDumper::VisitMethod(const FuncOp& func_op,
                           llvh::ArrayRef<Block*> order) {
  auto* mutable_func = const_cast<FuncOp*>(&func_op);
  Indent(os_, indent_);
  namer_.NewFunction(&func_op);
  // Number all instructions sequentially.
  for (auto* bb : order) {
    for (auto* inst : bb->InstRange()) {
      namer_.GetInstNumber(inst);
    }
  }

  PrintMethodHeader(mutable_func);
  os_ << "\n";

  if (false) {
    Indent(os_, indent_);
    os_ << "source location: ";
    PrintSourceLocation(func_op.GetLocation());
    os_ << "\n";
  }

  // Use IRVisitor dispatch to visit the basic blocks.
  for (auto* bb : order) {
    Visit(*bb);
  }

  Indent(os_, indent_);
  os_ << "function_end"
      << "\n";
  os_ << "\n";
}

void IRDumper::VisitBlock(const Block& block) {
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
  if (mutable_bb->GetType() == BlockType::BT_INST) {
    for (auto* inst : mutable_bb->InstRange()) {
      Visit(*inst);
    }
  } else {
    for (auto& op : *mutable_bb) {
      Visit(op);
    }
  }

  indent_ -= 2;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
