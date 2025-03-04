// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/context_binary_writer.h"

#include <utility>

#include "base/include/sorted_for_each.h"
#include "base/include/value/base_string.h"
#include "core/renderer/tasm/config.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "core/template_bundle/template_codec/version.h"
#include "quickjs/include/quickjs.h"
#ifdef OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {

ContextBinaryWriter::ContextBinaryWriter(
    Context* context, const tasm::CompileOptions& compile_options,
    const lepus::Value& trial_options, bool enableDebugInfo)
    : context_(context),
      compile_options_(compile_options),
      trial_options_(trial_options),
      need_lepus_debug_info_(enableDebugInfo) {
  feature_control_variables_ = lynx::tasm::Config::IsHigherOrEqual(
      compile_options_.target_sdk_version_, FEATURE_CONTROL_VERSION);
}

ContextBinaryWriter::~ContextBinaryWriter() = default;

void ContextBinaryWriter::encode() {
  if (context_->IsLepusNGContext()) {
    auto* qctx = QuickContext::Cast(context_);

    LEPUSContext* ctx = qctx->context();
    size_t out_buf_len;
    uint8_t* out_buf =
        LEPUS_WriteObject(ctx, &out_buf_len, qctx->GetTopLevelFunction(),
                          LEPUS_WRITE_OBJ_BYTECODE);
    HandleScope func_scope(ctx, out_buf, HANDLE_TYPE_DIR_HEAP_OBJ);
    WriteCompactU64(static_cast<uint64_t>(out_buf_len));
    WriteData(out_buf, out_buf_len, "quick bytecode");
    if (!qctx->GetGCFlag()) {
      lepus_free(qctx->context(), out_buf);
    }
    return;
  }

  // for VmContext
  SerializeGlobal();

  SerializeFunction(lepus::VMContext::Cast(context_)->GetRootFunction());

  SerializeTopVariables();
}

void ContextBinaryWriter::SerializeGlobal() {
  Global* global = VMContext::Cast(context_)->global();
  if (global == nullptr) return;

  size_t size = 0;
  auto iter = global->global_.begin();

  std::vector<base::String> names(global->global_content_.size());
  for (; iter != global->global_.end(); ++iter) {
    names[iter->second] = iter->first;
    // TODO: now don't serialize cfuntion!
    if (global->Get(iter->second)->IsCFunction()) {
      continue;
    }
    size++;
  }

  WriteCompactU32(size);

  for (size_t i = 0; i < global->global_content_.size(); ++i) {
    const Value& value = global->global_content_[i];
    // TODO: now don't serialize cfuntion!
    if (value.IsCFunction()) {
      continue;
    }
    EncodeUtf8Str(names[i].c_str(), names[i].length());
    EncodeValue(&value);
  }
}

void ContextBinaryWriter::SetFunctionIgnoreList(
    const std::vector<std::string>& ignored_funcs) {
  ignored_funcs_ = ignored_funcs;
}

void ContextBinaryWriter::SerializeFunction(fml::RefPtr<Function> function) {
  bool need_remove = false;
  for (size_t i = 0; i < ignored_funcs_.size(); i++) {
    if (function->function_name_.find(ignored_funcs_[i]) == 0) {
      need_remove = true;
      break;
    }
  }

  // const value
  // put function name to function's const values
  lepus::Value debug_info(lepus::Dictionary::Create());
  // __func_name__ for function's name
  debug_info.Table()->SetValue(Function::kFuncName,
                               function->GetFunctionName());
  // __function_id__ for function's id
  debug_info.Table()->SetValue(Function::kFuncId, function->GetFunctionId());

  if (NeedLepusDebugInfo()) {
    debug_info.Table()->SetValue(Function::kLineColInfo,
                                 function->GetLineInfo());
  }

  // add params size for lepus runtime params check
  debug_info.Table()->SetValue(Function::kParamsSize,
                               function->GetParamsSize());
  function->const_values_.push_back(debug_info);

  size_t size = need_remove ? 0 : function->const_values_.size();
  WriteCompactU32(size);
  for (size_t i = 0; i < size; ++i) {
    EncodeValue(&function->const_values_[i]);
  }

  // instruction
  size = need_remove ? 0 : function->op_codes_.size();
  WriteCompactU32(size);

  for (size_t i = 0; i < size; ++i) {
    WriteCompactU64((uint64_t)function->op_codes_[i].op_code_);
  }

  func_vec.push_back(function);
  func_map.insert(std::make_pair(function, func_vec.size() - 1));

  // up value info
  size = need_remove ? 0 : function->UpvaluesSize();
  WriteCompactU32(size);
  for (size_t i = 0; i < size; ++i) {
    UpvalueInfo* info = function->GetUpvalue(i);
    EncodeUtf8Str(info->name_.c_str());
    WriteCompactU64(info->register_);
    WriteByte(info->in_parent_vars_);
  }

  // switch info
  const base::Version version =
      !compile_options_.target_sdk_version_.size()
          ? MIN_SUPPORTED_LYNX_VERSION
          : base::Version(compile_options_.target_sdk_version_);
  if (lynx::tasm::Config::IsHigherOrEqual(version, FEATURE_CONTROL_VERSION_2)) {
    WriteCompactU32(0);
  }

  // children
  size = need_remove ? 0 : function->child_functions_.size();
  WriteCompactU32(size);
  for (auto child : function->child_functions_) {
    SerializeFunction(child);
  }
}

