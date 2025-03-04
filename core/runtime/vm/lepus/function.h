// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_FUNCTION_H_
#define CORE_RUNTIME_VM_LEPUS_FUNCTION_H_

#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/op_code.h"
#include "core/runtime/vm/lepus/regexp.h"
#include "core/runtime/vm/lepus/switch.h"
#include "core/runtime/vm/lepus/upvalue.h"

namespace lynx {
namespace lepus {
// hash function for unordered map which has std::pair as key
// from boost (functional/hash): see
// http://www.boost.org/doc/libs/1_35_0/doc/html/hash/combine.html template
template <typename T>
inline void hash_combine(std::size_t& seed, const T& val) {
  seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
// auxiliary generic functions to create a hash value using a seed
template <typename T>
inline void hash_val(std::size_t& seed, const T& val) {
  hash_combine(seed, val);
}
template <typename T, typename... Types>
inline void hash_val(std::size_t& seed, const T& val, const Types&... args) {
  hash_combine(seed, val);
  hash_val(seed, args...);
}

template <typename... Types>
inline std::size_t hash_val(const Types&... args) {
  std::size_t seed = 0;
  hash_val(seed, args...);
  return seed;
}

struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const {
    return hash_val(p.first, p.second);
  }
};

class Function : public fml::RefCountedThreadSafeStorage {
 public:
  constexpr static const char kFuncName[] = "__func_name__";
  constexpr static const char kLineColInfo[] = "__line_col_info__";
  constexpr static const char kParamsSize[] = "__params_sizs__";
  constexpr static const char* kFuncSource = "__function_source__";
  constexpr static const char kFuncId[] = "$$__function_id__$$";
  constexpr static const char kScopesName[] = "$$$scopes$$$";
  constexpr static const char* kLepusPrev = "$$lepus$$prev$$";
  constexpr static const char* kStartLine = "$$startline$$";
  constexpr static const char* kEndLine = "$$endline$$";
  constexpr static const char* kChilds = "$$childs$$";

  constexpr static const int32_t kLineBitsShiftBefore = 16;
  constexpr static const int32_t kLineBitsShift = 30;
  constexpr static const int32_t kTypeBitsShift = 28;
  constexpr static const int32_t kArrayIndexShift = 12;

  constexpr static const int32_t kTypeMask = 0xf;
  constexpr static const int32_t kArrayIndexMask = 0x0ffff000;
  constexpr static const int32_t kOffsetMask = 0xfff;
  static fml::RefPtr<Function> Create() {
    return fml::AdoptRef<Function>(new Function());
  }
  ~Function() override = default;

  void SetParamsSize(int32_t params_size) { params_size_ = params_size; }

  int32_t GetParamsSize();

  std::size_t OpCodeSize() { return op_codes_.size(); }

  const Instruction* GetOpCodes() const {
    return op_codes_.empty() ? nullptr : &op_codes_[0];
  }

  std::size_t AddInstruction(Instruction i) {
    op_codes_.push_back(i);
    debug_line_col_.push_back(current_line_col_);
    return op_codes_.size() - 1;
  }

  void ReleaseSelf() const override { delete this; }

  Instruction* GetInstruction(std::size_t index) { return &op_codes_[index]; }

  std::size_t AddConstNumber(double number);

  std::size_t AddConstString(const base::String& string);

  std::size_t AddConstRegExp(fml::RefPtr<RegExp> regexp);

  std::size_t AddConstBoolean(bool boolean);

  std::size_t AddConstValue(const Value& v);

  std::size_t AddChildFunction(fml::RefPtr<Function> function) {
    child_functions_.push_back(std::move(function));
    return child_functions_.size() - 1;
  }

  void SetFunctionId(int64_t function_id) { function_id_ = function_id; }

  BASE_EXPORT_FOR_DEVTOOL int64_t GetFunctionId();

  fml::RefPtr<Function> GetChildFunction(long index) {
    return child_functions_[index];
  }

  const auto& GetChildFunction() { return child_functions_; }

  inline Value* GetConstValue(std::size_t index) {
    return index < const_values_.size() ? &const_values_[index] : nullptr;
  }

  BASE_EXPORT_FOR_DEVTOOL const auto& GetConstValue() { return const_values_; }

  long SearchUpvalue(const base::String& name) {
    for (long i = 0; static_cast<size_t>(i) < upvalues_.size(); ++i) {
      if (upvalues_[i].name_ == name) {
        return i;
      }
    }
    return -1;
  }

  long AddUpvalue(base::String name, long register_index, bool in_parent_vars) {
    upvalues_.emplace_back(std::move(name), register_index, in_parent_vars);
    return upvalues_.size() - 1;
  }

  UpvalueInfo* GetUpvalue(int index) { return &upvalues_[index]; }

  SwitchInfo* GetSwitch(long index) { return switches_[index].get(); }

  std::size_t UpvaluesSize() { return upvalues_.size(); }

