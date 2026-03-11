// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_

#include <string>

#include "core/renderer/dom/fragment/event/platform_event_target.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
class PlatformRendererImpl;
class NativePaintingCtxPlatformRef;

class PlatformEventTargetHelper {
 public:
  explicit PlatformEventTargetHelper(NativePaintingCtxPlatformRef* platform_ref)
      : platform_ref_(platform_ref) {}

  fml::RefPtr<PlatformEventTarget> GetRootEventTarget();
  fml::RefPtr<PlatformEventTarget> GetEventTarget(int32_t id);

  fml::RefPtr<PlatformEventTarget> ReconstructEventTargetTreeRecursively(
      fml::RefPtr<PlatformRendererImpl> page_renderer);

  bool TargetIsParentOfAnotherTarget(
      const fml::RefPtr<PlatformEventTarget>& target,
      const fml::RefPtr<PlatformEventTarget>& another);

  // point: [x, y]
  void ConvertPointFromAncestorToDescendant(
      float res[2], const fml::RefPtr<PlatformEventTarget>& ancestor,
      const fml::RefPtr<PlatformEventTarget>& descendant, float point[2]);

  void ConvertPointFromDescendantToAncestor(
      float res[2], const fml::RefPtr<PlatformEventTarget>& descendant,
      const fml::RefPtr<PlatformEventTarget>& ancestor, float point[2]);

  void ConvertPointFromTargetToAnotherTarget(
      float res[2], const fml::RefPtr<PlatformEventTarget>& target,
      const fml::RefPtr<PlatformEventTarget>& another, float point[2]);

  void ConvertPointFromTargetToRootTarget(
      float res[2], const fml::RefPtr<PlatformEventTarget>& target,
      float point[2]);

  void ConvertPointFromTargetToScreen(
      float res[2], const fml::RefPtr<PlatformEventTarget>& target,
      float point[2]);

  // rect: [left, top, right, bottom]
  void ConvertRectFromAncestorToDescendant(
      float res[4], const fml::RefPtr<PlatformEventTarget>& ancestor,
      const fml::RefPtr<PlatformEventTarget>& descendant, float rect[4]);

  void ConvertRectFromDescendantToAncestor(
      float res[4], const fml::RefPtr<PlatformEventTarget>& descendant,
      const fml::RefPtr<PlatformEventTarget>& ancestor, float rect[4]);

  void ConvertRectFromTargetToAnotherTarget(
      float res[4], const fml::RefPtr<PlatformEventTarget>& target,
      const fml::RefPtr<PlatformEventTarget>& another, float rect[4]);

  void ConvertRectFromTargetToRootTarget(
      float res[4], const fml::RefPtr<PlatformEventTarget>& target,
      float rect[4]);

  void ConvertRectFromTargetToScreen(
      float res[4], const fml::RefPtr<PlatformEventTarget>& target,
      float rect[4]);

  bool CheckViewportIntersectWithRatio(float rect[4], float another[4],
                                       float ratio);

  void OffsetRect(float rect[4], float offset[2]);

  void SetDevicePixelRatio(float device_pixel_ratio) {
    device_pixel_ratio_ = device_pixel_ratio;
  }

  float GetDevicePixelRatio() { return device_pixel_ratio_; }

  void GetRootViewLocationOnScreen(float location[2]);
  void GetScreenSize(float size[2]);

  void InvokeMethod(
      int32_t id, const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

 private:
  void ApplyEventBundle(const fml::RefPtr<PlatformEventTarget>& target,
                        const PlatformEventBundle* bundle);
  void UpdateExposureTargetRegistration(
      const fml::RefPtr<PlatformEventTarget>& target, bool has_custom_event,
      bool has_global_event);

  // owned by NativePaintingCtxPlatformRef
  NativePaintingCtxPlatformRef* platform_ref_{nullptr};
  // the root node of the EventTarget Tree reconstructed from the DisplayList.
  fml::RefPtr<PlatformEventTarget> event_target_tree_{nullptr};
  // map from id to the EventTarget.
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformEventTarget>, 64>
      event_targets_;
  // device pixel ratio of the current display.
  float device_pixel_ratio_{1.0f};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
