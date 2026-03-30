// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_INSTRS_H_
#define CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_INSTRS_H_

#include <utility>

#include "core/runtime/lepus/ir/attributes.h"
#include "core/runtime/lepus/ir/attributes_base.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/literal.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class NeqCondBranchInst : public TerminatorInst {
  NON_COPYABLE(NeqCondBranchInst);

 public:
  enum OperandKind : uint8_t {
    LeftValueIdx = 0,
    RightValueIdx,
    TrueBlockIdx,
    FalseBlockIdx
  };

  explicit NeqCondBranchInst(Block* parent, OpBuilder* builder,
                             int64_t location, Value* left, Value* right,
                             Block* true_block, Block* false_block);

  DEF_DEFAULT_COPY_CONSTRUCTOR(NeqCondBranchInst, TerminatorInst);

  Block* GetTrueDest() const {
    return llvh::cast<Block>(GetOperand(TrueBlockIdx));
  }
  Block* GetFalseDest() const {
    return llvh::cast<Block>(GetOperand(FalseBlockIdx));
  }

  Value* GetLeftHandSide() const { return GetOperand(LeftValueIdx); }
  Value* GetRightHandSide() const { return GetOperand(RightValueIdx); }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  unsigned GetNumSuccessorsImpl() const { return 2; }
  Block* GetSuccessorImpl(unsigned idx) const {
    if (idx == 0) return GetTrueDest();
    if (idx == 1) return GetFalseDest();
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: NeqCondBranchInst::GetSuccessorImpl idx out of range");
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    if (LEPUS_UNLIKELY(idx > 1)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: NeqCondBranchInst::SetSuccessorImpl idx out of "
          "range");
    }
    SetOperand(llvh::cast<Value>(b), idx + TrueBlockIdx);
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::NeqCondBranchInstKind;
  }
};

class EqCondBranchInst : public TerminatorInst {
  NON_COPYABLE(EqCondBranchInst);

 public:
  enum OperandKind : uint8_t {
    LeftValueIdx = 0,
    RightValueIdx,
    TrueBlockIdx,
    FalseBlockIdx
  };

  explicit EqCondBranchInst(Block* parent, OpBuilder* builder, int64_t location,
                            Value* left, Value* right, Block* true_block,
                            Block* false_block);

  DEF_DEFAULT_COPY_CONSTRUCTOR(EqCondBranchInst, TerminatorInst);

  Block* GetTrueDest() const {
    return llvh::cast<Block>(GetOperand(TrueBlockIdx));
  }
  Block* GetFalseDest() const {
    return llvh::cast<Block>(GetOperand(FalseBlockIdx));
  }

  Value* GetLeftHandSide() const { return GetOperand(LeftValueIdx); }
  Value* GetRightHandSide() const { return GetOperand(RightValueIdx); }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  unsigned GetNumSuccessorsImpl() const { return 2; }
  Block* GetSuccessorImpl(unsigned idx) const {
    if (idx == 0) return GetTrueDest();
    if (idx == 1) return GetFalseDest();
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: EqCondBranchInst::GetSuccessorImpl idx out of range");
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    if (LEPUS_UNLIKELY(idx > 1)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: EqCondBranchInst::SetSuccessorImpl idx out of "
          "range");
    }
    SetOperand(llvh::cast<Value>(b), idx + TrueBlockIdx);
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::EqCondBranchInstKind;
  }
};

class CondBranchInst : public TerminatorInst {
  NON_COPYABLE(CondBranchInst);

 public:
  enum OperandKind : uint8_t { ConditionIdx = 0, TrueBlockIdx, FalseBlockIdx };

  explicit CondBranchInst(Block* parent, OpBuilder* builder, int64_t location,
                          Value* cond, Block* true_block, Block* false_block);

  DEF_DEFAULT_COPY_CONSTRUCTOR(CondBranchInst, TerminatorInst);

  Value* GetCondition() const { return GetOperand(ConditionIdx); }
  Block* GetTrueDest() const {
    return llvh::cast<Block>(GetOperand(TrueBlockIdx));
  }
  Block* GetFalseDest() const {
    return llvh::cast<Block>(GetOperand(FalseBlockIdx));
  }

  void SetTrueDest(Block* bb) {
    SetOperand(llvh::dyn_cast<Value>(bb), TrueBlockIdx);
  }
  void SetFalseDest(Block* bb) {
    SetOperand(llvh::dyn_cast<Value>(bb), FalseBlockIdx);
  }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const { return {}; }

