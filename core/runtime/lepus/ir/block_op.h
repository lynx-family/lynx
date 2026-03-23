// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_BLOCK_OP_H_
#define CORE_RUNTIME_LEPUS_IR_BLOCK_OP_H_

#include <iostream>
#include <ostream>
#include <string>

#include "core/runtime/lepus/ir/instruction.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/value.h"

namespace llvh {
class raw_ostream;
}  // namespace llvh

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;
class Value;
class Region;
class Instruction;
class TerminatorInst;

enum class BlockType : uint8_t {
  BT_TOPLEVEL = 0,
  BT_FUNC,
  BT_INST,
  BT_TYPE,
  BT_COUNT
};

class Block : public llvh::ilist_node<Block>, public Value {
  NON_COPYABLE(Block);

 public:
  friend class Region;
  friend class OpBuilder;

  using OpListType = llvh::iplist<Operation>;
  using iterator = OpListType::iterator;
  using reverse_iterator = OpListType::reverse_iterator;
  using const_iterator = OpListType::const_iterator;
  template <bool is_reverse>
  struct BlockIterator {
    using iterator = Block::OpListType::iterator;
    using const_iterator = Block::OpListType::const_iterator;
    static iterator begin(OpListType& list) { return list.begin(); }

    static const_iterator const_begin(const OpListType& list) {
      return list.begin();
    }

    static iterator end(OpListType& list) { return list.end(); }

    static const_iterator const_end(const OpListType& list) {
      return list.end();
    }
  };

  template <>
  struct BlockIterator<true> {
    using iterator = Block::OpListType::reverse_iterator;
    using const_iterator = Block::OpListType::const_reverse_iterator;
    static iterator begin(OpListType& list) { return list.rbegin(); }

    static const_iterator const_begin(const OpListType& list) {
      return list.rbegin();
    }

    static iterator end(OpListType& list) { return list.rend(); }

    static const_iterator const_end(const OpListType& list) {
      return list.rend();
    }
  };

  template <typename op_impl_type, bool is_reverse>
  class BlockOperationIterator {
    using iterator = typename BlockIterator<is_reverse>::iterator;
    using self_type = BlockOperationIterator<op_impl_type, is_reverse>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = op_impl_type;
    using difference_type = std::ptrdiff_t;
    using pointer = op_impl_type*;
    using reference = op_impl_type*;

   public:
    BlockOperationIterator() = default;
    BlockOperationIterator(Block* op, bool is_end = false) {
      auto& op_list = op->GetOpList();
      if (is_end)
        op_iterator_ = BlockIterator<is_reverse>::end(op_list);
      else
        op_iterator_ = BlockIterator<is_reverse>::begin(op_list);
    }
    BlockOperationIterator(const self_type& other) { this->operator=(other); }
    self_type& operator=(const self_type& other) {
      op_iterator_ = other.op_iterator_;
      return *this;
    }
    op_impl_type* operator*() {
      auto op = &*op_iterator_;
      if (auto impl = llvh::dyn_cast<op_impl_type>(op)) return impl;
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BlockOperationIterator type mismatch");
    }
    self_type& operator++() {
      ++op_iterator_;
      return *this;
    }
    self_type& operator--() {
      --op_iterator_;
      return *this;
    }
    bool operator==(const self_type& other) const {
      return op_iterator_ == other.op_iterator_;
    }
    bool operator!=(const self_type& other) const {
      return op_iterator_ != other.op_iterator_;
    }

   private:
    iterator op_iterator_;
  };

  template <typename op_impl_type, bool is_reverse>
  class BlockConstOperationIterator {
    using iterator = typename BlockIterator<is_reverse>::const_iterator;
    using self_type = BlockConstOperationIterator<op_impl_type, is_reverse>;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = op_impl_type;
    using difference_type = std::ptrdiff_t;
    using pointer = op_impl_type*;
    using reference = op_impl_type*;

