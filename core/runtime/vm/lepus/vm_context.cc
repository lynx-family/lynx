// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// #include "core/runtime/vm/lepus/vm_context.h"

#include "core/runtime/vm/lepus/vm_context.h"

#include <math.h>

#include <chrono>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/string/string_number_convert.h"
#include "base/include/value/base_string.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/path_parser.h"
// #include "ast_dump.h"
#include "base/include/string/string_utils.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/output_stream.h"
#include "core/runtime/vm/lepus/table.h"

#ifdef LEPUS_TEST
#include "core/runtime/vm/lepus/bytecode_print.h"
#endif

namespace lynx {
namespace lepus {

#define GET_CONST_VALUE(i) (function->GetConstValue(Instruction::GetParamBx(i)))
#define GET_Global_VALUE(i) (global()->Get(Instruction::GetParamBx(i)))
#define GET_Builtin_VALUE(i) (builtin()->Get(Instruction::GetParamBx(i)))
#define GET_REGISTER_A(i) (regs + Instruction::GetParamA(i))
#define GET_REGISTER_B(i) (regs + Instruction::GetParamB(i))
#define GET_REGISTER_C(i) (regs + Instruction::GetParamC(i))

#define GET_REGISTER_A_FROM_CTX(ctx) (ctx.regs + Instruction::GetParamA(ctx.i))
#define GET_REGISTER_B_FROM_CTX(ctx) (ctx.regs + Instruction::GetParamB(ctx.i))
#define GET_REGISTER_C_FROM_CTX(ctx) (ctx.regs + Instruction::GetParamC(ctx.i))

#define GET_UPVALUE_B(i) (closure->GetUpvalue(Instruction::GetParamB(i)))
#define GET_REGISTER_ABC(i) \
  a = GET_REGISTER_A(i);    \
  b = GET_REGISTER_B(i);    \
  c = GET_REGISTER_C(i);

#define GET_REGISTER_ABC_FROM_CTX(ctx) \
  a = GET_REGISTER_A_FROM_CTX(ctx);    \
  b = GET_REGISTER_B_FROM_CTX(ctx);    \
  c = GET_REGISTER_C_FROM_CTX(ctx);

#define DECL_ABC_FROM_CTX(ctx) \
  auto& a = ctx.a;             \
  auto& b = ctx.b;             \
  auto& c = ctx.c;

VMContext::~VMContext() { DestroyInspector(); }

void VMContext::Initialize() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "VMContext::Initialize");
  RegisterBuiltin(this);
  RegisterLepusVerion();
}

bool VMContext::Execute(Value* ret_val) {
  if (root_function_.get() == nullptr) {
    LOGE(
        "lepus-Execute: root_function_ is nullptr, template.lepus may be "
        "damaged!!");
    return false;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "Lepus.Execute");
  EnsureLynx();

  Value* top = heap().top_++;
  top->SetClosure(Closure::Create(fml::RefPtr<Function>(root_function_.get())));

  Value ret;
  if (current_frame_) {
    // not top frame
    CallFunction(heap().top_ - 1, 0, &ret);
  } else {
    // create top frame
    Frame top_frame;
    top_frame.register_ = heap_.base() + top_level_variables_.size() + 1;
    top_frame.prev_frame_ = &top_frame;
    current_frame_ = &top_frame;
    CallFunction(heap().top_ - 1, 0, &ret);
    current_frame_ = nullptr;
  }
  executed_ = true;
  if (ret_val) {
    *ret_val = ret;
  }
  return true;
}

Value* VMContext::CallPrologue(const base::String& name) {
  auto reg_info = top_level_variables_.find(name);
  if (reg_info == top_level_variables_.end()) {
    LOGE("lepus-call: not find " << name.str());
    return nullptr;
  }
  long reg = reg_info->second;
  Value* function = heap_.top_;
  *(heap_.top_++) = *(heap_.base() + reg + 1);
  return function;
}

Value VMContext::CallEpilogue(Value* function, size_t arg_count) {
  Value ret;
  if (current_frame_) {
    // not top frame
    CallFunction(function, arg_count, &ret);
  } else {
    // create top frame
    Frame top_frame;
    top_frame.register_ = heap_.base() + top_level_variables_.size() + 1;
    top_frame.prev_frame_ = &top_frame;
    current_frame_ = &top_frame;
    CallFunction(function, arg_count, &ret);
    current_frame_ = nullptr;
  }
  return ret;
}

Value VMContext::CallArgs(const base::String& name, const Value* args[],
                          size_t args_count,
                          [[maybe_unused]] bool pause_suppression_mode) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "VMContext::Call",
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("name", name.str());
              });
  if (auto function = CallPrologue(name); function != nullptr) {
    for (size_t i = 0; i < args_count; ++i) {
      *(heap_.top_++) = *args[i];
    }
    return CallEpilogue(function, args_count);
  }
  return Value();
}

Value VMContext::CallClosureArgs(const Value& closure, const Value* args[],
                                 size_t args_count) {
  Value ret;
  Value* function = heap_.top_;
  *(heap_.top_++) = closure;
  for (size_t i = 0; i < args_count; ++i) {
    *(heap_.top_++) = *args[i];
  }
  return CallEpilogue(function, args_count);
}

Value VMContext::PrepareClosureContext(const fml::RefPtr<lepus::Closure>& clo) {
  auto result = closure_context_;
  if (clo.get()) {
    closure_context_ = clo->GetContext();
  }
  return result;
}

#ifdef LEPUS_TEST
void VMContext::Dump() {
  Dumper dumper(root_function_.Get());
  dumper.Dump();
}
#endif

// check target's first level variable.
// 1. if update key is not path, simply add new k-v pair for the first level
// 2. if update key is value path, clone the first level k-v pair and update
//     the exact value.
bool VMContext::UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                             const Value& value) {
  if (path.empty()) {
    return false;
  }
  auto front_value_iter = path.begin();
  const auto& front_value = *front_value_iter;
  auto reg_info = top_level_variables_.find(front_value);

  long reg = 0;
  if (reg_info == top_level_variables_.end()) {
    if (enable_top_var_strict_mode_) {
#ifdef LEPUS_LOG
      LOGE("lepus-updateTopLevelVariable: not find variables " << name);
#endif
      return false;
    } else {
      reg = top_level_variables_.size();
      top_level_variables_.insert(std::make_pair(front_value, reg));
    }
  } else {
    reg = reg_info->second;
  }
  path.erase(front_value_iter);
  Value* ptr = heap_.base() + reg + 1;
  if (!path.empty() && ((ptr->IsTable() && ptr->Table()->IsConst()) ||
                        (ptr->IsArray() && ptr->Array()->IsConst()))) {
    *(heap_.base() + reg + 1) = Value::Clone(*ptr);
  }
  lepus::Value::UpdateValueByPath(*ptr, value, path);
  return true;
}

