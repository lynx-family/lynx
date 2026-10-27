// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/android/gesture_arena_android_codec.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace lynx::tasm::gesture {
namespace {

constexpr size_t kEncodedHeaderSize = 6;
constexpr size_t kConfigSize = 5;

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

}  // namespace

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

std::vector<MemberId> ReadMemberIds(JNIEnv* env, jintArray array) {
  auto values = ReadIntArray(env, array);
  return std::vector<MemberId>(values.begin(), values.end());
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

}  // namespace lynx::tasm::gesture
