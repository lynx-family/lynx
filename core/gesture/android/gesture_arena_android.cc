// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/android/gesture_arena_android.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/platform/android/jni_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "platform/android/lynx_android/src/main/jni/gen/GestureArenaManager_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/GestureArenaManager_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForGestureArenaManager(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

namespace lynx::tasm::gesture {
namespace {

constexpr size_t kEncodedHeaderSize = 6;
constexpr size_t kConfigSize = 5;

using GestureArenaAndroidPtr = std::shared_ptr<GestureArenaAndroid>;

GestureArenaAndroidPtr GetGestureArena(jlong native_ptr) {
  if (native_ptr == 0) {
    return {};
  }
  return *reinterpret_cast<GestureArenaAndroidPtr*>(native_ptr);
}

std::vector<jint> ReadIntArray(JNIEnv* env, jintArray array) {
  if (array == nullptr) {
    return {};
  }
  std::vector<jint> result(env->GetArrayLength(array));
  if (!result.empty()) {
    env->GetIntArrayRegion(array, 0, static_cast<jsize>(result.size()),
                           result.data());
  }
  return result;
}

std::vector<jdouble> ReadDoubleArray(JNIEnv* env, jdoubleArray array) {
  if (array == nullptr) {
    return {};
  }
  std::vector<jdouble> result(env->GetArrayLength(array));
  if (!result.empty()) {
    env->GetDoubleArrayRegion(array, 0, static_cast<jsize>(result.size()),
                              result.data());
  }
  return result;
}

std::vector<MemberId> ReadMemberIds(JNIEnv* env, jintArray array) {
  auto values = ReadIntArray(env, array);
  return std::vector<MemberId>(values.begin(), values.end());
}

std::vector<uint32_t> ReadRelations(const std::vector<jint>& data,
                                    size_t& offset, size_t count) {
  const size_t available = data.size() - std::min(offset, data.size());
  count = std::min(count, available);
  std::vector<uint32_t> relations;
  relations.reserve(count);
  for (size_t index = 0; index < count; ++index) {
    relations.push_back(static_cast<uint32_t>(data[offset++]));
  }
  return relations;
}

std::vector<GestureDefinition> DecodeDefinitions(
    const std::vector<jint>& data, const std::vector<jdouble>& configs,
    size_t count) {
  std::vector<GestureDefinition> definitions;
  definitions.reserve(count);
  size_t offset = 0;
  for (size_t index = 0; index < count; ++index) {
    if (data.size() - std::min(offset, data.size()) < kEncodedHeaderSize ||
        configs.size() < (index + 1) * kConfigSize) {
      break;
    }
    GestureDefinition definition;
    definition.gesture_id = static_cast<uint32_t>(data[offset++]);
    definition.gesture_type = static_cast<GestureType>(data[offset++]);
    const uint32_t callback_mask = static_cast<uint32_t>(data[offset++]);
    const size_t simultaneous_count = static_cast<size_t>(data[offset++]);
    const size_t wait_for_count = static_cast<size_t>(data[offset++]);
    const size_t continue_with_count = static_cast<size_t>(data[offset++]);
    definition.relations.simultaneous =
        ReadRelations(data, offset, simultaneous_count);
    definition.relations.wait_for = ReadRelations(data, offset, wait_for_count);
    definition.relations.continue_with =
        ReadRelations(data, offset, continue_with_count);
    for (uint32_t callback = 0; callback <= 7; ++callback) {
      if ((callback_mask & (1U << callback)) != 0) {
        definition.callbacks.push_back(
            static_cast<GestureCallbackType>(callback));
      }
    }
    const size_t config_offset = index * kConfigSize;
    definition.config.min_distance = configs[config_offset];
    definition.config.max_distance = configs[config_offset + 1];
    definition.config.min_duration_ms = configs[config_offset + 2];
    definition.config.max_duration_ms = configs[config_offset + 3];
    definition.config.tap_slop = configs[config_offset + 4];
    definitions.push_back(std::move(definition));
  }
  return definitions;
}

std::vector<double> ReadJavaDoubleArray(
    JNIEnv* env, const base::android::ScopedLocalJavaRef<jdoubleArray>& array) {
  if (array.IsNull()) {
    return {};
  }
  std::vector<double> result(env->GetArrayLength(array.Get()));
  if (!result.empty()) {
    env->GetDoubleArrayRegion(array.Get(), 0, static_cast<jsize>(result.size()),
                              result.data());
  }
  return result;
}

}  // namespace

GestureArenaAndroid::GestureArenaAndroid(JNIEnv* env, jobject owner)
    : owner_(env, owner), arena_(*this) {}

void GestureArenaAndroid::ReplaceMemberGestures(
    MemberId member_id, std::vector<GestureDefinition> definitions) {
  arena_.ReplaceMemberGestures(member_id, std::move(definitions));
}

void GestureArenaAndroid::RemoveMember(MemberId member_id) {
  arena_.RemoveMember(member_id);
}

void GestureArenaAndroid::HandleInput(
    const InputEvent& event, const std::vector<MemberId>& response_chain) {
  arena_.HandleInput(event, response_chain);
}

void GestureArenaAndroid::HandleTimer(TimerToken token,
                                      double monotonic_time_ms,
                                      int64_t epoch_time_ms) {
  arena_.HandleTimer(token, monotonic_time_ms, epoch_time_ms);
}

void GestureArenaAndroid::SetGestureState(MemberId member_id,
                                          uint32_t gesture_id,
                                          GestureStateCommand command) {
  if (arena_.ContainsGesture(member_id, gesture_id)) {
    arena_.SetGestureState(member_id, gesture_id, command);
  }
}

void GestureArenaAndroid::ResetAndDisconnect(JNIEnv* env) {
  arena_.Reset();
  owner_.Reset(env, nullptr);
}

bool GestureArenaAndroid::IsMemberValid(MemberId member_id) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  return !owner.IsNull() &&
         Java_GestureArenaManager_isMemberValid(env, owner.Get(), member_id);
}