bool VMContext::CheckTableShadowUpdatedWithTopLevelVariable(
    const lepus::Value& update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              "VMContext::CheckTableShadowUpdatedWithTopLevelVariable");
  bool enable_deep_check = false;
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
  if (lynx::tasm::LynxEnv::GetInstance().IsTableDeepCheckEnabled()) {
    enable_deep_check = true;
  }
#endif
  auto update_type = update.Type();
  if (update_type != ValueType::Value_Table) {
    return true;
  }
  // page new data from setData
  auto update_table_value = update.Table();
  // shadow compare new_data_table && top level
  // if any top level data are different, need update;
  for (auto& update_data_iterator : *update_table_value) {
    const auto& key = update_data_iterator.first.str();
    auto result = ParseValuePath(key);
    if (result.empty()) {
      return true;
    }
    auto front_value_iter = result.begin();
    auto reg_info = top_level_variables_.find(std::move(*front_value_iter));
    long reg = 0;
    if (reg_info == top_level_variables_.end()) {
      // target did not have this new key
      return true;
    } else {
      reg = reg_info->second;
    }
    result.erase(front_value_iter);
    Value* ptr = heap_.base() + reg + 1;

    for (auto it = result.begin(); it != result.end(); ++it) {
      if (ptr->IsTable()) {
        base::String key(*it);
        if (!ptr->Table()->Contains(key)) {
          // target table did not have this new key
          return true;
        }
        ptr = &(const_cast<Value&>(ptr->Table()->GetValue(key)));
      } else if (ptr->IsArray()) {
        int index;
        if (lynx::base::StringToInt(*it, &index, 10)) {
          if (static_cast<size_t>(index) >= ptr->Array()->size()) {
            // the array's size is smaller.
            return true;
          }
          ptr = &(const_cast<Value&>(ptr->Array()->get(index)));
        }
      }
    }

    lepus::Value update_item_value = update_data_iterator.second;
    if (!enable_deep_check &&
        tasm::CheckTableValueNotEqual(*ptr, update_item_value)) {
      return true;
    }
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
    if (enable_deep_check &&
        tasm::CheckTableDeepUpdated(*ptr, update_item_value, false)) {
      return true;
    }
#endif
  }
  return false;
}

void VMContext::ResetTopLevelVariable() {
  // `__globalProps` & `SystemInfo` are builtin variable, should not be cleared.
  // Reset should not clear callable value.
  for (const auto& iter : top_level_variables_) {
    if (base::BeginsWith(iter.first.str(), "$") ||
        iter.first.str() == "__globalProps" ||
        iter.first.str() == "SystemInfo") {
      continue;
    }

    long reg = iter.second;
    auto* value = heap_.base() + reg + 1;
    if (!value->IsCallable()) {
      value->SetNil();
    }
  }
}

void VMContext::ResetTopLevelVariableByVal(const Value& val) {
  if (val.IsTable()) {
    for (const auto& pair : *val.Table()) {
      // `__globalProps` & `SystemInfo` are builtin variable, should not be
      // cleared.
      if (pair.first.str() == "__globalProps" ||
          pair.first.str() == "SystemInfo") {
        continue;
      }
      auto reg_info = top_level_variables_.find(pair.first);
      if (reg_info == top_level_variables_.end()) {
        return;
      }
      long reg = reg_info->second;
      (*(heap_.base() + reg + 1)).SetNil();
    }
  }
}

std::unique_ptr<lepus::Value> VMContext::GetTopLevelVariable(
    bool ignore_callable) {
  auto dictionary = lepus::Dictionary::Create();
  for (auto it : top_level_variables_) {
    if (!base::BeginsWith(it.first.str(), "$")) {
      auto value = *(heap_.base() + it.second + 1);
      if (ignore_callable && value.IsCallable()) {
        continue;
      }
      dictionary->SetValue(it.first, std::move(value));
    }
  }
  return std::make_unique<lepus::Value>(dictionary);
}

bool VMContext::GetTopLevelVariableByName(const base::String& name,
                                          Value* ret) {
  auto variable = top_level_variables_.find(name);
  if (variable != top_level_variables_.end()) {
    *ret = *(heap_.base() + variable->second + 1);
    return true;
  }
  return false;
}

int32_t VMContext::CallFunction(Value* function, size_t argc, Value* ret) {
  if (unlikely(function->IsClosure())) {
    heap_.top_ = function + 1;
    auto lepus_function = function->GetClosure()->function();
    const Instruction* ins = lepus_function->GetOpCodes();
    Frame frame(heap_.top_, function, ret, ins,
                ins + lepus_function->OpCodeSize(), current_frame_, 0);
    if (is_debug_enabled_) {
      auto debug_delegate = debug_delegate_.lock();
      if (debug_delegate != nullptr) {
        frame.SetDebuggerFrameId(debug_delegate->GenerateDebuggerFrameId());
      }
    }
    current_frame_ = &frame;
    RunFrame();
    // pop frame, reset register address
    heap_.top_ = frame.prev_frame_->register_;
    current_frame_ = frame.prev_frame_;
    return 1;
  } else if (function->IsCFunction()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "VMContext::CallCFunction");
    heap_.top_ = function + argc + 1;
    Frame frame(function + 1, function, ret, nullptr, nullptr, current_frame_,
                0);
    if (is_debug_enabled_) {
      auto debug_delegate = debug_delegate_.lock();
      if (debug_delegate != nullptr) {
        frame.SetDebuggerFrameId(debug_delegate->GenerateDebuggerFrameId());
      }
    }
    current_frame_ = &frame;
    CFunction cfunction = function->Function();
    *ret = cfunction(this);
    heap_.top_ = frame.prev_frame_->register_;
    current_frame_ = frame.prev_frame_;
    if (current_exception_.has_value()) {
      return -1;
    }
    return 1;
  } else {
    return 0;
  }
}

