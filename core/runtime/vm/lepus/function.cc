// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/function.h"

#include "base/include/value/base_string.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {
std::size_t Function::AddConstNumber(double number) {
  Value v;
  if (lynx::base::StringConvertHelper::IsInt64Double(number)) {
    v.SetNumber(static_cast<int64_t>(number));
  } else {
    v.SetNumber(number);
  }
  return AddConstValue(v);
}

std::size_t Function::AddConstString(const base::String& string) {
  return AddConstValue(Value(string));
}

std::size_t Function::AddConstRegExp(fml::RefPtr<RegExp> regexp) {
  return AddConstValue(Value(std::move(regexp)));
}

std::size_t Function::AddConstBoolean(bool boolean) {
  return AddConstValue(Value(boolean));
}

std::size_t Function::AddConstValue(const Value& v) {
  for (size_t i = 0; i < const_values_.size(); i++) {
    if (const_values_[i] == v) {
      return i;
    }
  }
  const_values_.push_back(v);
  return const_values_.size() - 1;
}

void Function::DecodeLineCol(uint64_t line_col, int32_t& line, int32_t& col) {
  // line_col: bits[Function::kLineBitsShift-0]: col number
  // bits[63-Function::kLineBitsShift]: line number
  // line number and col number start from 1
  line = ((line_col) >> Function::kLineBitsShift) & 0xffffffff;
  col =
      (line_col) & ((static_cast<uint64_t>(1) << Function::kLineBitsShift) - 1);

  if (line == 0 && (col > (1 << kLineBitsShiftBefore))) {
    // fix line and column break change:
    line = (col >> kLineBitsShiftBefore) & 0xffff;
    col = col & 0xffff;
  }
}

// bits[31 - 28]: type
//    0: variabels
//    1: Closure
//    2: Closure Outside
// For Variables:
//    bits[27-0]: reg_index

// For Closure:
//    bits[27-12]: array_index
//    bits[11-0]: offset

// For Closure Outside:
//    bits[27-12]: array_index
//    bits[11-0]: current_context
uint32_t Function::EncodeVariableInfo(int32_t type, int32_t reg_index,
                                      int32_t array_index, int32_t offset) {
  if (type == 0) {
    return reg_index;
  } else if (type == 1) {
    return (1 << kTypeBitsShift) | (array_index << kArrayIndexShift) | offset;
  } else if (type == 2) {
    // type = 2
    return (2 << kTypeBitsShift) | (array_index << kArrayIndexShift) | offset;
  } else {
    NOTREACHED();
  }
  return 0;
}

void Function::DecodeVariableInfo(uint32_t val, int32_t& type,
                                  int32_t& reg_index, int32_t& array_index,
                                  int32_t& offset) {
  type = (val >> kTypeBitsShift) & kTypeMask;
  if (type == 0) {
    reg_index = val;
  } else if (type == 1 || type == 2) {
    array_index = (val & kArrayIndexMask) >> kArrayIndexShift;
    offset = val & kOffsetMask;
  }
  return;
}

std::string Function::GetFunctionName() {
  if (function_name_ != "") return function_name_;
  if (const_values_.empty()) return "";
  const auto& last = const_values_.back();
  if (last.IsTable()) {
    const auto& name =
        last.Table()->GetValue(BASE_STATIC_STRING(Function::kFuncName));
    if (name.IsString()) {
      function_name_ = name.StdString();
      return function_name_;
    }
  }
  return function_name_;
}

Value Function::GetLineInfo() {
  fml::RefPtr<CArray> info = CArray::Create();
  size_t len = debug_line_col_.size();
  for (size_t i = 0; i < len; i++) {
    info->emplace_back(debug_line_col_[i]);
  }
  return Value(std::move(info));
}

// get function from const_values_
int64_t Function::GetFunctionId() {
  if (function_id_ != 0) {
    return function_id_;
  }
  if (const_values_.size() > 0) {
    const auto& last = const_values_.back();
    if (last.IsTable()) {
      const auto& function_id =
          last.Table()->GetValue(BASE_STATIC_STRING(Function::kFuncId));
      if (function_id.IsInt64()) {
        function_id_ = function_id.Int64();
      }
      return function_id_;
    }
  }
  return function_id_;
}

// given pc index and line col info, set to debug_line_col_
void Function::SetLineInfo(int32_t index, int64_t line_col) {
  if (debug_line_col_.size() <= static_cast<size_t>(index)) {
    debug_line_col_.resize(index + 1);
  }
  debug_line_col_[index] = line_col;
}

void Function::PushDebugInfoToConstValues(const Value& value) {
  if (value.IsTable()) {
    if (const_values_.size() > 0) {
      const auto& last = const_values_.back();
      if (last.IsTable()) {
        // there is already a debuginfo table, just add property
        auto dict = value.Table();
        for (auto& iter : *dict) {
          last.Table()->SetValue(iter.first, iter.second);
        }
      } else {
        const_values_.push_back(value);
      }
    } else {
      const_values_.push_back(value);
    }
  }
}

