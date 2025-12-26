// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"

#include <stack>

#include "base/include/float_comparison.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx {
namespace tasm {

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::GetRootEventTarget() {
  return event_target_tree_;
}

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::ReconstructEventTargetTreeRecursively(
    fml::RefPtr<PlatformRendererImpl> page_renderer) {
  if (page_renderer == nullptr) {
    return nullptr;
  }

  auto children_renderer = page_renderer->Children();
  size_t child_renderer_idx = 0, child_renderer_size = children_renderer.size();
  const auto& display_list = page_renderer->GetDisplayList();
  auto renderer_offset = display_list.GetRenderOffset();
  auto ops = display_list.GetContentOpTypesData();
  auto int_data = display_list.GetContentIntData();
  auto float_data = display_list.GetContentFloatData();
  size_t ops_size = display_list.GetContentOpTypesSize();
  size_t ops_idx = 0, int_data_idx = 0, float_data_idx = 0;

  if (ops == nullptr) {
    return nullptr;
  }

  // the top of the stack is always the parent event target.
  std::stack<fml::RefPtr<PlatformEventTarget>> target_stack;
  while (ops_idx < ops_size) {
    int op = ops[ops_idx++];
    int int_param_cnt = int_data[int_data_idx++];
    int float_param_cnt = int_data[int_data_idx++];
    switch (op) {
      // crate the event target.
      case static_cast<int>(DisplayListOpType::kBegin): {
        int sign = 0;
        float left = 0.f, top = 0.f, width = 0.f, height = 0.f;
        if (int_param_cnt == 1) {
          sign = int_data[int_data_idx++];
        }
        if (float_param_cnt == 4) {
          left = float_data[float_data_idx++];
          top = float_data[float_data_idx++];
          width = float_data[float_data_idx++];
          height = float_data[float_data_idx++];
        }
        auto event_target = fml::MakeRefCounted<PlatformEventTarget>(
            sign, left, top, width, height);
        // the root event target.
        if (ops_idx == 1) {
          event_target->SetRendererOffsetX(renderer_offset[0]);
          event_target->SetRendererOffsetY(renderer_offset[1]);
          event_target_tree_ = event_target;
        }
        target_stack.push(event_target);
        break;
      }
      // create the sub event target tree.
      case static_cast<int>(DisplayListOpType::kDrawView): {
        int_data_idx += int_param_cnt;
        if (child_renderer_idx < child_renderer_size) {
          auto child_renderer = fml::static_ref_ptr_cast<PlatformRendererImpl>(
              children_renderer[child_renderer_idx++]);
          auto child_target =
              ReconstructEventTargetTreeRecursively(child_renderer);
          auto parent_target = target_stack.top();
          parent_target->AddChildTarget(child_target);
        }
        break;
      }
      // connect the child event target to the parent event target.
      case static_cast<int>(DisplayListOpType::kEnd): {
        auto child_target = target_stack.top();
        target_stack.pop();
        if (!target_stack.empty()) {
          auto parent_target = target_stack.top();
          parent_target->AddChildTarget(child_target);
        }
        break;
      }
      case static_cast<int>(DisplayListOpType::kFill):
      case static_cast<int>(DisplayListOpType::kText):
      case static_cast<int>(DisplayListOpType::kImage):
      case static_cast<int>(DisplayListOpType::kBorder):
      case static_cast<int>(DisplayListOpType::kClipRect): {
        int_data_idx += int_param_cnt;
        float_data_idx += float_param_cnt;
        break;
      }
      default:
        break;
    }
  }

  return event_target_tree_;
}

bool PlatformEventTargetHelper::TargetIsParentOfAnotherTarget(
    fml::RefPtr<PlatformEventTarget> target,
    fml::RefPtr<PlatformEventTarget> another) {
  if (!target || !another || target == another) {
    return false;
  }

  while (another != nullptr && another->ParentTarget() != nullptr) {
    if (target == another->ParentTarget()) {
      return true;
    }
    another = another->ParentTarget();
  }
  return false;
}

void PlatformEventTargetHelper::ConvertPointFromAncestorToDescendant(
    float res[2], fml::RefPtr<PlatformEventTarget> ancestor,
    fml::RefPtr<PlatformEventTarget> descendant, float point[2]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }

  base::InlineVector<fml::RefPtr<PlatformEventTarget>, 3> target_chain;
  fml::RefPtr<PlatformEventTarget> current_target = descendant;
  while (current_target != nullptr && current_target != ancestor) {
    target_chain.push_back(current_target);
    current_target = current_target->ParentTarget();
  }

  memcpy(res, point, sizeof(float) * 2);
  int size = static_cast<int>(target_chain.size());
  for (int i = size - 1; i >= 0; --i) {
    current_target = target_chain[i];
    res[0] += ancestor->ScrollOffsetX();
    res[1] += ancestor->ScrollOffsetY();
    res[0] -= current_target->RendererOffsetX();
    res[1] -= current_target->RendererOffsetY();
    res[0] -= current_target->Left();
    res[1] -= current_target->Top();
    res[0] += current_target->OffsetXForCalcPosition();
    res[1] += current_target->OffsetYForCalcPosition();
    // TODO(hexionghui): add transform support.
    ancestor = current_target;
  }
}

void PlatformEventTargetHelper::ConvertPointFromDescendantToAncestor(
    float res[2], fml::RefPtr<PlatformEventTarget> descendant,
    fml::RefPtr<PlatformEventTarget> ancestor, float point[2]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }

  memcpy(res, point, sizeof(float) * 2);
  // TODO(hexionghui): add transform support for descendant.

  while (descendant != nullptr && descendant->ParentTarget() &&
         descendant != ancestor) {
    res[0] += descendant->ScrollOffsetX();
    res[1] += descendant->ScrollOffsetY();
    res[0] += descendant->Left();
    res[1] += descendant->Top();
    res[0] -= descendant->OffsetXForCalcPosition();
    res[1] -= descendant->OffsetYForCalcPosition();
    descendant = descendant->ParentTarget();
    res[0] -= descendant->ScrollOffsetX();
    res[1] -= descendant->ScrollOffsetY();
    // TODO(hexionghui): add transform support.
  }
}

void PlatformEventTargetHelper::ConvertPointFromTargetToAnotherTarget(
    float res[2], fml::RefPtr<PlatformEventTarget> target,
    fml::RefPtr<PlatformEventTarget> another, float point[2]) {
  memcpy(res, point, sizeof(float) * 2);
  if (!target || !another || target == another) {
    return;
  }

  if (TargetIsParentOfAnotherTarget(target, another)) {
    ConvertPointFromAncestorToDescendant(res, target, another, point);
  } else if (TargetIsParentOfAnotherTarget(another, target)) {
    ConvertPointFromDescendantToAncestor(res, target, another, point);
  } else {
    fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
    if (!root) {
      return;
    }
    float root_point[2] = {point[0], point[1]};
    ConvertPointFromDescendantToAncestor(root_point, target, root, point);
    ConvertPointFromAncestorToDescendant(res, root, another, root_point);
  }
}

void PlatformEventTargetHelper::ConvertPointFromTargetToRootTarget(
    float res[2], fml::RefPtr<PlatformEventTarget> target, float point[2]) {
  fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
  if (!root || !target || target == root) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }
  ConvertPointFromDescendantToAncestor(res, target, root, point);
}

void PlatformEventTargetHelper::ConvertPointFromTargetToScreen(
    float res[2], fml::RefPtr<PlatformEventTarget> target, float point[2]) {
  fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
  if (!root || !target) {
    memcpy(res, point, sizeof(float) * 2);
    return;
  }
  ConvertPointFromTargetToRootTarget(res, target, point);

  // TODO(hexionghui): add root offset.
}

void PlatformEventTargetHelper::ConvertRectFromAncestorToDescendant(
    float res[4], fml::RefPtr<PlatformEventTarget> ancestor,
    fml::RefPtr<PlatformEventTarget> descendant, float rect[4]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }

  // rect: [left, top, right, bottom]
  float point_left_top[2] = {rect[0], rect[1]};
  float converted_point_left_top[2] = {rect[0], rect[1]};
  float point_right_top[2] = {rect[2], rect[1]};
  float converted_point_right_top[2] = {rect[2], rect[1]};
  float point_left_bottom[2] = {rect[0], rect[3]};
  float converted_point_left_bottom[2] = {rect[0], rect[3]};
  float point_right_bottom[2] = {rect[2], rect[3]};
  float converted_point_right_bottom[2] = {rect[2], rect[3]};
  ConvertPointFromAncestorToDescendant(converted_point_left_top, ancestor,
                                       descendant, point_left_top);
  ConvertPointFromAncestorToDescendant(converted_point_right_top, ancestor,
                                       descendant, point_right_top);
  ConvertPointFromAncestorToDescendant(converted_point_left_bottom, ancestor,
                                       descendant, point_left_bottom);
  ConvertPointFromAncestorToDescendant(converted_point_right_bottom, ancestor,
                                       descendant, point_right_bottom);

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