// report log box, but the program continues running
void VMContext::ReportLogBox(const std::string& exception_info, int& pc) {
  base::InlineVector<int, 32> frame_pc_;
  Frame* exception_frame = current_frame_;
  this->exception_info_ = exception_info;
  exception_info_.erase(exception_info_.find_last_not_of('\n') + 1,
                        exception_info_.size());
  exception_info_ = exception_info_ + "\n\n";
  frame_pc_.push_back(pc - 1);
  Frame* current_frame = current_frame_;
  while (current_frame) {
    current_frame = current_frame->prev_frame_;
    if (current_frame == current_frame->prev_frame_) break;
    frame_pc_.push_back(current_frame->current_pc_ - 1);
  }
  exception_info_.erase(exception_info_.find_last_not_of('\n') + 1,
                        exception_info_.size());
  this->exception_info_ += (" function name backtrace:\n" +
                            BuildBackTrace(frame_pc_, exception_frame));
  exception_info_ = "lepus exception:\n\n" + exception_info_;
  LOGE("lepus-ReportException: exception happened without catch "
       << this->exception_info_);
  ReportError(exception_info_);
}

void VMContext::ReportException(const std::string& exception_info, int& pc,
                                int& instruction_length,
                                fml::RefPtr<Closure>& current_frame_closure,
                                Function*& current_frame_function,
                                const Instruction*& current_frame_base,
                                Value*& current_frame_regs, bool report_logbox,
                                int32_t err_code) {
  base::InlineVector<int, 32> frame_pc_;
  Frame* exception_frame_ = current_frame_;
  bool find_caught_label = false;
  this->exception_info_ = exception_info;
  exception_info_.erase(exception_info_.find_last_not_of('\n') + 1,
                        exception_info_.size());
  exception_info_ = exception_info_ + "\n\n";
  frame_pc_.push_back(pc - 1);
  while (current_frame_) {
    Frame* current_frame = current_frame_;
    const Instruction* base = current_frame->instruction_;
    const Instruction* end = current_frame->end_;
    int length = static_cast<int>(end - base);
    int current_pc = current_frame->current_pc_;
    while (current_pc < length) {
      Instruction i = *(base + current_pc);
      current_pc++;
      current_frame->current_pc_ = current_pc;
      if (Instruction::GetOpCode(i) == TypeLabel_Catch) {
        pc = current_pc;
        find_caught_label = true;
        this->exception_info_ += (" function name backtrace:\n" +
                                  BuildBackTrace(frame_pc_, exception_frame_));
        instruction_length = static_cast<int>(current_frame_->end_ -
                                              current_frame_->instruction_);
        if (current_frame_->function_) {
          current_frame_closure = current_frame_->function_->GetClosure();
          current_frame_function = current_frame_closure->function().get();
        }
        current_frame_base = current_frame_->instruction_;
        current_frame_regs = current_frame_->register_;
        break;
      }
    }

    if (current_frame_ == current_frame_->prev_frame_ || find_caught_label) {
      break;
    }
    heap_.top_ = current_frame_->prev_frame_->register_;
    current_frame_ = current_frame_->prev_frame_;
    if (current_frame_ == current_frame_->prev_frame_) break;
    frame_pc_.push_back(current_frame_->current_pc_ - 1);
  }

  if (!find_caught_label) {
    instruction_length =
        static_cast<int>(current_frame_->end_ - current_frame_->instruction_);
    current_frame_base = current_frame_->instruction_;
    current_frame_regs = current_frame_->register_;
    if (current_frame_->function_) {
      current_frame_closure = current_frame_->function_->GetClosure();
      current_frame_function = current_frame_closure->function().get();
    }
    exception_info_.erase(exception_info_.find_last_not_of('\n') + 1,
                          exception_info_.size());
    this->exception_info_ += (" function name backtrace:\n" +
                              BuildBackTrace(frame_pc_, exception_frame_));
    exception_info_ = "lepus exception:\n\n" + exception_info_;
    LOGE("lepus-ReportException: exception happened without catch "
         << this->exception_info_);
    if (report_logbox) {
      ReportError(exception_info_, err_code);
    }
    return;
  } else {
    LOGE("lepus-CatchException: " << this->exception_info_);
  }
}