   public:
    BlockConstOperationIterator() = default;
    BlockConstOperationIterator(const Block* op, bool is_end = false) {
      auto& op_list = op->GetOpList();
      if (is_end)
        op_iterator_ = BlockIterator<is_reverse>::const_end(op_list);
      else
        op_iterator_ = BlockIterator<is_reverse>::const_begin(op_list);
    }
    const op_impl_type* operator*() const {
      auto op = &*op_iterator_;
      if (auto impl = llvh::dyn_cast<op_impl_type>(op)) return impl;
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BlockConstOperationIterator type mismatch");
    }
    BlockConstOperationIterator(const self_type& other) {
      this->operator=(other);
    }
    self_type& operator=(const self_type& other) {
      op_ = other.op_;
      op_iterator_ = other.op_iterator_;
      return *this;
    }
    self_type& operator++() {
      ++op_iterator_;
      return *this;
    }
    self_type& operator--() {
      --op_iterator_;
      return *this;
    }
    bool operator==(const self_type& other) const {
      return op_ == other.op_ && op_iterator_ == other.op_iterator_;
    }
    bool operator!=(const self_type& other) const {
      return op_ != other.op_ || op_iterator_ != other.op_iterator_;
    }

   private:
    const Block* op_ = nullptr;
    iterator op_iterator_;
  };

  explicit Block(OpBuilder* builder, int64_t location, Region* parent,
                 const BlockType type, llvh::StringRef& name);
  explicit Block(OpBuilder* builder, Region* region, Block* bb,
                 llvh::StringRef& name);
  IRContext* GetIRCtx() const {
    if (!ir_ctx_) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Block has null IRContext");
    }
    return ir_ctx_;
  }

 private:
  explicit Block(IRContext* ir_ctx, int64_t location = 0);

 public:
  /// iterator
  iterator begin() { return op_list_.begin(); }
  iterator end() { return op_list_.end(); }
  reverse_iterator rbegin() { return op_list_.rbegin(); }
  reverse_iterator rend() { return op_list_.rend(); }
  const_iterator begin() const { return op_list_.begin(); }
  const_iterator end() const { return op_list_.end(); }

#define DEF_ITERATOR_BY_IMPL_TYPE(Type, Prefix)                              \
  using Prefix##Iterator = BlockOperationIterator<Type, false>;              \
  using Prefix##ReverseIterator = BlockOperationIterator<Type, true>;        \
  using Prefix##ConstIterator = BlockConstOperationIterator<Type, false>;    \
  using Prefix##ConstReverseIterator =                                       \
      BlockConstOperationIterator<Type, true>;                               \
  using Prefix##IteratorRange = llvh::iterator_range<Prefix##Iterator>;      \
  using Prefix##ConstIteratorRange =                                         \
      llvh::iterator_range<const Prefix##ConstIterator>;                     \
                                                                             \
  Prefix##Iterator Prefix##Begin() { return Prefix##Iterator(this, false); } \
                                                                             \
  Prefix##Iterator Prefix##End() { return Prefix##Iterator(this, true); }    \
                                                                             \
  Prefix##ReverseIterator Prefix##Rbegin() {                                 \
    return Prefix##ReverseIterator(this, false);                             \
  }                                                                          \
                                                                             \
  Prefix##ReverseIterator Prefix##Rend() {                                   \
    return Prefix##ReverseIterator(this, true);                              \
  }                                                                          \
                                                                             \
  Prefix##ConstIterator Prefix##ConstBegin() const {                         \
    return Prefix##ConstIterator(this, false);                               \
  }                                                                          \
                                                                             \
  Prefix##ConstIterator Prefix##ConstEnd() const {                           \
    return Prefix##ConstIterator(this, true);                                \
  }                                                                          \
                                                                             \
  Prefix##ConstReverseIterator Prefix##ConstRbegin() const {                 \
    return Prefix##ConstReverseIterator(this, false);                        \
  }                                                                          \
                                                                             \
  Prefix##ConstReverseIterator Prefix##ConstRend() const {                   \
    return Prefix##ConstReverseIterator(this, true);                         \
  }                                                                          \
                                                                             \
  Prefix##IteratorRange Prefix##Range() {                                    \
    return Prefix##IteratorRange(Prefix##Begin(), Prefix##End());            \
  }                                                                          \
                                                                             \
  Prefix##ConstIteratorRange Prefix##ConstRange() const {                    \
    return Prefix##ConstIteratorRange(Prefix##ConstBegin(),                  \
                                      Prefix##ConstEnd());                   \
  }

  DEF_ITERATOR_BY_IMPL_TYPE(Instruction, Inst);
  DEF_ITERATOR_BY_IMPL_TYPE(FuncOp, Func);
  DEF_ITERATOR_BY_IMPL_TYPE(TypeOp, Type);

