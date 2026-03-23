// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_MODULE_OP_H_
#define CORE_RUNTIME_LEPUS_IR_MODULE_OP_H_

#include <iostream>
#include <ostream>
#include <utility>

// clang-format off
#include "core/runtime/lepus/ir/value_forward_declare.h"
// clang-format on

#include "core/runtime/lepus/ir/block_op.h"
#include "core/runtime/lepus/ir/func_op.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/literal.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/owning_folding_set.h"
#include "core/runtime/lepus/ir/region_op.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class Block;
class IRContext;
class FuncOp;
class ModuleOp;
class TerminatorInst;
class Region;

/// Deleter for Values.
struct ValueDeleter {
  void operator()(Value* v) { Value::Destroy(v); }
};

class ModuleOp : public llvh::ilist_node<ModuleOp>, public Operation {
  friend class Value;
  NON_COPYABLE(ModuleOp);

  enum ModRegionType : uint8_t { RT_IR = 0, RT_COUNT };
  enum ModBlockType : uint8_t {
    MRT_FUNC = 0,
    MRT_TOPLEVEL_CODE,
    MRT_TYPE,
    MRT_COUNT
  };

 private:
  template <typename op_impl_type, ModBlockType block_type>
  struct ModuleIterator {
    using block_iterator = Block::BlockOperationIterator<op_impl_type, false>;
    using self_type = ModuleIterator<op_impl_type, block_type>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = op_impl_type;
    using difference_type = std::ptrdiff_t;
    using pointer = op_impl_type*;
    using reference = op_impl_type*;

    ModuleIterator(ModuleOp* mod, bool is_end = false) {
      if (LEPUS_UNLIKELY(!mod)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleIterator constructed with nullptr ModuleOp");
      }
      if (LEPUS_UNLIKELY(mod->GetRegionSize() != 1)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleIterator expects ModuleOp to have exactly "
            "one region");
      }