  unsigned GetNumSuccessorsImpl() const { return 2; }
  Block* GetSuccessorImpl(unsigned idx) const {
    if (idx == 0) return GetTrueDest();
    if (idx == 1) return GetFalseDest();
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: CondBranchInst::GetSuccessorImpl idx out of range");
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    if (LEPUS_UNLIKELY(idx > 1)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: CondBranchInst::SetSuccessorImpl idx out of range");
    }
    SetOperand(llvh::cast<Value>(b), idx + TrueBlockIdx);
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::CondBranchInstKind;
  }
};

class SetCatchIdInst : public Instruction {
  NON_COPYABLE(SetCatchIdInst);

 public:
  explicit SetCatchIdInst(Block* parent, OpBuilder* builder, int64_t location)
      : Instruction(ValueKind::SetCatchIdInstKind, parent, builder, location) {
    SetType(TypeOp::CreateString(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetCatchIdInst, Instruction);
  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetCatchIdInstKind;
  }
};

// Bytecode label: TypeLabel_Catch.
// This must be preserved in bytecode layout because the VM finds catch blocks
// by scanning forward for this label when handling TypeLabel_Throw.
class CatchInst : public Instruction {
  NON_COPYABLE(CatchInst);

 public:
  explicit CatchInst(Block* parent, OpBuilder* builder, int64_t location)
      : Instruction(ValueKind::CatchInstKind, parent, builder, location) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(CatchInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Catch label is an observable control-flow boundary.
    return SideEffect{}.SetIsCatch().SetFirstInBlock();
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::CatchInstKind;
  }
};

class LoadConstInst : public SingleOperandInst {
  NON_COPYABLE(LoadConstInst);

 public:
  explicit LoadConstInst(Block* parent, OpBuilder* builder, int64_t location,
                         Literal* input, TypeOp* value_type)
      : SingleOperandInst(ValueKind::LoadConstInstKind, parent, builder,
                          location, input) {
    SetType(value_type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(LoadConstInst, SingleOperandInst);

  Literal* GetConst() const { return llvh::cast<Literal>(GetSingleOperand()); }

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::LoadConstInstKind;
  }
};

// Like LoadConstInst but materializes a fresh heap object when loading an
// aggregate (array/table) constant template from the const pool.
// This must NOT be marked idempotent because the produced reference identity
// is observable.
class LoadConstMaterializeInst : public SingleOperandInst {
  NON_COPYABLE(LoadConstMaterializeInst);

 public:
  explicit LoadConstMaterializeInst(Block* parent, OpBuilder* builder,
                                    int64_t location, Literal* input,
                                    TypeOp* value_type)
      : SingleOperandInst(ValueKind::LoadConstMaterializeInstKind, parent,
                          builder, location, input) {
    SetType(value_type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(LoadConstMaterializeInst, SingleOperandInst);

  Literal* GetConst() const { return llvh::cast<Literal>(GetSingleOperand()); }

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Allocates a fresh aggregate on the heap.
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LoadConstMaterializeInstKind;
  }
};

class GetGlobalInst : public SingleOperandInst {
  NON_COPYABLE(GetGlobalInst);

 public:
  explicit GetGlobalInst(Block* parent, OpBuilder* builder, int64_t location,
                         Literal* input, TypeOp* value_type)
      : SingleOperandInst(ValueKind::GetGlobalInstKind, parent, builder,
                          location, input) {
    SetType(value_type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetGlobalInst, SingleOperandInst);

  Literal* GetGlobalIndex() const {
    return llvh::cast<Literal>(GetSingleOperand());
  }

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetGlobalInstKind;
  }
};

class GetBuiltinInst : public SingleOperandInst {
  NON_COPYABLE(GetBuiltinInst);

 public:
  explicit GetBuiltinInst(Block* parent, OpBuilder* builder, int64_t location,
                          Literal* input, TypeOp* value_type)
      : SingleOperandInst(ValueKind::GetBuiltinInstKind, parent, builder,
                          location, input) {
    SetType(value_type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetBuiltinInst, SingleOperandInst);

  LiteralUint32* GetBuiltinIndex() const {
    return llvh::cast<LiteralUint32>(GetSingleOperand());
  }

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetBuiltinInstKind;
  }
};

class BinaryOperatorInst : public Instruction {
  NON_COPYABLE(BinaryOperatorInst);

 public:
  enum OperandKind : uint8_t { LeftHandSideIdx = 0, RightHandSideIdx };

  Value* GetLeftHandSide() const { return GetOperand(LeftHandSideIdx); }
  Value* GetRightHandSide() const { return GetOperand(RightHandSideIdx); }

  explicit BinaryOperatorInst(Block* parent, OpBuilder* builder,
                              int64_t location, Value* left, Value* right,
                              ValueKind kind, TypeOp* ret_type);

  DEF_DEFAULT_COPY_CONSTRUCTOR(BinaryOperatorInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return GetBinarySideEffect(GetLeftHandSide()->GetType(),
                               GetRightHandSide()->GetType(), GetKind());
  }

