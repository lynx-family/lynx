// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_ui_helper.h"

#include <limits>
#include <vector>

#include "base/include/float_comparison.h"
#include "core/animation/transforms/matrix44.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_root.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/transform.h"

namespace lynx {
namespace tasm {
namespace harmony {

bool LynxUIHelper::UIIsParentOfAnotherUI(UIBase* parent, UIBase* child) {
  if (!child || !parent || child == parent) {
    return false;
  }

  while (child->Parent() != nullptr) {
    if (parent == child->Parent()) {
      return true;
    }
    child = child->Parent();
  }
  return false;
}

void LynxUIHelper::ConvertPointFromAncestorToDescendant(float res[2],
                                                        UIBase* ancestor,
                                                        UIBase* descendant,
                                                        float point[2],
                                                        bool enable_transform) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }

  std::vector<UIBase*> ui_chain;
  UIBase* current_ui = descendant;
  while (current_ui != nullptr && current_ui != ancestor) {
    ui_chain.push_back(current_ui);
    current_ui = current_ui->Parent();
  }

  res[0] = point[0], res[1] = point[1];
  for (int i = ui_chain.size() - 1; i >= 0; --i) {
    current_ui = ui_chain[i];
    res[0] += ancestor->ScrollX();
    res[1] += ancestor->ScrollY();
    res[0] -= current_ui->ViewLeft();
    res[1] -= current_ui->ViewTop();
    res[0] += current_ui->OffsetXForCalcPosition();
    res[1] += current_ui->OffsetYForCalcPosition();
    if (enable_transform) {
      Transform* transform = current_ui->GetTransform();
      float width = current_ui->width_, height = current_ui->height_;
      if (transform) {
        transforms::Matrix44 invert_matrix;
        if (transform->GetTransformMatrix(width, height, 1.f, true)
                .invert(&invert_matrix)) {
          invert_matrix.mapPoint(res, res);
        }
      }
    }
    ancestor = current_ui;
  }
}

void LynxUIHelper::ConvertPointFromDescendantToAncestor(float res[2],
                                                        UIBase* descendant,
                                                        UIBase* ancestor,
                                                        float point[2],
                                                        bool enable_transform) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }

  res[0] = point[0], res[1] = point[1];
  if (enable_transform) {
    Transform* transform = descendant->GetTransform();
    float width = descendant->width_, height = descendant->height_;
    if (transform) {
      transform->GetTransformMatrix(width, height, 1.f, true)
          .mapPoint(res, res);
    }
  }

  while (descendant != nullptr && descendant->Parent() &&
         descendant != ancestor) {
    res[0] += descendant->ViewLeft();
    res[1] += descendant->ViewTop();
    res[0] -= descendant->OffsetXForCalcPosition();
    res[1] -= descendant->OffsetYForCalcPosition();
    descendant = descendant->Parent();
    res[0] -= descendant->ScrollX();
    res[1] -= descendant->ScrollY();
    if (enable_transform) {
      Transform* transform = descendant->GetTransform();
      float width = descendant->width_, height = descendant->height_;
      if (transform) {
        transform->GetTransformMatrix(width, height, 1.f, true)
            .mapPoint(res, res);
      }
    }
  }
}

void LynxUIHelper::ConvertPointFromUIToAnotherUI(float res[2], UIBase* ui,
                                                 UIBase* another,
                                                 float point[2],
                                                 bool enable_transform) {
  if (!ui || !another || ui == another) {
    return;
  }

  if (UIIsParentOfAnotherUI(ui, another)) {
    ConvertPointFromAncestorToDescendant(res, ui, another, point,
                                         enable_transform);
  } else if (UIIsParentOfAnotherUI(another, ui)) {
    ConvertPointFromDescendantToAncestor(res, ui, another, point,
                                         enable_transform);
  } else {
    UIBase* root = ui->GetContext()->Root();
    if (!root) {
      return;
    }
    float root_point[2] = {point[0], point[1]};
    ConvertPointFromDescendantToAncestor(root_point, ui, root, point,
                                         enable_transform);
    ConvertPointFromAncestorToDescendant(res, root, another, root_point,
                                         enable_transform);
  }
}

