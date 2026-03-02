// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_GESTURE_HANDLER_H_
#define CORE_PUBLIC_GESTURE_HANDLER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "core/public/pub_value.h"

namespace lynx {
namespace tasm {

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

// TODO(Xionghui He, Chenfeng Pan): Plan to make C++ class(es) related gesture
// handler unified in all backend.
class GestureDetector {
 public:
  virtual ~GestureDetector() = default;

  // Getter method for retrieving the gesture_id of GestureDetector.
  virtual uint32_t gesture_id() const = 0;

  // Getter method for retrieving the gesture_type of GestureDetector.
  virtual GestureType gesture_type() const = 0;

  virtual const pub::Value& gesture_config() const = 0;

  // Getter method for retrieving the vector of gesture callbacks in
  // GestureDetector.
  virtual std::vector<std::string> gesture_callback_names() const = 0;

  // Getter method for retrieving the relation map of GestureDetector.
  virtual const std::unordered_map<std::string, std::vector<uint32_t>>&
  relation_map() const = 0;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_PUBLIC_GESTURE_HANDLER_H_