// TODO
void ContextBinaryWriter::EncodeExpr() {}

void ContextBinaryWriter::SerializeTopVariables() {
  // encode top_level_variables_
  lepus::VMContext* ctx = lepus::VMContext::Cast(context_);
  size_t size = ctx->top_level_variables_.size();
  WriteCompactU32(size);
  base::sorted_for_each(ctx->top_level_variables_.begin(),
                        ctx->top_level_variables_.end(),
                        [this](const auto& it) {
                          EncodeUtf8Str(it.first.c_str());
                          int32_t index = it.second;
                          WriteCompactS32(index);
                        });
}

void ContextBinaryWriter::EncodeClosure(const fml::RefPtr<Closure>& value) {
  if (!value) return;

  size_t count = value->upvalues_.size();
  WriteCompactU32(count);
  for (size_t i = 0; i < count; i++) {
  }

  fml::RefPtr<Function> function = value->function_;
  size_t index = func_map[function];
  WriteCompactU32(index);
}

void ContextBinaryWriter::EncodeUtf8Str(const char* value) {
  WriteStringDirectly(value);
}

void ContextBinaryWriter::EncodeUtf8Str(const char* value, size_t length) {
  WriteStringDirectly(value, length);
}

void ContextBinaryWriter::EncodeTable(fml::RefPtr<Dictionary> dictionary,
                                      bool is_header) {
  if (!dictionary) return;
  size_t size = dictionary->size();
  WriteCompactU32(size);
  base::sorted_for_each(
      dictionary->begin(), dictionary->end(),
      [this, is_header](const auto& it) {
        // key
        // If encode happens in parsing header stage, nothing
        // in string_list, so write string directly
        if (is_header) {
          WriteStringDirectly(it.first.c_str());
        } else {
          EncodeUtf8Str(it.first.c_str(), it.first.length());
        }
        // value
        EncodeValue(&it.second, is_header);
      },
      // only need to compare key for table
      // this will compare the underlying std::string
      [](const auto& a, const auto& b) { return a.first < b.first; });
}

void ContextBinaryWriter::EncodeArray(fml::RefPtr<CArray> ary) {
  if (!ary) return;
  size_t size = ary->size();
  WriteCompactU32(size);
  for (size_t it = 0; it < ary->size(); ++it) {
    EncodeValue(&(ary->get(it)));
  }
}

void ContextBinaryWriter::EncodeDate(fml::RefPtr<CDate> date) {
  if (!date) return;
  tm date_ = date->get_date_();
  int ms_ = date->get_ms_();
  int language = date->get_language();
  WriteCompactS32(language);
  WriteCompactS32(ms_);
  WriteCompactS32(date_.tm_year);
  WriteCompactS32(date_.tm_mon);
  WriteCompactS32(date_.tm_mday);
  WriteCompactS32(date_.tm_hour);
  WriteCompactS32(date_.tm_min);
  WriteCompactS32(date_.tm_sec);
  WriteCompactS32(date_.tm_wday);
  WriteCompactS32(date_.tm_yday);
  WriteCompactS32(date_.tm_isdst);
  WriteCompactS32(date_.tm_gmtoff);
  return;
}