void PlatformEventTargetHelper::ConvertRectFromDescendantToAncestor(
    float res[4], fml::RefPtr<PlatformEventTarget> descendant,
    fml::RefPtr<PlatformEventTarget> ancestor, float rect[4]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }

  // rect: [left, top, right, bottom]
  float point_left_top[2] = {rect[0], rect[1]};
  float converted_point_left_top[2] = {rect[0], rect[1]};
  float point_right_top[2] = {rect[2], rect[1]};
  float converted_point_right_top[2] = {rect[2], rect[1]};
  float point_left_bottom[2] = {rect[0], rect[3]};
  float converted_point_left_bottom[2] = {rect[0], rect[3]};
  float point_right_bottom[2] = {rect[2], rect[3]};
  float converted_point_right_bottom[2] = {rect[2], rect[3]};
  ConvertPointFromDescendantToAncestor(converted_point_left_top, descendant,
                                       ancestor, point_left_top);
  ConvertPointFromDescendantToAncestor(converted_point_right_top, descendant,
                                       ancestor, point_right_top);
  ConvertPointFromDescendantToAncestor(converted_point_left_bottom, descendant,
                                       ancestor, point_left_bottom);
  ConvertPointFromDescendantToAncestor(converted_point_right_bottom, descendant,
                                       ancestor, point_right_bottom);

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

void PlatformEventTargetHelper::ConvertRectFromTargetToAnotherTarget(
    float res[4], fml::RefPtr<PlatformEventTarget> target,
    fml::RefPtr<PlatformEventTarget> another, float rect[4]) {
  memcpy(res, rect, sizeof(float) * 4);
  if (!target || !another || target == another) {
    return;
  }

  if (TargetIsParentOfAnotherTarget(target, another)) {
    ConvertRectFromAncestorToDescendant(res, target, another, rect);
  } else if (TargetIsParentOfAnotherTarget(another, target)) {
    ConvertRectFromDescendantToAncestor(res, target, another, rect);
  } else {
    fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
    if (!root) {
      return;
    }
    float root_rect[4] = {rect[0], rect[1], rect[2], rect[3]};
    ConvertRectFromDescendantToAncestor(root_rect, target, root, rect);
    ConvertRectFromAncestorToDescendant(res, root, another, root_rect);
  }
}

void PlatformEventTargetHelper::ConvertRectFromTargetToRootTarget(
    float res[4], fml::RefPtr<PlatformEventTarget> target, float rect[4]) {
  fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
  if (!root) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }
  ConvertRectFromDescendantToAncestor(res, target, root, rect);
}

void PlatformEventTargetHelper::ConvertRectFromTargetToScreen(
    float res[4], fml::RefPtr<PlatformEventTarget> target, float rect[4]) {
  fml::RefPtr<PlatformEventTarget> root = event_target_tree_;
  if (!root) {
    memcpy(res, rect, sizeof(float) * 4);
    return;
  }
  ConvertRectFromDescendantToAncestor(res, target, root, rect);

  // TODO(hexionghui): add root offset.
}

bool PlatformEventTargetHelper::CheckViewportIntersectWithRatio(
    float rect[4], float another[4], float ratio) {
  float left = fmax(rect[0], another[0]);
  float right = fmin(rect[2], another[2]);
  float top = fmax(rect[1], another[1]);
  float bottom = fmin(rect[3], another[3]);
  if (!base::FloatsLarger(right - left, 0) ||
      !base::FloatsLarger(bottom - top, 0)) {
    return false;
  }
  float intersect_ratio = ((right - left) * (bottom - top)) /
                          ((rect[2] - rect[0]) * (rect[3] - rect[1]));
  return !base::IsZero(intersect_ratio) &&
         base::FloatsLargerOrEqual(intersect_ratio, ratio);
}

void PlatformEventTargetHelper::OffsetRect(float rect[4], float offset[2]) {
  rect[0] += offset[0];
  rect[1] += offset[1];
  rect[2] += offset[0];
  rect[3] += offset[1];
}

}  // namespace tasm
}  // namespace lynx
