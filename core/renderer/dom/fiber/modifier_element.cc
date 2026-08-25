// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <array>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {

namespace {

// Modifier IR operation codes shared with the JavaScript and native producers.
constexpr int kModifierOpConcat = 1;
constexpr int kModifierOpPadding = 7;
constexpr int kModifierOpSize = 8;
constexpr int kModifierOpFill = 9;
constexpr int kModifierOpAspectRatio = 10;

constexpr int kLayoutModifierAxisWidth = 1;
constexpr int kLayoutModifierAxisHeight = 1 << 1;
constexpr int kLayoutModifierAllAxes =
    kLayoutModifierAxisWidth | kLayoutModifierAxisHeight;

// Cap total visited nodes (not per-path depth) so a malformed cyclic or
// branching concat chain stays bounded while long linear chains still parse.
constexpr int kMaxModifierNodeVisits = 512;
constexpr size_t kNoModifierFrame = std::numeric_limits<size_t>::max();

BASE_STATIC_STRING_DECL(kOp, "op");
BASE_STATIC_STRING_DECL(kPrevious, "previous");
BASE_STATIC_STRING_DECL(kLeft, "left");
BASE_STATIC_STRING_DECL(kRight, "right");
BASE_STATIC_STRING_DECL(kSpecifiedAxes, "specifiedAxes");
BASE_STATIC_STRING_DECL(kStart, "start");
BASE_STATIC_STRING_DECL(kTop, "top");
BASE_STATIC_STRING_DECL(kEnd, "end");
BASE_STATIC_STRING_DECL(kBottom, "bottom");
BASE_STATIC_STRING_DECL(kWidth, "width");
BASE_STATIC_STRING_DECL(kHeight, "height");
BASE_STATIC_STRING_DECL(kFraction, "fraction");
BASE_STATIC_STRING_DECL(kRatio, "ratio");

struct ModifierOperation {
  int op{0};
  size_t carrier_index{0};
  size_t target_frame_index{kNoModifierFrame};
  int specified_axes{0};
  int applied_axes{0};
  bool apply_aspect_ratio{false};
  std::array<double, 4> layout_values{};
};

struct ModifierFramePlan {
  std::vector<ModifierOperation> operations;
  std::vector<int> frame_child_constrained_axes;
  size_t frame_count{0};
  bool owner_uses_layout_box{false};
};

lepus::Value ReadModifierProperty(const lepus::Value& node,
                                  const base::String& key) {
  // Modifier objects may arrive from the JavaScript runtime with primitive
  // properties represented as JSValue wrappers. Normalize one primitive at a
  // time without deeply converting the linked/callable Modifier graph.
  return node.GetProperty(key).ToLepusValue();
}

bool ReadIntegerProperty(const lepus::Value& node, const base::String& key,
                         int* result) {
  const auto value = ReadModifierProperty(node, key);
  if (!value.IsNumber() || !std::isfinite(value.Number()) ||
      std::trunc(value.Number()) != value.Number() ||
      value.Number() < std::numeric_limits<int>::min() ||
      value.Number() > std::numeric_limits<int>::max()) {
    return false;
  }
  *result = static_cast<int>(value.Number());
  return true;
}

bool ReadFiniteNumberProperty(const lepus::Value& node, const base::String& key,
                              double* result) {
  const auto value = ReadModifierProperty(node, key);
  if (!value.IsNumber() || !std::isfinite(value.Number())) {
    return false;
  }
  *result = value.Number();
  return true;
}

bool IsLayoutOperation(int op) {
  switch (op) {
    case kModifierOpPadding:
    case kModifierOpSize:
    case kModifierOpFill:
    case kModifierOpAspectRatio:
      return true;
    default:
      return false;
  }
}

bool ParseLayoutOperation(const lepus::Value& node,
                          ModifierOperation* operation, std::string* error) {
  auto& values = operation->layout_values;
  switch (operation->op) {
    case kModifierOpPadding:
      if (!ReadFiniteNumberProperty(node, kStart, &values[0]) ||
          values[0] < 0 || !ReadFiniteNumberProperty(node, kTop, &values[1]) ||
          values[1] < 0 || !ReadFiniteNumberProperty(node, kEnd, &values[2]) ||
          values[2] < 0 ||
          !ReadFiniteNumberProperty(node, kBottom, &values[3]) ||
          values[3] < 0) {
        *error = "invalid padding parameters";
        return false;
      }
      return true;
    case kModifierOpSize:
      if (!ReadIntegerProperty(node, kSpecifiedAxes,
                               &operation->specified_axes) ||
          operation->specified_axes <= 0 ||
          (operation->specified_axes & ~kLayoutModifierAllAxes) != 0 ||
          !ReadFiniteNumberProperty(node, kWidth, &values[0]) ||
          values[0] < 0 ||
          !ReadFiniteNumberProperty(node, kHeight, &values[1]) ||
          values[1] < 0) {
        *error = "invalid size parameters";
        return false;
      }
      return true;
    case kModifierOpFill:
      if (!ReadIntegerProperty(node, kSpecifiedAxes,
                               &operation->specified_axes) ||
          operation->specified_axes <= 0 ||
          (operation->specified_axes & ~kLayoutModifierAllAxes) != 0 ||
          !ReadFiniteNumberProperty(node, kFraction, &values[0]) ||
          values[0] < 0 || values[0] > 1) {
        *error = "invalid fill parameters";
        return false;
      }
      return true;
    case kModifierOpAspectRatio:
      if (!ReadFiniteNumberProperty(node, kRatio, &values[0]) ||
          values[0] <= 0) {
        *error = "invalid aspectRatio parameters";
        return false;
      }
      return true;
    default:
      *error = "unsupported layout operation";
      return false;
  }
}

bool ParseModifierOperation(const lepus::Value& node,
                            ModifierOperation* operation, std::string* error) {
  switch (operation->op) {
    case kModifierOpPadding:
    case kModifierOpSize:
    case kModifierOpFill:
    case kModifierOpAspectRatio:
      return ParseLayoutOperation(node, operation, error);
    default:
      *error = "unsupported Modifier operation";
      return false;
  }
}

bool FlattenModifierNode(const lepus::Value& node,
                         std::vector<ModifierOperation>* operations,
                         int& remaining_visits, std::string* error) {
  if (node.IsEmpty()) {
    return true;
  }
  if (!node.IsObject()) {
    *error = "chain link should be an object";
    return false;
  }
  if (remaining_visits <= 0) {
    *error = "Modifier chain exceeds the visit limit or contains a cycle";
    return false;
  }
  --remaining_visits;

  const auto op_value = ReadModifierProperty(node, kOp);
  if (op_value.IsEmpty()) {
    return FlattenModifierNode(node.GetProperty(kPrevious), operations,
                               remaining_visits, error);
  }

  int op = 0;
  if (!op_value.IsNumber() || !std::isfinite(op_value.Number()) ||
      std::trunc(op_value.Number()) != op_value.Number() ||
      op_value.Number() < std::numeric_limits<int>::min() ||
      op_value.Number() > std::numeric_limits<int>::max()) {
    *error = "op should be an integer";
    return false;
  }
  op = static_cast<int>(op_value.Number());
  if (op == kModifierOpConcat) {
    return FlattenModifierNode(node.GetProperty(kLeft), operations,
                               remaining_visits, error) &&
           FlattenModifierNode(node.GetProperty(kRight), operations,
                               remaining_visits, error);
  }
  if (!FlattenModifierNode(node.GetProperty(kPrevious), operations,
                           remaining_visits, error)) {
    return false;
  }

  ModifierOperation operation;
  operation.op = op;
  if (!ParseModifierOperation(node, &operation, error)) {
    return false;
  }
  operations->push_back(std::move(operation));
  return true;
}

bool BuildModifierFramePlan(const lepus::Value& modifier_tail,
                            ModifierFramePlan* plan, std::string* error,
                            size_t* error_position) {
  int remaining_visits = kMaxModifierNodeVisits;
  if (!FlattenModifierNode(modifier_tail, &plan->operations, remaining_visits,
                           error)) {
    *error_position = plan->operations.size();
    return false;
  }

  for (const auto& operation : plan->operations) {
    if (operation.op == kModifierOpPadding) {
      ++plan->frame_count;
    }
  }
  plan->frame_child_constrained_axes.resize(plan->frame_count);

  // Padding ends one carrier segment and is the only operation that creates a
  // physical frame. The suffix after the last padding is carried by the owner.
  size_t carrier_index = 0;
  int constrained_axes = 0;
  for (size_t position = 0; position < plan->operations.size(); ++position) {
    auto& operation = plan->operations[position];
    operation.carrier_index = carrier_index;
    const size_t target_frame_index =
        carrier_index < plan->frame_count ? carrier_index : kNoModifierFrame;

    if (IsLayoutOperation(operation.op)) {
      operation.target_frame_index = target_frame_index;
      switch (operation.op) {
        case kModifierOpPadding:
          plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
          ++carrier_index;
          break;
        case kModifierOpSize:
        case kModifierOpFill:
          operation.applied_axes = operation.specified_axes & ~constrained_axes;
          if (operation.op == kModifierOpFill &&
              operation.layout_values[0] != 1.0 &&
              operation.applied_axes != 0 && carrier_index != 0) {
            *error =
                "fractional fill inside a padding frame is not representable";
            *error_position = position;
            return false;
          }
          constrained_axes |= operation.applied_axes;
          if (target_frame_index == kNoModifierFrame &&
              operation.applied_axes != 0) {
            plan->owner_uses_layout_box = true;
          }
          break;
        case kModifierOpAspectRatio:
          if (constrained_axes == kLayoutModifierAxisWidth) {
            operation.applied_axes = kLayoutModifierAxisHeight;
          } else if (constrained_axes == kLayoutModifierAxisHeight) {
            operation.applied_axes = kLayoutModifierAxisWidth;
          } else if (constrained_axes == kLayoutModifierAllAxes) {
            break;
          } else {
            *error =
                "aspect ratio without one constrained axis is not "
                "representable";
            *error_position = position;
            return false;
          }
          operation.apply_aspect_ratio = true;
          constrained_axes |= operation.applied_axes;
          if (target_frame_index == kNoModifierFrame) {
            plan->owner_uses_layout_box = true;
          }
          break;
        default:
          break;
      }
    }
  }
  if (!plan->frame_child_constrained_axes.empty() &&
      plan->frame_child_constrained_axes.back() != 0) {
    plan->owner_uses_layout_box = true;
  }
  return true;
}

Element* LayoutCarrierAt(
    size_t index, Element* owner,
    const std::vector<fml::RefPtr<ModifierElement>>& frames) {
  return index < frames.size() ? frames[index].get() : owner;
}

lepus::Value CSSUnitValue(double value, const char* unit) {
  std::ostringstream css_value;
  css_value << value << unit;
  return lepus::Value(css_value.str());
}

void ApplyLayoutOperation(const ModifierOperation& operation,
                          Element* carrier) {
  const auto& values = operation.layout_values;
  switch (operation.op) {
    case kModifierOpPadding:
      carrier->SetStyle(kPropertyIDPaddingLeft, CSSUnitValue(values[0], "px"));
      carrier->SetStyle(kPropertyIDPaddingTop, CSSUnitValue(values[1], "px"));
      carrier->SetStyle(kPropertyIDPaddingRight, CSSUnitValue(values[2], "px"));
      carrier->SetStyle(kPropertyIDPaddingBottom,
                        CSSUnitValue(values[3], "px"));
      break;
    case kModifierOpSize:
      if ((operation.applied_axes & kLayoutModifierAxisWidth) != 0) {
        carrier->SetStyle(kPropertyIDWidth, CSSUnitValue(values[0], "px"));
      }
      if ((operation.applied_axes & kLayoutModifierAxisHeight) != 0) {
        carrier->SetStyle(kPropertyIDHeight, CSSUnitValue(values[1], "px"));
      }
      break;
    case kModifierOpFill: {
      const auto value = CSSUnitValue(values[0] * 100, "%");
      if ((operation.applied_axes & kLayoutModifierAxisWidth) != 0) {
        carrier->SetStyle(kPropertyIDWidth, value);
      }
      if ((operation.applied_axes & kLayoutModifierAxisHeight) != 0) {
        carrier->SetStyle(kPropertyIDHeight, value);
      }
      break;
    }
    case kModifierOpAspectRatio:
      if (operation.apply_aspect_ratio) {
        carrier->SetStyle(kPropertyIDAspectRatio, lepus::Value(values[0]));
      }
      break;
    default:
      break;
  }
}

void ApplyLayoutChildConstraint(int constrained_axes, Element* child) {
  if ((constrained_axes & kLayoutModifierAxisWidth) != 0) {
    child->SetStyle(kPropertyIDWidth, lepus::Value("100%"));
  }
  if ((constrained_axes & kLayoutModifierAxisHeight) != 0) {
    child->SetStyle(kPropertyIDHeight, lepus::Value("100%"));
  }
}

void ApplyLayoutOperations(
    const ModifierFramePlan& plan,
    const std::vector<fml::RefPtr<ModifierElement>>& frames, Element* owner) {
  if (plan.owner_uses_layout_box) {
    owner->SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
  }
  for (const auto& operation : plan.operations) {
    if (!IsLayoutOperation(operation.op)) {
      continue;
    }
    ApplyLayoutOperation(
        operation, LayoutCarrierAt(operation.carrier_index, owner, frames));
    if (operation.op == kModifierOpFill && operation.layout_values[0] == 1.0) {
      for (size_t index = 0; index < operation.carrier_index; ++index) {
        auto* carrier = LayoutCarrierAt(index, owner, frames);
        if ((operation.applied_axes & kLayoutModifierAxisWidth) != 0) {
          carrier->SetStyle(kPropertyIDWidth, lepus::Value("100%"));
        }
        if ((operation.applied_axes & kLayoutModifierAxisHeight) != 0) {
          carrier->SetStyle(kPropertyIDHeight, lepus::Value("100%"));
        }
      }
    }
  }
}

void ApplyLayoutChildConstraints(
    const ModifierFramePlan& plan,
    const std::vector<fml::RefPtr<ModifierElement>>& frames, Element* owner) {
  for (size_t frame_index = 0; frame_index < frames.size(); ++frame_index) {
    Element* child =
        frame_index + 1 < frames.size() ? frames[frame_index + 1].get() : owner;
    ApplyLayoutChildConstraint(plan.frame_child_constrained_axes[frame_index],
                               child);
  }
}

void ClearModifierOwnedProperties(Element* element) {
  element->RemoveAllInlineStyles();
  element->RemoveAllModifierAttributes();
  element->GetEventListenerMap()->Clear();
  element->RemoveAllEvents();
}

void NotifyNodeCreated(Element* node) {
  EXEC_EXPR_FOR_INSPECTOR(
      node->element_manager()->PrepareNodeForInspector(node));
}

void NotifyNodeAdded(Element* node) {
  EXEC_EXPR_FOR_INSPECTOR({
    node->element_manager()->CheckAndProcessSlotForInspector(node);
    node->element_manager()->OnElementNodeAddedForInspector(node);
  });
}

void NotifyNodeRemoved(Element* node) {
  EXEC_EXPR_FOR_INSPECTOR(
      node->element_manager()->OnElementNodeRemovedForInspector(node));
}

fml::RefPtr<ModifierElement> CreateFrame(Element* owner) {
  auto* manager = owner->element_manager();
  auto frame = manager->CreateFiberModifierElement();
  frame->SetParentComponentUniqueIdForFiber(
      owner->GetParentComponentUniqueIdForFiber());
  frame->set_style_sheet_manager(owner->style_sheet_manager());
  NotifyNodeCreated(frame.get());
  return frame;
}

bool ValidateMountRoot(Element* owner, Element* current_mount_root,
                       std::string* error) {
  if (owner == nullptr || current_mount_root == nullptr) {
    *error = "owner and current mount root should be Fiber Elements";
    return false;
  }
  if (owner->will_destroy() || current_mount_root->will_destroy() ||
      owner->element_manager() == nullptr ||
      owner->element_manager() != current_mount_root->element_manager()) {
    *error = "owner and current mount root should use the same ElementManager";
    return false;
  }
  if (current_mount_root == owner) {
    if (owner->parent() != nullptr && owner->parent()->is_modifier()) {
      *error = "owner is already mounted below a ModifierElement root";
      return false;
    }
    return true;
  }

  if (!current_mount_root->is_modifier()) {
    *error = "current mount root should be the owner or a ModifierElement";
    return false;
  }
  if (current_mount_root->parent() != nullptr &&
      current_mount_root->parent()->is_modifier()) {
    *error = "current ModifierElement should be the outermost frame";
    return false;
  }

  int remaining_visits = kMaxModifierNodeVisits;
  Element* frame = current_mount_root;
  while (frame != owner) {
    if (remaining_visits-- <= 0 || frame == nullptr || frame->will_destroy() ||
        !frame->is_modifier() ||
        frame->element_manager() != owner->element_manager() ||
        frame->GetChildCount() != 1) {
      *error = "current ModifierElement frame chain is incomplete";
      return false;
    }
    frame = frame->GetChildAt(0);
  }
  return true;
}

fml::RefPtr<Element> InstallModifierFramePlan(const ModifierFramePlan& plan,
                                              Element* owner,
                                              Element* current_mount_root) {
  if (plan.frame_count == 0 && current_mount_root == owner) {
    const std::vector<fml::RefPtr<ModifierElement>> frames;
    ClearModifierOwnedProperties(owner);
    ApplyLayoutOperations(plan, frames, owner);
    return fml::RefPtr<Element>(owner);
  }

  fml::RefPtr<Element> old_root(current_mount_root);
  Element* external_parent = old_root->parent();
  const int32_t old_index = external_parent == nullptr
                                ? -1
                                : external_parent->IndexOf(old_root.get());

  std::vector<fml::RefPtr<ModifierElement>> frames;
  frames.reserve(plan.frame_count);
  for (size_t index = 0; index < plan.frame_count; ++index) {
    frames.push_back(CreateFrame(owner));
  }

  if (external_parent != nullptr) {
    NotifyNodeRemoved(old_root.get());
    external_parent->RemoveNode(old_root, false);
  }
  if (current_mount_root != owner) {
    auto owner_ref = fml::RefPtr<Element>(owner);
    NotifyNodeRemoved(owner);
    owner->parent()->RemoveNode(owner_ref, false);
  }

  ClearModifierOwnedProperties(owner);

  ApplyLayoutOperations(plan, frames, owner);
  ApplyLayoutChildConstraints(plan, frames, owner);

  if (frames.empty()) {
    auto owner_ref = fml::RefPtr<Element>(owner);
    if (external_parent != nullptr) {
      external_parent->InsertNode(owner_ref, old_index);
      NotifyNodeAdded(owner);
    }
    return owner_ref;
  }

  fml::RefPtr<Element> new_root(owner);
  for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
    auto frame = fml::RefPtr<Element>(*it);
    frame->InsertNode(new_root);
    NotifyNodeAdded(new_root.get());
    new_root = std::move(frame);
  }

