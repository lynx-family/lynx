// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_ui_renderer.h"

#include <cctype>
#include <string_view>

#include "base/include/timer/time_utils.h"
#include "clay/ui/event/key_codes.h"
#include "platform/embedder/clay_input_event_target.h"

namespace lynx {
namespace embedder {

namespace {

constexpr int32_t kSyntheticTouchDeviceId = 1;
// Keep persistent synthetic mouse state separate from native mouse devices.
constexpr int32_t kSyntheticMouseDeviceId = 2;
static constexpr const char* kMousePressed = "mousePressed";
static constexpr const char* kMouseMoved = "mouseMoved";
static constexpr const char* kMouseReleased = "mouseReleased";
static constexpr const char* kMouseWheel = "mouseWheel";
static constexpr const char* kMouseLeftButton = "left";
static constexpr const char* kMouseRightButton = "right";
static constexpr const char* kMouseMiddleButton = "middle";
static constexpr const char* kMouseBackButton = "back";
static constexpr const char* kMouseForwardButton = "forward";
static constexpr const char* kKeyDown = "keyDown";
static constexpr const char* kRawKeyDown = "rawKeyDown";
static constexpr const char* kKeyUp = "keyUp";
static constexpr const char* kChar = "char";

struct KeyCodes {
  uint64_t physical;
  uint64_t logical;
};

struct NamedKey {
  std::string_view name;
  KeyCodes codes;
};

// Clay routes key input by logical key; physical is a non-empty placeholder.
constexpr KeyCodes kUnidentifiedKey = {clay::keycodes::kPhysicalKeyA,
                                       clay::keycodes::kLogicalUnidentified};

constexpr NamedKey kNamedKeys[] = {
    {"Enter", {clay::keycodes::kPhysicalEnter, clay::keycodes::kLogicalEnter}},
    {"Backspace",
     {clay::keycodes::kPhysicalBackspace, clay::keycodes::kLogicalBackspace}},
    {"Delete",
     {clay::keycodes::kPhysicalDelete, clay::keycodes::kLogicalDelete}},
    {"Tab", {clay::keycodes::kPhysicalTab, clay::keycodes::kLogicalTab}},
    {"Escape",
     {clay::keycodes::kPhysicalEscape, clay::keycodes::kLogicalEscape}},
    {"ArrowLeft",
     {clay::keycodes::kPhysicalArrowLeft, clay::keycodes::kLogicalArrowLeft}},
    {"ArrowRight",
     {clay::keycodes::kPhysicalArrowRight, clay::keycodes::kLogicalArrowRight}},
    {"ArrowUp",
     {clay::keycodes::kPhysicalArrowUp, clay::keycodes::kLogicalArrowUp}},
    {"ArrowDown",
     {clay::keycodes::kPhysicalArrowDown, clay::keycodes::kLogicalArrowDown}},
    {"Home", {clay::keycodes::kPhysicalHome, clay::keycodes::kLogicalHome}},
    {"End", {clay::keycodes::kPhysicalEnd, clay::keycodes::kLogicalEnd}},
    {"Shift",
     {clay::keycodes::kPhysicalShiftLeft, clay::keycodes::kLogicalShiftLeft}},
    {"Control",
     {clay::keycodes::kPhysicalControlLeft,
      clay::keycodes::kLogicalControlLeft}},
    {"Alt",
     {clay::keycodes::kPhysicalAltLeft, clay::keycodes::kLogicalAltLeft}},
    {"Meta",
     {clay::keycodes::kPhysicalMetaLeft, clay::keycodes::kLogicalMetaLeft}},
};

KeyCodes ResolveKeyCodes(std::string_view key) {
  for (const auto& named_key : kNamedKeys) {
    if (key == named_key.name) {
      return named_key.codes;
    }
  }
  // Printable ASCII keys use their lowercase code point as the logical key.
  if (key.size() == 1 && std::isprint(static_cast<unsigned char>(key[0]))) {
    return {kUnidentifiedKey.physical,
            static_cast<uint64_t>(
                std::tolower(static_cast<unsigned char>(key[0])))};
  }
  return kUnidentifiedKey;
}

int64_t ButtonMask(std::string_view button) {
  if (button == kMouseLeftButton) {
    return kClayPointerMouseButtonsMousePrimary;
  }
  if (button == kMouseRightButton) {
    return kClayPointerMouseButtonsMouseSecondary;
  }
  if (button == kMouseMiddleButton) {
    return kClayPointerMouseButtonsMouseMiddle;
  }
  if (button == kMouseBackButton) {
    return kClayPointerMouseButtonsMouseBack;
  }
  if (button == kMouseForwardButton) {
    return kClayPointerMouseButtonsMouseForward;
  }
  return 0;
}

}  // namespace

LynxUIRenderer::~LynxUIRenderer() { InvalidateInputEvents(); }

std::shared_ptr<devtool::input::InputEventTarget>
LynxUIRenderer::GetInputEventTarget() {
  if (!input_event_target_) {
    input_event_target_ = std::make_shared<ClayInputEventTarget>(
        [this]() {
          return ClayInputViewport{width_, height_, pixel_ratio_};
        },
        [this](const ClayPointerEvent& event) { SendPointerEvent(event); });
  }
  return input_event_target_;
}

void LynxUIRenderer::CancelInputEvents() {
  if (input_event_target_) {
    input_event_target_->CancelActivePointers();
  }
}

void LynxUIRenderer::InvalidateInputEvents() {
  if (input_event_target_) {
    input_event_target_->Invalidate();
  }
}

void LynxUIRenderer::DispatchSyntheticPointerEvent(ClayPointerEvent event) {
  event.struct_size = sizeof(event);
  event.x *= pixel_ratio_;
  event.y *= pixel_ratio_;
  event.scroll_delta_x *= pixel_ratio_;
  event.scroll_delta_y *= pixel_ratio_;

  const auto send = [this](ClayPointerEvent pointer_event) {
    pointer_event.timestamp = base::CurrentTimeMicroseconds();
    SendPointerEvent(pointer_event);
  };
  if (event.device_kind == kClayPointerDeviceKindMouse) {
    // Clay adds the mouse device on first input and retains its position across
    // gestures. Removing it after each hover would discard subsequent motion.
    send(event);
    return;
  }
  const auto send_lifecycle = [&event, &send](ClayPointerPhase phase) {
    // Add/Remove events carry no button/scroll payload.
    ClayPointerEvent lifecycle = event;
    lifecycle.phase = phase;
    lifecycle.buttons = 0;
    lifecycle.signal_kind = kClayPointerSignalKindNone;
    lifecycle.scroll_delta_x = 0;
    lifecycle.scroll_delta_y = 0;
    lifecycle.is_precise_scroll = 0;
    send(lifecycle);
  };

  // A touch device exists only for the duration of its gesture.
  if (event.phase == kClayPointerPhaseDown) {
    send_lifecycle(kClayPointerPhaseAdd);
  }
  send(event);
  if (event.phase == kClayPointerPhaseUp) {
    send_lifecycle(kClayPointerPhaseRemove);
  }
}

void LynxUIRenderer::EmulateMouseSyntheticEvent(const char* event_type, float x,
                                                float y, const char* button,
                                                float delta_x, float delta_y,
                                                int modifiers,
                                                int click_count) {
  // ClayPointerEvent has no modifier/click-count fields; kept for API parity.
  (void)modifiers;
  (void)click_count;
  if (!event_type) {
    return;
  }
  const std::string_view type(event_type);
  if (type != kMousePressed && type != kMouseMoved && type != kMouseReleased &&
      type != kMouseWheel) {
    return;
  }

  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.device = kSyntheticMouseDeviceId;
  event.device_kind = kClayPointerDeviceKindMouse;

  const int64_t button_mask = ButtonMask(button ? button : "");
  if (type == kMousePressed) {
    event.phase = kClayPointerPhaseDown;
    // Empty mask still means a primary click.
    event.buttons =
        button_mask != 0 ? button_mask : kClayPointerMouseButtonsMousePrimary;
  } else if (type == kMouseReleased) {
    event.phase = kClayPointerPhaseUp;
    event.buttons = 0;
  } else if (type == kMouseMoved) {
    // Zero mask on move is an intentional hover; caller owns drag state.
    event.phase =
        button_mask != 0 ? kClayPointerPhaseMove : kClayPointerPhaseHover;
    event.buttons = button_mask;
  } else {
    event.phase = kClayPointerPhaseHover;
    event.buttons = 0;
    event.signal_kind = kClayPointerSignalKindScroll;
    event.scroll_delta_x = delta_x;
    event.scroll_delta_y = delta_y;
    event.is_precise_scroll = 1;
  }

  DispatchSyntheticPointerEvent(event);
}

void LynxUIRenderer::EmulateTouchSyntheticEvent(const char* event_type, float x,
                                                float y, const char* button,
                                                float delta_x, float delta_y,
                                                int modifiers,
                                                int click_count) {
  (void)modifiers;
  (void)click_count;
  (void)button;
  (void)delta_x;
  (void)delta_y;
  if (!event_type) {
    return;
  }
  const std::string_view type(event_type);
  // Clay touch has no wheel; wheel is downgraded upstream in the proxy.
  if (type != kMousePressed && type != kMouseMoved && type != kMouseReleased) {
    return;
  }

  ClayPointerEvent event = {};
  event.x = x;
  event.y = y;
  event.device = kSyntheticTouchDeviceId;
  event.device_kind = kClayPointerDeviceKindTouch;
  // Touch never carries mouse-button state on Clay.
  event.buttons = 0;

  if (type == kMousePressed) {
    event.phase = kClayPointerPhaseDown;
  } else if (type == kMouseReleased) {
    event.phase = kClayPointerPhaseUp;
  } else {
    // Touch move is always a drag; no unpressed hover.
    event.phase = kClayPointerPhaseMove;
  }

  DispatchSyntheticPointerEvent(event);
}

void LynxUIRenderer::DispatchKeyEvent(const std::string& type,
                                      const std::string& key,
                                      const std::string& text, int modifiers,
                                      bool auto_repeat) {
  // ClayKeyEvent has no modifier field; modifier keys arrive as key events.
  (void)modifiers;
  const bool is_char = type == kChar;
  if (type != kKeyDown && type != kRawKeyDown && type != kKeyUp && !is_char) {
    return;
  }

  // char only inserts text, so it must not trigger key bindings.
  const KeyCodes codes = is_char ? kUnidentifiedKey : ResolveKeyCodes(key);
  ClayKeyEvent event = {};
  event.struct_size = sizeof(event);
  event.timestamp = static_cast<double>(base::CurrentTimeMicroseconds());
  event.physical = codes.physical;
  event.logical = codes.logical;
  event.synthesized = true;
  if (type == kKeyUp) {
    event.type = kClayKeyEventTypeUp;
  } else {
    event.type = auto_repeat && !is_char ? kClayKeyEventTypeRepeat
                                         : kClayKeyEventTypeDown;
    // rawKeyDown never produces text input.
    if (type != kRawKeyDown && !text.empty()) {
      event.character = text.c_str();
    }
  }
  SendKeyEvent(event);
}

}  // namespace embedder
}  // namespace lynx
