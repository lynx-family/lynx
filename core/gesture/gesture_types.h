// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_TYPES_H_
#define CORE_GESTURE_GESTURE_TYPES_H_

#include <cstdint>
#include <string>
#include <vector>

#include "core/public/gesture_handler.h"

namespace lynx::tasm::gesture {

using MemberId = int64_t;
using TimerToken = uint64_t;

enum class InputType {
  kDown,
  kMove,
  kUp,
  kCancel,
  kFlingFrame,
};

enum class GestureState {
  kInit,
  kBegin,
  kActive,
  kFail,
  kEnd,
  kCancel,
};

enum class GestureStateCommand {
  kActive = 1,
  kFail = 2,
  kEnd = 3,
};

enum class GestureCallbackType {
  kTouchesDown,
  kTouchesMove,
  kTouchesUp,
  kTouchesCancel,
  kBegin,
  kStart,
  kUpdate,
  kEnd,
};

enum class GestureDirection {
  kUndetermined,
  kHorizontal,
  kVertical,
};

struct Point {
  double x = 0;
  double y = 0;
};

struct InputEvent {
  // Coordinates and deltas use logical pixels. Touch velocity keeps the
  // platform finger direction and uses logical pixels per second.
  InputType type = InputType::kDown;
  uint64_t sequence_id = 0;
  int64_t pointer_id = 0;
  double monotonic_time_ms = 0;
  int64_t epoch_time_ms = 0;
  Point screen;
  Point page;
  Point client;
  Point delta;
  Point velocity;
  bool fling_finished = false;
};

struct ScrollState {
  double x = 0;
  double y = 0;
  bool is_at_start = false;
  bool is_at_end = false;
};

struct ScrollResult {
  Point consumed;
  Point unconsumed;
};

struct GestureEvent {
  MemberId member_id = 0;
  uint32_t gesture_id = 0;
  GestureType gesture_type = GestureType::PAN;
  GestureCallbackType callback = GestureCallbackType::kBegin;
  InputType source = InputType::kDown;
  uint64_t sequence_id = 0;
  int64_t pointer_id = 0;
  int64_t timestamp_epoch_ms = 0;
  Point screen;
  Point page;
  Point client;
  Point local;
  Point delta;
  ScrollState scroll;
};

struct GestureConfig {
  double min_distance = 0;
  double max_distance = 10;
  double min_duration_ms = 500;
  double max_duration_ms = 500;
  double tap_slop = 3;
};

struct GestureRelations {
  std::vector<uint32_t> simultaneous;
  std::vector<uint32_t> wait_for;
  std::vector<uint32_t> continue_with;
};

struct GestureDefinition {
  uint32_t gesture_id = 0;
  GestureType gesture_type = GestureType::PAN;
  GestureConfig config;
  GestureRelations relations;
  std::vector<GestureCallbackType> callbacks;
};

struct HandlerKey {
  MemberId member_id = 0;
  uint32_t gesture_id = 0;

  bool operator==(const HandlerKey& other) const {
    return member_id == other.member_id && gesture_id == other.gesture_id;
  }
};

const char* GestureCallbackName(GestureCallbackType callback);
const char* GestureInputTypeName(InputType type);

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_TYPES_H_
