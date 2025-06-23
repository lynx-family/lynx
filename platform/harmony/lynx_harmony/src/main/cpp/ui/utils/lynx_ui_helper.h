// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_HELPER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_HELPER_H_

#include <string>

namespace lynx {
namespace tasm {
namespace harmony {
class UIBase;

class LynxUIHelper {
 public:
  static bool UIIsParentOfAnotherUI(UIBase* parent, UIBase* child);

  static void ConvertPointFromAncestorToDescendant(
      float res[2], UIBase* ancestor, UIBase* descendant, float point[2],
      bool enable_transform = true);

  static void ConvertPointFromDescendantToAncestor(
      float res[2], UIBase* descendant, UIBase* ancestor, float point[2],
      bool enable_transform = true);

  static void ConvertPointFromUIToAnotherUI(float res[2], UIBase* ui,
                                            UIBase* another, float point[2],
                                            bool enable_transform = true);

  static void ConvertPointFromUIToRootUI(float res[2], UIBase* ui,
                                         float point[2],
                                         bool enable_transform = true);

  static void ConvertPointFromUIToScreen(float res[2], UIBase* ui,
                                         float point[2],
                                         bool enable_transform = true);

  static void ConvertRectFromAncestorToDescendant(float res[4],
                                                  UIBase* ancestor,
                                                  UIBase* descendant,
                                                  float rect[4],
                                                  bool enable_transform = true);

  static void ConvertRectFromDescendantToAncestor(float res[4],
                                                  UIBase* descendant,
                                                  UIBase* ancestor,
                                                  float rect[4],
                                                  bool enable_transform = true);

  static void ConvertRectFromUIToAnotherUI(float res[4], UIBase* ui,
                                           UIBase* another, float rect[4],
                                           bool enable_transform = true);

  static void ConvertRectFromUIToRootUI(float res[4], UIBase* ui, float rect[4],
                                        bool enable_transform = true);

  static void ConvertRectFromUIToScreen(float res[4], UIBase* ui, float rect[4],
                                        bool enable_transform = true);

  static bool CheckViewportIntersectWithRatio(float rect[4], float another[4],
                                              float ratio);

  static void OffsetRect(float rect[4], float offset[2]);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UI_HELPER_H_
