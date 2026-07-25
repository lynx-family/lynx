// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/input_request_handler.h"

#include <cmath>
#include <limits>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/base/mouse_event.h"
#include "devtool/lynx_devtool/input/input_event_target.h"
#include "devtool/lynx_devtool/input/synthetic_gesture_controller.h"
#include "devtool/lynx_devtool/input/synthetic_tap_gesture.h"

namespace lynx {
namespace devtool {

namespace {

constexpr int kDefaultTapDurationMs = 50;
constexpr int kDefaultTapCount = 1;
constexpr int kMaxSyntheticTapCount = 200;
constexpr int64_t kMaxSyntheticTapSequenceDurationMs = 10000;

class TapGestureResponse {
 public:
  TapGestureResponse(std::shared_ptr<MessageSender> sender, int64_t id,
                     int tap_count)
      : sender_(std::move(sender)), id_(id), remaining_(tap_count) {}

  void OnGestureResult(input::SyntheticGestureResult result) {
    if (responded_) {
      return;
    }
    if (result != input::SyntheticGestureResult::kDone) {
      responded_ = true;
      sender_->SendErrorResponse(id_, kServerError,
                                 "Input.synthesizeTapGesture failed");
      return;
    }
    if (--remaining_ == 0) {
      responded_ = true;
      sender_->SendOKResponse(id_);
    }
  }

