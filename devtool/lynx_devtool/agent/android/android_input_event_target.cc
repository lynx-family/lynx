// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/android_input_event_target.h"

#include <cmath>

#include "base/include/platform/android/jni_utils.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

enum class AndroidPointerEventType : jint {
  kDown = 0,
  kUp = 2,
  kCancel = 3,
};

bool ToAndroidPointerEventType(PointerEventType event_type,
                               AndroidPointerEventType* android_type) {
  if (!android_type) {
    return false;
  }
  switch (event_type) {
    case PointerEventType::kDown:
      *android_type = AndroidPointerEventType::kDown;
      return true;
    case PointerEventType::kUp:
      *android_type = AndroidPointerEventType::kUp;
      return true;
    case PointerEventType::kCancel:
      *android_type = AndroidPointerEventType::kCancel;
      return true;
    case PointerEventType::kMove:
    case PointerEventType::kScroll:
      return false;
  }
  return false;
}

}  // namespace

AndroidInputEventTarget::AndroidInputEventTarget(
    JNIEnv* env, jobject delegate, JavaPointerEventInjector injector)
    : weak_java_delegate_(env, delegate), injector_(injector) {}

PointerCapabilities AndroidInputEventTarget::GetPointerCapabilities() const {
  PointerCapabilities capabilities;
  capabilities.default_source_type = PointerSourceType::kTouch;
  capabilities.supports_touch = true;
  return capabilities;
}

bool AndroidInputEventTarget::InjectPointerEvent(const PointerEvent& event) {
  if (!injector_ || event.source_type != PointerSourceType::kTouch ||
      event.pointers.size() != 1) {
    return false;
  }

  const Pointer* pointer = event.FindPointer(event.action_pointer_id);
  AndroidPointerEventType event_type;
  if (!pointer || !std::isfinite(pointer->x) || !std::isfinite(pointer->y) ||
      !std::isfinite(event.delta_x) || !std::isfinite(event.delta_y) ||
      !ToAndroidPointerEventType(event.type, &event_type)) {
    return false;
  }

  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> delegate(
      weak_java_delegate_);
  if (delegate.IsNull()) {
    return false;
  }

  return injector_(env, delegate.Get(), static_cast<jint>(event_type),
                   pointer->x, pointer->y, event.delta_x, event.delta_y,
                   pointer->id, event.modifiers, event.timestamp_us);
}

}  // namespace input
}  // namespace devtool
}  // namespace lynx