void ContextBinaryWriter::EncodeValue(const Value* value, bool is_header) {
  if (value == nullptr) {
    // TODO
    value = new Value();
  }
  if (value->IsJSValue()) {
    value->ToLepusValue();
  }
  if (!feature_control_variables_ && value->IsNumber()) {
    WriteU8(ValueType::Value_Double);
  } else {
    WriteU8(value->Type());
  }
  switch (value->Type()) {
    case ValueType::Value_Int64:
      if (feature_control_variables_) {
        WriteCompactU64(value->Int64());
      } else {
        WriteCompactD64(value->Number());
      }
      break;
    case ValueType::Value_UInt32:
      WriteCompactU32(value->UInt32());
      break;
    case ValueType::Value_Int32:
      WriteCompactS32(value->Int32());
      break;
    case ValueType::Value_Double:
      WriteCompactD64(value->Number());
      break;
    case ValueType::Value_String:
      if (is_header) {
        // If encode happens in parsing header stage, nothing in string_list, so
        // read string directly
        WriteStringDirectly(value->CString(), value->StdString().length());
      } else {
        EncodeUtf8Str(value->CString(), value->String().length());
      }
      break;
    case ValueType::Value_Table:
      EncodeTable(value->Table(), is_header);
      break;
    case ValueType::Value_Bool:
      WriteByte(value->Bool());
      break;
    case ValueType::Value_Array:
      EncodeArray(value->Array());
      break;
    case ValueType::Value_Closure:
      EncodeClosure(value->GetClosure());
      break;
    case ValueType::Value_CPointer:
    case ValueType::Value_RefCounted:
      // Nothing to do
      break;
    case ValueType::Value_CFunction:
      // TODO...
      // DCHECK(0);
      break;
    case ValueType::Value_Nil:
    case ValueType::Value_Undefined:
      break;
    case ValueType::Value_CDate:
      EncodeDate(value->Date());
      break;
    case ValueType::Value_RegExp: {
      Value pattern = lepus::Value(value->RegExp()->get_pattern().str());
      EncodeValue(&pattern);
      Value flag = lepus::Value(value->RegExp()->get_flags().str());
      EncodeValue(&flag);
      break;
    }
    case ValueType::Value_NaN:
      WriteByte(value->NaN());
      break;
    default:
      break;
  }
}

void ContextBinaryWriter::EncodeCSSValue(const tasm::CSSValue& css_value) {
  bool enable_css_parser = false;
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_VALUE_VERSION) &&
      compile_options_.enable_css_parser_) {
    enable_css_parser = true;
  }

  bool enable_css_variable = false;
  if (lynx::tasm::Config::IsHigherOrEqual(compile_options_.target_sdk_version_,
                                          FEATURE_CSS_STYLE_VARIABLES) &&
      compile_options_.enable_css_variable_) {
    enable_css_variable = true;
  }
  EncodeCSSValue(css_value, enable_css_parser, enable_css_variable);
}

void ContextBinaryWriter::EncodeCSSValue(const tasm::CSSValue& css_value,
                                         bool enable_css_parser,
                                         bool enable_css_variable) {
  if (enable_css_parser) {
    // pattern
    WriteCompactU32((uint16_t)css_value.GetPattern());
    // value
    EncodeValue(&(css_value.GetValue()));
  } else {
    EncodeValue(&(css_value.GetValue()));
  }

  if (enable_css_variable) {
    // value_type
    WriteCompactU32((uint16_t)css_value.GetValueType());
    // default_value
    EncodeUtf8Str(css_value.GetDefaultValue().c_str());
    if (lynx::tasm::Config::IsHigherOrEqual(
            compile_options_.target_sdk_version_, LYNX_VERSION_2_14)) {
      // default_value_map
      if (css_value.GetDefaultValueMapOpt().has_value()) {
        EncodeValue(css_value.GetDefaultValueMapOpt().operator->());
      } else {
        auto empty_default_value_map = Value();
        EncodeValue(&empty_default_value_map);
      }
    }
  }
}

}  // namespace lepus
}  // namespace lynx