      auto* ir_region = mod->GetIRRegion();
      if (LEPUS_UNLIKELY(!ir_region)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleIterator failed to get IR region");
      }
      if (LEPUS_UNLIKELY(ModBlockType::MRT_COUNT !=
                         ir_region->GetBlockSize())) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleIterator block layout mismatch in IR "
            "region");
      }

      auto* block = ir_region->GetBlock(block_type);
      if (LEPUS_UNLIKELY(!block)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleIterator failed to get block for requested "
            "type");
      }
      iterator = block_iterator(block, is_end);
    }

    ModuleIterator(const self_type& other) { this->operator=(other); }

    ModuleIterator(const self_type&& other) {
      this->iterator = std::move(other.iterator);
    }

    self_type& operator=(const self_type& other) {
      iterator = other.iterator;
      return *this;
    }

    op_impl_type* operator*() { return *iterator; }

    self_type& operator++() {
      ++iterator;
      return *this;
    }

    self_type& operator--() {
      --iterator;
      return *this;
    }

    bool operator==(const self_type& other) {
      return iterator == other.iterator;
    }

    bool operator!=(const self_type& other) {
      return iterator != other.iterator;
    }

   private:
    block_iterator iterator;
  };

  template <typename op_impl_type, ModBlockType block_type>
  struct ModuleConstIterator {
    using block_iterator =
        Block::BlockConstOperationIterator<op_impl_type, false>;
    using self_type = ModuleConstIterator<op_impl_type, block_type>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = op_impl_type;
    using difference_type = std::ptrdiff_t;
    using pointer = op_impl_type*;
    using reference = op_impl_type*;

    ModuleConstIterator(const ModuleOp* mod, bool is_end = false) {
      if (LEPUS_UNLIKELY(!mod)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleConstIterator constructed with nullptr "
            "ModuleOp");
      }
      if (LEPUS_UNLIKELY(mod->GetRegionSize() != 1)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleConstIterator expects ModuleOp to have "
            "exactly one region");
      }

      auto* ir_region = mod->GetIRRegion();
      if (LEPUS_UNLIKELY(!ir_region)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleConstIterator failed to get IR region");
      }
      if (LEPUS_UNLIKELY(ModBlockType::MRT_COUNT !=
                         ir_region->GetBlockSize())) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleConstIterator block layout mismatch in IR "
            "region");
      }

      auto* block = ir_region->GetBlock(block_type);
      if (LEPUS_UNLIKELY(!block)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: ModuleConstIterator failed to get block for "
            "requested type");
      }
      iterator = block_iterator(block, is_end);
    }

    ModuleConstIterator(const self_type& other) { this->operator=(other); }

    ModuleConstIterator(const self_type&& other) {
      this->iterator = std::move(other.iterator);
    }

    self_type& operator=(const self_type& other) {
      iterator = other.iterator;
      return *this;
    }

    const op_impl_type* operator*() const { return *iterator; }

    self_type& operator++() {
      ++iterator;
      return *this;
    }

    self_type& operator--() {
      --iterator;
      return *this;
    }

    bool operator==(const self_type& other) const {
      return iterator == other.iterator;
    }

    bool operator!=(const self_type& other) const {
      return iterator != other.iterator;
    }

   private:
    block_iterator iterator;
  };

 public:
  void SetRootFunction(FuncOp* func) { root_function_ = func; }
  FuncOp* GetRootFunction() { return root_function_; }
  int32_t GenerateClosureFunctionId() { return closure_function_id_++; }

 private:
  llvh::StringRef name_ = "";
  int32_t closure_function_id_ = 0;

  // type of value
  LiteralBool literal_false_{false};
  LiteralBool literal_true_{true};
  LiteralNull literal_null_{};
  LiteralUndefined literal_undefined_{};

  OwningFoldingSet<LiteralInt8, ValueDeleter> literal_int8s_{};
  OwningFoldingSet<LiteralInt32, ValueDeleter> literal_int32s_{};
  OwningFoldingSet<LiteralUint8, ValueDeleter> literal_uint8s_{};
  OwningFoldingSet<LiteralUint32, ValueDeleter> literal_uint32s_{};
  OwningFoldingSet<LiteralFloat64, ValueDeleter> literal_float64s_{};

  uint64_t region_uuid_ = 0;
  FuncOp* root_function_ = nullptr;

 public:
  explicit ModuleOp(Block* parent, OpBuilder* builder, int64_t location,
                    IRContext* ir_ctx, llvh::StringRef name);
  ~ModuleOp() = default;
  void Init();

  IRContext* GetIRCtx() const;

  uint64_t GetRegionUUID() { return region_uuid_++; }

  // --------------- iterator --------------------
  using func_list_iterator = ModuleIterator<FuncOp, ModBlockType::MRT_FUNC>;
  using func_list_const_iterator =
      ModuleConstIterator<FuncOp, ModBlockType::MRT_FUNC>;
  using inst_list_iterator =
      ModuleIterator<FuncOp, ModBlockType::MRT_TOPLEVEL_CODE>;
  using inst_list_const_iterator =
      ModuleConstIterator<FuncOp, ModBlockType::MRT_TOPLEVEL_CODE>;

  func_list_iterator begin() { return func_list_iterator(this, false); }
  func_list_iterator end() { return func_list_iterator(this, true); }
  func_list_const_iterator begin() const {
    return func_list_const_iterator(this, false);
  }
  func_list_const_iterator end() const {
    return func_list_const_iterator(this, true);
  }
  inst_list_iterator InstBegin() { return inst_list_iterator(this, false); }
  inst_list_iterator InstEnd() { return inst_list_iterator(this, true); }
  inst_list_const_iterator InstBegin() const {
    return inst_list_const_iterator(this, false);
  }
  inst_list_const_iterator InstEnd() const {
    return inst_list_const_iterator(this, true);
  }

  // ------------------- iterator range ---------------------------
  using func_list_range = llvh::iterator_range<func_list_iterator>;
  using func_list_const_range = llvh::iterator_range<func_list_const_iterator>;
  using inst_list_range = llvh::iterator_range<inst_list_iterator>;
  using inst_list_const_range = llvh::iterator_range<inst_list_const_iterator>;

  func_list_range FuncRange() { return func_list_range(begin(), end()); }
  inst_list_range InstRange() {
    return inst_list_range(InstBegin(), InstEnd());
  }

  // -------------------- get constant literal value ---------------
  LiteralNull* GetLiteralNull() { return &literal_null_; }
  LiteralUndefined* GetLiteralUndefined() { return &literal_undefined_; }
  LiteralInt8* GetLiteralInt8(int8_t value);
  LiteralInt32* GetLiteralInt32(int64_t value);
  LiteralUint8* GetLiteralUint8(uint8_t value);
  LiteralUint32* GetLiteralUint32(uint32_t value);
  LiteralFloat64* GetLiteralFloat64(double value);
  LiteralBool* GetLiteralBool(bool value);
  Region* GetIRRegion() const {
    if (LEPUS_UNLIKELY(GetRegionSize() <= ModRegionType::RT_IR)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ModuleOp::GetIRRegion region index out of range");
    }
    return GetRegion(ModRegionType::RT_IR);
  }
  Block* GetTypeBlock() const {
    return GetIRRegion()->GetBlock(ModBlockType::MRT_TYPE);
  }
  Block* GetInstBlock() const {
    return GetIRRegion()->GetBlock(ModBlockType::MRT_TOPLEVEL_CODE);
  }
  Block* GetFunctionBlock() const {
    return GetIRRegion()->GetBlock(ModBlockType::MRT_FUNC);
  }
  void Dump(StageMode mode = StageMode::SM_HIR, std::ostream& os = std::cout);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::ModuleOpKind;
  }
};

/// This represents a function parameter.
class Parameter : public Value {
  friend class FuncOp;
  Parameter(const Parameter&) = delete;
  void operator=(const Parameter&) = delete;

  /// The function that contains this paramter.
  FuncOp* parent_;

 public:
  explicit Parameter(FuncOp* parent, int32_t param_index);

  FuncOp* GetParent() const { return parent_; }
  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::ParameterKind;
  }

  int32_t GetParamIndex() const { return param_index_; }

 private:
  int32_t param_index_;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_MODULE_OP_H_
