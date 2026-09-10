// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/pointer_event.h"

#include "base/include/timer/time_utils.h"
#include "core/event/event_target.h"

namespace lynx {
namespace event {
namespace {

bool IsPointerEnterOrLeave(const std::string& event_name) {
  return event_name == "pointerenter" || event_name == "pointerleave";
}

}  // namespace

PointerEvent::PointerEvent(const std::string& event_name,
                           const lepus::Value& event_param, int64_t time_stamp)
    : Event(event_name,
            time_stamp != 0
                ? time_stamp
                : static_cast<int64_t>(base::CurrentSystemTimeMilliseconds()),
            EventType::kPointerEvent, Capture::kYes,
            IsPointerEnterOrLeave(event_name) ? Bubbles::kNo : Bubbles::kYes,
            IsPointerEnterOrLeave(event_name) || event_name == "pointercancel"
                ? Cancelable::kNo
                : Cancelable::kYes,
            IsPointerEnterOrLeave(event_name) ? ComposedMode::kComposed
                                              : ComposedMode::kScoped,
            PhaseType::kNone) {
  if (event_param.IsTable()) {
    for (const auto& item : *event_param.Table()) {
      detail_.Table()->SetValue(item.first, item.second);
    }
  }
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  detail_.Table()->SetValue(kTimestamp, time_stamp_);
  NormalizeParams(*detail_.Table());
  related_target_sign_ = ExtractRelatedTargetSign(*detail_.Table());
}

void PointerEvent::NormalizeParams(lepus::Dictionary& params) {
  BASE_STATIC_STRING_DECL(kOffsetX, "offsetX");
  BASE_STATIC_STRING_DECL(kOffsetY, "offsetY");
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  BASE_STATIC_STRING_DECL(kPressure, "pressure");
  BASE_STATIC_STRING_DECL(kButtons, "buttons");
  if (!params.Contains(kOffsetX)) {
    const auto& x = params.GetValue(kX);
    params.SetValue(kOffsetX, x.IsNumber() ? x : lepus::Value(0));
  }
  if (!params.Contains(kOffsetY)) {
    const auto& y = params.GetValue(kY);
    params.SetValue(kOffsetY, y.IsNumber() ? y : lepus::Value(0));
  }
  if (!params.Contains(kWidth)) {
    params.SetValue(kWidth, 1);
  }
  if (!params.Contains(kHeight)) {
    params.SetValue(kHeight, 1);
  }
  if (!params.Contains(kPressure)) {
    const auto& buttons = params.GetValue(kButtons);
    params.SetValue(kPressure,
                    buttons.IsNumber() && buttons.Number() != 0 ? 0.5 : 0.0);
  }
  BASE_STATIC_STRING_DECL(kTangentialPressure, "tangentialPressure");
  BASE_STATIC_STRING_DECL(kTiltX, "tiltX");
  BASE_STATIC_STRING_DECL(kTiltY, "tiltY");
  BASE_STATIC_STRING_DECL(kTwist, "twist");
  for (const auto& field : {kTangentialPressure, kTiltX, kTiltY, kTwist}) {
    if (!params.Contains(field)) {
      params.SetValue(field, 0);
    }
  }
}

int32_t PointerEvent::ExtractRelatedTargetSign(lepus::Dictionary& params) {
  const auto& value =
      params.GetValue(BASE_STATIC_STRING(kPointerRelatedTargetSign));
  int32_t sign = value.IsNumber() ? static_cast<int32_t>(value.Number())
                                  : kInvalidPointerRelatedTargetSign;
  params.Erase(BASE_STATIC_STRING(kPointerRelatedTargetSign));
  return sign;
}

void PointerEvent::HandleEventBaseDetail(bool is_core_event) {
  Event::HandleEventBaseDetail(is_core_event);
  BASE_STATIC_STRING_DECL(kRelatedTarget, "relatedTarget");
  detail_.SetProperty(kRelatedTarget,
                      related_target_
                          ? related_target_->GetEventTargetInfo(is_core_event)
                          : lepus::Value());
}

}  // namespace event
}  // namespace lynx
