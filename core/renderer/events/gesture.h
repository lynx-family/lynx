// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_GESTURE_H_
#define CORE_RENDERER_EVENTS_GESTURE_H_
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

struct GestureCallback {
  // The name of the callback.
  base::String name_;

  // The worklet module associated with the callback.
  lepus::Value lepus_script_;

  // The worklet function associated with the callback.
  lepus::Value lepus_function_;

  // The Worklet object associated with the callback in fiber
  lepus::Value lepus_object_;

  // The Context of lepus / lepusNG
  lepus::Context* ctx_ = nullptr;

  // Default constructor for GestureCallback.
  GestureCallback() = default;

  // Parameterized constructor for GestureCallback.
  GestureCallback(const base::String& name, const lepus::Value& lepus_script,
                  const lepus::Value& lepus_function)
      : name_(name),
        lepus_script_(lepus_script),
        lepus_function_(lepus_function) {}

  // constructor with lepus object
  GestureCallback(const base::String& name, const lepus::Value& lepus_object,
                  lepus::Context* ctx)
      : name_(name), lepus_object_(lepus_object), ctx_(ctx) {}
};

// Enum class for representing different types of gestures.
enum class GestureType : unsigned int {
  PAN = 0,         // Pan gesture
  FLING = 1,       // Fling gesture
  DEFAULT = 2,     // Default gesture
  TAP = 3,         // Tap gesture
  LONG_PRESS = 4,  // Long press gesture
  ROTATION = 5,    // Rotation gesture
  PINCH = 6,       // Pinch gesture
  NATIVE = 7       // Native gesture
};

// Constants representing relation map keys for GestureDetector.
static constexpr const char kGestureSimultaneous[] = "simultaneous";
static constexpr const char kGestureWaitFor[] = "waitFor";
static constexpr const char kGestureContinueWith[] = "continueWith";

// Class representing a GestureDetector.
class GestureDetector {
 public:
  // Constructor for GestureDetector.
  GestureDetector(
      const uint32_t gesture_id, const GestureType gesture_type,
      const std::vector<GestureCallback> gesture_callback_vec,
      const std::unordered_map<std::string, std::vector<uint32_t>> relation_map)
      : gesture_id_(gesture_id),
        gesture_type_(gesture_type),
        gesture_callback_vec_(gesture_callback_vec),
        relation_map_(relation_map){};

  // Constructor for GestureDetector.
  GestureDetector(
      const uint32_t gesture_id, const GestureType gesture_type,
      const std::vector<GestureCallback> gesture_callback_vec,
      const std::unordered_map<std::string, std::vector<uint32_t>> relation_map,
      const lepus::Value& gesture_config)
      : gesture_id_(gesture_id),
        gesture_type_(gesture_type),
        gesture_callback_vec_(gesture_callback_vec),
        relation_map_(relation_map),
        gesture_config_(gesture_config){};

  // Getter method for retrieving the gesture_id of GestureDetector.
  uint32_t gesture_id() const { return gesture_id_; }

  // Getter method for retrieving the gesture_type of GestureDetector.
  GestureType gesture_type() const { return gesture_type_; }

  const lepus::Value& gesture_config() const { return gesture_config_; }

  // Getter method for retrieving the vector of gesture callbacks in
  // GestureDetector.
  const std::vector<GestureCallback>& gesture_callbacks() const {
    return gesture_callback_vec_;
  }

  // Getter method for retrieving the relation map of GestureDetector.
  const std::unordered_map<std::string, std::vector<uint32_t>>& relation_map()
      const {
    return relation_map_;
  }

 private:
  // Private member variables of GestureDetector.
  uint32_t gesture_id_ = 0;
  GestureType gesture_type_ = GestureType::PAN;
  std::vector<GestureCallback> gesture_callback_vec_;
  std::unordered_map<std::string, std::vector<uint32_t>> relation_map_;
  lepus::Value gesture_config_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_EVENTS_GESTURE_H_
