// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/android/android_input_event_target.h"

#include <cmath>
#include <vector>

#include "base/include/platform/android/jni_utils.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

enum class AndroidPointerEventType : jint {
  kDown = 0,
  kMove = 1,
  kUp = 2,
  kCancel = 3,
};

constexpr size_t kMaxTouchPoints = 2;

bool ToAndroidPointerEventType(PointerEventType event_type,
                               AndroidPointerEventType* android_type) {
  if (!android_type) {
    return false;
  }
  switch (event_type) {
    case PointerEventType::kDown:
      *android_type = AndroidPointerEventType::kDown;
      return true;
    case PointerEventType::kMove:
      *android_type = AndroidPointerEventType::kMove;
      return true;
    case PointerEventType::kUp:
      *android_type = AndroidPointerEventType::kUp;
      return true;
    case PointerEventType::kCancel:
      *android_type = AndroidPointerEventType::kCancel;
      return true;
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
  capabilities.max_touch_points = kMaxTouchPoints;
  return capabilities;
}

bool AndroidInputEventTarget::InjectPointerEvent(const PointerEvent& event) {
  if (!injector_ || event.source_type != PointerSourceType::kTouch ||
      event.pointers.empty() || event.pointers.size() > kMaxTouchPoints) {
    return false;
  }

  AndroidPointerEventType event_type;
  if (!event.FindPointer(event.action_pointer_id) ||
      !std::isfinite(event.delta_x) || !std::isfinite(event.delta_y) ||
      !ToAndroidPointerEventType(event.type, &event_type)) {
    return false;
  }

  std::vector<jint> pointer_ids;
  std::vector<jfloat> xs;
  std::vector<jfloat> ys;
  pointer_ids.reserve(event.pointers.size());
  xs.reserve(event.pointers.size());
  ys.reserve(event.pointers.size());
  for (const auto& pointer : event.pointers) {
    if (!std::isfinite(pointer.x) || !std::isfinite(pointer.y)) {
      return false;
    }
    pointer_ids.push_back(pointer.id);
    xs.push_back(pointer.x);
    ys.push_back(pointer.y);
  }

  JNIEnv* env = lynx::base::android::AttachCurrentThread();
  lynx::base::android::ScopedLocalJavaRef<jobject> delegate(
      weak_java_delegate_);
  if (delegate.IsNull()) {
    return false;
  }

  const jsize pointer_count = static_cast<jsize>(event.pointers.size());
  lynx::base::android::ScopedLocalJavaRef<jintArray> j_pointer_ids(
      env, env->NewIntArray(pointer_count));
  lynx::base::android::ScopedLocalJavaRef<jfloatArray> j_xs(
      env, env->NewFloatArray(pointer_count));
  lynx::base::android::ScopedLocalJavaRef<jfloatArray> j_ys(
      env, env->NewFloatArray(pointer_count));
  if (j_pointer_ids.IsNull() || j_xs.IsNull() || j_ys.IsNull()) {
    return false;
  }
  env->SetIntArrayRegion(j_pointer_ids.Get(), 0, pointer_count,
                         pointer_ids.data());
  env->SetFloatArrayRegion(j_xs.Get(), 0, pointer_count, xs.data());
  env->SetFloatArrayRegion(j_ys.Get(), 0, pointer_count, ys.data());

  return injector_(env, delegate.Get(), static_cast<jint>(event_type),
                   j_pointer_ids.Get(), j_xs.Get(), j_ys.Get(),
                   event.action_pointer_id, event.delta_x, event.delta_y,
                   event.modifiers, event.timestamp_us);
}

}  // namespace input
}  // namespace devtool
}  // namespace lynx
