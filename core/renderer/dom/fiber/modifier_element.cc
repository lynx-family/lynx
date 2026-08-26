// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/events/events.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/runtime/mts_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace tasm {

namespace {

// Modifier IR operation codes shared with the JavaScript and native producers.
constexpr int kModifierOpConcat = 1;
constexpr int kModifierOpStyleNumber = 2;
constexpr int kModifierOpStyleString = 3;
constexpr int kModifierOpAttribute = 4;
constexpr int kModifierOpCallback = 5;
constexpr int kModifierOpEvent = 6;
constexpr int kModifierOpPadding = 7;
constexpr int kModifierOpSize = 8;
constexpr int kModifierOpFill = 9;
constexpr int kModifierOpAspectRatio = 10;

constexpr int kModifierCallbackKindClick = 1;
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
BASE_STATIC_STRING_DECL(kPropertyId, "propertyId");
BASE_STATIC_STRING_DECL(kValue, "value");
BASE_STATIC_STRING_DECL(kName, "name");
BASE_STATIC_STRING_DECL(kCallbackKind, "callbackKind");
BASE_STATIC_STRING_DECL(kCallback, "callback");
BASE_STATIC_STRING_DECL(kEventName, "eventName");
BASE_STATIC_STRING_DECL(kEventType, "eventType");
BASE_STATIC_STRING_DECL(kSpecifiedAxes, "specifiedAxes");
BASE_STATIC_STRING_DECL(kStart, "start");
BASE_STATIC_STRING_DECL(kTop, "top");
BASE_STATIC_STRING_DECL(kEnd, "end");
BASE_STATIC_STRING_DECL(kBottom, "bottom");
BASE_STATIC_STRING_DECL(kWidth, "width");
BASE_STATIC_STRING_DECL(kHeight, "height");
BASE_STATIC_STRING_DECL(kFraction, "fraction");
BASE_STATIC_STRING_DECL(kRatio, "ratio");
BASE_STATIC_STRING_DECL(kTapEventName, "tap");
BASE_STATIC_STRING_DECL(kBindEventType, "bindEvent");

struct ModifierOperation {
  int op{0};
  size_t carrier_index{0};
  size_t target_frame_index{kNoModifierFrame};
  CSSPropertyID style_id{kPropertyStart};
  int specified_axes{0};
  int applied_axes{0};
  bool apply_aspect_ratio{false};
  std::array<double, 4> layout_values{};
  base::String name;
  base::String event_type;
  lepus::Value value;
  lepus::Value callback;
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

bool IsParentDataStyle(CSSPropertyID id) {
  switch (id) {
    case kPropertyIDMinWidth:
    case kPropertyIDMinHeight:
    case kPropertyIDFlexGrow:
    case kPropertyIDFlexShrink:
    case kPropertyIDFlexBasis:
    case kPropertyIDAlignSelf:
    case kPropertyIDJustifySelf:
      return true;
    default:
      return false;
  }
}

bool IsBoundaryStyle(CSSPropertyID id) {
  switch (id) {
    case kPropertyIDBackgroundColor:
    case kPropertyIDBoxShadow:
    case kPropertyIDBorderWidth:
    case kPropertyIDBorderColor:
    case kPropertyIDBorderStyle:
    case kPropertyIDBorderRadius:
    case kPropertyIDBorderTopLeftRadius:
    case kPropertyIDBorderTopRightRadius:
    case kPropertyIDBorderBottomLeftRadius:
    case kPropertyIDBorderBottomRightRadius:
    case kPropertyIDBorderStartStartRadius:
    case kPropertyIDBorderStartEndRadius:
    case kPropertyIDBorderEndStartRadius:
    case kPropertyIDBorderEndEndRadius:
    case kPropertyIDOverflow:
      return true;
    default:
      return false;
  }
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
    case kModifierOpStyleNumber:
    case kModifierOpStyleString: {
      int property_id = 0;
      if (!ReadIntegerProperty(node, kPropertyId, &property_id)) {
        *error = "propertyId should be an integer";
        return false;
      }
      auto id = static_cast<CSSPropertyID>(property_id);
      operation->value = node.GetProperty(kValue);
      const auto value = operation->value.ToLepusValue();
      const bool expected_type =
          (operation->op == kModifierOpStyleNumber && value.IsNumber() &&
           std::isfinite(value.Number())) ||
          (operation->op == kModifierOpStyleString && value.IsString());
      if (!CSSProperty::IsPropertyValid(id) || !expected_type) {
        *error = "invalid style property or value";
        return false;
      }
      operation->style_id = id;
      return true;
    }
    case kModifierOpAttribute: {
      const auto name = ReadModifierProperty(node, kName);
      operation->value = node.GetProperty(kValue);
      const auto value = operation->value.ToLepusValue();
      if (!name.IsString() || name.String().empty() ||
          (!value.IsString() && !value.IsNumber() && !value.IsBool())) {
        *error = "invalid attribute name or value";
        return false;
      }
      operation->name = name.String();
      return true;
    }
    case kModifierOpCallback: {
      int callback_kind = 0;
      operation->callback = node.GetProperty(kCallback);
      if (!ReadIntegerProperty(node, kCallbackKind, &callback_kind) ||
          callback_kind != kModifierCallbackKindClick ||
          !operation->callback.ToLepusValue().IsCallable()) {
        *error = "unsupported callback kind or callback";
        return false;
      }
      return true;
    }
    case kModifierOpEvent: {
      const auto event_name = ReadModifierProperty(node, kEventName);
      const auto event_type = ReadModifierProperty(node, kEventType);
      operation->callback = node.GetProperty(kCallback);
      if (!event_name.IsString() || event_name.String().empty() ||
          !event_type.IsString() ||
          !operation->callback.ToLepusValue().IsCallable()) {
        *error = "invalid event name, type, or callback";
        return false;
      }
      const auto& type = event_type.StdString();
      if (type != kEventBindEvent && type != kEventCatchEvent &&
          type != kEventCaptureBind && type != kEventCaptureCatch) {
        *error = "unsupported local event type";
        return false;
      }
      operation->name = event_name.String();
      operation->event_type = event_type.String();
      return true;
    }
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
    } else if (operation.op == kModifierOpCallback ||
               operation.op == kModifierOpEvent) {
      operation.target_frame_index = target_frame_index;
    } else if (operation.op == kModifierOpStyleNumber ||
               operation.op == kModifierOpStyleString) {
      if (IsParentDataStyle(operation.style_id) && plan->frame_count != 0) {
        operation.target_frame_index = 0;
      } else if (IsBoundaryStyle(operation.style_id)) {
        operation.target_frame_index = target_frame_index;
      }
    }
  }
  if (!plan->frame_child_constrained_axes.empty() &&
      plan->frame_child_constrained_axes.back() != 0) {
    plan->owner_uses_layout_box = true;
  }
  return true;
}

