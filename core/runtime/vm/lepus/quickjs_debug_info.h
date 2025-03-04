// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_QUICKJS_DEBUG_INFO_H_
#define CORE_RUNTIME_VM_LEPUS_QUICKJS_DEBUG_INFO_H_

#include <string>
#include <utility>

extern "C" {
#include "quickjs/include/quickjs.h"
}
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson.h"

namespace lynx {
namespace tasm {
struct LepusDebugInfo;
}

namespace lepus {
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
                                      const std::string &, bool);

  static rapidjson::Value BuildJsDebugInfo(LEPUSContext *ctx, LEPUSValue,
                                           const std::string &,
                                           rapidjson::Document::AllocatorType &,
                                           bool);

 private:
  static rapidjson::Value BuildFunctionInfo(
      LEPUSContext *, LEPUSFunctionBytecode *, bool,
      rapidjson::Document::AllocatorType &);
  static rapidjson::Value GetFunctionLineAndColInfo(
      LEPUSContext *, const LEPUSFunctionBytecode *,
      rapidjson::Document::AllocatorType &);

  rapidjson::Document document_;
  rapidjson::Value template_debug_data_{rapidjson::kObjectType};
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_QUICKJS_DEBUG_INFO_H_
