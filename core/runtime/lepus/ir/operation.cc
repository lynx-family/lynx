// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/operation.h"

#include <string>

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/region_op.h"

namespace lynx {
namespace lepus {
namespace ir {
Operation::Operation(ValueKind kind, Block* parent, OpBuilder* builder,
                     int64_t location)
    : Value(kind, builder), parent_(parent), location_(location) {
  if (parent) ir_ctx_ = parent->GetIRCtx();
  dialect_ = DialectRegistry::SharedInstance()->GetDialect(GetKind());
}

Operation::~Operation() {
  llvh::for_each(regions_,
                 [&](std::unique_ptr<Region>& region) { region.reset(); });
}

bool Operation::IsModuleOp() const { return llvh::isa<ModuleOp>(this); }

bool Operation::IsFuncOp() const { return llvh::isa<FuncOp>(this); }

Block* Operation::GetParent() const { return parent_; }

Operation* Operation::GetParentOp() const {
  if (IsFuncOp()) return llvh::cast<FuncOp>(this)->GetParentOp();

  return GetParentRegion()->GetParent();
}

Region* Operation::GetParentRegion() const {
  if (llvh::isa<FuncOp>(this)) return nullptr;

  auto* region = GetParent()->GetParent();
  if (LEPUS_UNLIKELY(!region)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Operation::GetParentRegion found nullptr parent "
        "region");
  }
  return region;
}

FuncOp* Operation::GetFunction() const {
  return GetParentRegion()->GetFunction();
}

void Operation::RemoveFromParent() { GetParent()->Remove(this); }

void Operation::EraseFromParent() {
  // Release this instruction from the use-list of other instructions.
  if (auto inst = llvh::dyn_cast<Instruction>(this))
    for (unsigned i = 0; i < inst->GetNumOperands(); i++)
      inst->SetOperand(nullptr, i);
  GetParent()->Erase(this);
}

Block& Operation::front() {
  if (!llvh::isa<FuncOp>(this) || GetRegions().size() != 1) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Operation::front is only valid for FuncOp with a "
        "single region");
  }
  return GetSingleRegion()->Front();
}

uint64_t Operation::GetRegionUUID() {
  if (auto func_op = llvh::dyn_cast<FuncOp>(this))
    return func_op->GetRegionUUID();
  else if (auto mod_op = llvh::dyn_cast<ModuleOp>(this))
    return mod_op->GetRegionUUID();

  throw ::lynx::lepus::CompileException(
      "Lepus IR error: Operation::GetRegionUUID is only supported for "
      "FuncOp/ModuleOp");
}

const Dialect* Operation::GetDialect() const {
  if (LEPUS_UNLIKELY(!dialect_)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Operation::GetDialect returned nullptr dialect");
  }
  return dialect_;
}

std::string Operation::GetBaseName() const {
  // `GetKindStr()` returns `std::string` (owning), so build a full op name
  // using `std::string` to avoid mixing with `llvh::StringRef`.
  return GetDialect()->GetName() + "." + GetKindStr();
}

}  // namespace ir

}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::Operation>::deleteNode(
    ::lynx::lepus::ir::Operation* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