Element* OperationTarget(
    const ModifierOperation& operation, Element* owner,
    const std::vector<fml::RefPtr<ModifierElement>>& frames) {
  if (operation.target_frame_index == kNoModifierFrame) {
    return owner;
  }
  return frames[operation.target_frame_index].get();
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
  // Modifier click/local events are always registered as callable listeners
  // (see AddModifierEventListener), so the listener map must be cleared
  // unconditionally on replacement, independent of EnableEventHandleRefactor().
  element->GetEventListenerMap()->Clear();
  element->RemoveAllEvents();
}

// Registers a Modifier click/local event callback directly as a callable event
// listener, mirroring the base branch's FiberSetModifierToElement behavior. The
// closure is dispatched through registration_context so the callback fires in
// the runtime that registered it, independent of the default entry runtime.
void AddModifierEventListener(Element* element, const base::String& name,
                              const base::String& type,
                              const lepus::Value& callback,
                              runtime::MTSRuntime* registration_context) {
  element->SetJSEventHandler(name, base::String(), base::String());
  element->AddEventListener(
      name.str(),
      std::make_unique<event::ClosureEventListener>(
          [callback, registration_context](lepus::Value args) {
            if (!args.IsArray() || args.Array()->size() != 3) {
              return;
            }
            if (registration_context == nullptr) {
              return;
            }
            registration_context->CallClosure(
                callback, lepus_value::ShallowCopy(args.Array()->get(1)));
          },
          GetEventListenerOptions(type),
          event::ClosureEventListener::ClosureType::kCore, callback));
}

