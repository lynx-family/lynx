// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_CONTEXT_BINARY_WRITER_H_
#define CORE_RUNTIME_VM_LEPUS_CONTEXT_BINARY_WRITER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/css_value.h"
#include "core/runtime/vm/lepus/binary_writer.h"
#include "core/runtime/vm/lepus/function.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "core/template_bundle/template_codec/version.h"

namespace lynx {
namespace lepus {

class Closure;
class Dictionary;
class CArray;
class CDate;
class Value;
class Function;
class Context;

class ContextBinaryWriter : public BinaryWriter {
 public:
  ContextBinaryWriter(Context* context,
                      const tasm::CompileOptions& compile_options = {},
                      const lepus::Value& trial_options = lepus::Value{},
                      bool enableDebugInfo = false);
  virtual ~ContextBinaryWriter();
  void encode();

  // protected:
  void SerializeGlobal();

  void SetFunctionIgnoreList(const std::vector<std::string>& ignored_funcs);
  void SerializeFunction(fml::RefPtr<Function> function);
  void EncodeExpr();

  void SerializeTopVariables();
  void EncodeClosure(const fml::RefPtr<Closure>& value);
  void EncodeTable(fml::RefPtr<Dictionary> dictionary, bool is_header = false);
  void EncodeArray(fml::RefPtr<CArray> ary);
  void EncodeDate(fml::RefPtr<CDate> date);
  void EncodeUtf8Str(const char* value, size_t length);
  void EncodeUtf8Str(const char* value);
  void EncodeValue(const Value* value, bool is_header = false);
  void EncodeCSSValue(const tasm::CSSValue& css_value);
  void EncodeCSSValue(const tasm::CSSValue& css_value, bool enable_css_parser,
                      bool enable_css_variable);

  inline const Context* context() const { return context_; }
  bool NeedLepusDebugInfo() { return need_lepus_debug_info_; }

 protected:
  Context* context_;
  const tasm::CompileOptions compile_options_;
  const lepus_value trial_options_;
  bool need_lepus_debug_info_;
  // for serialize/deserialize
  std::unordered_map<fml::RefPtr<Function>, int> func_map;
  std::vector<fml::RefPtr<Function>> func_vec;

  // functions inside the list will not be serialized (reduce output file size)
  std::vector<std::string> ignored_funcs_;

 private:
  // if target_sdk_version > FEATURE_CONTROL_VERSION;
  bool feature_control_variables_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_CONTEXT_BINARY_WRITER_H_
