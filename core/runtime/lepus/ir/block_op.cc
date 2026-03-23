// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/block_op.h"

#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/raw_ostream.h"
#include "core/runtime/lepus/ir/region_op.h"
#include "core/runtime/lepus/ir/value.h"
#include "core/runtime/lepus/ir/value_forward_declare.h"

#ifdef LEPUS_TEST
#include "core/runtime/lepus/ir/ir_dumper.h"
#endif

namespace lynx {
namespace lepus {
namespace ir {
Block::Block(OpBuilder* builder, int64_t location, Region* parent,
             const BlockType type, llvh::StringRef& name)
    : Value(ValueKind::BlockKind, builder),
      parent_(parent),
      ir_ctx_(parent->GetIRCtx()),
      type_(type),
      name_(name),
      location_(location) {
  if (!parent) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: invalid parent region for Block");
  }
  parent->AddBlock(this);
}

Block::Block(OpBuilder* builder, Region* region, Block* bb,
             llvh::StringRef& name)
    : Value(ValueKind::BlockKind, builder),
      parent_(region),
      ir_ctx_(region->GetIRCtx()),
      type_(bb->GetType()),
      name_(name),
      location_(bb->GetLocation()) {
  if (!parent_) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: invalid parent region for Block");
  }
  region->AddBlock(this);
}

Block::Block(IRContext* ir_ctx, int64_t location)
    : Value(ValueKind::BlockKind, ir_ctx->GetOpBuilder()),
      parent_(nullptr),
      ir_ctx_(ir_ctx),
      type_(BlockType::BT_INST),
      name_("tmp"),
      location_(location) {}
Operation* Block::GetParentOp() { return GetParent()->GetParent(); }

std::string Block::GetNameWithType() const {
  std::string result = "";
  if (type_ != BlockType::BT_INST && type_ != BlockType::BT_FUNC)
    result = GetTypeString().str();

  result += ".";
  if (name_.empty()) return result + "block";
  return result + name_.str();
}

bool Block::HasTerminalInst() const {
  if (LEPUS_LIKELY(empty())) return false;

  if (GetType() != BlockType::BT_TOPLEVEL && GetType() != BlockType::BT_INST) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: HasTerminalInst expects toplevel/inst block");
  }
  auto* inst = Back();
  if (!inst) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: block Back() returned nullptr");
  }
  return llvh::isa<TerminatorInst>(inst);
}

void Block::Remove(Operation* op) {
  update_inst_id_ = true;
  op->ClearBlock();
  op_list_.remove(op);
}
void Block::Erase(Operation* op) {
  update_inst_id_ = true;
  op->ClearBlock();
  op_list_.erase(op);
}
void Block::PushBack(Operation* op) {
  update_inst_id_ = true;
  op_list_.push_back(op);
}

void Block::Dump(std::ostream& os) const {
#ifdef LEPUS_TEST
  IRPrinter ir_printer(ir_ctx_, os);
  ir_printer.namer_.NewFunction(this->GetParent()->GetFunction());
  os << GetNameWithType() << "\n";
  for (auto& op : op_list_) {
    ir_printer.PrintInstruction(
        llvh::dyn_cast<Instruction>(const_cast<Operation*>(&op)));
    os << "\n";
  }
  os << "\n";
#else
  // Dump utilities are only available when LEPUS_TEST is enabled.
  (void)os;
#endif
}

void Block::PrintAsOperand(std::ostream& os, bool) const {
  // Use the address of the basic block when LLVM prints the CFG.
  os << "BB#" << GetName();
}

void Block::PrintAsOperand(llvh::raw_ostream& os, bool) const {
  // Keep LLVM utilities working (e.g. DominatorTree printing) while allowing
  // std::ostream-based dumpers in our codebase.
  os << "BB#" << GetName();
}

TerminatorInst* Block::GetTerminator() {
  if (op_list_.empty()) return nullptr;
  return llvh::dyn_cast<TerminatorInst>(&op_list_.back());
}

const TerminatorInst* Block::GetTerminator() const {
  if (op_list_.empty()) return nullptr;
  return llvh::dyn_cast<TerminatorInst>(&op_list_.back());
}

void Block::RemoveFromParent() {
  [[maybe_unused]] bool ret = GetParent()->RemoveBlock(this);
  if (!ret) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: failed to remove block from parent region");
  }
}

void Block::EraseFromParent() {
  // Erase all of the instructions in the block before deleting the block.
  // We are starting to delete from the start of the block. Naturally we will
  // have forward dependencies between instructions. To allow safe deletion
  // we replace all uses with the invalid null value. SetOperand knows how
  // to deal with null values.
  while (begin() != end()) {
    begin()->ReplaceAllUsesWith(nullptr);
    begin()->EraseFromParent();
  }

  if (HasUsers()) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: cannot erase block with non-empty use list");
  }
  // // Erase the block itself:
  RemoveFromParent();
}

bool Block::IsEntryBlock() const { return GetParent()->IsEntryBlock(this); }
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::Block>::deleteNode(
    ::lynx::lepus::ir::Block* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
