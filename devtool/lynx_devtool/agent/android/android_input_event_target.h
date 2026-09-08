// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_ANDROID_INPUT_EVENT_TARGET_H_
#define LYNX_DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_ANDROID_INPUT_EVENT_TARGET_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"
#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {
namespace input {

class AndroidInputEventTarget final : public InputEventTarget {
 public:
  using JavaPointerEventInjector = bool (*)(JNIEnv*, jobject, jint, jfloat,
                                            jfloat, jfloat, jfloat, jint, jint,
                                            jlong);

  AndroidInputEventTarget(JNIEnv* env, jobject delegate,
                          JavaPointerEventInjector injector);

  PointerCapabilities GetPointerCapabilities() const override;
  bool InjectPointerEvent(const PointerEvent& event) override;

 private:
  lynx::base::android::ScopedWeakGlobalJavaRef<jobject> weak_java_delegate_;
  JavaPointerEventInjector injector_;
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // LYNX_DEVTOOL_LYNX_DEVTOOL_AGENT_ANDROID_ANDROID_INPUT_EVENT_TARGET_H_
