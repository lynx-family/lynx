// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_CODEC_H_
#define CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_CODEC_H_

#include <vector>

#include "core/gesture/android/gesture_arena_android.h"

namespace lynx::tasm::gesture {

std::vector<jint> ReadIntArray(JNIEnv* env, jintArray array);
std::vector<MemberId> ReadMemberIds(JNIEnv* env, jintArray array);
// JNI records use six header integers, followed by relation IDs.
// Config doubles are min/max distance, min/max duration, then tap slop.
std::vector<GestureDefinition> DecodeDefinitions(
    const std::vector<jint>& data, const std::vector<jdouble>& configs,
    size_t count);

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_CODEC_H_