#undef DEF_ITERATOR_BY_IMPL_TYPE

  /// A debug utility that dumps the textual representation of the IR to \p os,
  /// defaults to stdout.
  LEPUS_DUMP_METHOD void Dump(std::ostream& os = std::cout) const;

  /// Used by LLVM's graph trait.
  void PrintAsOperand(std::ostream& os, bool) const;
  void PrintAsOperand(llvh::raw_ostream& os, bool) const;

  /// \brief Returns the terminator instruction if the block is well formed or
  /// null if the block is not well formed.
  TerminatorInst* GetTerminator();
  const TerminatorInst* GetTerminator() const;

  void PushBack(Operation* op);
  void RemoveFromParent();
  void EraseFromParent();
  void Remove(Operation* op);
  void Erase(Operation* op);
  void Insert(Block::iterator insert_point, Operation* op,
              bool reset_parent = true) {
    update_inst_id_ = true;
    GetOpList().insert(insert_point, op);
    if (reset_parent) op->SetParent(this);
  }
  Region* GetParent() const { return parent_; }
  void SetParent(Region* parent) { this->parent_ = parent; }
  inline size_t size() const { return op_list_.size(); }
  inline bool empty() const { return op_list_.empty(); }
  Operation* GetParentOp();
  bool HasTerminalInst() const;
  bool IsEntryBlock() const;

  // -------- wrapper for llvh GenericDomTree -----------
  Operation* getParentOp() { return GetParentOp(); }
  Region* getParent() const { return GetParent(); }
  void printAsOperand(llvh::raw_ostream& os, bool val) const {
    PrintAsOperand(os, val);
  }
  void printAsOperand(std::ostream& os, bool val) const {
    PrintAsOperand(os, val);
  }

  // -------- getter and setter -----------
  BlockType GetType() const { return type_; }
  OpListType& GetOpList() { return op_list_; }
  const OpListType& GetOpList() const { return op_list_; }
  int64_t GetLocation() const { return location_; }
  void SetLocation(int64_t loc) { location_ = loc; }

  // ------------ iterator ---------------
  inline Instruction* Front() { return *InstBegin(); }
  inline const Instruction* Front() const { return *InstConstBegin(); }
  inline Instruction* Back() {
    auto iter = InstEnd();
    return *--iter;
  }
  inline const Instruction* Back() const {
    auto iter = InstConstEnd();
    return *--iter;
  }
  // --------------------- debugger -------------
  llvh::StringRef GetTypeString() const {
    switch (type_) {
      case BlockType::BT_FUNC:
        return "block.func";
      case BlockType::BT_INST:
        return "block.inst";
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: invalid BlockType");
    }
  }

  // ----------------- debug ----------------------
  std::string GetName() const { return name_.str(); }
  std::string GetNameWithType() const;

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::BlockKind;
  }

 private:
  OpListType op_list_{};
  Region* parent_ = nullptr;
  IRContext* ir_ctx_ = nullptr;
  BlockType type_ = BlockType::BT_COUNT;
  llvh::StringRef name_;
  int64_t location_;
  bool update_inst_id_ = true;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_BLOCK_OP_H_