Point GestureArenaAndroid::ConvertPageToMember(MemberId member_id,
                                               const Point& page_point) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (owner.IsNull()) {
    return page_point;
  }
  const auto values = ReadJavaDoubleArray(
      env, Java_GestureArenaManager_convertPageToMember(
               env, owner.Get(), member_id, page_point.x, page_point.y));
  return values.size() >= 2 ? Point{values[0], values[1]} : page_point;
}

ScrollState GestureArenaAndroid::GetScrollState(MemberId member_id) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (owner.IsNull()) {
    return {};
  }
  const auto values = ReadJavaDoubleArray(
      env,
      Java_GestureArenaManager_getScrollState(env, owner.Get(), member_id));
  if (values.size() < 4) {
    return {};
  }
  return {values[0], values[1], values[2] != 0, values[3] != 0};
}

GestureDirection GestureArenaAndroid::GetScrollDirection(
    MemberId member_id) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (owner.IsNull()) {
    return GestureDirection::kUndetermined;
  }
  const int direction =
      Java_GestureArenaManager_getScrollDirection(env, owner.Get(), member_id);
  if (direction < 0) {
    return GestureDirection::kHorizontal;
  }
  if (direction > 0) {
    return GestureDirection::kVertical;
  }
  return GestureDirection::kUndetermined;
}

bool GestureArenaAndroid::CanConsumeGesture(MemberId member_id,
                                            GestureDirection direction,
                                            const Point& delta) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  return !owner.IsNull() && Java_GestureArenaManager_canConsumeGesture(
                                env, owner.Get(), member_id,
                                static_cast<jint>(direction), delta.x, delta.y);
}

bool GestureArenaAndroid::ShouldConsumeGesture(MemberId member_id) const {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  return !owner.IsNull() && Java_GestureArenaManager_shouldConsumeGesture(
                                env, owner.Get(), member_id);
}

ScrollResult GestureArenaAndroid::ScrollBy(MemberId member_id,
                                           const Point& delta) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (owner.IsNull()) {
    return {{}, delta};
  }
  const auto values = ReadJavaDoubleArray(
      env, Java_GestureArenaManager_scrollBy(env, owner.Get(), member_id,
                                             delta.x, delta.y));
  if (values.size() < 4) {
    return {{}, delta};
  }
  return {{values[0], values[1]}, {values[2], values[3]}};
}

void GestureArenaAndroid::OnGestureRecognized(MemberId member_id) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (!owner.IsNull()) {
    Java_GestureArenaManager_onGestureRecognized(env, owner.Get(), member_id);
  }
}

void GestureArenaAndroid::OnGestureStateChanged(MemberId member_id,
                                                uint32_t gesture_id,
                                                GestureState state) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (!owner.IsNull()) {
    Java_GestureArenaManager_onGestureStateChanged(
        env, owner.Get(), member_id, gesture_id, static_cast<jint>(state));
  }
}

void GestureArenaAndroid::DispatchGestureEvent(const GestureEvent& event) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (owner.IsNull()) {
    return;
  }
  Java_GestureArenaManager_dispatchGestureEvent(
      env, owner.Get(), event.member_id, event.gesture_id,
      static_cast<jint>(event.callback), static_cast<jint>(event.source),
      event.timestamp_epoch_ms, event.local.x, event.local.y, event.page.x,
      event.page.y, event.client.x, event.client.y, event.delta.x,
      event.delta.y, event.scroll.x, event.scroll.y, event.scroll.is_at_start,
      event.scroll.is_at_end);
}

void GestureArenaAndroid::ScheduleTimer(TimerToken token, double delay_ms) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (!owner.IsNull()) {
    Java_GestureArenaManager_scheduleTimer(env, owner.Get(), token, delay_ms);
  }
}

