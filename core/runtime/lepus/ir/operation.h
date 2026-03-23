// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_OPERATION_H_
#define CORE_RUNTIME_LEPUS_IR_OPERATION_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/lepus/ir/dialect.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist_node.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class Region;
class IRContext;
class Value;
class FuncOp;
class Block;
class Operation;

class Operation : public llvh::ilist_node<Operation>, public Value {
  NON_COPYABLE(Operation);

  using RegionListTy = llvh::SmallVector<std::unique_ptr<Region>, 1>;

 public:
  struct OperandRegionIterator {
    using region_iterator = RegionListTy::iterator;

    OperandRegionIterator(const Operation* op, bool is_end = false) {
      auto& regions = const_cast<Operation*>(op)->GetRegions();
      if (is_end)
        iterator = regions.end();
      else
        iterator = regions.begin();
    }
    OperandRegionIterator(const OperandRegionIterator& other) {
      this->operator=(other);
    }
    OperandRegionIterator& operator=(const OperandRegionIterator& other) {
      iterator = other.iterator;
      return *this;
    }

    Region* operator*() const { return (*iterator).get(); }

    OperandRegionIterator& operator++() {
      ++iterator;
      return *this;
    }

    bool operator==(const OperandRegionIterator& other) {
      return iterator == other.iterator;
    }

    bool operator!=(const OperandRegionIterator& other) {
      return iterator != other.iterator;
    }

    region_iterator iterator;
  };

 public:
  explicit Operation(ValueKind kind, Block* parent, OpBuilder* builder,
                     int64_t location = 0);
  explicit Operation(ValueKind kind, Attributes* attrs, Block* parent,
                     OpBuilder* builder, int64_t location = 0);
  ~Operation();
  IRContext* GetIRCtx() const {
    if (!ir_ctx_) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Operation has null IRContext");
    }
    return ir_ctx_;
  }
  void SetIRCtx(IRContext* ir_ctx) {
    if (!ir_ctx) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: SetIRCtx received nullptr");
    }
    this->ir_ctx_ = ir_ctx;
  }

  template <typename... Args>
  Region* AddRegion(Args&&... args) {
    auto region = std::make_unique<Region>(std::forward<Args>(args)...);
    region->SetUUID();
    regions_.emplace_back(std::move(region));
    return regions_.back().get();
  }
  RegionListTy& GetRegions() { return regions_; }
  const RegionListTy& GetRegions() const { return regions_; }
  Region* GetRegion(uint16_t idx) const {
    if (LEPUS_UNLIKELY(idx >= GetRegionSize())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: region index overflow");
    }
    return GetRegions()[idx].get();
  }
  uint32_t GetRegionSize() const { return regions_.size(); }
  Region* GetSingleRegion() const {
    if (LEPUS_UNLIKELY(GetRegionSize() != 1)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: expected exactly one region");
    }
    return GetRegions()[0].get();
  }
  Region* GetParentRegion() const;
  Block* GetParent() const;
  void ClearBlock() { parent_ = nullptr; }
  Block* GetBlock() const { return GetParent(); }
  void SetParent(Block* parent) { this->parent_ = parent; }
  int64_t GetLocation() const { return location_; }

  void SetLocation(int64_t loc) { location_ = loc; }
  bool HasLocation() const { return location_ != 0; }
  const Dialect* GetDialect() const;
  std::string GetBaseName() const;
  FuncOp* GetFunction() const;
  Operation* GetParentOp() const;
  void RemoveFromParent();
  void EraseFromParent();
  Block& front();
  uint64_t GetRegionUUID();
  void SetBlockId(uint32_t block_id) { this->block_id_ = block_id; }
  uint32_t GetBlockId() const { return block_id_; }

  // ------------------ judge --------------------------------
  bool IsFuncOp() const;
  bool IsModuleOp() const;

  // ------------------ iterator -----------------------------
  OperandRegionIterator RegionBegin() const {
    return OperandRegionIterator(this, false);
  }

  OperandRegionIterator RegionEnd() const {
    return OperandRegionIterator(this, true);
  }

  llvh::iterator_range<OperandRegionIterator> RegionRange() {
    return llvh::iterator_range<OperandRegionIterator>(RegionBegin(),
                                                       RegionEnd());
  }

  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), Value);
  }

 protected:
  Block* parent_ = nullptr;
  IRContext* ir_ctx_ = nullptr;
  RegionListTy regions_;
  uint32_t block_id_ = -1;
  const Dialect* dialect_ = nullptr;
  int64_t location_ = 0;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_OPERATION_H_
