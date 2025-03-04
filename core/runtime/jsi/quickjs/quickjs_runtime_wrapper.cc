// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jsi/quickjs/quickjs_runtime_wrapper.h"

#include <mutex>

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/cutils.h"
#ifdef __cplusplus
}
#endif

#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jsi/quickjs/quickjs_host_function.h"
#include "core/runtime/jsi/quickjs/quickjs_host_object.h"

namespace lynx {
namespace piper {

using detail::QuickjsHostFunctionProxy;
using detail::QuickjsHostObjectProxy;

LEPUSClassID QuickjsRuntimeInstance::s_function_id_ = 0;
LEPUSClassID QuickjsRuntimeInstance::s_object_id_ = 0;

QuickjsRuntimeInstance::~QuickjsRuntimeInstance() {
  LOGE("LYNX free quickjs runtime start");
  if (rt_) {
    LEPUS_FreeRuntime(rt_);
  }
  GetFunctionIdContainer().erase(rt_);
  GetObjectIdContainer().erase(rt_);

  LOGI("LYNX free quickjs runtime end. " << this << " LEPUSRuntime: " << rt_);
}

LepusIdContainer& QuickjsRuntimeInstance::GetObjectIdContainer() {
  static thread_local LepusIdContainer sObjectIdContainer;
  return sObjectIdContainer;
}

LepusIdContainer& QuickjsRuntimeInstance::GetFunctionIdContainer() {
  static thread_local LepusIdContainer sFunctionIdContainer;
  return sFunctionIdContainer;
}

void QuickjsRuntimeInstance::InitQuickjsRuntime(bool is_sync) {
  LEPUSRuntime* rt;
  rt = LEPUS_NewRuntimeWithMode(0);
  if (!rt) {
    LOGE("init quickjs runtime failed!");
    return;
  }
  if (tasm::LynxEnv::GetInstance().IsDisableTracingGC()) {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS_RC");
  } else {
    LEPUS_SetRuntimeInfo(rt, "Lynx_JS");
  }
  if (trig_mem_info_event_) {
    RegisterGCInfoCallback(rt, trig_mem_info_event_);
  }
  rt_ = rt;

  static std::once_flag s_init_id_flag;
  static LEPUSClassDef s_function_class_def;
  static LEPUSClassExoticMethods s_exotic_method;
  static LEPUSClassDef s_object_class_def;
  std::call_once(s_init_id_flag, [] {
    LEPUS_NewClassID(&s_function_id_);
    LEPUS_NewClassID(&s_object_id_);
    // init function class def
    s_function_class_def.class_name = "LynxFunctionDef";
    s_function_class_def.finalizer = QuickjsHostFunctionProxy::hostFinalizer;
    s_function_class_def.call = QuickjsHostFunctionProxy::FunctionCallback;

    // init exotic method
    s_exotic_method.get_own_property = QuickjsHostObjectProxy::getOwnProperty;
    s_exotic_method.get_own_property_names =
        QuickjsHostObjectProxy::getPropertyNames;
    s_exotic_method.get_property = QuickjsHostObjectProxy::getProperty;
    s_exotic_method.set_property = QuickjsHostObjectProxy::setProperty;

    // init object class def
    s_object_class_def.class_name = "LynxObjectClassDef";
    s_object_class_def.finalizer = QuickjsHostObjectProxy::hostFinalizer;
    s_object_class_def.exotic = &s_exotic_method;
  });

  LEPUS_NewClass(rt_, s_function_id_, &s_function_class_def);
  LEPUS_NewClass(rt_, s_object_id_, &s_object_class_def);

  if (is_sync) {
    AddToIdContainer();
  }
#if LYNX_ENABLE_FROZEN_MODE
  // Due to the fact that QuickJS’s GC traverses all objects in a Stop The World
  // fashion to try to free the circular objects, it causes extra time
  // consumption once QuickJS triggers GC. Currently, the default GC threshold
  // of QuickJS is 256 bytes, and even a slight change in the JS Framework may
  // cause changes in the GC timing. The impact is that in performance
  // degradation tests, some indicators may experience significant fluctuations
  // due to changes in the GC timing. To avoid the GC from fluctuating the
  // performance, when LYNX_ENABLE_FROZEN_MODE is enabled, set the GC of the JS
  // QuickJSRuntime to INT_MAX. In the long run, a reasonable GC threshold needs
  // to be set for QuickJS.
  LEPUS_SetGCThreshold(rt_, INT_MAX);
#endif
  LOGI("lynx InitQuickjsRuntime success");
}

void QuickjsRuntimeInstance::AddToIdContainer() {
  GetFunctionIdContainer().insert({rt_, s_function_id_});
  GetObjectIdContainer().insert({rt_, s_object_id_});
}

}  // namespace piper
}  // namespace lynx
