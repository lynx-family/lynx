// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_

#include <array>
#include <string>
#include <unordered_set>

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
  const base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformEventTarget>,
                                   64>&
  GetEventTargets() const {
    return event_targets_;
  }
  void RefreshScrollOffsets();
  void RefreshScrollOffsets(const fml::RefPtr<PlatformEventTarget>& root);
  void ClearEventTargets();
  // Remove cached targets belonging to an event root.
  void RemoveEventTargetsInEventRoot(int32_t root_id);
  bool IsActiveEventRoot(int32_t root_id) const;
  void AddActiveEventRoot(int32_t root_id);
  fml::RefPtr<PlatformEventTarget> RemoveActiveEventRoot(int32_t root_id);
  void ClearActiveEventRoots();
  bool SetEventRootOffsetToPageRoot(int32_t root_id, float offset_x,
                                    float offset_y);
  void RemoveEventRootOffsetToPageRoot(int32_t root_id);
  fml::RefPtr<PlatformEventTarget> GetEventRootTree(int32_t root_id) const;
  void SetEventRootTree(int32_t root_id,
                        const fml::RefPtr<PlatformEventTarget>& root);
  const std::unordered_set<int32_t>& GetActiveEventRootIds() const {
    return active_event_root_ids_;
  }
  const base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformEventTarget>,
                                   16>&
  GetEventRootTrees() const {
    return event_target_trees_;
  }

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

  void ConvertPointFromTargetToPageRootTarget(
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

  void ConvertRectFromTargetToPageRootTarget(
      float res[4], const fml::RefPtr<PlatformEventTarget>& target,
      float rect[4]);

  void ConvertRectFromTargetToScreen(
      float res[4], const fml::RefPtr<PlatformEventTarget>& target,
      float rect[4]);

  bool CheckViewportIntersectWithRatio(float rect[4], float another[4],
                                       float ratio);

  bool GetTreeRootOffsetToPageRootTarget(
      const fml::RefPtr<PlatformEventTarget>& target, float offset[2]);

  void OffsetPoint(float point[2], const float offset[2]);
  void OffsetRect(float rect[4], const float offset[2]);

  void SetDevicePixelRatio(float device_pixel_ratio) {
    device_pixel_ratio_ = device_pixel_ratio;
  }

  float GetDevicePixelRatio() { return device_pixel_ratio_; }

  void GetRootViewLocationOnScreen(float location[2]);
  void GetScreenSize(float size[2]);
  void GetPlatformRendererScrollOffset(int32_t sign, float offset[2]);

  void InvokeMethod(
      int32_t id, const std::string& method, const lepus::Value& params,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

 private:
  bool IsScrollContainer(PlatformRendererType type, int32_t sign);

  void ApplyEventBundle(const fml::RefPtr<PlatformEventTarget>& target,
                        const PlatformEventBundle* bundle);
  void RefreshScrollOffsetsRecursively(
      const fml::RefPtr<PlatformEventTarget>& target);
  fml::RefPtr<PlatformEventTarget> GetTreeRoot(
      const fml::RefPtr<PlatformEventTarget>& target);
  fml::RefPtr<PlatformEventTarget> ReconstructEventTargetTreeRecursively(
      fml::RefPtr<PlatformRendererImpl> page_renderer, int32_t tree_root_id);
  void UpdateExposureTargetRegistration(
      const fml::RefPtr<PlatformEventTarget>& target, bool has_custom_event,
      bool has_global_event);

  // owned by NativePaintingCtxPlatformRef
  NativePaintingCtxPlatformRef* platform_ref_{nullptr};
  // map from id to the EventTarget.
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformEventTarget>, 64>
      event_targets_;
  std::unordered_set<int32_t> active_event_root_ids_;
  // map from tree root id to the root EventTarget of each event target tree.
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformEventTarget>, 16>
      event_target_trees_;
  // map from independent tree root id to its platform offset in page-root
  // coordinates.
  base::InlineOrderedFlatMap<int32_t, std::array<float, 2>, 16>
      event_root_offsets_to_page_root_;
  // device pixel ratio of the current display.
  float device_pixel_ratio_{1.0f};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