void LynxUIHelper::ConvertPointFromUIToRootUI(float res[2], UIBase* ui,
                                              float point[2],
                                              bool enable_transform) {
  UIBase* root = ui->GetContext()->Root();
  if (!root) {
    return;
  }
  ConvertPointFromDescendantToAncestor(res, ui, root, point, enable_transform);
}

void LynxUIHelper::ConvertPointFromUIToScreen(float res[2], UIBase* ui,
                                              float point[2],
                                              bool enable_transform) {
  UIRoot* root = ui->GetContext()->Root();
  if (!root) {
    return;
  }
  ConvertPointFromUIToRootUI(res, ui, point, enable_transform);

  ArkUI_IntOffset page_offset;
  if (enable_transform) {
    OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->GetProxyNode(),
                                                        &page_offset);
  } else {
    OH_ArkUI_NodeUtils_GetLayoutPositionInScreen(root->GetProxyNode(),
                                                 &page_offset);
  }
  float scaled_density = root->GetContext()->ScaledDensity();

  res[0] += page_offset.x / scaled_density;
  res[1] += page_offset.y / scaled_density;
}

void LynxUIHelper::ConvertRectFromAncestorToDescendant(float res[4],
                                                       UIBase* ancestor,
                                                       UIBase* descendant,
                                                       float rect[4],
                                                       bool enable_transform) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }

  float point_left_top[2] = {rect[0], rect[1]};
  float converted_point_left_top[2] = {rect[0], rect[1]};
  float point_right_top[2] = {rect[2], rect[1]};
  float converted_point_right_top[2] = {rect[2], rect[1]};
  float point_left_bottom[2] = {rect[0], rect[3]};
  float converted_point_left_bottom[2] = {rect[0], rect[3]};
  float point_right_bottom[2] = {rect[2], rect[3]};
  float converted_point_right_bottom[2] = {rect[2], rect[3]};
  ConvertPointFromAncestorToDescendant(converted_point_left_top, ancestor,
                                       descendant, point_left_top,
                                       enable_transform);
  ConvertPointFromAncestorToDescendant(converted_point_right_top, ancestor,
                                       descendant, point_right_top,
                                       enable_transform);
  ConvertPointFromAncestorToDescendant(converted_point_left_bottom, ancestor,
                                       descendant, point_left_bottom,
                                       enable_transform);
  ConvertPointFromAncestorToDescendant(converted_point_right_bottom, ancestor,
                                       descendant, point_right_bottom,
                                       enable_transform);

  res[0] = fmin(
      fmin(converted_point_left_top[0], converted_point_right_top[0]),
      fmin(converted_point_left_bottom[0], converted_point_right_bottom[0]));
  res[1] = fmin(
      fmin(converted_point_left_top[1], converted_point_right_top[1]),
      fmin(converted_point_left_bottom[1], converted_point_right_bottom[1]));
  res[2] = fmax(
      fmax(converted_point_left_top[0], converted_point_right_top[0]),
      fmax(converted_point_left_bottom[0], converted_point_right_bottom[0]));
  res[3] = fmax(
      fmax(converted_point_left_top[1], converted_point_right_top[1]),
      fmax(converted_point_left_bottom[1], converted_point_right_bottom[1]));
}

void LynxUIHelper::ConvertRectFromDescendantToAncestor(float res[4],
                                                       UIBase* descendant,
                                                       UIBase* ancestor,
                                                       float rect[4],
                                                       bool enable_transform) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }

  float point_left_top[2] = {rect[0], rect[1]};
  float converted_point_left_top[2] = {rect[0], rect[1]};
  float point_right_top[2] = {rect[2], rect[1]};
  float converted_point_right_top[2] = {rect[2], rect[1]};
  float point_left_bottom[2] = {rect[0], rect[3]};
  float converted_point_left_bottom[2] = {rect[0], rect[3]};
  float point_right_bottom[2] = {rect[2], rect[3]};
  float converted_point_right_bottom[2] = {rect[2], rect[3]};
  ConvertPointFromDescendantToAncestor(converted_point_left_top, descendant,
                                       ancestor, point_left_top,
                                       enable_transform);
  ConvertPointFromDescendantToAncestor(converted_point_right_top, descendant,
                                       ancestor, point_right_top,
                                       enable_transform);
  ConvertPointFromDescendantToAncestor(converted_point_left_bottom, descendant,
                                       ancestor, point_left_bottom,
                                       enable_transform);
  ConvertPointFromDescendantToAncestor(converted_point_right_bottom, descendant,
                                       ancestor, point_right_bottom,
                                       enable_transform);

  res[0] = fmin(
      fmin(converted_point_left_top[0], converted_point_right_top[0]),
      fmin(converted_point_left_bottom[0], converted_point_right_bottom[0]));
  res[1] = fmin(
      fmin(converted_point_left_top[1], converted_point_right_top[1]),
      fmin(converted_point_left_bottom[1], converted_point_right_bottom[1]));
  res[2] = fmax(
      fmax(converted_point_left_top[0], converted_point_right_top[0]),
      fmax(converted_point_left_bottom[0], converted_point_right_bottom[0]));
  res[3] = fmax(
      fmax(converted_point_left_top[1], converted_point_right_top[1]),
      fmax(converted_point_left_bottom[1], converted_point_right_bottom[1]));
}