  void SelectType(TypeOp* left_ty, TypeOp* right_ty, ValueKind kind);

  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), BinaryOperatorInst);
  }

  /// Calculate the side effect of a binary operator, given inferred types of
  /// its arguments.
  static SideEffect GetBinarySideEffect(TypeOp* left_ty, TypeOp* right_ty,
                                        ValueKind op);
};

class GetTableInst : public Instruction {
  NON_COPYABLE(GetTableInst);

 public:
  enum OperandKind : uint8_t { ObjectIdx = 0, PropIdx };

  explicit GetTableInst(Block* parent, OpBuilder* builder, int64_t location,
                        Value* object, Value* key)
      : Instruction(ValueKind::GetTableInstKind, parent, builder, location) {
    PushOperand(object);
    PushOperand(key);
    if (object->GetType()->IsStringType() && key->GetType()->IsStringType()) {
      SetType(TypeOp::CreateStringProtoAPI(builder));
    } else {
      SetType(TypeOp::CreateAnyType(builder));
    }
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetTableInst, Instruction);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::GetTableInstKind;
  }
  static bool HasOutput() { return true; }
  static bool IsTyped() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap().SetIdempotent();
  }

  Value* GetObject() const { return GetOperand(ObjectIdx); }
  Value* GetProp() const { return GetOperand(PropIdx); }
};

class GetTableConstStringKeyInst : public Instruction {
  NON_COPYABLE(GetTableConstStringKeyInst);

 public:
  enum OperandKind : uint8_t { ObjectIdx = 0, ConstIdx };

  explicit GetTableConstStringKeyInst(Block* parent, OpBuilder* builder,
                                      int64_t location, Value* object,
                                      Value* const_index, TypeOp* ret_type)
      : Instruction(ValueKind::GetTableConstStringKeyInstKind, parent, builder,
                    location) {
    PushOperand(object);
    PushOperand(const_index);
    SetType(ret_type ? ret_type : TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetTableConstStringKeyInst, Instruction);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::GetTableConstStringKeyInstKind;
  }
  static bool HasOutput() { return true; }
  static bool IsTyped() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap().SetIdempotent();
  }

  Value* GetObject() const { return GetOperand(ObjectIdx); }
  Value* GetConstIndex() const { return GetOperand(ConstIdx); }
};

class GetStringLengthInst : public SingleOperandInst {
  NON_COPYABLE(GetStringLengthInst);

 public:
  explicit GetStringLengthInst(Block* parent, OpBuilder* builder,
                               int64_t location, Value* str)
      : SingleOperandInst(ValueKind::GetStringLengthInstKind, parent, builder,
                          location, str) {
    SetType(TypeOp::CreateInt64(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetStringLengthInst, SingleOperandInst);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::GetStringLengthInstKind;
  }
  static bool HasOutput() { return true; }
  static bool IsTyped() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  Value* GetStr() const { return GetSingleOperand(); }
};

class SetTableInst : public Instruction {
  NON_COPYABLE(SetTableInst);

 public:
  enum OperandKind : uint8_t { ObjectIdx = 0, PropIdx, StoreValIdx };

  explicit SetTableInst(Block* parent, OpBuilder* builder, int64_t location,
                        Value* obj, Value* prop, Value* store_val)
      : Instruction(ValueKind::SetTableInstKind, parent, builder, location) {
    PushOperand(obj);
    PushOperand(prop);
    PushOperand(store_val);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetTableInst, Instruction);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::SetTableInstKind;
  }
  static bool HasOutput() { return false; }
  static bool IsTyped() { return true; }
  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetWriteHeap();
  }

  Value* GetObject() const { return GetOperand(ObjectIdx); }
  Value* GetProp() const { return GetOperand(PropIdx); }
  Value* GetStoreVal() const { return GetOperand(StoreValIdx); }
};

class SetTableConstStringKeyInst : public Instruction {
  NON_COPYABLE(SetTableConstStringKeyInst);

 public:
  enum OperandKind : uint8_t { ObjectIdx = 0, ConstIdx, StoreValIdx };

  explicit SetTableConstStringKeyInst(Block* parent, OpBuilder* builder,
                                      int64_t location, Value* obj,
                                      Value* const_index, Value* store_val)
      : Instruction(ValueKind::SetTableConstStringKeyInstKind, parent, builder,
                    location) {
    PushOperand(obj);
    PushOperand(const_index);
    PushOperand(store_val);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetTableConstStringKeyInst, Instruction);

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::SetTableConstStringKeyInstKind;
  }
  static bool HasOutput() { return false; }
  static bool IsTyped() { return true; }
  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetWriteHeap();
  }

  Value* GetObject() const { return GetOperand(ObjectIdx); }
  Value* GetConstIndex() const { return GetOperand(ConstIdx); }
  Value* GetStoreVal() const { return GetOperand(StoreValIdx); }
};

