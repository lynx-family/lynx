// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_POINTER_EVENT_H_
#define CORE_EVENT_POINTER_EVENT_H_

#include <cstdint>
#include <string>

#include "core/event/event.h"

namespace lynx {
namespace event {

inline constexpr char kPointerRelatedTargetSign[] = "__lynxRelatedTargetSign";
inline constexpr int32_t kInvalidPointerRelatedTargetSign = -1;

class PointerEvent : public Event {
 public:
  PointerEvent(const std::string& event_name, const lepus::Value& event_param,
               int64_t time_stamp = 0);

  int32_t related_target_sign() const { return related_target_sign_; }
  void set_related_target(fml::WeakPtr<EventTarget> related_target) {
    related_target_ = related_target;
  }

  static int32_t ExtractRelatedTargetSign(lepus::Dictionary& params);
  static void NormalizeParams(lepus::Dictionary& params);

  void HandleEventBaseDetail(bool is_core_event = false) override;

 private:
  int32_t related_target_sign_{kInvalidPointerRelatedTargetSign};
  fml::WeakPtr<EventTarget> related_target_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_POINTER_EVENT_H_