void GestureArenaAndroid::CancelTimer(TimerToken token) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (!owner.IsNull()) {
    Java_GestureArenaManager_cancelTimer(env, owner.Get(), token);
  }
}

bool GestureArenaAndroid::StartFling(const Point& velocity) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  return !owner.IsNull() && Java_GestureArenaManager_startFling(
                                env, owner.Get(), velocity.x, velocity.y);
}

void GestureArenaAndroid::StopFling() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> owner(owner_);
  if (!owner.IsNull()) {
    Java_GestureArenaManager_stopFling(env, owner.Get());
  }
}

}  // namespace lynx::tasm::gesture

jlong CreateGestureArena(JNIEnv* env, jobject jcaller) {
  if (!lynx::tasm::LynxEnv::GetInstance().EnableUnifiedGestureHandler()) {
    return 0;
  }
  auto arena =
      std::make_shared<lynx::tasm::gesture::GestureArenaAndroid>(env, jcaller);
  return reinterpret_cast<jlong>(
      new lynx::tasm::gesture::GestureArenaAndroidPtr(std::move(arena)));
}

void DestroyGestureArena(JNIEnv* env, jobject jcaller, jlong native_ptr) {
  if (native_ptr == 0) {
    return;
  }
  auto* handle = reinterpret_cast<lynx::tasm::gesture::GestureArenaAndroidPtr*>(
      native_ptr);
  auto arena = *handle;
  if (arena) {
    arena->ResetAndDisconnect(env);
  }
  delete handle;
}

void ReplaceMemberGestures(JNIEnv* env, jobject jcaller, jlong native_ptr,
                           jint member_id, jint gesture_count,
                           jintArray gesture_data, jdoubleArray configs) {
  if (native_ptr == 0 || gesture_count < 0) {
    return;
  }
  auto arena = lynx::tasm::gesture::GetGestureArena(native_ptr);
  if (!arena) {
    return;
  }
  arena->ReplaceMemberGestures(
      member_id, lynx::tasm::gesture::DecodeDefinitions(
                     lynx::tasm::gesture::ReadIntArray(env, gesture_data),
                     lynx::tasm::gesture::ReadDoubleArray(env, configs),
                     static_cast<size_t>(gesture_count)));
}

void RemoveMember(JNIEnv* env, jobject jcaller, jlong native_ptr,
                  jint member_id) {
  if (native_ptr != 0) {
    auto arena = lynx::tasm::gesture::GetGestureArena(native_ptr);
    if (arena) {
      arena->RemoveMember(member_id);
    }
  }
}

void HandleInput(JNIEnv* env, jobject jcaller, jlong native_ptr, jint type,
                 jlong sequence_id, jint pointer_id, jdouble monotonic_time,
                 jlong epoch_time, jdouble screen_x, jdouble screen_y,
                 jdouble page_x, jdouble page_y, jdouble client_x,
                 jdouble client_y, jdouble delta_x, jdouble delta_y,
                 jdouble velocity_x, jdouble velocity_y,
                 jboolean fling_finished, jintArray response_chain) {
  if (native_ptr == 0 || type < 0 || type > 4) {
    return;
  }
  lynx::tasm::gesture::InputEvent event;
  event.type = static_cast<lynx::tasm::gesture::InputType>(type);
  event.sequence_id = sequence_id;
  event.pointer_id = pointer_id;
  event.monotonic_time_ms = monotonic_time;
  event.epoch_time_ms = epoch_time;
  event.screen = {screen_x, screen_y};
  event.page = {page_x, page_y};
  event.client = {client_x, client_y};
  event.delta = {delta_x, delta_y};
  event.velocity = {velocity_x, velocity_y};
  event.fling_finished = fling_finished;
  auto arena = lynx::tasm::gesture::GetGestureArena(native_ptr);
  if (arena) {
    arena->HandleInput(event,
                       lynx::tasm::gesture::ReadMemberIds(env, response_chain));
  }
}

void HandleTimer(JNIEnv* env, jobject jcaller, jlong native_ptr, jlong token,
                 jdouble monotonic_time, jlong epoch_time) {
  if (native_ptr != 0) {
    auto arena = lynx::tasm::gesture::GetGestureArena(native_ptr);
    if (arena) {
      arena->HandleTimer(token, monotonic_time, epoch_time);
    }
  }
}

void SetGestureState(JNIEnv* env, jobject jcaller, jlong native_ptr,
                     jint member_id, jint gesture_id, jint state) {
  if (native_ptr == 0 || state < 1 || state > 3) {
    return;
  }
  auto arena = lynx::tasm::gesture::GetGestureArena(native_ptr);
  if (arena) {
    arena->SetGestureState(
        member_id, static_cast<uint32_t>(gesture_id),
        static_cast<lynx::tasm::gesture::GestureStateCommand>(state));
  }
}
