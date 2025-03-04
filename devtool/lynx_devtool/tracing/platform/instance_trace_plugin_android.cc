// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include "devtool/lynx_devtool/tracing/platform/instance_trace_plugin_android.h"

#include "core/base/android/jni_helper.h"
#include "devtool/lynx_devtool/build/gen/InstanceTrace_jni.h"

static jlong CreateInstanceTrace(JNIEnv* env, jobject jcaller) {
  auto* trace_plugin =
      new lynx::trace::InstanceTracePluginAndroid(env, jcaller);
  return reinterpret_cast<jlong>(trace_plugin);
}

namespace lynx {
namespace trace {

std::unique_ptr<InstanceCounterTrace::Impl>
    InstanceTracePluginAndroid::empty_counter_trace_ =
        std::make_unique<InstanceCounterTrace::Impl>();

bool InstanceTracePluginAndroid::RegisterJNIUtils(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

InstanceTracePluginAndroid::InstanceTracePluginAndroid(JNIEnv* env,
                                                       jobject owner)
    : weak_owner_(env, owner),
      counter_trace_impl_(std::make_unique<InstanceCounterTraceImpl>()) {}

InstanceTracePluginAndroid::~InstanceTracePluginAndroid() {
  counter_trace_impl_.reset(nullptr);
}

void InstanceTracePluginAndroid::DispatchBegin() {
  InstanceCounterTrace::SetImpl(counter_trace_impl_.get());
}

void InstanceTracePluginAndroid::DispatchEnd() {
  InstanceCounterTrace::SetImpl(empty_counter_trace_.get());
}

std::string InstanceTracePluginAndroid::Name() { return "instance"; }

}  // namespace trace
}  // namespace lynx
#endif