void LynxUIHelper::ConvertRectFromUIToAnotherUI(float res[4], UIBase* ui,
                                                UIBase* another, float rect[4],
                                                bool enable_transform) {
  if (!ui || !another || ui == another) {
    return;
  }

  if (UIIsParentOfAnotherUI(ui, another)) {
    ConvertRectFromAncestorToDescendant(res, ui, another, rect,
                                        enable_transform);
  } else if (UIIsParentOfAnotherUI(another, ui)) {
    ConvertRectFromDescendantToAncestor(res, ui, another, rect,
                                        enable_transform);
  } else {
    UIBase* root = ui->GetContext()->Root();
    if (!root) {
      return;
    }
    float root_rect[4] = {0};
    memcpy(root_rect, rect, sizeof(float) * 4);
    ConvertRectFromDescendantToAncestor(root_rect, ui, root, rect,
                                        enable_transform);
    ConvertRectFromAncestorToDescendant(res, root, another, root_rect,
                                        enable_transform);
  }
}

void LynxUIHelper::ConvertRectFromUIToRootUI(float res[4], UIBase* ui,
                                             float rect[4],
                                             bool enable_transform) {
  UIBase* root = ui->GetContext()->Root();
  if (!root) {
    return;
  }
  ConvertRectFromDescendantToAncestor(res, ui, root, rect, enable_transform);
}

void LynxUIHelper::ConvertRectFromUIToScreen(float res[4], UIBase* ui,
                                             float rect[4],
                                             bool enable_transform) {
  UIRoot* root = ui->GetContext()->Root();
  if (!root) {
    return;
  }
  ConvertRectFromUIToRootUI(res, ui, rect, enable_transform);

  ArkUI_IntOffset page_offset;
  if (enable_transform) {
    OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(root->GetProxyNode(),
                                                        &page_offset);
  } else {
    OH_ArkUI_NodeUtils_GetLayoutPositionInScreen(root->GetProxyNode(),
                                                 &page_offset);
  }

  float scaled_density = root->GetContext()->ScaledDensity();
  float offset_x_lynxview = page_offset.x / scaled_density;
  float offset_y_lynxview = page_offset.y / scaled_density;
  res[0] += offset_x_lynxview;
  res[1] += offset_y_lynxview;
  res[2] += offset_x_lynxview;
  res[3] += offset_y_lynxview;
}

bool LynxUIHelper::CheckViewportIntersectWithRatio(float rect[4],
                                                   float another[4],
                                                   float ratio) {
  float left = fmax(rect[0], another[0]);
  float right = fmin(rect[2], another[2]);
  float top = fmax(rect[1], another[1]);
  float bottom = fmin(rect[3], another[3]);
  if (!base::FloatsLarger(right - left, 0) ||
      !base::FloatsLarger(bottom - top, 0)) {
    return false;
  }
  float intersect_ratio = ((right - left) * (bottom - top)) /
                          (rect[2] - rect[0]) * (rect[3] - rect[1]);
  return base::FloatsLarger(intersect_ratio, ratio);
}

void LynxUIHelper::OffsetRect(float rect[4], float offset[2]) {
  rect[0] += offset[0];
  rect[1] += offset[1];
  rect[2] += offset[0];
  rect[3] += offset[1];
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