  std::size_t AddSwitch(long key_type, long min, long max, long default_offset,
                        SwitchType type,
                        std::vector<std::pair<long, long>> switch_table) {
    auto* info =
        new SwitchInfo(key_type, min, max, default_offset, type, switch_table);
    switches_.push_back(std::unique_ptr<SwitchInfo>(info));
    return switches_.size() - 1;
  }

  void set_index(size_t index) { index_ = index; }

  std::size_t index() { return index_; }

  void SetFunctionName(const std::string& function_name) {
    function_name_ = function_name;
  }

  BASE_EXPORT_FOR_DEVTOOL std::string GetFunctionName();

  void SetUpvalueArray(const std::unordered_map<
                       std::pair<lynx::base::String, uint64_t>, long, pair_hash>
                           upvalue_array) {
    upvalue_array_.clear();
    upvalue_array_ = upvalue_array;
  }

  const std::unordered_map<std::pair<lynx::base::String, uint64_t>, long,
                           pair_hash>&
  GetUpvalueArray() {
    return upvalue_array_;
  }

  void SetCurrentLineCol(int64_t num) { current_line_col_ = num; }

  int64_t CurrentLineCol() { return current_line_col_; }

  BASE_EXPORT_FOR_DEVTOOL Value GetLineInfo();

  BASE_EXPORT_FOR_DEVTOOL void SetLineInfo(int32_t index, int64_t line_col);

  BASE_EXPORT_FOR_DEVTOOL void PushDebugInfoToConstValues(const Value& value);

  BASE_EXPORT_FOR_DEVTOOL void GetLineCol(int index, int& line, int& col);

  void SetSource(const std::string& source) { source_ = source; }
  const std::string GetSource() { return source_; }
  BASE_EXPORT_FOR_DEVTOOL Value& GetScope();
  BASE_EXPORT_FOR_DEVTOOL static void DecodeLineCol(uint64_t line_col,
                                                    int32_t& line,
                                                    int32_t& col);

  static uint32_t EncodeVariableInfo(int32_t type, int32_t reg_index,
                                     int32_t array_index, int32_t offset);

  BASE_EXPORT_FOR_DEVTOOL static void DecodeVariableInfo(uint32_t val,
                                                         int32_t& type,
                                                         int32_t& reg_index,
                                                         int32_t& array_index,
                                                         int32_t& offset);

  void SetScope(Value& scopes) { scopes_ = scopes; }

  void DumpScope();

  const auto& block_scope_stack() { return block_scope_stack_; }

  void PushBSStack(uint64_t id);

  void PopBSStack();

  const auto& loop_block_stack() { return loop_block_stack_; }

  void PushLoopBlockStack(uint64_t id);

  void PopLoopBlockStack();

  uint64_t GetLoopBlockStack();

 protected:
  Function() = default;

 private:
  std::vector<Instruction> op_codes_;

  base::InlineVector<Value, 8> const_values_;

  base::InlineVector<UpvalueInfo, 4> upvalues_;

  base::InlineVector<std::unique_ptr<SwitchInfo>, 4> switches_;

  base::InlineVector<fml::RefPtr<Function>, 4> child_functions_;

  int32_t params_size_ = -1;

  // we use function-id and pc-index to generate sourceMap, then sourceMap treat
  // function-id as line number
  // and treat pc-index as column number, but sourceMap assume tha line number
  // is start from 1, so the function_id will start from 1, other than 0
  int64_t function_id_ = 0;
  std::string function_name_;
  std::size_t index_ = 0;
  int64_t current_line_col_ = -1;

  // blew is for debug
  std::vector<int64_t> debug_line_col_;
  // root function will hold compile source
  std::string source_;
  Value scopes_;

  friend class ContextBinaryWriter;
  friend class BaseBinaryReader;

  std::unordered_map<std::pair<lynx::base::String, uint64_t>, long, pair_hash>
      upvalue_array_;

  // These two stacks are used by code generator only.
  base::Stack<uint64_t> block_scope_stack_;
  base::Stack<uint64_t> loop_block_stack_;
};

class Closure : public fml::RefCountedThreadSafeStorage {
 public:
  static fml::RefPtr<Closure> Create(fml::RefPtr<Function> function) {
    return fml::AdoptRef<Closure>(new Closure(std::move(function)));
  }
  void set_function(fml::RefPtr<Function> function) {
    function_ = std::move(function);
  }
  ~Closure() override = default;

  fml::RefPtr<Function> function() { return function_; }

  void AddUpvalue(Value* value) { upvalues_.push_back(value); }
  Value* GetUpvalue(long index) { return upvalues_[index]; }
  const auto& GetUpvalues() { return upvalues_; }
  void ClearUpvalues() { upvalues_.clear(); }
  void ReleaseSelf() const override { delete this; }

  void SetContext(Value v) { context_ = std::move(v); }
  Value GetContext() { return context_; }

 protected:
  Closure() = default;
  Closure(fml::RefPtr<Function> function) : function_(std::move(function)) {}

 private:
  friend class ContextBinaryWriter;
  friend class BaseBinaryReader;
  friend class VMContext;

  base::InlineVector<Value*, 4> upvalues_;
  Value context_;
  fml::RefPtr<Function> function_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_FUNCTION_H_