void Function::GetLineCol(int32_t index, int32_t& line, int32_t& col) {
  Value debug_info;
  if (const_values_.size() > 0) {
    const auto& last = const_values_.back();
    if (index >= 0 && last.IsTable()) {
      debug_info =
          last.Table()->GetValue(BASE_STATIC_STRING(Function::kLineColInfo));
    }
  }
  if (!debug_info.IsArray()) {
    debug_info = GetLineInfo();
  }
  if (debug_info.IsArray() &&
      debug_info.Array()->size() > static_cast<size_t>(index)) {
    const auto& number = debug_info.Array()->get(index);
    int64_t line_col = 0;
    if (number.IsInt64()) {
      line_col = static_cast<int64_t>(number.Int64());
    } else if (number.IsNumber()) {
      line_col = static_cast<int64_t>(number.Number());
    } else {
      NOTREACHED();
    }

    return Function::DecodeLineCol(line_col, line, col);
  }
  line = -1;
  col = -1;
}

Value& Function::GetScope() {
  if (scopes_.IsNil()) {
    if (!const_values_.empty()) {
      const auto& last = const_values_.back();
      if (last.IsTable()) {
        scopes_ =
            last.Table()->GetValue(BASE_STATIC_STRING(Function::kScopesName));
      }
    }
  }
  return scopes_;
}

void Function::PushBSStack(uint64_t id) { block_scope_stack_.push(id); }

void Function::PopBSStack() { block_scope_stack_.pop(); }

void Function::PushLoopBlockStack(uint64_t id) { loop_block_stack_.push(id); }

void Function::PopLoopBlockStack() { loop_block_stack_.pop(); }

uint64_t Function::GetLoopBlockStack() { return loop_block_stack_.top(); }

#ifdef LEPUS_TEST
static void DumpEmptySpaces(int32_t intend) {
  for (size_t i = 0; i < intend; i++) {
    std::cout << " ";
  }
}

static void DumpBlockScope(const Value& scopes_, int32_t intend) {
  if (!scopes_.IsTable()) return;
  int32_t line;
  int32_t col;
  Value start = scopes_.GetProperty(Function::kStartLine);
  Value end = scopes_.GetProperty(Function::kEndLine);

  Function::DecodeLineCol(start.Number(), line, col);
  DumpEmptySpaces(intend);
  std::cout << "ScopeLine: (" << line << ":" << col << ") => ";
  Function::DecodeLineCol(end.Number(), line, col);
  std::cout << "(" << line << ":" << col << ")" << std::endl;

  for (const auto& it : *scopes_.Table()) {
    int32_t type = -1;
    int32_t reg_index = -1;
    int32_t array_index = -1;
    int32_t offset = -1;
    if (!it.second.IsUInt32()) continue;
    Function::DecodeVariableInfo(it.second.UInt32(), type, reg_index,
                                 array_index, offset);
    if (type == 0) {
      DumpEmptySpaces(intend);
      std::cout << it.first.c_str() << "  : " << reg_index << " : NORMAL"
                << std::endl;
    } else if (type == 1) {
      DumpEmptySpaces(intend);
      std::cout << it.first.c_str() << " :array_index(" << array_index
                << ") :offset(" << offset << ")"
                << " :Closure" << std::endl;
    } else if (type == 2) {
      DumpEmptySpaces(intend);
      std::cout << it.first.c_str() << " :array_index(" << array_index
                << ") :current_context(" << offset << ")"
                << " :Closure_Outside" << std::endl;
    } else {
      std::cout << "wrong decode type, please check";
    }
  }
  std::cout << std::endl;

  Value childs = scopes_.GetProperty(Function::kChilds);
  if (!childs.Array()) return;

  size_t size = childs.Array()->size();
  for (size_t i = 0; i < size; i++) {
    DumpBlockScope(childs.Array()->get(i), intend + 1);
  }
}
void Function::DumpScope() {
  std::cout << "----ScopeInfo:-----" << std::endl;
  DumpBlockScope(scopes_, 0);
}
#endif

int32_t Function::GetParamsSize() {
  if (params_size_ != -1) {
    return params_size_;
  }
  int32_t params_size = -1;
  if (!const_values_.empty()) {
    const auto& last = const_values_.back();
    if (last.IsTable()) {
      const auto& name =
          last.Table()->GetValue(BASE_STATIC_STRING(Function::kParamsSize));
      if (name.IsNumber()) {
        params_size = name.Number();
      }
    }
  }
  params_size_ = params_size;
  return params_size_;
}

}  // namespace lepus
}  // namespace lynx