 private:
  std::shared_ptr<MessageSender> sender_;
  int64_t id_;
  int remaining_;
  bool responded_{false};
};

bool ParseFiniteFloat(const Json::Value& value, float& result) {
  if (!value.isNumeric()) {
    return false;
  }
  const double parsed = value.asDouble();
  if (!std::isfinite(parsed) ||
      parsed < -static_cast<double>(std::numeric_limits<float>::max()) ||
      parsed > static_cast<double>(std::numeric_limits<float>::max())) {
    return false;
  }
  result = static_cast<float>(parsed);
  return true;
}

const char* SourceTypeToString(input::PointerSourceType source_type) {
  switch (source_type) {
    case input::PointerSourceType::kDefault:
      return "default";
    case input::PointerSourceType::kTouch:
      return "touch";
    case input::PointerSourceType::kMouse:
      return "mouse";
  }
  return "unknown";
}

bool ParseGestureSourceType(const Json::Value& params,
                            input::PointerSourceType& source_type) {
  if (!params.isMember("gestureSourceType") ||
      params["gestureSourceType"].isNull()) {
    source_type = input::PointerSourceType::kDefault;
    return true;
  }
  if (!params["gestureSourceType"].isString()) {
    return false;
  }

  const std::string value = params["gestureSourceType"].asString();
  if (value == "default") {
    source_type = input::PointerSourceType::kDefault;
  } else if (value == "touch") {
    source_type = input::PointerSourceType::kTouch;
  } else if (value == "mouse") {
    source_type = input::PointerSourceType::kMouse;
  } else {
    return false;
  }
  return true;
}

struct ValidatedTapGesture {
  std::shared_ptr<input::InputEventTarget> target;
  float x = 0.f;
  float y = 0.f;
  int duration_ms = kDefaultTapDurationMs;
  int tap_count = kDefaultTapCount;
  input::PointerSourceType source_type = input::PointerSourceType::kDefault;
};

bool IsValidGesture(
    const Json::Value& params,
    const std::shared_ptr<DevToolPlatformFacade>& platform_facade,
    ValidatedTapGesture& gesture, CDPErrorCode& error_code,
    std::string& error_message) {
  if (!params.isObject() || !ParseFiniteFloat(params["x"], gesture.x) ||
      !ParseFiniteFloat(params["y"], gesture.y)) {
    error_code = kInvalidParams;
    error_message = "Invalid params: expected finite numeric x and y";
    return false;
  }

  if (!platform_facade) {
    error_code = kServerError;
    error_message = "Input target is unavailable";
    return false;
  }

  gesture.target = platform_facade->GetInputEventTarget();
  if (!gesture.target) {
    error_code = kServerError;
    error_message = "Not implemented: Input.synthesizeTapGesture";
    return false;
  }

  if (!ParseGestureSourceType(params, gesture.source_type)) {
    error_code = kInvalidParams;
    error_message =
        "Invalid params: expected gestureSourceType default, touch, or mouse";
    return false;
  }

  const auto capabilities = gesture.target->GetPointerCapabilities();
  if (gesture.source_type == input::PointerSourceType::kDefault) {
    gesture.source_type = capabilities.default_source_type;
  }
  if (gesture.source_type == input::PointerSourceType::kDefault ||
      !capabilities.Supports(gesture.source_type)) {
    error_code = kServerError;
    error_message =
        std::string("Not implemented: Input.synthesizeTapGesture source ") +
        SourceTypeToString(gesture.source_type);
    return false;
  }

  if (params.isMember("duration")) {
    if (!params["duration"].isInt() || params["duration"].asInt() < 0) {
      error_code = kInvalidParams;
      error_message = "Invalid params: duration must be a non-negative integer";
      return false;
    }
    gesture.duration_ms = params["duration"].asInt();
  }
  if (params.isMember("tapCount")) {
    if (!params["tapCount"].isInt() || params["tapCount"].asInt() < 0) {
      error_code = kInvalidParams;
      error_message = "Invalid params: tapCount must be a non-negative integer";
      return false;
    }
    gesture.tap_count = params["tapCount"].asInt();
  }

  if (gesture.tap_count > kMaxSyntheticTapCount) {
    error_code = kInvalidParams;
    error_message = "Invalid params: tapCount exceeds 200";
    return false;
  }
  const int64_t sequence_duration_ms =
      static_cast<int64_t>(gesture.duration_ms) * gesture.tap_count;
  if (sequence_duration_ms > kMaxSyntheticTapSequenceDurationMs) {
    error_code = kInvalidParams;
    error_message = "Invalid params: tap sequence duration exceeds 10000 ms";
    return false;
  }
  return true;
}

}  // namespace

InputRequestHandler::InputRequestHandler(
    const std::weak_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator) {}

InputRequestHandler::~InputRequestHandler() = default;

bool InputRequestHandler::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  const bool facade_changed =
      devtool_platform_facade_ != devtool_platform_facade;
  devtool_platform_facade_ = devtool_platform_facade;
  // The caller resets the gesture controller on the UI thread when the facade
  // changed; the facade itself is updated synchronously so touch/insert
  // requests observe it immediately.
  return facade_changed;
}

void InputRequestHandler::Reset() { synthetic_gesture_controller_.reset(); }

void InputRequestHandler::EmulateTouchFromMouseEvent(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  auto input = std::make_shared<MouseEvent>();
  input->button_ = params["button"].asString();
  input->click_count_ = params["clickCount"].asInt();
  input->delta_x_ = params["deltaX"].asFloat();
  input->delta_y_ = params["deltaY"].asFloat();
  input->modifiers_ = params["modifiers"].asInt();
  input->type_ = params["type"].asString();
  input->x_ = params["x"].asInt();
  input->y_ = params["y"].asInt();
  devtool_platform_facade_->EmulateTouch(input);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InputRequestHandler::InsertText(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  Json::Value params = message["params"];

  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  devtool_platform_facade_->InsertText(params["text"].asString());

  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InputRequestHandler::EnsureSyntheticGestureController(
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  if (synthetic_gesture_controller_) {
    return;
  }
  synthetic_gesture_controller_ = input::SyntheticGestureController::Create(
      devtool_platform_facade_->GetInputEventTarget(), task_runner);
}

void InputRequestHandler::SynthesizeTapGesture(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  const int64_t id = message["id"].asInt64();
  const Json::Value& params = message["params"];
  ValidatedTapGesture gesture;
  CDPErrorCode error_code = kServerError;
  std::string error_message;
  if (!IsValidGesture(params, devtool_platform_facade_, gesture, error_code,
                      error_message)) {
    sender->SendErrorResponse(id, error_code, error_message);
    return;
  }
  if (gesture.tap_count == 0) {
    sender->SendOKResponse(id);
    return;
  }
  auto devtool_mediator = devtool_mediator_wp_.lock();
  const auto task_runner =
      devtool_mediator ? devtool_mediator->GetUITaskRunner() : nullptr;
  if (!task_runner) {
    sender->SendErrorResponse(id, kServerError,
                              "Input UI task runner is unavailable");
    return;
  }

  EnsureSyntheticGestureController(task_runner);
  auto response =
      std::make_shared<TapGestureResponse>(sender, id, gesture.tap_count);
  for (int tap_index = 0; tap_index < gesture.tap_count; ++tap_index) {
    synthetic_gesture_controller_->QueueSyntheticGesture(
        std::make_unique<input::SyntheticTapGesture>(
            gesture.x, gesture.y, gesture.duration_ms, gesture.source_type),
        [response](input::SyntheticGestureResult result) {
          response->OnGestureResult(result);
        });
  }
}

}  // namespace devtool
}  // namespace lynx
