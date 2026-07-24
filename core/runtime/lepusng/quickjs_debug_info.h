// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUSNG_QUICKJS_DEBUG_INFO_H_
#define CORE_RUNTIME_LEPUSNG_QUICKJS_DEBUG_INFO_H_

#include <string>
#include <utility>

#include "quickjs/include/quickjs.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson.h"

namespace lynx {
namespace tasm {
struct LepusDebugInfo;
}  // namespace tasm

namespace lepus {

// Abbreviated JSON keys for vardef debug-info (to reduce serialized size)
constexpr char kKeyVarDefs[] = "vd";           // vardefs
constexpr char kKeyVarDefName[] = "n";         // var def name
constexpr char kKeyVarDefScopeLevel[] = "sl";  // var def scope_level
constexpr char kKeyVarDefScopeNext[] = "sn";   // var def scope_next
constexpr char kKeyVarDefFlags[] = "f";        // var def flags

class QuickContext;

class QuickjsDebugInfoBuilder {
 public:
  void AddDebugInfo(const std::string &filename,
                    const tasm::LepusDebugInfo &debug_info, QuickContext *ctx);

  rapidjson::Value TakeDebugInfo() {
    rapidjson::Value value{rapidjson::kObjectType};
    std::swap(value, template_debug_data_);
    return value;
  }

  static std::string BuildJsDebugInfo(LEPUSContext *, LEPUSValue,
                                      const std::string &,
                                      bool debuginfo_outside,
                                      bool var_defs_outside = false,
                                      bool compact_debug_info = false);

  static rapidjson::Value BuildJsDebugInfo(LEPUSContext *ctx, LEPUSValue,
                                           const std::string &,
                                           rapidjson::Document::AllocatorType &,
                                           bool debuginfo_outside,
                                           bool var_defs_outside = false,
                                           bool compact_debug_info = false);

 private:
  static rapidjson::Value BuildFunctionInfo(
      LEPUSContext *, LEPUSFunctionBytecode *, bool is_top_level,
      bool var_defs_outside, bool compact_debug_info,
      rapidjson::Document::AllocatorType &);
  static rapidjson::Value GetFunctionLineAndColInfo(
      LEPUSContext *, const LEPUSFunctionBytecode *,
      rapidjson::Document::AllocatorType &, bool compact_debug_info);

  rapidjson::Document document_;
  rapidjson::Value template_debug_data_{rapidjson::kObjectType};
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUSNG_QUICKJS_DEBUG_INFO_H_