  if (external_parent != nullptr) {
    external_parent->InsertNode(new_root, old_index);
    NotifyNodeAdded(new_root.get());
  }
  return new_root;
}

}  // namespace

ModifierElement::ModifierElement(ElementManager* manager)
    : Element(manager, BASE_STATIC_STRING(kElementViewTag)) {
  SetStyle(kPropertyIDDisplay, lepus::Value("box"));
  SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
}

bool ComposeModifierApplicator::ValidateTopology(ComposeElementHandle* handle,
                                                 std::string* error_message) {
  if (handle == nullptr || handle->content_element() == nullptr ||
      handle->mount_root() == nullptr) {
    *error_message =
        "Compose handle should retain a Content Element and mount root";
    return false;
  }
  return ValidateMountRoot(handle->content_element().get(),
                           handle->mount_root().get(), error_message);
}

ComposeModifierApplicator::ApplyResult ComposeModifierApplicator::Apply(
    ComposeElementHandle* handle, const lepus::Value& modifier_tail,
    runtime::MTSRuntime*) {
  if (handle == nullptr || handle->content_element() == nullptr ||
      handle->mount_root() == nullptr) {
    return {false, 0,
            "Compose handle should retain a Content Element and mount root"};
  }
  std::string error;
  if (!ValidateTopology(handle, &error)) {
    return {false, 0, std::move(error)};
  }
  auto owner = handle->content_element();
  auto current_mount_root = handle->mount_root();

  ModifierFramePlan plan;
  size_t error_position = 0;
  if (!BuildModifierFramePlan(modifier_tail, &plan, &error, &error_position)) {
    return {false, error_position, std::move(error)};
  }

  auto mount_root =
      InstallModifierFramePlan(plan, owner.get(), current_mount_root.get());
  handle->SetMountRoot(std::move(mount_root));
  return {true, 0, {}};
}

}  // namespace tasm
}  // namespace lynx