class NewTableInst : public Instruction {
  NON_COPYABLE(NewTableInst);

 public:
  explicit NewTableInst(Block* parent, OpBuilder* builder, int64_t location)
      : Instruction(ValueKind::NewTableInstKind, parent, builder, location) {
    SetType(TypeOp::CreateTable(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(NewTableInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::NewTableInstKind;
  }
};

class NewArrayInst : public Instruction {
  NON_COPYABLE(NewArrayInst);

 public:
  explicit NewArrayInst(Block* parent, OpBuilder* builder, int64_t location,
                        ArgList items)
      : Instruction(ValueKind::NewArrayInstKind, parent, builder, location) {
    llvh::for_each(items, [&](Value* item) { PushOperand(item); });
    SetType(TypeOp::CreateArray(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(NewArrayInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::NewArrayInstKind;
  }

  uint8_t GetArraySize() const { return GetNumOperands(); }
};

class ToStringInst : public SingleOperandInst {
  NON_COPYABLE(ToStringInst);

 public:
  explicit ToStringInst(Block* parent, OpBuilder* builder, int64_t location,
                        Value* src)
      : SingleOperandInst(ValueKind::ToStringInstKind, parent, builder,
                          location, src) {
    SetType(TypeOp::CreateString(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(ToStringInst, SingleOperandInst);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  Value* GetSrc() const { return GetSingleOperand(); }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::ToStringInstKind;
  }
};

class CallInst : public Instruction {
  NON_COPYABLE(CallInst);

 public:
  enum OperandKind : uint8_t { methodIdx = 0, argIdx };
  explicit CallInst(Block* parent, OpBuilder* builder, int64_t location,
                    Value* function, ArgList& args)
      : Instruction(ValueKind::CallInstKind, parent, builder, location) {
    RegisterAttr(SpecificAttr::SA_DeepCloneCall,
                 Attributes::GetBoolAttr(BoolAttrEntry, false));
    RegisterAttr(SpecificAttr::SA_ReadonlyCall,
                 Attributes::GetBoolAttr(BoolAttrEntry, false));
    RegisterAttr(SpecificAttr::SA_BuiltinFuncName,
                 Attributes::GetStringAttr(StringAttrEntry, ""));
    SetType(TypeOp::CreateAnyType(builder));
    PushOperand(function);
    llvh::for_each(args, [&](Value* arg) { PushOperand(arg); });
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(CallInst, Instruction);

  static bool HasOutput() { return true; }

  Value* GetFunction() const { return GetOperand(methodIdx); }
  size_t GetNumArguments() const { return GetNumOperands() - argIdx; }

  Value* GetArgument(unsigned idx) { return GetOperand(argIdx + idx); }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect::CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::CallInstKind;
  }
};

class PhiInst : public Instruction {
  NON_COPYABLE(PhiInst);

 public:
  using ValueListType = llvh::SmallVector<Value*, 8>;
  using BlockListType = llvh::SmallVector<Block*, 8>;

  /// \returns the number of phi incoming values.
  unsigned GetNumEntries() const;

  /// Returns the n'th pair of value-basicblock that represent a case
  /// destination.
  std::pair<Value*, Block*> GetEntry(unsigned i) const;

  /// Update the n'th pair of value-basicblock that represent a case
  /// destination.
  void UpdateEntry(unsigned i, Value* val, Block* bb);

  void ForceUpdateEntry(unsigned i, Value* val, Block* bb);

  /// Add a pair of value-basicblock that represent a case destination.
  void AddEntry(Value* val, Block* bb);

  /// Remove an entry pair (incoming basic block and value) at index \p index.
  void RemoveEntry(unsigned index);

  /// Remove an entry pair (incoming basic block and value) for incoming block
  // \p bb.
  void RemoveEntry(Block* bb);

  void RecalculateResultType();

  explicit PhiInst(Block* parent, OpBuilder* builder, int64_t location,
                   const ValueListType& values, const BlockListType& blocks);

  DEF_DEFAULT_COPY_CONSTRUCTOR(PhiInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetFirstInBlock();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::PhiInstKind;
  }

 private:
  /// Remove an entry without recalculating the result type.
  void RemoveEntryHelper(unsigned index);
};

class FunctionEndInst : public TerminatorInst {
  NON_COPYABLE(FunctionEndInst);

 public:
  explicit FunctionEndInst(Block* parent, OpBuilder* builder, int64_t location)
      : TerminatorInst(ValueKind::FunctionEndInstKind, parent, builder,
                       location) {
    SetType(TypeOp::CreateAnyType(builder));
  }
  DEF_DEFAULT_COPY_CONSTRUCTOR(FunctionEndInst, TerminatorInst);
  static bool HasOutput() { return false; }
  [[nodiscard]] SideEffect GetSideEffectImpl() const { return {}; }
  unsigned GetNumSuccessorsImpl() const { return 0; }
  Block* GetSuccessorImpl(unsigned idx) const {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FunctionEndInst has no successor");
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: FunctionEndInst has no successor");
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::FunctionEndInstKind;
  }
};

class UnaryOperatorInst : public SingleOperandInst {
  NON_COPYABLE(UnaryOperatorInst);

 public:
  explicit UnaryOperatorInst(Block* parent, OpBuilder* builder,
                             int64_t location, Value* value, ValueKind kind);

  DEF_DEFAULT_COPY_CONSTRUCTOR(UnaryOperatorInst, SingleOperandInst);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const { return {}; };

  void SelectType(TypeOp* value, ValueKind kind);

  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), UnaryOperatorInst);
  }
};

class ReturnInst : public TerminatorInst {
  NON_COPYABLE(ReturnInst);

 public:
  enum OperandKind : uint8_t { ReturnValueIdx = 0 };

  explicit ReturnInst(Block* parent, OpBuilder* builder, int64_t location,
                      Value* val)
      : TerminatorInst(ValueKind::ReturnInstKind, parent, builder, location) {
    PushOperand(val);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(ReturnInst, TerminatorInst);

  Value* GetValue() const { return GetOperand(ReturnValueIdx); }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const { return {}; }

  unsigned GetNumSuccessorsImpl() const { return 0; }
  Block* GetSuccessorImpl(unsigned idx) const {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ReturnInst has no successor");
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: ReturnInst has no successor");
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::ReturnInstKind;
  }
};

class PopContextInst : public Instruction {
  NON_COPYABLE(PopContextInst);

 public:
  explicit PopContextInst(Block* parent, OpBuilder* builder, int64_t location)
      : Instruction(ValueKind::PopContextInstKind, parent, builder, location) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(PopContextInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::PopContextInstKind;
  }
};

class PushContextInst : public SingleOperandInst {
  NON_COPYABLE(PushContextInst);

 public:
  explicit PushContextInst(Block* parent, OpBuilder* builder, int64_t location,
                           Value* context)
      : SingleOperandInst(ValueKind::PushContextInstKind, parent, builder,
                          location, context) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(PushContextInst, SingleOperandInst);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::PushContextInstKind;
  }
};

class PushBlockContextInst : public Instruction {
  NON_COPYABLE(PushBlockContextInst);

 public:
  explicit PushBlockContextInst(Block* parent, OpBuilder* builder,
                                int64_t location)
      : Instruction(ValueKind::PushBlockContextInstKind, parent, builder,
                    location) {
    SetType(TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(PushBlockContextInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::PushBlockContextInstKind;
  }
};

class PopBlockContextInst : public Instruction {
  NON_COPYABLE(PopBlockContextInst);

 public:
  explicit PopBlockContextInst(Block* parent, OpBuilder* builder,
                               int64_t location)
      : Instruction(ValueKind::PopBlockContextInstKind, parent, builder,
                    location) {}

  DEF_DEFAULT_COPY_CONSTRUCTOR(PopBlockContextInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::PopBlockContextInstKind;
  }
};

class CreateBlockContextInst : public SingleOperandInst {
  NON_COPYABLE(CreateBlockContextInst);

 public:
  explicit CreateBlockContextInst(Block* parent, OpBuilder* builder,
                                  int64_t location, Value* array_size)
      : SingleOperandInst(ValueKind::CreateBlockContextInstKind, parent,
                          builder, location, array_size) {
    SetType(TypeOp::CreateArray(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(CreateBlockContextInst, SingleOperandInst);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::CreateBlockContextInstKind;
  }

  LiteralUint32* GetContextArraySize() {
    return llvh::cast<LiteralUint32>(GetSingleOperand());
  }
};

class CreateFunctionContextInst : public SingleOperandInst {
  NON_COPYABLE(CreateFunctionContextInst);

 public:
  explicit CreateFunctionContextInst(Block* parent, OpBuilder* builder,
                                     int64_t location, Value* array_size)
      : SingleOperandInst(ValueKind::CreateFunctionContextInstKind, parent,
                          builder, location, array_size) {
    SetType(TypeOp::CreateArray(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(CreateFunctionContextInst, SingleOperandInst);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::CreateFunctionContextInstKind;
  }

  LiteralUint8* GetContextArraySize() {
    return llvh::cast<LiteralUint8>(GetSingleOperand());
  }
};

class ThrowInst : public TerminatorInst {
  NON_COPYABLE(ThrowInst);

 public:
  enum OperandKind : uint8_t { ThrownValueIdx = 0, CatchBlockIdx };

  explicit ThrowInst(Block* parent, OpBuilder* builder, int64_t location,
                     Value* thrown_value, Block* catch_block = nullptr)
      : TerminatorInst(ValueKind::ThrowInstKind, parent, builder, location) {
    PushOperand(thrown_value);
    if (catch_block) {
      PushOperand(llvh::cast<Value>(catch_block));
    }
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(ThrowInst, TerminatorInst);

  static bool HasOutput() { return false; }

  SideEffect GetSideEffectImpl() const { return SideEffect{}.SetThrow(); }

  Value* GetThrownValue() const { return GetOperand(ThrownValueIdx); }

  Block* GetCatchDest() const {
    // Optional successor: the catch label block that will handle this throw.
    if (GetNumOperands() <= CatchBlockIdx) return nullptr;
    return llvh::cast<Block>(GetOperand(CatchBlockIdx));
  }

  void SetCatchDest(Block* bb) {
    if (GetCatchDest() == nullptr) {
      PushOperand(llvh::cast<Value>(bb));
    } else {
      SetOperand(llvh::cast<Value>(bb), CatchBlockIdx);
    }
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::ThrowInstKind;
  }

  unsigned GetNumSuccessorsImpl() const { return GetCatchDest() ? 1 : 0; }
  Block* GetSuccessorImpl(unsigned idx) const {
    if (LEPUS_UNLIKELY(idx != 0)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ThrowInst::GetSuccessorImpl idx out of range");
    }
    auto* dest = GetCatchDest();
    if (LEPUS_UNLIKELY(!dest)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ThrowInst has no catch destination when queried");
    }
    return dest;
  }
  void SetSuccessorImpl(unsigned idx, Block* b) {
    if (LEPUS_UNLIKELY(idx != 0)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: ThrowInst::SetSuccessorImpl idx out of range");
    }
    SetCatchDest(b);
  }
};

class GetContextSlotMovInst : public Instruction {
  NON_COPYABLE(GetContextSlotMovInst);

  enum OperandKind : uint8_t { ContextIdx = 0, IndexIdx };

 public:
  explicit GetContextSlotMovInst(Block* parent, OpBuilder* builder,
                                 int64_t location, Value* context, Value* index)
      : Instruction(ValueKind::GetContextSlotMovInstKind, parent, builder,
                    location) {
    PushOperand(context);
    PushOperand(index);
    SetType(TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetContextSlotMovInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  LiteralUint8* GetIndex() const {
    return llvh::cast<LiteralUint8>(GetOperand(IndexIdx));
  }
  Value* GetContext() const { return GetOperand(ContextIdx); }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetContextSlotMovInstKind;
  }
};

class GetContextSlotInst : public Instruction {
  NON_COPYABLE(GetContextSlotInst);

 public:
  enum OperandKind : uint8_t { DepthIdx = 0, IndexIdx };
  explicit GetContextSlotInst(Block* parent, OpBuilder* builder,
                              int64_t location, Value* depth, Value* index)
      : Instruction(ValueKind::GetContextSlotInstKind, parent, builder,
                    location) {
    PushOperand(depth);
    PushOperand(index);
    SetType(TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetContextSlotInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  LiteralUint8* GetIndex() const {
    return llvh::cast<LiteralUint8>(GetOperand(IndexIdx));
  }
  LiteralUint8* GetDepth() const {
    return llvh::cast<LiteralUint8>(GetOperand(DepthIdx));
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetContextSlotInstKind;
  }
};

class SetContextSlotInst : public Instruction {
  NON_COPYABLE(SetContextSlotInst);

 public:
  enum OperandKind : uint8_t { DepthIdx = 0, IndexIdx, ToStoreIdx };

  explicit SetContextSlotInst(Block* parent, OpBuilder* builder,
                              int64_t location, Value* depth, Value* index,
                              Value* to_store)
      : Instruction(ValueKind::SetContextSlotInstKind, parent, builder,
                    location) {
    PushOperand(depth);
    PushOperand(index);
    PushOperand(to_store);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetContextSlotInst, Instruction);

  Value* GetToStore() const { return GetOperand(ToStoreIdx); }
  LiteralUint8* GetDepth() const {
    auto* v = GetOperand(DepthIdx);
    return llvh::dyn_cast<LiteralUint8>(v);
  }

  LiteralUint8* GetIndex() const {
    auto* v = GetOperand(IndexIdx);
    return llvh::dyn_cast<LiteralUint8>(v);
  }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // should not has throw sideEffect
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetContextSlotInstKind;
  }
};

class SetContextSlotMovInst : public Instruction {
  NON_COPYABLE(SetContextSlotMovInst);

 public:
  enum OperandKind : uint8_t { ContextIdx = 0, IndexIdx, ToStoreIdx };

  explicit SetContextSlotMovInst(Block* parent, OpBuilder* builder,
                                 int64_t location, Value* context, Value* index,
                                 Value* to_store)
      : Instruction(ValueKind::SetContextSlotMovInstKind, parent, builder,
                    location) {
    PushOperand(context);
    PushOperand(index);
    PushOperand(to_store);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetContextSlotMovInst, Instruction);

  Value* GetToStore() const { return GetOperand(ToStoreIdx); }
  Value* GetContext() const { return GetOperand(ContextIdx); }

  LiteralUint8* GetIndex() const {
    auto* v = GetOperand(IndexIdx);
    return llvh::dyn_cast<LiteralUint8>(v);
  }

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // should not has throw sideEffect
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetContextSlotMovInstKind;
  }
};

class CreateClosureInst : public Instruction {
  NON_COPYABLE(CreateClosureInst);

 public:
  explicit CreateClosureInst(Block* parent, OpBuilder* builder,
                             int64_t location, uint32_t index)
      : Instruction(ValueKind::CreateClosureInstKind, parent, builder,
                    location) {
    SetType(TypeOp::CreateClosure(builder));
    SetChildrenIndex(index);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(CreateClosureInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.CreateExecute();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::CreateClosureInstKind;
  }
};

class GetGlobalLynxInst : public Instruction {
  NON_COPYABLE(GetGlobalLynxInst);

 public:
  explicit GetGlobalLynxInst(Block* parent, OpBuilder* builder,
                             int64_t location)
      : Instruction(ValueKind::GetGlobalLynxInstKind, parent, builder,
                    location) {
    SetType(TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetGlobalLynxInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetGlobalLynxInstKind;
  }
};

class LoadNullOrUndefinedInst : public SingleOperandInst {
  NON_COPYABLE(LoadNullOrUndefinedInst);

 public:
  explicit LoadNullOrUndefinedInst(Block* parent, OpBuilder* builder,
                                   int64_t location, LiteralInt8* const_type)
      : SingleOperandInst(ValueKind::LoadNullOrUndefinedInstKind, parent,
                          builder, location, const_type) {
    SetType(TypeOp::CreateNullOrUndefined(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(LoadNullOrUndefinedInst, SingleOperandInst);

  LiteralInt8* GetLoadNilType() const {
    return llvh::cast<LiteralInt8>(GetSingleOperand());
  }

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const { return SideEffect{}; }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::LoadNullOrUndefinedInstKind;
  }
};

class LoadToplevelVarsInst : public Instruction {
  NON_COPYABLE(LoadToplevelVarsInst);

 public:
  explicit LoadToplevelVarsInst(Block* parent, OpBuilder* builder,
                                int64_t location)
      : Instruction(ValueKind::LoadToplevelVarsInstKind, parent, builder,
                    location) {
    SetType(TypeOp::CreateTable(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(LoadToplevelVarsInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::LoadToplevelVarsInstKind;
  }
};

class GetUpvalueInst : public Instruction {
  NON_COPYABLE(GetUpvalueInst);

 public:
  explicit GetUpvalueInst(Block* parent, OpBuilder* builder, int64_t location,
                          FuncOp* func, Literal* index)
      : Instruction(ValueKind::GetUpvalueInstKind, parent, builder, location) {
    SetType(TypeOp::CreateAnyType(builder));
    PushOperand(func);
    PushOperand(index);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetUpvalueInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetReadHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetUpvalueInstKind;
  }

  FuncOp* GetFunc() const { return llvh::cast<FuncOp>(GetOperand(0)); }
  Literal* GetIndex() const { return llvh::cast<Literal>(GetOperand(1)); }
};

class SetUpvalueInst : public Instruction {
  NON_COPYABLE(SetUpvalueInst);

 public:
  explicit SetUpvalueInst(Block* parent, OpBuilder* builder, int64_t location,
                          FuncOp* func, Literal* index, Value* src)
      : Instruction(ValueKind::SetUpvalueInstKind, parent, builder, location) {
    PushOperand(func);
    PushOperand(index);
    PushOperand(src);
    SetType(TypeOp::CreateAnyType(builder));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetUpvalueInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetUpvalueInstKind;
  }

  FuncOp* GetFunc() const { return llvh::cast<FuncOp>(GetOperand(0)); }
  Literal* GetIndex() const { return llvh::cast<Literal>(GetOperand(1)); }
  Value* GetSrc() const { return GetOperand(2); }
};

class MovInst : public SingleOperandInst {
  NON_COPYABLE(MovInst);

 public:
  explicit MovInst(Block* parent, OpBuilder* builder, int64_t location,
                   Value* input)
      : SingleOperandInst(ValueKind::MovInstKind, parent, builder, location,
                          input) {
    SetType(input->GetType());
    RegisterAttr(SpecificAttr::SA_CallFuncMov,
                 Attributes::GetBoolAttr(BoolAttrEntry, false));
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(MovInst, SingleOperandInst);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    return SideEffect{}.SetIdempotent();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::MovInstKind;
  }
};

// GetToplevelClosureVarInst: Read a toplevel closure variable
// This instruction is used when a child function reads a toplevel closure
// variable. It stores the original register number of the closure variable
// (before IR optimization).
class GetToplevelClosureVarInst : public Instruction {
  NON_COPYABLE(GetToplevelClosureVarInst);

 public:
  explicit GetToplevelClosureVarInst(Block* parent, OpBuilder* builder,
                                     int64_t location, Literal* closure_reg,
                                     TypeOp* type)
      : Instruction(ValueKind::GetToplevelClosureVarInstKind, parent, builder,
                    location) {
    PushOperand(closure_reg);
    SetType(type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetToplevelClosureVarInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Reading from toplevel closure variable may observe external modifications
    return SideEffect{}.SetReadHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetToplevelClosureVarInstKind;
  }

  // Get the original register number (before IR optimization)
  Literal* GetClosureReg() const { return llvh::cast<Literal>(GetOperand(0)); }
};

// SetToplevelClosureVarInst: Write to a toplevel closure variable
// This instruction is used when writing to a toplevel closure variable.
// It stores the original register number of the closure variable (before IR
// optimization).
class SetToplevelClosureVarInst : public Instruction {
  NON_COPYABLE(SetToplevelClosureVarInst);

 public:
  explicit SetToplevelClosureVarInst(Block* parent, OpBuilder* builder,
                                     int64_t location, Literal* closure_reg,
                                     Value* src)
      : Instruction(ValueKind::SetToplevelClosureVarInstKind, parent, builder,
                    location) {
    PushOperand(closure_reg);
    PushOperand(src);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetToplevelClosureVarInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Writing to toplevel closure variable affects child functions
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetToplevelClosureVarInstKind;
  }

  // Get the original register number (before IR optimization)
  Literal* GetClosureReg() const { return llvh::cast<Literal>(GetOperand(0)); }
  Value* GetSrc() const { return GetOperand(1); }
};

class SetToplevelVarInst : public Instruction {
  NON_COPYABLE(SetToplevelVarInst);
  enum OperandKind : uint8_t { ToplevelRegIdx = 0, SrcIdx };

 public:
  explicit SetToplevelVarInst(Block* parent, OpBuilder* builder,
                              int64_t location, Literal* toplevel_reg,
                              Value* src)
      : Instruction(ValueKind::SetToplevelVarInstKind, parent, builder,
                    location) {
    PushOperand(toplevel_reg);
    PushOperand(src);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(SetToplevelVarInst, Instruction);

  static bool HasOutput() { return false; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Writing to toplevel closure variable affects child functions
    return SideEffect{}.SetWriteHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::SetToplevelVarInstKind;
  }

  // Get the original register number (before IR optimization)
  Literal* GetToplevelReg() const {
    return llvh::cast<Literal>(GetOperand(ToplevelRegIdx));
  }
  Value* GetSrc() const { return GetOperand(SrcIdx); }
  void SetToplevelReg(Literal* reg);
};

class GetToplevelVarInst : public Instruction {
  NON_COPYABLE(GetToplevelVarInst);

 public:
  explicit GetToplevelVarInst(Block* parent, OpBuilder* builder,
                              int64_t location, Literal* toplevel_reg,
                              TypeOp* type)
      : Instruction(ValueKind::GetToplevelVarInstKind, parent, builder,
                    location) {
    PushOperand(toplevel_reg);
    SetType(type);
  }

  DEF_DEFAULT_COPY_CONSTRUCTOR(GetToplevelVarInst, Instruction);

  static bool HasOutput() { return true; }

  [[nodiscard]] SideEffect GetSideEffectImpl() const {
    // Reading from toplevel variable may observe external modifications
    return SideEffect{}.SetReadHeap();
  }

  static bool classof(const Value* v) {
    ValueKind kind = v->GetKind();
    return kind == ValueKind::GetToplevelVarInstKind;
  }

  // Get the original register number (before IR optimization)
  Literal* GetToplevelReg() const { return llvh::cast<Literal>(GetOperand(0)); }

  void SetToplevelReg(Literal* reg);
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_DIALECTS_MIR_MIR_INSTRS_H_
