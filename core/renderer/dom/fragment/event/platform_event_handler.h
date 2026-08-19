// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_HANDLER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_HANDLER_H_

#include <array>
#include <deque>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/dom/fragment/event/platform_event_target.h"

namespace lynx {
namespace tasm {

class PlatformInputEvent;
class PlatformPointerEvent;
class NativePaintingCtxPlatformRef;

class PlatformEventHandler {
 public:
  class PlatformEventTargetDetail {
   public:
    PlatformEventTargetDetail(fml::RefPtr<PlatformEventTarget> target,
                              float down_point[2]);

    void GetDownPoint(float down_point[2]);
    void GetPrePoint(float pre_point[2]);
    void SetPrePoint(float pre_point[2]);
    fml::RefPtr<PlatformEventTarget> Target();

   private:
    fml::RefPtr<PlatformEventTarget> target_;
    float down_point_[2]{std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max()};
    float pre_point_[2]{std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()};
  };

  explicit PlatformEventHandler(NativePaintingCtxPlatformRef* platform_ref)
      : platform_ref_(platform_ref) {}

  bool OnInputEvent(fml::RefPtr<PlatformEventTarget> target_tree,
                    int int_event_data[], float float_event_data[],
                    bool enable_event_through_inherit_from_page);
  void OnTap();
  void OnLongPress();
  void DispatchPointerEvent(const std::string& name,
                            const lepus::Value& target_pointer_map);

  bool EventThrough();
  int32_t HitTargetSign() const { return hit_target_sign_; }
  int32_t RendererHostSign() const { return renderer_host_sign_; }
  bool IgnoreFocus() const { return ignore_focus_; }
  bool CanRespondFocus();

  void SetTapSlop(const std::string& tap_slop);
  void SetLongPressDuration(int32_t long_press_duration);
  void SetHasPointerPseudo(bool has_pointer_pseudo);

 private:
  void InitPointerEnv(PlatformPointerEvent& event);
  void ResetPointerEnv(PlatformPointerEvent& event);
  void InitClickEnv();
  void ResetClickEnv();
  void RecordScrollOffsetsForTap();
  bool HasScrollContainerScrolledForTap();

  void OnPointerDown(PlatformPointerEvent& event);
  void OnPointerMove(PlatformPointerEvent& event);
  void OnPointerUp(PlatformPointerEvent& event);
  void OnPointerCancel(PlatformPointerEvent& event);

  void HandlePointerDown(PlatformPointerEvent& event);
  void HandlePointerMove(PlatformPointerEvent& event);
  void HandlePointerUp(PlatformPointerEvent& event);
  void HandlePointerCancel(PlatformPointerEvent& event);

  void DispatchGestureEvent(const std::string& name, float root_point[2]);
  fml::RefPtr<PlatformEventTarget> FindTarget(float pointer_x, float pointer_y);
  void ResetFocusInfo();
  bool CanRespondTap(fml::RefPtr<PlatformEventTarget> target);
  void ActivePseudoStatus();
  void DeactivatePseudoStatus(LynxPseudoStatus status);
  bool IsPointerMoveOutside(fml::RefPtr<PlatformEventTarget> target);
  void GetTargetPoint(fml::RefPtr<PlatformEventTarget> target,
                      float target_point[2], float page_point[2]);
  void AddTargetPointerMap(lepus::Value& target_pointer_map,
                           PlatformPointerEvent& event);

  // owned by NativePaintingCtxPlatformRef
  NativePaintingCtxPlatformRef* platform_ref_{nullptr};

  // state
  fml::RefPtr<PlatformEventTarget> target_tree_{nullptr};
  fml::RefPtr<PlatformEventTarget> first_target_{nullptr};
  std::vector<fml::RefPtr<PlatformEventTarget>> event_target_chain_;
  std::deque<fml::RefPtr<PlatformEventTarget>> click_target_chain_;
  std::unordered_map<int, PlatformEventTargetDetail> target_pointer_map_;
  std::unordered_map<int32_t, std::array<float, 2>> scroll_offset_for_tap_;
  int32_t hit_target_sign_{-1};
  int32_t renderer_host_sign_{-1};
  bool ignore_focus_{false};
  bool has_pointer_moved_{false};
  bool first_pointer_moved_{false};
  bool first_pointer_outside_{false};
  float first_pointer_down_point_[2]{0.f};

  // config
  unsigned int tap_slop_{5};
  bool has_pointer_pseudo_{false};
  bool enable_event_through_inherit_from_page_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_HANDLER_H_
