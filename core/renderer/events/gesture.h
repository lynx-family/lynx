// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_EVENTS_GESTURE_H_
#define CORE_RENDERER_EVENTS_GESTURE_H_
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/public/gesture_handler.h"
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
  runtime::MTSRuntime* ctx_ = nullptr;

  // Default constructor for GestureCallback.
  GestureCallback() = default;

  // Parameterized constructor for GestureCallback.
  GestureCallback(const base::String& name, const lepus::Value& lepus_script,
                  const lepus::Value& lepus_function,
                  runtime::MTSRuntime* ctx = nullptr)
      : name_(name),
        lepus_script_(lepus_script),
        lepus_function_(lepus_function),
        ctx_(ctx) {}

  // constructor with lepus object
  GestureCallback(const base::String& name, const lepus::Value& lepus_object,
                  runtime::MTSRuntime* ctx)
      : name_(name), lepus_object_(lepus_object), ctx_(ctx) {}
};

enum class LynxInterceptGestureStatus : unsigned int {
  LynxInterceptGestureStateUnset = 0,
  LynxInterceptGestureStateFalse = 1,
  LynxInterceptGestureStateTrue = 2
};

// Constants representing relation map keys for GestureDetector.
static constexpr const char kGestureSimultaneous[] = "simultaneous";
static constexpr const char kGestureWaitFor[] = "waitFor";
static constexpr const char kGestureContinueWith[] = "continueWith";

class GestureDetectorImpl : public GestureDetector {
 public:
  ~GestureDetectorImpl() override = default;
  // Constructor for GestureDetectorImpl.
  GestureDetectorImpl(
      const uint32_t gesture_id, const GestureType gesture_type,
      const std::vector<GestureCallback> gesture_callback_vec,
      const std::unordered_map<std::string, std::vector<uint32_t>> relation_map)
      : gesture_id_(gesture_id),
        gesture_type_(gesture_type),
        gesture_callback_vec_(gesture_callback_vec),
        relation_map_(relation_map),
        gesture_config_(PubLepusValue(lepus::Value())) {}

  // Constructor for GestureDetectorImpl.
  GestureDetectorImpl(
      const uint32_t gesture_id, const GestureType gesture_type,
      const std::vector<GestureCallback> gesture_callback_vec,
      const std::unordered_map<std::string, std::vector<uint32_t>> relation_map,
      const lepus::Value&& gesture_config)
      : gesture_id_(gesture_id),
        gesture_type_(gesture_type),
        gesture_callback_vec_(gesture_callback_vec),
        relation_map_(relation_map),
        gesture_config_(PubLepusValue(gesture_config)) {}

  // Getter method for retrieving the gesture_id of GestureDetectorImpl.
  uint32_t gesture_id() const override { return gesture_id_; }

  // Getter method for retrieving the gesture_type of GestureDetectorImpl.
  GestureType gesture_type() const override { return gesture_type_; }

  const lepus::Value& gesture_config_in_lepus_value() const {
    return gesture_config_.backend_value();
  }

  // Getter method for retrieving the vector of gesture callbacks in
  // GestureDetectorImpl.
  const std::vector<GestureCallback>& gesture_callbacks() const {
    return gesture_callback_vec_;
  }

  // Getter method for retrieving the relation map of GestureDetectorImpl.
  const std::unordered_map<std::string, std::vector<uint32_t>>& relation_map()
      const override {
    return relation_map_;
  }

  const pub::Value& gesture_config() const override { return gesture_config_; }

  // Getter method for retrieving the vector of gesture callbacks in
  // GestureDetectorImpl.
  std::vector<std::string> gesture_callback_names() const override {
    std::vector<std::string> res;
    for (auto& cb : gesture_callback_vec_) {
      res.push_back(cb.name_.str());
    }
    return res;
  }

 private:
  uint32_t gesture_id_ = 0;
  GestureType gesture_type_ = GestureType::PAN;
  std::vector<GestureCallback> gesture_callback_vec_;
  std::unordered_map<std::string, std::vector<uint32_t>> relation_map_;
  PubLepusValue gesture_config_;
};

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_EVENTS_GESTURE_H_
