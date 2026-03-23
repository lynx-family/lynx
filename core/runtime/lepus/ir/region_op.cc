// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// clang-format off
#include "core/runtime/lepus/ir/value_forward_declare.h"
// clang-format on

#include "core/runtime/lepus/ir/region_op.h"

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {
Region::Region(Operation* parent, const llvh::StringRef name)
    : parent_(parent), name_(name) {
  // If the parent is a function op, region belongs to that function.
}

Block* Region::GetUniqueBlock() {
  if (LEPUS_UNLIKELY(blocks_.size() != 1)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetUniqueBlock expects exactly one block");
  }
  return &*blocks_.begin();
}

IRContext* Region::GetIRCtx() {
  if (LEPUS_UNLIKELY(!parent_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetIRCtx called with nullptr parent "
        "operation");
  }
  return parent_->GetIRCtx();
}

Block* Region::GetBlock(uint32_t idx) {
  if (LEPUS_UNLIKELY(idx >= blocks_.size())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetBlock index out of range");
  }
  auto iter = blocks_.begin();
  while (idx--) iter++;
  return &*iter;
}

Block* Region::GetEntryBlock() {
  if (LEPUS_UNLIKELY(blocks_.empty())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetEntryBlock called on empty region");
  }
  return &*blocks_.begin();
}

void Region::AddBlock(Block* block) {
  block->SetParent(this);
  blocks_.push_back(block);
}

bool Region::RemoveBlock(Block* block) {
  if (LEPUS_UNLIKELY(!block)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::RemoveBlock called with nullptr block");
  }
  auto iter = llvh::find_if(
      GetBlocks(), [&](Block& tmp_block) { return &tmp_block == block; });
  if (iter != GetBlocks().end()) {
    GetBlocks().remove(iter);
    return true;
  }
  return false;
}

Operation* Region::GetTopLevelOperation() const {
  auto parent = GetParent();
  if (LEPUS_UNLIKELY(!parent)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetTopLevelOperation called with nullptr "
        "parent operation");
  }
  while (parent) {
    if (llvh::isa<ModuleOp>(parent)) return parent;
    if (parent->IsFuncOp()) return parent;
    parent = parent->GetParentOp();
  }

  throw ::lynx::lepus::CompileException(
      "Lepus IR error: Region::GetTopLevelOperation failed to find "
      "ModuleOp/FuncOp");
}

FuncOp* Region::GetFunction() const {
  Operation* op = GetTopLevelOperation();
  if (LEPUS_UNLIKELY(!llvh::isa<FuncOp>(op))) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetFunction called in non-function (toplevel) "
        "region");
  }
  return llvh::cast<FuncOp>(op);
}

Operation* Region::GetNearestFuncOrClassOrModule() const {
  auto parent = GetParent();
  if (LEPUS_UNLIKELY(!parent)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::GetNearestFuncOrClassOrModule called with "
        "nullptr parent operation");
  }
  while (parent) {
    if ((parent->IsFuncOp()) || parent->IsModuleOp()) return parent;
    parent = parent->GetParentOp();
  }
  return nullptr;
}

void Region::SetUUID() {
  auto* op = GetNearestFuncOrClassOrModule();
  if (LEPUS_UNLIKELY(!op)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Region::SetUUID failed to find enclosing "
        "FuncOp/ModuleOp");
  }
  this->uuid_ = op->GetRegionUUID();
}

Block& Region::Front() { return blocks_.front(); }

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