std::string VMContext::BuildBackTrace(const base::Vector<int>& pc,
                                      Frame* exception_frame_) {
  Frame* current_frame = exception_frame_;
  std::string backtrace_info;
  size_t index = 0;
  while (current_frame) {
    int current_pc = index >= pc.size() ? -1 : pc[index++];
    fml::RefPtr<Closure> current_closure =
        current_frame->function_->GetClosure();
    fml::RefPtr<Function> current_function = current_closure->function();
    if (!current_function.get()) break;

    // if there is no template_debug.json, send line + col
    // else send function id + pc index + template_debug.json url
    if (debug_info_url_.empty()) {
      // line + col
      int32_t line = -1;
      int32_t col = -1;
      current_function->GetLineCol(current_pc, line, col);
      backtrace_info += ("\tat " + current_function->GetFunctionName() + " :" +
                         std::to_string(line) + ":" + std::to_string(col));

    } else {
      // function id + pc_index
      backtrace_info += ("\tat " + current_function->GetFunctionName() + ":" +
                         std::to_string(current_function->GetFunctionId()) +
                         ":" + std::to_string(current_pc));
    }
    current_frame = current_frame->prev_frame_;
    if (current_frame == current_frame->prev_frame_) {
      break;
    } else {
      backtrace_info += "\n";
    }
  }

  if (!debug_info_url_.empty()) {
    // add template_debug.json url to backtrace info
    backtrace_info += "\ntemplate_debug_url:" + debug_info_url_;
  }

  return backtrace_info;
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Neg_UnlikelyPath(Value*& a) {
  char* endptr;
  double t = strtod(a->CString(), &endptr);
  if (*endptr) {
    a->SetNan(true);
  } else {
    if (t != static_cast<int64_t>(t)) {
      a->SetNumber(-t);
    } else {
      a->SetNumber(-static_cast<int64_t>(t));
    }
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Pos(Value*& a) {
  if (a->IsString()) {
    char* endptr;
    double t = strtod(a->CString(), &endptr);
    if (*endptr) {
      a->SetNan(true);
    } else {
      if (t != static_cast<int64_t>(t)) {
        a->SetNumber(t);
      } else {
        a->SetNumber(static_cast<int64_t>(t));
      }
    }
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Add_UnlikelyPath_B_Number(
    Value*& a, Value*& b, Value*& c) {
  char buffer[128];
  const char* num_str;
  if ((num_str = base::StringConvertHelper::NumberToString(b->Number(), buffer,
                                                           sizeof(buffer)))) {
    // processed as int
    a->SetString(num_str + c->StdString());
  } else {
    std::ostringstream stm;
    if (b->IsInt64()) {
      stm << b->Int64();
    } else {
      stm << base::StringConvertHelper::DoubleToString(b->Number());
    }
    stm << c->StdString();
    a->SetString(stm.str());
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Add_UnlikelyPath_C_Number(
    Value*& a, Value*& b, Value*& c) {
  char buffer[128];
  const char* num_str;
  if ((num_str = base::StringConvertHelper::NumberToString(c->Number(), buffer,
                                                           sizeof(buffer)))) {
    // processed as int
    a->SetString(b->StdString() + num_str);
  } else {
    std::ostringstream stm;
    stm << b->StdString();
    if (c->IsInt64()) {
      stm << c->Int64();
    } else {
      stm << base::StringConvertHelper::DoubleToString(c->Number());
    }
    a->SetString(stm.str());
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Mod(RunFrameContext& ctx) {
  DECL_ABC_FROM_CTX(ctx);
  GET_REGISTER_ABC_FROM_CTX(ctx);

  if (c->Number() == 0) {
    *a = Value();
    LOGE("lepus-mode: div 0");
    return;
  }
  Value b_tmp = *b;
  Value c_tmp = *c;
  if (b->IsInt64() && c->IsInt64()) {
    a->SetNumber(b->Int64() / c->Int64());
    a->SetNumber(b_tmp.Int64() - a->Int64() * c_tmp.Int64());
  } else {
    a->SetNumber(int(b->Number() / c->Number()));
    a->SetNumber(b_tmp.Number() - a->Number() * c_tmp.Number());
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_Pow(RunFrameContext& ctx) {
  DECL_ABC_FROM_CTX(ctx);
  GET_REGISTER_ABC_FROM_CTX(ctx);

  if (b->IsInt64() && c->IsInt64()) {
    a->SetNumber(static_cast<int64_t>(pow(b->Int64(), c->Int64())));
  } else if (b->IsNumber() && c->IsNumber()) {
    a->SetNumber(pow(b->Number(), c->Number()));
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_BitOr(RunFrameContext& ctx) {
  DECL_ABC_FROM_CTX(ctx);
  GET_REGISTER_ABC_FROM_CTX(ctx);

  if (b->IsNumber() && c->IsNumber()) {
    if (b->IsInt64() && c->IsInt64()) {
      a->SetNumber(b->Int64() | c->Int64());
    } else {
      int64_t x = static_cast<int64_t>(b->Number()) & 0xffffffff;
      int64_t y = static_cast<int64_t>(c->Number()) & 0xffffffff;
      a->SetNumber(x | y);
    }
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_BitAnd(RunFrameContext& ctx) {
  DECL_ABC_FROM_CTX(ctx);
  GET_REGISTER_ABC_FROM_CTX(ctx);

  if (b->IsNumber() && c->IsNumber()) {
    if (b->IsInt64() && c->IsInt64()) {
      a->SetNumber(b->Int64() & c->Int64());
    } else {
      int64_t x = static_cast<int64_t>(b->Number()) & 0xffffffff;
      int64_t y = static_cast<int64_t>(c->Number()) & 0xffffffff;
      a->SetNumber(x & y);
    }
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_BitXor(RunFrameContext& ctx) {
  DECL_ABC_FROM_CTX(ctx);
  GET_REGISTER_ABC_FROM_CTX(ctx);

  if (b->IsNumber() && c->IsNumber()) {
    if (b->IsInt64() && c->IsInt64()) {
      a->SetNumber(b->Int64() ^ c->Int64());
    } else {
      int64_t x = static_cast<int64_t>(b->Number()) & 0xffffffff;
      int64_t y = static_cast<int64_t>(c->Number()) & 0xffffffff;
      a->SetNumber(x ^ y);
    }
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_GetTable_UnlikelyPath_String(
    Value*& a, Value*& b, Value*& c) {
  if (c->IsNumber()) {
    auto b_str = b->String();
    int index = c->Number();
    DCHECK(index >= 0);
    if (static_cast<size_t>(index) >= b_str.length_utf16()) {
      *a = Value(base::String());
    } else {
      auto c_offset =
          base::Utf8IndexToCIndexForUtf16(b_str.c_str(), b_str.length(), index);
      auto result_begin = b_str.c_str() + c_offset;
      auto result_len = *result_begin != 0
                            ? (base::InlineUTF8SequenceLength(*result_begin))
                            : 0;
      *a = Value(base::String(result_begin, result_len));
    }
  } else {
#ifdef LEPUS_LOG
    LOGE("lepus: GetTable for base::String, key error is " << c->Type());
#endif
    *a = Value();
  }
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Op_CreateBlockContext(
    RunFrameContext& ctx) {
  auto& a = ctx.a;
  a = GET_REGISTER_A_FROM_CTX(ctx);
  long array_size = Instruction::GetParamB(ctx.i) + 1;

  *a = Value(CArray::Create());
  a->Array()->resize(array_size);
  auto current_closure = current_frame_->function_->GetClosure();
  Value pre_context = current_closure->GetContext();

  a->SetProperty(0, pre_context);
  for (auto i = 1; i < array_size; i++) {
    a->SetProperty(i, pre_context.GetProperty(i));
  }
  closure_context_ = *a;
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Label_EnterBlock(
    fml::RefPtr<Closure>& closure) {
  closure->SetContext(closure_context_);
  if (!closure_context_.IsNil()) {
    closures_.AddClosure(closure, executed_);
  }
  block_context_.push(PrepareClosureContext(closure));
}

LEPUS_NOT_INLINE void VMContext::RunFrame_Label_LeaveBlock() {
  Value current_context = std::move(closure_context_);
  long array_size = current_context.Array()->size();
  closure_context_ = std::move(block_context_.top());
  for (auto i = 1; i < array_size; i++) {
    closure_context_.SetProperty(i, current_context.GetProperty(i));
  }
  block_context_.pop();
}

void VMContext::RunFrame() {
  if (current_frame_ == nullptr) return;
  // function is retained by closure, so we only retain the closure by RefPtr.
  fml::RefPtr<Closure> closure = current_frame_->function_->GetClosure();
  Function* function = closure->function().get();
  Value* a = nullptr;
  Value* b = nullptr;
  Value* c = nullptr;
  const Instruction* base = current_frame_->instruction_;
  Value* regs = current_frame_->register_;
  __builtin_prefetch(base);
  __builtin_prefetch(regs);
  int length =
      static_cast<int>(current_frame_->end_ - current_frame_->instruction_);
  int pc = 0;
  VMContext::ContextScope vcs(this, closure);
  RunFrameContext run_frame_ctx{.a = a, .b = b, .c = c, .regs = regs};
  while (pc < length) {
    if (is_debug_enabled_) {
      auto debug_delegate = debug_delegate_.lock();
      if (debug_delegate != nullptr) {
        debug_delegate->UpdateCurrentPC(pc);
      }
    }
    const Instruction i = *(base + pc);
    run_frame_ctx.i = i;
    pc++;
    switch (Instruction::GetOpCode(i)) {
      case TypeOp_LoadNil: {
        // LoadNil is not extracted as RunFrame_Op_LoadNil() because it is
        // definitely executed frequently.
        // LoadNil use reg_b to decide actions:
        // 0: load nil
        // 1: load undefined when enable_null_prop_ad_undef_ is true
        // 2: load top level variables in globalThis
        // 3: load "lynx" in global_ as lynx
        a = GET_REGISTER_A(i);
        long reg_b = Instruction::GetParamB(i);
        if (enable_null_prop_as_undef_ && reg_b == 1) {
          a->SetUndefined();
        } else if (reg_b == 2) {
          *a = *GetTopLevelVariable();
        } else if (reg_b == 3) {
          // Now, only generate reg_b==3 when targetSdkVersion >= 2.8. Detail
          // can be seen in code_generator.cc. So the possible scenarios are as
          // follows
          // clang-format off
          // sdkVersion    targetSdkVersion    expectations
          //  < 2.8         < 2.8             will not generate reg_b==3, no bugs
          //  < 2.8         >= 2.8            will report error since targetSdkVersion > sdkVersion, no bugs
          //  >= 2.8        < 2.8             will not generate reg_b==3, no bugs
          //  >= 2.8        >= 2.8            will generate reg_b==3, and sdk >= 2.8 can handle this, no bugs.
          // clang-format on
          BASE_STATIC_STRING_DECL(kGlobalLynx, "lynx");
          auto* ptr = SearchGlobalData(kGlobalLynx);
          if (ptr == nullptr) {
            *a = lepus::Value();
          } else {
            *a = *ptr;
          }
        } else {
          a->SetNil();
        }
        break;
      }
      case TypeOp_SetCatchId:
        a = GET_REGISTER_A(i);
        a->SetString(std::move(exception_info_));
        break;
      case TypeOp_LoadConst:
        a = GET_REGISTER_A(i);
        b = GET_CONST_VALUE(i);
        *a = *b;
        break;
      case TypeOp_Move:
        a = GET_REGISTER_A(i);
        b = GET_REGISTER_B(i);
        *a = *b;
        break;
      case TypeOp_GetContextSlot:
      case TypeOp_SetContextSlot: {
        a = GET_REGISTER_A(i);
        long index = Instruction::GetParamB(i);
        long offset = Instruction::GetParamC(i);
        auto op_code = Instruction::GetOpCode(i);
        Value array = closure->GetContext();
        while (offset > 0) {
          array = array.Array()->get(0);
          offset--;
        }
        if (op_code == TypeOp_GetContextSlot) {
          *a = array.Array()->get(index);
        } else {
          array.Array()->set(static_cast<int>(index), *a);
        }
        break;
      }
      case TypeOp_GetUpvalue: {
        a = GET_REGISTER_A(i);
        b = GET_UPVALUE_B(i);
        *a = *b;
        break;
      }
      case TypeOp_SetUpvalue: {
        a = GET_REGISTER_A(i);
        b = GET_UPVALUE_B(i);
        *b = *a;
        break;
      }
      case TypeOp_GetGlobal:
        a = GET_REGISTER_A(i);
        b = GET_Global_VALUE(i);
        *a = *b;
        break;
      case TypeOp_SetGlobal:
        break;
      case TypeOp_GetBuiltin:
        a = GET_REGISTER_A(i);
        b = GET_Builtin_VALUE(i);
        *a = *b;
        break;
      case TypeOp_Closure: {
        a = GET_REGISTER_A(i);
        long index = Instruction::GetParamBx(i);
        GenerateClosure(a, index);
      } break;
      case TypeOp_Call: {
        a = GET_REGISTER_A(i);
        long argc = Instruction::GetParamB(i);
        c = GET_REGISTER_C(i);
        current_frame_->current_pc_ = pc;
        if (likely(a->IsClosure())) {
          auto lepus_function = a->GetClosure()->function();
          int32_t params_size = lepus_function->GetParamsSize();
          if (params_size > static_cast<int32_t>(argc)) {
            ReportLogBox("Do not support default function params on function " +
                             lepus_function->GetFunctionName() + ".",
                         pc);
          }
        }
        int32_t result = CallFunction(a, argc, c);
        if (result < 0) {
          // exception
          ReportException(*std::exchange(current_exception_, std::nullopt), pc,
                          length, closure, function, base, regs, true,
                          std::exchange(err_code_, error::E_MTS_RUNTIME_ERROR));
        } else if (result == 0) {
          // failed: not a function
          ReportException(std::string(TYPEERROR) + ", not a function.", pc,
                          length, closure, function, base, regs, true);
        } else if (pc < current_frame_->current_pc_) {
          pc = length;
        }
        break;
      }
      case TypeOp_Ret:
        a = GET_REGISTER_A(i);
        if (current_frame_->return_ != nullptr) {
          *current_frame_->return_ = *a;
        }
        return;
      case TypeOp_JmpFalse:
        a = GET_REGISTER_A(i);
        if (a->IsFalse()) pc += -1 + Instruction::GetParamsBx(i);
        break;
      case TypeOp_JmpTrue:
        a = GET_REGISTER_A(i);
        if (a->IsTrue()) pc += -1 + Instruction::GetParamsBx(i);
        break;
      case TypeOp_Jmp:
        pc += -1 + Instruction::GetParamsBx(i);
        break;
      case TypeLabel_Catch:
        break;
      case TypeLabel_Throw: {
        a = GET_REGISTER_A(i);
        std::ostringstream msg;
        msg << a;
        ReportException(msg.str(), pc, length, closure, function, base, regs,
                        false);
        break;
      }
      case TypeOp_SetContextSlotMove: {
        a = GET_REGISTER_A(i);
        long array_index = Instruction::GetParamB(i);
        c = GET_REGISTER_C(i);
        a->Array()->set(static_cast<int>(array_index), *c);
        break;
      }
      case TypeOp_GetContextSlotMove: {
        a = GET_REGISTER_A(i);
        long array_index = Instruction::GetParamB(i);
        c = GET_REGISTER_C(i);
        *a = c->Array()->get(array_index);
        break;
      }
      case TypeOp_Typeof: {
        static constexpr const char kUndefined[] = "undefined";
        static constexpr const char kObject[] = "object";
        static constexpr const char kBoolean[] = "boolean";
        static constexpr const char kNumber[] = "number";
        static constexpr const char kString[] = "string";
        static constexpr const char kFunction[] = "function";
        static constexpr const char kLepusObject[] = "lepusobject";
        a = GET_REGISTER_A(i);
        switch (a->Type()) {
          case lepus::ValueType::Value_Undefined:
            a->SetString(BASE_STATIC_STRING(kUndefined));
            break;
          case lepus::ValueType::Value_Nil:
          case lepus::ValueType::Value_Table:
          case lepus::ValueType::Value_Array:
            a->SetString(BASE_STATIC_STRING(kObject));
            break;
          case lepus::ValueType::Value_Bool:
            a->SetString(BASE_STATIC_STRING(kBoolean));
            break;
          case lepus::ValueType::Value_Double:
          case lepus::ValueType::Value_Int32:
          case lepus::ValueType::Value_Int64:
          case lepus::ValueType::Value_UInt32:
          case lepus::ValueType::Value_UInt64:
            a->SetString(BASE_STATIC_STRING(kNumber));
            break;
          case lepus::ValueType::Value_String:
            a->SetString(BASE_STATIC_STRING(kString));
            break;
          case lepus::ValueType::Value_Closure:
          case lepus::ValueType::Value_CFunction:
            a->SetString(BASE_STATIC_STRING(kFunction));
            break;
          case lepus::ValueType::Value_JSObject:
            a->SetString(BASE_STATIC_STRING(kLepusObject));
            break;
          default:
            a->SetString(BASE_STATIC_STRING(kObject));
            break;
        }
        break;
      }
      case TypeOp_Neg:
        a = GET_REGISTER_A(i);
        if (a->IsInt64()) {
          a->SetNumber(-a->Int64());
        } else if (a->IsNumber()) {
          a->SetNumber(-a->Number());
        } else if (a->IsString()) {
          RunFrame_Op_Neg_UnlikelyPath(a);
        }
        break;
      case TypeOp_Pos:
        a = GET_REGISTER_A(i);
        RunFrame_Op_Pos(a);
        break;
      case TypeOp_Not:
        a = GET_REGISTER_A(i);
        a->SetBool(!a->Bool());
        break;
      case TypeOp_BitNot:
        a = GET_REGISTER_A(i);
        if (a->IsNumber()) {
          if (a->IsInt64())
            a->SetNumber(~(a->Int64()));
          else {
            int64_t x = static_cast<int64_t>(a->Number()) & 0xffffffff;
            a->SetNumber(~x);
          }
        }
        break;
      case TypeOp_And:
        //&&
        GET_REGISTER_ABC(i);
        if (b->IsTrue()) {
          *a = *c;
        } else {
          *a = *b;
        }
        break;
      case TypeOp_Or:
        //||
        GET_REGISTER_ABC(i);
        if (!b->IsFalse()) {
          *a = *b;
        } else {
          *a = *c;
        }
        break;
      case TypeOp_Len:
        break;
      case TypeOp_Add:
        GET_REGISTER_ABC(i);
        // most cases are string + string
        // some cases are int + string
        // we just optimized those two case
        if (b->IsString() && c->IsString()) {
          a->SetString(b->StdString() + c->StdString());
          break;
        }

        if (b->IsNumber() && c->IsNumber()) {
          if (b->IsInt64() && c->IsInt64()) {
            a->SetNumber(b->Int64() + c->Int64());
          } else {
            a->SetNumber(b->Number() + c->Number());
          }
          break;
        }

        if (b->IsNumber()) {
          RunFrame_Op_Add_UnlikelyPath_B_Number(a, b, c);
        } else if (c->IsNumber()) {
          RunFrame_Op_Add_UnlikelyPath_C_Number(a, b, c);
        } else {
          // may string + null or null + string
          a->SetString(b->StdString() + c->StdString());
        }
        break;
      case TypeOp_Sub:
        GET_REGISTER_ABC(i);
        if (b->IsInt64() && c->IsInt64()) {
          a->SetNumber(b->Int64() - c->Int64());
        } else {
          a->SetNumber(b->Number() - c->Number());
        }
        break;
      case TypeOp_Mul:
        GET_REGISTER_ABC(i);
        if (b->IsInt64() && c->IsInt64()) {
          a->SetNumber(b->Int64() * c->Int64());
        } else {
          a->SetNumber(b->Number() * c->Number());
        }
        break;
      case TypeOp_Div: {
        GET_REGISTER_ABC(i);
        if (c->Number() == 0) {
          *a = Value();
          LOGE("lepus-div: div 0");
          break;
        }
        double ans = b->Number() / c->Number();
        if (lynx::base::StringConvertHelper::IsInt64Double(ans)) {
          a->SetNumber(static_cast<int64_t>(ans));
        } else {
          a->SetNumber(ans);
        }
        break;
      }
      case TypeOp_Pow:
        RunFrame_Op_Pow(run_frame_ctx);
        break;
      case TypeOp_Mod:
        RunFrame_Op_Mod(run_frame_ctx);
        break;
      case TypeOp_BitOr:
        RunFrame_Op_BitOr(run_frame_ctx);
        break;
      case TypeOp_BitAnd:
        RunFrame_Op_BitAnd(run_frame_ctx);
        break;
      case TypeOp_BitXor:
        RunFrame_Op_BitXor(run_frame_ctx);
        break;
      case TypeOp_Less:
        GET_REGISTER_ABC(i);
        if (b->IsNumber() && c->IsNumber()) {
          a->SetBool(b->Number() < c->Number());
        } else if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() < c->StdString());
        } else {
          a->SetBool(false);
        }
        break;
      case TypeOp_Greater:
        GET_REGISTER_ABC(i);
        if (b->IsNumber() && c->IsNumber()) {
          a->SetBool(b->Number() > c->Number());
        } else if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() > c->StdString());
        } else {
          a->SetBool(false);
        }
        break;
      case TypeOp_Equal:
        GET_REGISTER_ABC(i);
        if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() == c->StdString());
        } else {
          a->SetBool(*b == *c);
        }
        break;
      case TypeOp_AbsEqual:
        GET_REGISTER_ABC(i);
        if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() == c->StdString());
        } else {
          a->SetBool(*b == *c);
        }
        break;
      case TypeOp_UnEqual:
        GET_REGISTER_ABC(i);
        a->SetBool(*b != *c);
        break;
      case TypeOp_AbsUnEqual:
        GET_REGISTER_ABC(i);
        a->SetBool(*b != *c);
        break;
      case TypeOp_LessEqual:
        GET_REGISTER_ABC(i);
        if (b->IsNumber() && c->IsNumber()) {
          a->SetBool((b->Number() <= c->Number()));
        } else if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() <= c->StdString());
        } else {
          a->SetBool(false);
        }
        break;
      case TypeOp_GreaterEqual:
        GET_REGISTER_ABC(i);
        if (b->IsNumber() && c->IsNumber()) {
          a->SetBool((b->Number() >= c->Number()));
        } else if (b->IsString() && c->IsString()) {
          a->SetBool(b->StdString() >= c->StdString());
        } else {
          a->SetBool(false);
        }
        break;
      case TypeOp_NewArray: {
        a = GET_REGISTER_A(i);
        long argc = Instruction::GetParamB(i);
        auto arr = CArray::Create();
        arr->reserve(argc);
        for (int i = 0; i < argc; ++i) {
          arr->push_back(*(a + i + 1));
        }
        *a = Value(std::move(arr));
      } break;
      case TypeOp_CreateContext: {
        a = GET_REGISTER_A(i);
        // context + data
        long array_size = Instruction::GetParamB(i) + 1;
        auto arr = CArray::Create();
        arr->resize(array_size);
        Frame* frame = current_frame_;
        auto current_closure = frame->function_->GetClosure();
        arr->set(0, current_closure->GetContext());
        *a = Value(std::move(arr));
        closure_context_ = *a;
        break;
      }
      case TypeOp_PushContext: {
        a = GET_REGISTER_A(i);
        context_.push(*a);
        break;
      }
      case TypeOp_PopContext: {
        context_.pop();
        break;
      }
      case TypeOp_NewTable:
        a = GET_REGISTER_A(i);
        a->SetTable(Dictionary::Create());
        break;
      case TypeOp_SetTable:
        GET_REGISTER_ABC(i);
        if (a->IsTable() && b->IsString()) {
          a->Table()->SetValue(b->String(), *c);
        } else if (a->IsArray() && b->IsNumber()) {
          a->Array()->set(static_cast<int>(b->Number()), *c);
        } else if (a->IsTable() && b->IsNumber()) {
          std::ostringstream s;
          s << b->Number();
          a->Table()->SetValue(s.str(), *c);
        }
        break;
      case TypeOp_GetTable:
        GET_REGISTER_ABC(i);

        if (b->IsNil() || b->IsUndefined()) {
          a->SetNil();
          if (enable_strict_check_) {
            std::string key = "";
            if (c->IsString()) {
              key = c->StdString();
            }
            ReportException("Cannot read " + key + " of null ", pc, length,
                            closure, function, base, regs,
                            enable_strict_check_);
            break;
          } else {
#ifdef LEPUS_LOG
            if (c->IsString()) {
              LOGE("lepus: Cannot read property " << c->StdString()
                                                  << " of undefined.");
            } else if (c->IsNumber()) {
              LOGE("lepus: Cannot read property " << c->Number()
                                                  << " of undefined.");
            } else {
              LOGE("lepus: Cannot read property of undefined");
            }
#endif
          }
          enable_null_prop_as_undef_ ? a->SetUndefined() : a->SetNil();
          break;
        }
        switch (b->Type()) {
          case Value_Table:
            if (c->IsString()) {
              *a =
                  b->Table()->GetValue(c->String(), enable_null_prop_as_undef_);
            } else if (c->IsNumber()) {
              std::ostringstream s;
              s << c->Number();
              *a = b->Table()->GetValue(s.str(), enable_null_prop_as_undef_);
            } else {
              a->SetNil();
            }
            break;
          case Value_Array:
            if (c->IsNumber()) {
              *a = b->Array()->get(c->Number());
            } else if (c->IsString()) {
              auto c_str = c->String();
              const auto& c_str_value = c_str.str();
              if (c_str_value == "length") {
                *a = Value(static_cast<int64_t>((b->Array()->size())));
              } else if (unlikely(b->Array()->GetIsMatchResult())) {
                if (c_str_value == "input") {
                  *a = b->Array()->GetMatchInput();
                } else if (c_str_value == "index") {
                  *a = b->Array()->GetMatchIndex();
                } else if (c_str_value == "groups") {
                  *a = b->Array()->GetMatchGroups();
                }
              } else {
                *a = array_prototype_.Table()->GetValue(c_str);
              }
            } else {
#ifdef LEPUS_LOG
              LOGE("lepus: GetTable for Array, key error is " << c->Type());
#endif
              *a = Value();
            }
            break;
          case Value_String:
            if (c->IsString()) {
              auto c_str = c->String();
              const auto& c_str_value = c_str.str();
              if (c_str_value == "length") {
                *a = Value(static_cast<int64_t>((b->String().length_utf16())));
              } else {
                *a = string_prototype_.Table()->GetValue(c_str);
              }
            } else {
              RunFrame_Op_GetTable_UnlikelyPath_String(a, b, c);
            }
            break;
          case Value_CDate:
            if (c->IsString()) {
              *a = date_prototype_.Table()->GetValue(c->String());
            } else {
              *a = Value();
            }
            break;
          case Value_RegExp:
            if (c->IsString()) {
              *a = regexp_prototype_.Table()->GetValue(c->String());
            } else {
              *a = Value();
            }
            break;
          default:
            if (b->IsNumber() && c->IsString()) {
              *a = number_prototype_.Table()->GetValue(c->String());
            } else {
#ifdef LEPUS_LOG
              LOGE("lepus: GetTable unknown, receiver type  "
                   << b->Type() << ", key type " << c->Type());
#endif
              *a = Value();
            }
            break;
        }
        break;
      case TypeOp_Switch: {
        a = GET_REGISTER_A(i);
        long index = Instruction::GetParamBx(i);
        long jmp = function->GetSwitch(index)->Switch(a);
        pc += -1 + jmp;
      } break;
      case TypeOp_Inc:
        a = GET_REGISTER_A(i);
        if (a->IsNumber()) {
          if (a->IsInt64()) {
            a->SetNumber(a->Int64() + 1);
          } else {
            a->SetNumber(a->Number() + 1);
          }
        }
        break;
      case TypeOp_Dec:
        a = GET_REGISTER_A(i);
        if (a->IsNumber()) {
          if (a->IsInt64()) {
            a->SetNumber(a->Int64() - 1);
          } else {
            a->SetNumber(a->Number() - 1);
          }
        }
        break;
      case TypeOp_Noop:
        break;
      case TypeLabel_EnterBlock:
        RunFrame_Label_EnterBlock(closure);
        break;
      case TypeLabel_LeaveBlock:
        RunFrame_Label_LeaveBlock();
        break;
      case TypeOp_CreateBlockContext:
        RunFrame_Op_CreateBlockContext(run_frame_ctx);
        break;
      default:
        break;
    }
  }
  if (current_frame_->return_ != nullptr) {
    current_frame_->return_->SetNil();
  }
}

void VMContext::GenerateClosure(Value* value, long index) {
  Frame* frame = current_frame_;
  auto current_closure = frame->function_->GetClosure();
  auto function = current_closure->function()->GetChildFunction(index);
  auto closure = Closure::Create(function);

  std::size_t upvalues_count = function->UpvaluesSize();
  closure->upvalues_.reserve(upvalues_count);
  for (int i = 0; static_cast<size_t>(i) < upvalues_count; ++i) {
    UpvalueInfo* info = function->GetUpvalue(i);
    if (info->in_parent_vars_) {
      Value* v = frame->register_ + info->register_;
      closure->AddUpvalue(v);
    } else {
      closure->AddUpvalue(current_closure->GetUpvalue(info->register_));
    }
  }
  closure->SetContext(closure_context_);
  value->SetClosure(closure);

  if (!closure_context_.IsNil()) {
    closures_.AddClosure(closure, executed_);
  }
}

fml::RefPtr<Function> VMContext::GetRootFunction() {
  return fml::RefPtr<Function>(root_function_.get());
}

Frame* VMContext::GetCurrentFrame() { return current_frame_; }

/**
 * @brief Iterate through the array and delete elements with a reference count
 * of 1
 * For saving reverse the array's time, lepus processes up to one hundred
 * elements at a time, and the remaining elements are processed in the next
 * round.
 */
void VMContext::ClosureManager::ClearClosure() {
  int64_t i = 0;
  int64_t step = all_closures_after_executed_.size() > 100
                     ? 100
                     : all_closures_after_executed_.size();
  while (i++ < step) {
    if (itr_ < all_closures_after_executed_.size()) {
      if (all_closures_after_executed_[itr_]->HasOneRef()) {
        all_closures_after_executed_.erase(
            all_closures_after_executed_.begin() + itr_);
      }
      itr_++;
    } else {
      itr_ = 0;
    }
  }
}

void VMContext::ClosureManager::AddClosure(fml::RefPtr<lepus::Closure>& closure,
                                           bool context_executed) {
  ClearClosure();
  if (context_executed) {
    all_closures_after_executed_.push_back(closure);
  } else {
    all_closures_before_executed_.push_back(closure);
  }
}

void VMContext::ClosureManager::CleanUpClosuresCreatedAfterExecuted() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "CleanUpClosuresCreatedAfterExecuted");
  for (auto& itr : all_closures_after_executed_) {
    itr->SetContext(Value());
  }
  all_closures_after_executed_.clear();
  itr_ = 0;
}

void VMContext::SetGlobalData(const base::String& name, Value value) {
  global_.Add(name, std::move(value));
}

lepus::Value VMContext::GetGlobalData(const base::String& name) {
  auto ptr = global_.Find(name);
  if (ptr == nullptr) {
    return lepus::Value();
  }
  return lepus::Value(*ptr);
}

VMContext::ClosureManager::~ClosureManager() {
  CleanUpClosuresCreatedAfterExecuted();
  for (auto& itr : all_closures_before_executed_) {
    itr->SetContext(Value());
  }
  itr_ = 0;
}

void VMContext::RegisterMethodToLynx() {
#ifndef LEPUS_PC
  tasm::Utils::RegisterMethodToLynx(this, lynx_);
#endif
}

void VMContext::RegisterLepusVerion() {
  BASE_STATIC_STRING_DECL(kLepusVersion, "__lepus_version__");
  static constexpr const char kLepusVersionValue[] = LYNX_LEPUS_VERSION;
  builtin_.Set(kLepusVersion, Value(BASE_STATIC_STRING(kLepusVersionValue)));
}

void VMContext::CleanClosuresInCycleReference() {
  closures_.CleanUpClosuresCreatedAfterExecuted();
}

bool VMContext::DeSerialize(const ContextBundle& bundle, bool, Value* ret,
                            const char* file_name) {
  auto bundle_copy = static_cast<const VMContextBundle&>(bundle);
  return MoveContextBundle(bundle_copy);
}

bool VMContext::MoveContextBundle(VMContextBundle& bundle) {
  for (auto& pair : bundle.lepus_root_global_) {
    global_.Add(std::move(pair.first), std::move(pair.second));
  }

  root_function_.swap(bundle.lepus_root_function_);
  top_level_variables_.swap(bundle.lepus_top_variables_);
  return true;
}

void VMContext::RegisterCtxBuiltin(const tasm::ArchOption& option) {
#ifndef LEPUS_PC
  tasm::Utils::RegisterBuiltin(this);
  tasm::Renderer::RegisterBuiltin(this, option);
#endif
  return;
}

void VMContext::ApplyConfig(
    const std::shared_ptr<tasm::PageConfig>& page_config,
    const tasm::CompileOptions& options) {
  SetEnableStrictCheck(page_config->GetEnableLepusStrictCheck());
  bool data_strict_mode =
      page_config->GetDSL() == tasm::PackageInstanceDSL::REACT
          ? false
          : page_config->GetDataStrictMode();
  SetEnableTopVarStrictMode(data_strict_mode);
  SetNullPropAsUndef(page_config->GetEnableLepusNullPropAsUndef());
  SetDebugInfoURL(options.template_debug_url_);
  return;
}

bool VMContextBundle::IsLepusNG() const { return false; }

lepus::Value VMContext::ReportFatalError(const std::string& error_message,
                                         bool exit, int32_t code) {
  if (exit) {
    LOGE("VMContext::ReportFatalError: " << error_message);
    abort();
  }

  current_exception_ = std::make_optional(error_message);
  err_code_ = code;
  return lepus::Value();
}

lepus::Value VMContext::GetCurrentThis(lepus::Value* argv, int32_t offset) {
  return *(argv + offset);
}

}  // namespace lepus
}  // namespace lynx
