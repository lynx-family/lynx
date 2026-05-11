// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_FUNC_OP_H_
#define CORE_RUNTIME_LEPUS_IR_FUNC_OP_H_

#include <ostream>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/runtime/lepus/function.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/region_op.h"

namespace lynx {
namespace lepus {

namespace ir {
class FuncOp;
class TypeOp;
class Parameter;

class FuncOp : public llvh::ilist_node<FuncOp>, public Operation {
  friend class Value;
  NON_COPYABLE(FuncOp);

 public:
  using iterator = Region::iterator;
  using const_iterator = Region::const_iterator;
  using ParamListType = llvh::SmallVector<Parameter*, 4>;

  explicit FuncOp(Block* parent, OpBuilder* builder, int64_t location,
                  std::string& name);

  ~FuncOp();
  TypeOp* GetRetTy() const { return ret_ty_; }
  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::FuncOpKind;
  }

  iterator begin() { return GetSingleRegion()->begin(); }
  iterator end() { return GetSingleRegion()->end(); }
  const_iterator begin() const { return GetSingleRegion()->begin(); }
  const_iterator end() const { return GetSingleRegion()->end(); }
  Block& Front() { return GetSingleRegion()->Front(); }
  size_t size() const { return GetSingleRegion()->GetBlockSize(); }
  size_t GetBlockSize() const { return std::distance(begin(), end()); }
  Parameter* CreateParam(int32_t param_index);
  const ParamListType& GetParams() const { return params_; }
  uint32_t GetParamSize() const { return params_.size(); }
  void AddParam(Parameter* p);

  Operation* GetParentOp() const;
  uint64_t GetRegionUUID() { return region_uuid_++; }
  std::string GetName() const { return name_; }
  void Init(fml::RefPtr<Function>& function);
  std::string GetNameForDump() const;
  fml::RefPtr<Function>& GetLepusFunction() { return lepus_function_; }
  void Dump(StageMode mode, std::ostream& os = std::cout) const;

  // for toplevel closure var, record old register and corresponding value
  void RecordClosureVarRegAndValue(uint32_t closure_reg, Value* value);
  Value* GetClosureVarGivenReg(uint32_t closure_reg);

  // for toplevel closure var: record upvalue index -> toplevel shared slot.
  // Usually this is the physical register index after root/toplevel RA.
  // When root-function deopt is enabled, this instead stores the root-function
  // deopt slot so nested optimized functions can still interoperate with the
  // deoptimized root frame.
  void RecordUpvalueIndex2ToplevelReg(long index, long reg_index);
  long GetClosureVarToplevelReg(long index);

  void SetTopLevelFunction() { is_toplevel_function_ = true; }
  bool IsToplevelFunc() const { return is_toplevel_function_; }
  void UpdateClosureVar(Value* origin, Value* update);
  llvh::SmallDenseMap<uint32_t, Value*, 8>& GetClosureVarReg2ValueMap() {
    return closure_reg_to_value_;
  }
  void InsertToplevelClosureVarReg(unsigned reg) {
    toplevel_closure_var_regs_.insert(reg);
  }
  llvh::SmallDenseSet<unsigned, 8>& GetToplevelClosureVarRegs() {
    return toplevel_closure_var_regs_;
  }
  // DeepClone's body can usually skip instruction selection because direct
  // builtin lowering handles its common call sites. Some transformed call
  // patterns still require emitting the function body explicitly.
  void MarkDeepCloneLoweringRequired() {
    can_skip_deep_clone_lowering_ = false;
  }
  bool CanSkipDeepCloneLowering() const {
    return can_skip_deep_clone_lowering_;
  }

 private:
  bool is_toplevel_function_ = false;
  std::string name_ = "";
  uint64_t region_uuid_ = 0;
  ParamListType params_{};
  fml::RefPtr<Function> lepus_function_ = nullptr;
  TypeOp* ret_ty_ = nullptr;
  // key: register id of global closure var before opt
  // value: the value of global closure var
  llvh::SmallDenseMap<uint32_t, Value*, 8> closure_reg_to_value_{};
  llvh::SmallDenseSet<unsigned, 8> toplevel_closure_var_regs_{};

  // key: upvalue index in this function
  // value: physical register index in the toplevel function
  std::unordered_map<long, long> upvalue_index_to_toplevel_reg_;
  bool can_skip_deep_clone_lowering_ = true;
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_FUNC_OP_H_
