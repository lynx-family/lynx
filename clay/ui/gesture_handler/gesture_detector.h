// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/public/value.h"

namespace clay {

enum class GestureHandlerType {
  Pan,
  Fling,
  Default,
  Tap,
  LongPress,
  Rotation,
  Pinch,
  Native,
};

class GestureDetector;

using GestureMap =
    std::unordered_map<uint32_t, std::shared_ptr<GestureDetector>>;

class GestureDetector {
 public:
  GestureDetector(uint32_t gesture_id, GestureHandlerType gesture_handler_type,
                  const std::vector<std::string>& callback_names,
                  const std::unordered_map<std::string, std::vector<uint32_t>>&
                      relation_map,
                  Value config_map = {})
      : gesture_id_(gesture_id),
        gesture_handler_type_(gesture_handler_type),
        gesture_callback_names_(callback_names),
        relation_map_(relation_map),
        config_map_(std::move(config_map)) {}

  const std::vector<std::string>& GestureCallbackNames() {
    return gesture_callback_names_;
  }

  GestureHandlerType gesture_type() const { return gesture_handler_type_; }

  uint32_t gesture_id() const { return gesture_id_; }

  const std::unordered_map<std::string, std::vector<uint32_t>> relation_map() {
    return relation_map_;
  }

  const Value& config_map() { return config_map_; }

 private:
  uint32_t gesture_id_;
  GestureHandlerType gesture_handler_type_;
  std::vector<std::string> gesture_callback_names_;
  std::unordered_map<std::string, std::vector<uint32_t>> relation_map_;
  Value config_map_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_
