// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
#define CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "base/include/vector.h"
#include "core/runtime/js/jsi/jsi.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace piper {
using LepusIdContainer = std::unordered_map<LEPUSRuntime*, LEPUSClassID>;
class QuickjsRuntimeInstance : public VMInstance, public GCObserver {
 public:
  QuickjsRuntimeInstance() = default;
  virtual ~QuickjsRuntimeInstance();

  void InitQuickjsRuntime(bool is_sync = true, uint32_t runtime_mode = 0);
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

  /**
   * @brief Called when the garbage collection (GC) operation is completed to
   * handle the post-GC memory information.
   *
   * This method is a concrete implementation of the `GCObserver` interface. It
   * will receive a string containing memory-related information after the
   * garbage collection operation ends.
   *
   * @param mem_info A string containing post-GC memory information. Its format
   * could be JSON or a specific custom format.
   */
  void OnGC(std::string mem_info) override;
  void AddObserver(JSIObserver* obs);
  void RemoveObserver(JSIObserver* obs);

  JSRuntimeType GetRuntimeType() override {
    return piper::JSRuntimeType::quickjs;
  }

  // Must exec in use thread.
  void AddToIdContainer();

 private:
  LEPUSRuntime* rt_;
  base::LinearFlatSet<JSIObserver*> obs_set_ptr_;
  static LEPUSClassID s_function_id_;
  static LEPUSClassID s_object_id_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_QUICKJS_QUICKJS_RUNTIME_WRAPPER_H_
