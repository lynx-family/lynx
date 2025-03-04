// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
#define CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
using LepusIdContainer = std::unordered_map<LEPUSRuntime*, LEPUSClassID>;
class QuickjsRuntimeInstance : public VMInstance {
 public:
  QuickjsRuntimeInstance() = default;
  virtual ~QuickjsRuntimeInstance();

  void InitQuickjsRuntime(bool is_sync = true);
  inline LEPUSRuntime* Runtime() { return rt_; }
  LEPUSClassID getFunctionId() { return s_function_id_; }
  LEPUSClassID getObjectId() { return s_object_id_; }

  static LEPUSClassID getFunctionId(LEPUSContext* ctx) {
    LEPUSRuntime* rt = LEPUS_GetRuntime(ctx);
    return getFunctionId(rt);
  }

  static LEPUSClassID getFunctionId(LEPUSRuntime* rt) {
    auto it = GetFunctionIdContainer().find(rt);
    DCHECK(it != GetFunctionIdContainer().end());
    if (it != GetFunctionIdContainer().end()) {
      return it->second;
    }
    return 0;
  }

  static LEPUSClassID getObjectId(LEPUSContext* ctx) {
    LEPUSRuntime* rt = LEPUS_GetRuntime(ctx);
    return getObjectId(rt);
  }

  static LEPUSClassID getObjectId(LEPUSRuntime* rt) {
    auto it = GetObjectIdContainer().find(rt);
    if (it != GetObjectIdContainer().end()) {
      return it->second;
    }
    return 0;
  }

  static LepusIdContainer& GetObjectIdContainer();
  static LepusIdContainer& GetFunctionIdContainer();

  JSRuntimeType GetRuntimeType() { return piper::JSRuntimeType::quickjs; }

  // Must exec in use thread.
  void AddToIdContainer();

 private:
  LEPUSRuntime* rt_;
  static LEPUSClassID s_function_id_;
  static LEPUSClassID s_object_id_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
