// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"

#include <cstring>
#include <stack>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
#include "base/include/value/array.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

namespace lynx {
namespace tasm {

struct PlatformEventPropNameHash {
  size_t operator()(PlatformEventPropName name) const {
    return static_cast<size_t>(name);
  }
};

using EventPropValueSetter = void (*)(PlatformEventTarget*,
                                      const lepus::Value&);

bool IsOverlayRenderer(const fml::RefPtr<PlatformRendererImpl>& renderer) {
  return renderer != nullptr && renderer->IsOverlay();
}

void CopyPoint(float res[2], const float point[2]) {
  std::memmove(res, point, sizeof(float) * 2);
}

void CopyRect(float res[4], const float rect[4]) {
  std::memmove(res, rect, sizeof(float) * 4);
}

void SetUserInteractionEnabled(PlatformEventTarget* target,
                               const lepus::Value& value) {
  target->SetUserInteractionEnabled(
      !base::IsZero(EventPropValueToFloat(value)));
}

void SetNativeInteractionEnabled(PlatformEventTarget* target,
                                 const lepus::Value& value) {
  target->SetNativeInteractionEnabled(
      !base::IsZero(EventPropValueToFloat(value)));
}

void SetExposureScreenMarginLeft(PlatformEventTarget* target,
                                 const lepus::Value& value) {
  target->SetExposureScreenMarginLeft(EventPropValueToFloat(value));
}

void SetExposureScreenMarginRight(PlatformEventTarget* target,
                                  const lepus::Value& value) {
  target->SetExposureScreenMarginRight(EventPropValueToFloat(value));
}

void SetExposureScreenMarginTop(PlatformEventTarget* target,
                                const lepus::Value& value) {
  target->SetExposureScreenMarginTop(EventPropValueToFloat(value));
}

void SetExposureScreenMarginBottom(PlatformEventTarget* target,
                                   const lepus::Value& value) {
  target->SetExposureScreenMarginBottom(EventPropValueToFloat(value));
}

void SetExposureUIMarginLeft(PlatformEventTarget* target,
                             const lepus::Value& value) {
  target->SetExposureUIMarginLeft(EventPropValueToFloat(value));
}

void SetExposureUIMarginRight(PlatformEventTarget* target,
                              const lepus::Value& value) {
  target->SetExposureUIMarginRight(EventPropValueToFloat(value));
}

void SetExposureUIMarginTop(PlatformEventTarget* target,
                            const lepus::Value& value) {
  target->SetExposureUIMarginTop(EventPropValueToFloat(value));
}

void SetExposureUIMarginBottom(PlatformEventTarget* target,
                               const lepus::Value& value) {
  target->SetExposureUIMarginBottom(EventPropValueToFloat(value));
}

void SetExposureArea(PlatformEventTarget* target, const lepus::Value& value) {
  target->SetExposureArea(EventPropValueToFloat(value));
}

void SetEnableExposureUIClip(PlatformEventTarget* target,
                             const lepus::Value& value) {
  target->SetEnableExposureUIClip(base::IsZero(EventPropValueToFloat(value))
                                      ? LynxEventPropStatus::kDisable
                                      : LynxEventPropStatus::kEnable);
}

LynxEventPropStatus EventPropValueToStatus(const lepus::Value& value) {
  if (value.IsNil() || value.IsUndefined()) {
    return LynxEventPropStatus::kUndefined;
  }
  if (value.IsString()) {
    const auto& string_value = value.StdString();
    if (string_value == "true") {
      return LynxEventPropStatus::kEnable;
    }
    if (string_value == "false") {
      return LynxEventPropStatus::kDisable;
    }
  }
  return base::IsZero(EventPropValueToFloat(value))
             ? LynxEventPropStatus::kDisable
             : LynxEventPropStatus::kEnable;
}

void SetEventThrough(PlatformEventTarget* target, const lepus::Value& value) {
  target->SetEventThrough(EventPropValueToStatus(value));
}

bool ParseEventThroughSizeValue(
    const lepus::Value& value,
    PlatformEventTarget::EventThroughSizeValue* result) {
  if (result == nullptr) {
    return false;
  }
  if (!value.IsString()) {
    return false;
  }

  const std::string string_value = value.StdString();
  const std::string_view string_view = string_value;
  if (string_view.size() >= 2 &&
      string_view.rfind("px") == string_view.size() - 2) {
    int32_t number = 0;
    if (!ParseIntStrict(string_view.substr(0, string_view.size() - 2),
                        &number)) {
      return false;
    }
    result->type = PlatformEventTarget::EventThroughSizeValue::Type::kDevicePx;
    result->value = static_cast<float>(number);
    return true;
  }
  if (!string_view.empty() &&
      string_view.rfind("%") == string_view.size() - 1) {
    int32_t number = 0;
    if (!ParseIntStrict(string_view.substr(0, string_view.size() - 1),
                        &number)) {
      return false;
    }
    result->type =
        PlatformEventTarget::EventThroughSizeValue::Type::kPercentage;
    result->value = static_cast<float>(number) / 100.f;
    return true;
  }
  return false;
}

void ParseEventThroughRegions(
    const lepus::Value& value,
    std::vector<PlatformEventTarget::EventThroughRegion>* regions) {
  if (regions == nullptr || !value.IsArray()) {
    return;
  }

  auto array = value.Array();
  if (!array) {
    return;
  }
  for (size_t i = 0; i < array->size(); ++i) {
    const auto& region_value = array->get(i);
    if (!region_value.IsArray()) {
      continue;
    }
    auto region_array = region_value.Array();
    if (!region_array || region_array->size() != 4) {
      continue;
    }
    PlatformEventTarget::EventThroughRegion region;
    bool valid_region = true;
    for (size_t j = 0; j < region.size(); ++j) {
      if (!ParseEventThroughSizeValue(region_array->get(j), &region[j])) {
        valid_region = false;
        break;
      }
    }
    if (valid_region) {
      regions->push_back(region);
    }
  }
}

void SetEventThroughActiveRegions(PlatformEventTarget* target,
                                  const lepus::Value& value) {
  std::vector<PlatformEventTarget::EventThroughRegion> regions;
  ParseEventThroughRegions(value, &regions);
  target->SetEventThroughActiveRegions(std::move(regions));
}

void SetEventsPassThrough(PlatformEventTarget* target,
                          const lepus::Value& value) {
  target->SetEventsPassThrough(EventPropValueToStatus(value));
}

void SetIDSelector(PlatformEventTarget* target, const lepus::Value& value) {
  if (value.IsString()) {
    target->SetIDSelector(value.StdString());
  }
}

void SetExposureId(PlatformEventTarget* target, const lepus::Value& value) {
  bool has_valid_value = false;
  std::string exposure_id_value;
  if (value.IsString()) {
    exposure_id_value = value.StdString();
    has_valid_value = true;
  } else if (value.IsNumber()) {
    exposure_id_value = std::to_string(value.Number());
    has_valid_value = true;
  }
  if (has_valid_value) {
    target->SetExposureId(exposure_id_value);
  }
}

void SetExposureScene(PlatformEventTarget* target, const lepus::Value& value) {
  if (value.IsString()) {
    target->SetExposureScene(value.StdString());
  }
}

void SetDataset(PlatformEventTarget* target, const lepus::Value& value) {
  target->SetDataset(value);
}

const std::unordered_map<PlatformEventPropName, EventPropValueSetter,
                         PlatformEventPropNameHash>&
GetEventPropSetterMap() {
  static const std::unordered_map<PlatformEventPropName, EventPropValueSetter,
                                  PlatformEventPropNameHash>
      map = {
          {PlatformEventPropName::kUserInteractionEnabled,
           &SetUserInteractionEnabled},
          {PlatformEventPropName::kNativeInteractionEnabled,
           &SetNativeInteractionEnabled},
          {PlatformEventPropName::kExposureScreenMarginLeft,
           &SetExposureScreenMarginLeft},
          {PlatformEventPropName::kExposureScreenMarginRight,
           &SetExposureScreenMarginRight},
          {PlatformEventPropName::kExposureScreenMarginTop,
           &SetExposureScreenMarginTop},
          {PlatformEventPropName::kExposureScreenMarginBottom,
           &SetExposureScreenMarginBottom},
          {PlatformEventPropName::kExposureUIMarginLeft,
           &SetExposureUIMarginLeft},
          {PlatformEventPropName::kExposureUIMarginRight,
           &SetExposureUIMarginRight},
          {PlatformEventPropName::kExposureUIMarginTop,
           &SetExposureUIMarginTop},
          {PlatformEventPropName::kExposureUIMarginBottom,
           &SetExposureUIMarginBottom},
          {PlatformEventPropName::kExposureArea, &SetExposureArea},
          {PlatformEventPropName::kEnableExposureUIClip,
           &SetEnableExposureUIClip},
          {PlatformEventPropName::kIDSelector, &SetIDSelector},
          {PlatformEventPropName::kExposureId, &SetExposureId},
          {PlatformEventPropName::kExposureScene, &SetExposureScene},
          {PlatformEventPropName::kDataset, &SetDataset},
          {PlatformEventPropName::kEventThrough, &SetEventThrough},
          {PlatformEventPropName::kEventThroughActiveRegions,
           &SetEventThroughActiveRegions},
          {PlatformEventPropName::kEventsPassThrough, &SetEventsPassThrough},
      };
  return map;
}

void PlatformEventTargetHelper::ApplyEventBundle(
    const fml::RefPtr<PlatformEventTarget>& target,
    const PlatformEventBundle* bundle) {
  if (target == nullptr || bundle == nullptr) {
    return;
  }

  target->SetEventSet(bundle->EventNames());

  const auto& setter_map = GetEventPropSetterMap();
  for (const auto& it : bundle->EventProps()) {
    const auto prop_name = it.first;
    const auto& value = it.second;
    if (prop_name == PlatformEventPropName::kUnknown) {
      continue;
    }
    auto setter_it = setter_map.find(prop_name);
    if (setter_it != setter_map.end()) {
      setter_it->second(target.get(), value);
    }
  }

  bool has_custom_event = false;
  for (const auto& name : bundle->EventNames()) {
    if (name == PlatformEventName::kUIAppear ||
        name == PlatformEventName::kUIDisappear) {
      has_custom_event = true;
      break;
    }
  }
  bool has_global_event = !target->ExposureId().empty();
  UpdateExposureTargetRegistration(target, has_custom_event, has_global_event);
}

void PlatformEventTargetHelper::UpdateExposureTargetRegistration(
    const fml::RefPtr<PlatformEventTarget>& target, bool has_custom_event,
    bool has_global_event) {
  if (platform_ref_ == nullptr || target == nullptr) {
    return;
  }
  if (!has_custom_event && !has_global_event) {
    return;
  }

  BASE_STATIC_STRING_DECL(kUniqueId, "unique-id");
  BASE_STATIC_STRING_DECL(kIsCustomEvent, "is-custom-event");
  BASE_STATIC_STRING_DECL(kIsGlobalEvent, "is-global-event");
  BASE_STATIC_STRING_DECL(kInterceptGlobalEvent, "intercept-global-event");

  const auto unique_id = std::to_string(target->Sign()) + "_" +
                         target->ExposureId() + "_" + target->ExposureScene();
  auto option = lepus::Dictionary::Create();
  option->SetValue(kUniqueId, unique_id);
  option->SetValue(kIsCustomEvent, has_custom_event);
  option->SetValue(kIsGlobalEvent, has_global_event);
  option->SetValue(kInterceptGlobalEvent, false);

  platform_ref_->AddPlatformEventTargetToExposure(target, lepus::Value(option));
}

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::GetRootEventTarget() {
  return GetEventRootTree(kRootId);
}

fml::RefPtr<PlatformEventTarget> PlatformEventTargetHelper::GetEventTarget(
    int32_t id) {
  if (auto it = event_targets_.find(id); it != event_targets_.end()) {
    return it->second;
  }
  return nullptr;
}

void PlatformEventTargetHelper::RefreshScrollOffsets() {
  RefreshScrollOffsets(GetRootEventTarget());
}

void PlatformEventTargetHelper::RefreshScrollOffsets(
    const fml::RefPtr<PlatformEventTarget>& root) {
  RefreshScrollOffsetsRecursively(root);
}

void PlatformEventTargetHelper::ClearEventTargets() {
  event_targets_.clear();
  event_target_trees_.clear();
}

void PlatformEventTargetHelper::RemoveEventTargetsInEventRoot(int32_t root_id) {
  std::vector<int32_t> removed_signs;
  for (const auto& it : event_targets_) {
    const auto& target = it.second;
    if (target != nullptr && target->RootId() == root_id) {
      removed_signs.push_back(it.first);
    }
  }
  for (auto sign : removed_signs) {
    event_targets_.erase(sign);
  }
}

bool PlatformEventTargetHelper::IsActiveEventRoot(int32_t root_id) const {
  return active_event_root_ids_.count(root_id) > 0;
}

void PlatformEventTargetHelper::AddActiveEventRoot(int32_t root_id) {
  if (root_id == kRootId) {
    return;
  }
  active_event_root_ids_.insert(root_id);
}

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::RemoveActiveEventRoot(int32_t root_id) {
  if (root_id == kRootId) {
    return nullptr;
  }
  active_event_root_ids_.erase(root_id);
  RemoveEventRootOffsetToPageRoot(root_id);
  auto tree = GetEventRootTree(root_id);
  RemoveEventTargetsInEventRoot(root_id);
  event_target_trees_.erase(root_id);
  return tree;
}

void PlatformEventTargetHelper::ClearActiveEventRoots() {
  for (const auto root_id : active_event_root_ids_) {
    RemoveEventTargetsInEventRoot(root_id);
    event_target_trees_.erase(root_id);
  }
  active_event_root_ids_.clear();
  event_root_offsets_to_page_root_.clear();
}

bool PlatformEventTargetHelper::SetEventRootOffsetToPageRoot(int32_t root_id,
                                                             float offset_x,
                                                             float offset_y) {
  if (root_id == kRootId) {
    return false;
  }
  if (auto it = event_root_offsets_to_page_root_.find(root_id);
      it != event_root_offsets_to_page_root_.end() &&
      it->second[0] == offset_x && it->second[1] == offset_y) {
    return false;
  }
  event_root_offsets_to_page_root_.insert_or_assign(
      root_id, std::array<float, 2>{offset_x, offset_y});
  return true;
}

void PlatformEventTargetHelper::RemoveEventRootOffsetToPageRoot(
    int32_t root_id) {
  event_root_offsets_to_page_root_.erase(root_id);
}

fml::RefPtr<PlatformEventTarget> PlatformEventTargetHelper::GetEventRootTree(
    int32_t root_id) const {
  auto tree_it = event_target_trees_.find(root_id);
  return tree_it == event_target_trees_.end() ? nullptr : tree_it->second;
}

void PlatformEventTargetHelper::SetEventRootTree(
    int32_t root_id, const fml::RefPtr<PlatformEventTarget>& root) {
  event_target_trees_.insert_or_assign(root_id, root);
}

void PlatformEventTargetHelper::RefreshScrollOffsetsRecursively(
    const fml::RefPtr<PlatformEventTarget>& target) {
  if (target == nullptr) {
    return;
  }
  target->RefreshScrollOffset();
  for (const auto& child : target->ChildrenTargets()) {
    RefreshScrollOffsetsRecursively(child);
  }
}

bool PlatformEventTargetHelper::IsScrollContainer(PlatformRendererType type,
                                                  int32_t sign) {
  switch (type) {
    case PlatformRendererType::kScroll:
    case PlatformRendererType::kList:
      return true;
    case PlatformRendererType::kExtended:
      return platform_ref_ != nullptr &&
             platform_ref_->IsPlatformRendererScrollable(sign);
    default:
      return false;
  }
}

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::ReconstructEventTargetTreeRecursively(
    fml::RefPtr<PlatformRendererImpl> page_renderer) {
  if (page_renderer == nullptr) {
    return nullptr;
  }
  auto root_id = page_renderer->GetId();
  RemoveEventTargetsInEventRoot(root_id);
  event_target_trees_.erase(root_id);
  auto root = ReconstructEventTargetTreeRecursively(page_renderer, root_id);
  if (root != nullptr) {
    SetEventRootTree(root_id, root);
  }
  return root;
}

fml::RefPtr<PlatformEventTarget>
PlatformEventTargetHelper::ReconstructEventTargetTreeRecursively(
    fml::RefPtr<PlatformRendererImpl> page_renderer, int32_t tree_root_id) {
  if (page_renderer == nullptr) {
    return nullptr;
  }

  auto children_renderer = page_renderer->Children();
  size_t child_renderer_idx = 0, child_renderer_size = children_renderer.size();
  const auto& display_list = page_renderer->GetDisplayList();
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
  fml::RefPtr<PlatformEventTarget> root_event_target = nullptr;
  while (ops_idx < ops_size) {
    int op = ops[ops_idx++];
    int int_param_cnt = int_data[int_data_idx++];
    int float_param_cnt = int_data[int_data_idx++];
    size_t int_param_end = int_data_idx + int_param_cnt;
    size_t float_param_end = float_data_idx + float_param_cnt;
    switch (op) {
      // crate the event target.
      case static_cast<int>(DisplayListOpType::kBegin): {
        int sign = 0;
        auto type = PlatformRendererType::kUnknown;
        float left = 0.f, top = 0.f, width = 0.f, height = 0.f;
        if (int_param_cnt >= 2) {
          sign = int_data[int_data_idx++];
          type = static_cast<PlatformRendererType>(int_data[int_data_idx++]);
        }
        if (float_param_cnt == 4) {
          left = float_data[float_data_idx++];
          top = float_data[float_data_idx++];
          width = float_data[float_data_idx++];
          height = float_data[float_data_idx++];
        }
        auto event_target = fml::MakeRefCounted<PlatformEventTarget>(
            this, tree_root_id, sign, left, top, width, height);
        event_target->SetPlatformRendererType(type);
        event_target->SetScrollContainer(IsScrollContainer(type, sign));
        ApplyEventBundle(event_target,
                         platform_ref_->GetPlatformEventBundle(sign));
        event_targets_[sign] = event_target;
        if (root_event_target == nullptr) {
          root_event_target = event_target;
        }
        target_stack.push(event_target);
        break;
      }
      // create the sub event target tree.
      case static_cast<int>(DisplayListOpType::kDrawView): {
        if (child_renderer_idx < child_renderer_size) {
          auto child_renderer = fml::static_ref_ptr_cast<PlatformRendererImpl>(
              children_renderer[child_renderer_idx++]);
          if (child_renderer->GetId() != tree_root_id &&
              IsOverlayRenderer(child_renderer)) {
            break;
          }
          auto child_target = ReconstructEventTargetTreeRecursively(
              child_renderer, tree_root_id);
          if (target_stack.empty() || child_target == nullptr) {
            break;
          }
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
      default:
        break;
    }
    int_data_idx = int_param_end;
    float_data_idx = float_param_end;
  }

  return root_event_target;
}

bool PlatformEventTargetHelper::TargetIsParentOfAnotherTarget(
    const fml::RefPtr<PlatformEventTarget>& target,
    const fml::RefPtr<PlatformEventTarget>& another) {
  if (!target || !another || target == another) {
    return false;
  }

  auto current = another;
  while (current != nullptr && current->ParentTarget() != nullptr) {
    if (target == current->ParentTarget()) {
      return true;
    }
    current = current->ParentTarget();
  }
  return false;
}

fml::RefPtr<PlatformEventTarget> PlatformEventTargetHelper::GetTreeRoot(
    const fml::RefPtr<PlatformEventTarget>& target) {
  if (target == nullptr) {
    return nullptr;
  }
  const auto root_id = target->RootId();
  return GetEventRootTree(root_id);
}

void PlatformEventTargetHelper::ConvertPointFromAncestorToDescendant(
    float res[2], const fml::RefPtr<PlatformEventTarget>& ancestor,
    const fml::RefPtr<PlatformEventTarget>& descendant, float point[2]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    CopyPoint(res, point);
    return;
  }

  base::InlineVector<fml::RefPtr<PlatformEventTarget>, 3> target_chain;
  fml::RefPtr<PlatformEventTarget> current_target = descendant;
  fml::RefPtr<PlatformEventTarget> current_ancestor = ancestor;
  while (current_target != nullptr && current_target != current_ancestor) {
    target_chain.push_back(current_target);
    current_target = current_target->ParentTarget();
  }

  CopyPoint(res, point);
  int size = static_cast<int>(target_chain.size());
  for (int i = size - 1; i >= 0; --i) {
    current_target = target_chain[i];
    res[0] += current_ancestor->ScrollOffsetX();
    res[1] += current_ancestor->ScrollOffsetY();
    res[0] -= current_target->Left();
    res[1] -= current_target->Top();
    res[0] += current_target->OffsetXForCalcPosition();
    res[1] += current_target->OffsetYForCalcPosition();
    // TODO(hexionghui): add transform support.
    current_ancestor = current_target;
  }
}

void PlatformEventTargetHelper::ConvertPointFromDescendantToAncestor(
    float res[2], const fml::RefPtr<PlatformEventTarget>& descendant,
    const fml::RefPtr<PlatformEventTarget>& ancestor, float point[2]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    CopyPoint(res, point);
    return;
  }

  CopyPoint(res, point);
  // TODO(hexionghui): add transform support for descendant.

  auto current_descendant = descendant;
  while (current_descendant != nullptr && current_descendant->ParentTarget() &&
         current_descendant != ancestor) {
    res[0] += current_descendant->ScrollOffsetX();
    res[1] += current_descendant->ScrollOffsetY();
    res[0] += current_descendant->Left();
    res[1] += current_descendant->Top();
    res[0] -= current_descendant->OffsetXForCalcPosition();
    res[1] -= current_descendant->OffsetYForCalcPosition();
    current_descendant = current_descendant->ParentTarget();
    res[0] -= current_descendant->ScrollOffsetX();
    res[1] -= current_descendant->ScrollOffsetY();
    // TODO(hexionghui): add transform support.
  }
}

void PlatformEventTargetHelper::ConvertPointFromTargetToAnotherTarget(
    float res[2], const fml::RefPtr<PlatformEventTarget>& target,
    const fml::RefPtr<PlatformEventTarget>& another, float point[2]) {
  CopyPoint(res, point);
  if (!target || !another || target == another) {
    return;
  }

  if (TargetIsParentOfAnotherTarget(target, another)) {
    ConvertPointFromAncestorToDescendant(res, target, another, point);
  } else if (TargetIsParentOfAnotherTarget(another, target)) {
    ConvertPointFromDescendantToAncestor(res, target, another, point);
  } else {
    fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
    if (!root) {
      return;
    }
    float root_point[2] = {point[0], point[1]};
    ConvertPointFromDescendantToAncestor(root_point, target, root, point);
    ConvertPointFromAncestorToDescendant(res, root, another, root_point);
  }
}

void PlatformEventTargetHelper::ConvertPointFromTargetToRootTarget(
    float res[2], const fml::RefPtr<PlatformEventTarget>& target,
    float point[2]) {
  fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
  if (!root || !target || target == root) {
    CopyPoint(res, point);
    return;
  }
  ConvertPointFromDescendantToAncestor(res, target, root, point);
}

bool PlatformEventTargetHelper::GetTreeRootOffsetToPageRootTarget(
    const fml::RefPtr<PlatformEventTarget>& target, float offset[2]) {
  offset[0] = 0.f;
  offset[1] = 0.f;
  auto root = GetTreeRoot(target);
  if (!root || !target) {
    return false;
  }
  if (root->RootId() == kRootId) {
    return true;
  }
  if (const auto offset_it =
          event_root_offsets_to_page_root_.find(root->RootId());
      offset_it != event_root_offsets_to_page_root_.end()) {
    offset[0] = offset_it->second[0];
    offset[1] = offset_it->second[1];
    return true;
  }
  offset[0] = root->Left() - root->OffsetXForCalcPosition();
  offset[1] = root->Top() - root->OffsetYForCalcPosition();
  return true;
}

void PlatformEventTargetHelper::ConvertPointFromTargetToPageRootTarget(
    float res[2], const fml::RefPtr<PlatformEventTarget>& target,
    float point[2]) {
  ConvertPointFromTargetToRootTarget(res, target, point);
  float offset[2] = {0.f, 0.f};
  if (GetTreeRootOffsetToPageRootTarget(target, offset)) {
    OffsetPoint(res, offset);
  }
}

void PlatformEventTargetHelper::ConvertPointFromTargetToScreen(
    float res[2], const fml::RefPtr<PlatformEventTarget>& target,
    float point[2]) {
  fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
  if (!root || !target) {
    CopyPoint(res, point);
    return;
  }
  ConvertPointFromTargetToPageRootTarget(res, target, point);

  float origin[2] = {0, 0};
  GetRootViewLocationOnScreen(origin);
  OffsetPoint(res, origin);
}

void PlatformEventTargetHelper::ConvertRectFromAncestorToDescendant(
    float res[4], const fml::RefPtr<PlatformEventTarget>& ancestor,
    const fml::RefPtr<PlatformEventTarget>& descendant, float rect[4]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    CopyRect(res, rect);
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
    float res[4], const fml::RefPtr<PlatformEventTarget>& descendant,
    const fml::RefPtr<PlatformEventTarget>& ancestor, float rect[4]) {
  if (!descendant || !ancestor || descendant == ancestor) {
    CopyRect(res, rect);
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
    float res[4], const fml::RefPtr<PlatformEventTarget>& target,
    const fml::RefPtr<PlatformEventTarget>& another, float rect[4]) {
  CopyRect(res, rect);
  if (!target || !another || target == another) {
    return;
  }

  if (TargetIsParentOfAnotherTarget(target, another)) {
    ConvertRectFromAncestorToDescendant(res, target, another, rect);
  } else if (TargetIsParentOfAnotherTarget(another, target)) {
    ConvertRectFromDescendantToAncestor(res, target, another, rect);
  } else {
    fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
    if (!root) {
      return;
    }
    float root_rect[4] = {rect[0], rect[1], rect[2], rect[3]};
    ConvertRectFromDescendantToAncestor(root_rect, target, root, rect);
    ConvertRectFromAncestorToDescendant(res, root, another, root_rect);
  }
}

void PlatformEventTargetHelper::ConvertRectFromTargetToRootTarget(
    float res[4], const fml::RefPtr<PlatformEventTarget>& target,
    float rect[4]) {
  fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
  if (!root) {
    CopyRect(res, rect);
    return;
  }
  ConvertRectFromDescendantToAncestor(res, target, root, rect);
}

void PlatformEventTargetHelper::ConvertRectFromTargetToPageRootTarget(
    float res[4], const fml::RefPtr<PlatformEventTarget>& target,
    float rect[4]) {
  ConvertRectFromTargetToRootTarget(res, target, rect);
  float offset[2] = {0.f, 0.f};
  if (GetTreeRootOffsetToPageRootTarget(target, offset)) {
    OffsetRect(res, offset);
  }
}

void PlatformEventTargetHelper::ConvertRectFromTargetToScreen(
    float res[4], const fml::RefPtr<PlatformEventTarget>& target,
    float rect[4]) {
  fml::RefPtr<PlatformEventTarget> root = GetTreeRoot(target);
  if (!root) {
    CopyRect(res, rect);
    return;
  }
  ConvertRectFromTargetToPageRootTarget(res, target, rect);

  float origin[2] = {0, 0};
  GetRootViewLocationOnScreen(origin);
  OffsetRect(res, origin);
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

void PlatformEventTargetHelper::OffsetPoint(float point[2],
                                            const float offset[2]) {
  point[0] += offset[0];
  point[1] += offset[1];
}

void PlatformEventTargetHelper::OffsetRect(float rect[4],
                                           const float offset[2]) {
  rect[0] += offset[0];
  rect[1] += offset[1];
  rect[2] += offset[0];
  rect[3] += offset[1];
}

void PlatformEventTargetHelper::GetScreenSize(float size[2]) {
  platform_ref_->GetScreenSize(size);
}

void PlatformEventTargetHelper::GetRootViewLocationOnScreen(float location[2]) {
  platform_ref_->GetRootViewLocationOnScreen(location);
}

void PlatformEventTargetHelper::GetPlatformRendererScrollOffset(
    int32_t sign, float offset[2]) {
  platform_ref_->GetPlatformRendererScrollOffset(sign, offset);
}

void PlatformEventTargetHelper::InvokeMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (method == "boundingClientRect") {
    if (auto it = event_targets_.find(id); it != event_targets_.end()) {
      auto event_target = it->second;
      float result[4] = {0, 0, event_target->Width(), event_target->Height()};
      ConvertRectFromTargetToPageRootTarget(result, event_target, result);

      BASE_STATIC_STRING_DECL(kId, "id");
      BASE_STATIC_STRING_DECL(kDataset, "dataset");
      BASE_STATIC_STRING_DECL(kLeft, "left");
      BASE_STATIC_STRING_DECL(kTop, "top");
      BASE_STATIC_STRING_DECL(kRight, "right");
      BASE_STATIC_STRING_DECL(kBottom, "bottom");
      BASE_STATIC_STRING_DECL(kWidth, "width");
      BASE_STATIC_STRING_DECL(kHeight, "height");

      auto ret = lepus::Dictionary::Create();
      ret->SetValue(kId, event_target->IDSelector());
      auto dataset = event_target->Dataset();
      ret->SetValue(kDataset, dataset.IsEmpty()
                                  ? lepus::Value(lepus::Dictionary::Create())
                                  : dataset);
      ret->SetValue(kLeft, result[0] / device_pixel_ratio_);
      ret->SetValue(kTop, result[1] / device_pixel_ratio_);
      ret->SetValue(kRight, result[2] / device_pixel_ratio_);
      ret->SetValue(kBottom, result[3] / device_pixel_ratio_);
      ret->SetValue(kWidth, (result[2] - result[0]) / device_pixel_ratio_);
      ret->SetValue(kHeight, (result[3] - result[1]) / device_pixel_ratio_);
      callback(LynxGetUIResult::SUCCESS, lepus::Value(ret));
    } else {
      callback(LynxGetUIResult::NODE_NOT_FOUND,
               lepus::Value("node not found: " + std::to_string(id)));
    }
    return;
  }

  callback(LynxGetUIResult::UNKNOWN,
           lepus::Value("method not supported: " + method));
}

}  // namespace tasm
}  // namespace lynx