void ApplyNonLayoutOperation(
    const ModifierOperation& operation, Element* owner,
    const std::vector<fml::RefPtr<ModifierElement>>& frames,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  Element* target = OperationTarget(operation, owner, frames);
  switch (operation.op) {
    case kModifierOpStyleNumber:
    case kModifierOpStyleString:
      target->SetStyle(operation.style_id, operation.value.ToLepusValue());
      break;
    case kModifierOpAttribute:
      owner->SetModifierAttribute(operation.name,
                                  operation.value.ToLepusValue(deep_convert));
      break;
    case kModifierOpCallback:
      // A click maps to a bubbling "tap" binding (tasm::kEventBindEvent).
      AddModifierEventListener(target, kTapEventName, kBindEventType,
                               operation.callback, registration_context);
      break;
    case kModifierOpEvent:
      AddModifierEventListener(target, operation.name, operation.event_type,
                               operation.callback, registration_context);
      break;
    default:
      break;
  }
}

void ApplyNonLayoutOperations(
    const ModifierFramePlan& plan, Element* owner,
    const std::vector<fml::RefPtr<ModifierElement>>& frames,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  for (const auto& operation : plan.operations) {
    if (!IsLayoutOperation(operation.op)) {
      ApplyNonLayoutOperation(operation, owner, frames, registration_context,
                              deep_convert);
    }
  }
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

fml::RefPtr<Element> InstallModifierFramePlan(
    const ModifierFramePlan& plan, Element* owner, Element* current_mount_root,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  if (plan.frame_count == 0 && current_mount_root == owner) {
    const std::vector<fml::RefPtr<ModifierElement>> frames;
    ClearModifierOwnedProperties(owner);
    ApplyLayoutOperations(plan, frames, owner);
    ApplyNonLayoutOperations(plan, owner, frames, registration_context,
                             deep_convert);
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

  // When replacing a mounted, non-empty frame chain, move the attached owner
  // from the old innermost frame to the new innermost frame. First mounts,
  // detached chains, and zero-frame plans keep the existing remove-and-insert
  // path.
  const bool move_attached_owner =
      external_parent != nullptr && current_mount_root != owner &&
      !frames.empty() && owner->render_parent() != nullptr;

  if (external_parent != nullptr) {
    NotifyNodeRemoved(old_root.get());
    external_parent->RemoveNode(old_root, false);
  }
  if (current_mount_root != owner) {
    NotifyNodeRemoved(owner);
    if (!move_attached_owner) {
      owner->parent()->RemoveNode(fml::RefPtr<Element>(owner), false);
    }
  }

  ClearModifierOwnedProperties(owner);

  ApplyLayoutOperations(plan, frames, owner);
  ApplyNonLayoutOperations(plan, owner, frames, registration_context,
                           deep_convert);
  ApplyLayoutChildConstraints(plan, frames, owner);

  if (frames.empty()) {
    auto owner_ref = fml::RefPtr<Element>(owner);
    if (external_parent != nullptr) {
      external_parent->InsertNode(owner_ref, old_index);
      NotifyNodeAdded(owner);
    }
    return owner_ref;
  }

  fml::RefPtr<Element> new_root;
  auto frame = frames.rbegin();
  if (move_attached_owner) {
    (*frame)->MoveNodeToIndex(fml::RefPtr<Element>(owner), 0);
    NotifyNodeAdded(owner);
    new_root = *frame;
    ++frame;
  } else {
    new_root = fml::RefPtr<Element>(owner);
  }

  for (auto it = frame; it != frames.rend(); ++it) {
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
  if (manager != nullptr) {
    SetDefaultOverflow(manager->GetDefaultOverflowVisible());
  }
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
    runtime::MTSRuntime* registration_context) {
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
  auto* manager = owner->element_manager();

  ModifierFramePlan plan;
  size_t error_position = 0;
  if (!BuildModifierFramePlan(modifier_tail, &plan, &error, &error_position)) {
    return {false, error_position, std::move(error)};
  }

  auto mount_root = InstallModifierFramePlan(
      plan, owner.get(), current_mount_root.get(), registration_context,
      manager->GetEnableParallelElement());
  handle->SetMountRoot(std::move(mount_root));
  return {true, 0, {}};
}

}  // namespace tasm
}  // namespace lynx
