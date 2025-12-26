// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_

#include "core/renderer/dom/fragment/event/platform_event_target.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
class PlatformRendererImpl;

class PlatformEventTargetHelper {
 public:
  fml::RefPtr<PlatformEventTarget> GetRootEventTarget();

  fml::RefPtr<PlatformEventTarget> ReconstructEventTargetTreeRecursively(
      fml::RefPtr<PlatformRendererImpl> page_renderer);

  bool TargetIsParentOfAnotherTarget(fml::RefPtr<PlatformEventTarget> target,
                                     fml::RefPtr<PlatformEventTarget> another);

  // point: [x, y]
  void ConvertPointFromAncestorToDescendant(
      float res[2], fml::RefPtr<PlatformEventTarget> ancestor,
      fml::RefPtr<PlatformEventTarget> descendant, float point[2]);

  void ConvertPointFromDescendantToAncestor(
      float res[2], fml::RefPtr<PlatformEventTarget> descendant,
      fml::RefPtr<PlatformEventTarget> ancestor, float point[2]);

  void ConvertPointFromTargetToAnotherTarget(
      float res[2], fml::RefPtr<PlatformEventTarget> target,
      fml::RefPtr<PlatformEventTarget> another, float point[2]);

  void ConvertPointFromTargetToRootTarget(
      float res[2], fml::RefPtr<PlatformEventTarget> target, float point[2]);

  void ConvertPointFromTargetToScreen(float res[2],
                                      fml::RefPtr<PlatformEventTarget> target,
                                      float point[2]);

  // rect: [left, top, right, bottom]
  void ConvertRectFromAncestorToDescendant(
      float res[4], fml::RefPtr<PlatformEventTarget> ancestor,
      fml::RefPtr<PlatformEventTarget> descendant, float rect[4]);

  void ConvertRectFromDescendantToAncestor(
      float res[4], fml::RefPtr<PlatformEventTarget> descendant,
      fml::RefPtr<PlatformEventTarget> ancestor, float rect[4]);

  void ConvertRectFromTargetToAnotherTarget(
      float res[4], fml::RefPtr<PlatformEventTarget> target,
      fml::RefPtr<PlatformEventTarget> another, float rect[4]);

  void ConvertRectFromTargetToRootTarget(
      float res[4], fml::RefPtr<PlatformEventTarget> target, float rect[4]);

  void ConvertRectFromTargetToScreen(float res[4],
                                     fml::RefPtr<PlatformEventTarget> target,
                                     float rect[4]);

  bool CheckViewportIntersectWithRatio(float rect[4], float another[4],
                                       float ratio);

  void OffsetRect(float rect[4], float offset[2]);

 private:
  // the root node of the EventTarget Tree reconstructed from the DisplayList.
  fml::RefPtr<PlatformEventTarget> event_target_tree_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_TARGET_HELPER_H_
