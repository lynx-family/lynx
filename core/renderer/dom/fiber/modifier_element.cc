// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/modifier_element.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "base/include/flex_optional.h"
#include "base/include/string/string_utils.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/compose_draw_behind.h"
#include "core/renderer/dom/fiber/compose_element_handle.h"
#include "core/renderer/dom/fiber/compose_graphics_layer.h"
#include "core/renderer/dom/fiber/compose_modifier_applicator.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/view_fragment_behavior.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/events/events.h"
#include "core/renderer/utils/base/tasm_constants.h"
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
constexpr int kModifierOpGraphicsLayer = 11;
constexpr int kModifierOpDrawBehind = 12;
constexpr int kModifierOpWeightFill = 13;
constexpr int kModifierOpOffset = 14;
constexpr int kModifierOpSizeRange = 15;
constexpr int kModifierOpAttachment = 16;

bool IsDrawOperation(int op) {
  return op == kModifierOpGraphicsLayer || op == kModifierOpDrawBehind;
}

constexpr int kModifierCallbackKindClick = 1;
constexpr int kLayoutModifierAxisWidth = 1;
constexpr int kLayoutModifierAxisHeight = 1 << 1;
constexpr int kLayoutModifierAllAxes =
    kLayoutModifierAxisWidth | kLayoutModifierAxisHeight;

// Cap total visited nodes (not per-path depth) so a malformed cyclic or
// branching concat chain stays bounded while long linear chains still parse.
constexpr int kMaxModifierNodeVisits = 512;
constexpr size_t kNoModifierFrame = std::numeric_limits<size_t>::max();

// IR fields use camelCase; Element attributes use kebab-case. Target sizes
// remain physical pixels on both sides of this mapping.
BASE_STATIC_STRING_DECL(kShareWithSiblingsField, "shareWithSiblings");
BASE_STATIC_STRING_DECL(kShareTouchTargetAttribute, "share-touch-target");
BASE_STATIC_STRING_DECL(kMinimumTouchTargetSizeAttribute,
                        "minimum-touch-target-size");
BASE_STATIC_STRING_DECL(kMinimumTouchTargetPxField, "minTouchTargetPx");
BASE_STATIC_STRING_DECL(kEventTargetField, "eventTarget");
BASE_STATIC_STRING_DECL(kAttachField, "attach");

BASE_STATIC_STRING_DECL(kOp, "op");
BASE_STATIC_STRING_DECL(kPrevious, "previous");
BASE_STATIC_STRING_DECL(kResolve, "resolve");
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
BASE_STATIC_STRING_DECL(kFill, "fill");
BASE_STATIC_STRING_DECL(kRatio, "ratio");
BASE_STATIC_STRING_DECL(kX, "x");
BASE_STATIC_STRING_DECL(kY, "y");
BASE_STATIC_STRING_DECL(kMinWidth, "minWidth");
BASE_STATIC_STRING_DECL(kMinHeight, "minHeight");
BASE_STATIC_STRING_DECL(kMaxWidth, "maxWidth");
BASE_STATIC_STRING_DECL(kMaxHeight, "maxHeight");
BASE_STATIC_STRING_DECL(kTapEventName, "tap");
BASE_STATIC_STRING_DECL(kBindEventType, "bindEvent");

struct ModifierOperation {
  int op{0};
  size_t carrier_index{0};
  size_t target_frame_index{kNoModifierFrame};
  std::array<size_t, 2> fill_propagation_start{};
  CSSPropertyID style_id{kPropertyStart};
  int specified_axes{0};
  int applied_axes{0};
  bool apply_aspect_ratio{false};
  bool wrap_has_prefix{false};
  int wrap_axes{0};
  double layout_values[4]{};
  bool introduces_frame{false};
  bool event_target{false};
  bool share_touch_target{false};
  float minimum_touch_target_px{0.f};
  base::String name;
  base::String event_type;
  lepus::Value value;
  lepus::Value callback;
};

struct ModifierFramePlan {
  base::Vector<ModifierOperation> operations;
  base::Vector<int> frame_child_constrained_axes;
  size_t frame_count{0};
  bool owner_uses_layout_box{false};
  bool has_draw_callbacks{false};
  bool has_offset{false};
  bool has_size_range{false};
  bool has_wrap_content{false};
  base::flex_optional<bool> weight_fill;
  bool has_attachments{false};
};

lepus::Value ReadModifierProperty(const lepus::Value& node,
                                  const base::String& key) {
  // Modifier objects may arrive from the JavaScript runtime with primitive
  // properties represented as JSValue wrappers. Normalize one primitive at a
  // time without deeply converting the linked/callable Modifier graph.
  return node.GetProperty(key).ToLepusValue();
}

bool ReadIntegerValue(const lepus::Value& value, int* result) {
  if (!value.IsNumber()) return false;
  const double number = value.Number();
  // Modifier integers accept negative zero as zero.
  if (number != 0 && !base::StringConvertHelper::IsInt32Double(number)) {
    return false;
  }
  *result = static_cast<int>(number);
  return true;
}

bool ReadIntegerProperty(const lepus::Value& node, const base::String& key,
                         int* result) {
  return ReadIntegerValue(ReadModifierProperty(node, key), result);
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

// Route these styles to the outermost frame: it is the child seen by the layout
// parent. Keep legacy min/flex routing for compatibility. Routing does not
// decide which value wins when a property appears more than once in the
// modifier chain.
bool IsParentDataStyle(CSSPropertyID id) {
  switch (id) {
    case kPropertyIDMinWidth:
    case kPropertyIDMinHeight:
    case kPropertyIDLinearWeight:
    case kPropertyIDFlexGrow:
    case kPropertyIDFlexShrink:
    case kPropertyIDFlexBasis:
    case kPropertyIDAlignSelf:
    case kPropertyIDJustifySelf:
    case kPropertyIDXBoxMatchParentSize:
      return true;
    default:
      return false;
  }
}

// Each bit identifies a Compose parent-data field whose first authored value
// wins. Zero leaves duplicate handling unchanged, including legacy min/flex
// styles. WeightFill is a separate operation, collected by
// BuildModifierFramePlan.
uint8_t ComposeParentDataStyleFlag(CSSPropertyID id) {
  switch (id) {
    case kPropertyIDLinearWeight:
      return 1 << 0;
    case kPropertyIDAlignSelf:
      return 1 << 1;
    case kPropertyIDJustifySelf:
      return 1 << 2;
    case kPropertyIDXBoxMatchParentSize:
      return 1 << 3;
    default:
      return 0;
  }
}

bool IsBoundaryStyle(CSSPropertyID id) {
  switch (id) {
    case kPropertyIDBackgroundColor:
    case kPropertyIDBackgroundImage:
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
    case kModifierOpOffset:
    case kModifierOpSizeRange:
      return true;
    default:
      return false;
  }
}

bool ParseLayoutOperation(const lepus::Value& node,
                          ModifierOperation* operation, std::string* error) {
  auto& values = operation->layout_values;
  switch (operation->op) {
    case kModifierOpSizeRange:
      if (!ReadIntegerProperty(node, kSpecifiedAxes,
                               &operation->specified_axes) ||
          operation->specified_axes <= 0 ||
          (operation->specified_axes & ~kLayoutModifierAllAxes) != 0 ||
          !ReadFiniteNumberProperty(node, kMinWidth, &values[0]) ||
          !ReadFiniteNumberProperty(node, kMinHeight, &values[1]) ||
          !ReadFiniteNumberProperty(node, kMaxWidth, &values[2]) ||
          !ReadFiniteNumberProperty(node, kMaxHeight, &values[3])) {
        *error = "invalid size range parameters";
        return false;
      }
      for (size_t axis = 0; axis < 2; ++axis) {
        if (values[axis] < 0 ||
            (values[axis + 2] != -1 && values[axis + 2] < values[axis]) ||
            values[axis] > std::numeric_limits<float>::max() ||
            values[axis + 2] > std::numeric_limits<float>::max()) {
          *error = "invalid size range bounds";
          return false;
        }
      }
      return true;
    case kModifierOpOffset:
      if (!ReadFiniteNumberProperty(node, kX, &values[0]) ||
          !ReadFiniteNumberProperty(node, kY, &values[1]) ||
          std::abs(values[0]) > std::numeric_limits<float>::max() ||
          std::abs(values[1]) > std::numeric_limits<float>::max()) {
        *error = "invalid offset parameters";
        return false;
      }
      return true;
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
    case kModifierOpWeightFill:
      operation->value = ReadModifierProperty(node, kFill);
      if (!operation->value.IsBool()) {
        *error = "weight fill should be boolean";
        return false;
      }
      return true;
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
    case kModifierOpAttachment: {
      operation->callback = node.GetProperty(kAttachField);
      double minimum = 0;
      if (!operation->callback.ToLepusValue().IsCallable() ||
          !ReadFiniteNumberProperty(node, kMinimumTouchTargetPxField,
                                    &minimum) ||
          minimum < 0) {
        *error = "invalid Modifier attachment";
        return false;
      }
      const auto event_target = ReadModifierProperty(node, kEventTargetField);
      if (!event_target.IsBool()) {
        *error = "invalid event target marker";
        return false;
      }
      operation->event_target = event_target.Bool();
      operation->share_touch_target =
          ReadModifierProperty(node, kShareWithSiblingsField).Bool();
      operation->minimum_touch_target_px = minimum;
      return true;
    }
    case kModifierOpPadding:
    case kModifierOpSize:
    case kModifierOpFill:
    case kModifierOpAspectRatio:
    case kModifierOpOffset:
    case kModifierOpSizeRange:
      return ParseLayoutOperation(node, operation, error);
    case kModifierOpGraphicsLayer:
    case kModifierOpDrawBehind:
      operation->callback = node.GetProperty(kResolve);
      if (!operation->callback.ToLepusValue().IsCallable()) {
        *error = "drawing resolve should be callable";
        return false;
      }
      return true;
    default:
      *error = "unsupported Modifier operation";
      return false;
  }
}

bool FlattenModifierNode(const lepus::Value& node,
                         base::Vector<ModifierOperation>* operations,
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
  if (!ReadIntegerValue(op_value, &op)) {
    *error = "op should be an integer";
    return false;
  }
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

int WrapContentStyleAxis(const ModifierOperation& operation) {
  if (operation.op != kModifierOpStyleString ||
      (operation.style_id != kPropertyIDWidth &&
       operation.style_id != kPropertyIDHeight)) {
    return 0;
  }
  const auto value = operation.value.ToLepusValue();
  if (value.StdString() != "fit-content" &&
      value.StdString() != "max-content") {
    return 0;
  }
  return operation.style_id == kPropertyIDWidth ? kLayoutModifierAxisWidth
                                                : kLayoutModifierAxisHeight;
}

// wrapContentSize emits adjacent width/height styles followed by both
// alignments. Axis-specific wraps have an alignment between their dimension
// styles, keeping separately authored modifiers on separate boundaries.
const ModifierOperation* WrapStyleBeforeAlignment(
    const base::Vector<ModifierOperation>& operations, size_t position,
    CSSPropertyID alignment) {
  if (alignment == kPropertyIDAlignItems && position > 0 &&
      operations[position - 1].op == kModifierOpStyleString &&
      operations[position - 1].style_id == kPropertyIDJustifyItems) {
    --position;
  }
  if (position == 0 || !WrapContentStyleAxis(operations[position - 1])) {
    return nullptr;
  }
  const auto& previous = operations[position - 1];
  const auto* wrap =
      previous.wrap_axes != 0 ? &previous : &operations[position - 2];
  const int axis = alignment == kPropertyIDJustifyItems
                       ? kLayoutModifierAxisWidth
                       : kLayoutModifierAxisHeight;
  return (wrap->wrap_axes & axis) != 0 ? wrap : nullptr;
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

  // Reuse physical layout/layer boundaries for attachments. Only pointer
  // attachments sharing one carrier need an additional boundary.
  ModifierOperation* previous_target = nullptr;
  for (auto& operation : plan->operations) {
    if (operation.op == kModifierOpAttachment) {
      plan->has_attachments = true;
      if (operation.event_target) {
        if (previous_target) previous_target->introduces_frame = true;
        previous_target = &operation;
      }
    } else if (IsDrawOperation(operation.op) ||
               operation.op == kModifierOpPadding ||
               operation.op == kModifierOpOffset ||
               operation.op == kModifierOpSizeRange ||
               WrapContentStyleAxis(operation)) {
      previous_target = nullptr;
    }
  }

  int sizing_prefix_axes = 0;
  for (size_t position = 0; position < plan->operations.size(); ++position) {
    auto& operation = plan->operations[position];
    // Each Compose weight() emits LinearWeight and WeightFill, even for the
    // default fill=true. Keep the first fill with the first weight; has_value()
    // also preserves an explicit false instead of letting a later fill replace
    // it.
    if (operation.op == kModifierOpWeightFill &&
        !plan->weight_fill.has_value()) {
      plan->weight_fill = operation.value.Bool();
    }
    if (operation.op == kModifierOpSize || operation.op == kModifierOpFill) {
      sizing_prefix_axes |= operation.specified_axes;
    } else if (operation.op == kModifierOpAspectRatio) {
      sizing_prefix_axes = kLayoutModifierAllAxes;
    }
    if (operation.introduces_frame) {
      ++plan->frame_count;
      sizing_prefix_axes = 0;
    } else if (const int axis = WrapContentStyleAxis(operation)) {
      if (position > 0 && axis == kLayoutModifierAxisHeight &&
          plan->operations[position - 1].wrap_axes == kLayoutModifierAllAxes) {
        continue;
      }
      operation.wrap_axes = axis;
      if (axis == kLayoutModifierAxisWidth &&
          position + 1 < plan->operations.size() &&
          WrapContentStyleAxis(plan->operations[position + 1]) ==
              kLayoutModifierAxisHeight) {
        operation.wrap_axes |= kLayoutModifierAxisHeight;
      }
      operation.wrap_has_prefix =
          (sizing_prefix_axes & operation.wrap_axes) != 0;
      plan->frame_count += operation.wrap_has_prefix ? 2 : 1;
      plan->has_wrap_content = true;
      sizing_prefix_axes = 0;
    } else if (IsDrawOperation(operation.op)) {
      plan->has_draw_callbacks = true;
      // Keep preceding effects outside the following Element's drawing hook.
      ++plan->frame_count;
      sizing_prefix_axes = 0;
    } else if (operation.op == kModifierOpPadding ||
               operation.op == kModifierOpOffset ||
               operation.op == kModifierOpSizeRange) {
      plan->has_size_range |= operation.op == kModifierOpSizeRange;
      plan->has_offset |= operation.op == kModifierOpOffset;
      ++plan->frame_count;
      sizing_prefix_axes = 0;
    }
  }
  plan->frame_child_constrained_axes.resize<true>(plan->frame_count);

  // Padding and drawing hooks introduce physical boundaries. Each hook belongs
  // to the following carrier (possibly the stable Content Element). This keeps
  // earlier drawing outside it and preserves multiple hooks in Modifier order.
  size_t carrier_index = 0;
  int constrained_axes = 0;
  std::array<size_t, 2> fill_propagation_start{};
  for (size_t position = 0; position < plan->operations.size(); ++position) {
    auto& operation = plan->operations[position];
    operation.carrier_index = carrier_index;
    operation.fill_propagation_start = fill_propagation_start;
    const size_t target_frame_index =
        carrier_index < plan->frame_count ? carrier_index : kNoModifierFrame;

    if (operation.wrap_axes != 0) {
      constrained_axes &= ~operation.wrap_axes;
      if (operation.wrap_has_prefix) {
        // Keep the outer size/fill separate from intrinsic size styles.
        plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
        ++carrier_index;
      } else if (carrier_index != 0) {
        // Fixed incoming ranges already carry the sizes. Do not overwrite
        // intrinsic CSS sizes with the usual 100% carrier hints.
        plan->frame_child_constrained_axes[carrier_index - 1] &=
            ~operation.wrap_axes;
      }
      operation.target_frame_index = carrier_index;
      plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
      ++carrier_index;
      for (size_t axis = 0; axis < fill_propagation_start.size(); ++axis) {
        if ((operation.wrap_axes & (1 << axis)) != 0) {
          fill_propagation_start[axis] = carrier_index;
        }
      }
    } else if (WrapContentStyleAxis(operation)) {
      operation.target_frame_index =
          plan->operations[position - 1].target_frame_index;
    } else if (IsDrawOperation(operation.op)) {
      plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
      ++carrier_index;
      operation.target_frame_index =
          carrier_index < plan->frame_count ? carrier_index : kNoModifierFrame;
    } else if (IsLayoutOperation(operation.op)) {
      operation.target_frame_index = target_frame_index;
      switch (operation.op) {
        case kModifierOpPadding:
        case kModifierOpOffset:
        case kModifierOpSizeRange:
          plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
          ++carrier_index;
          if (operation.op == kModifierOpSizeRange &&
              carrier_index == plan->frame_count) {
            plan->owner_uses_layout_box = true;
          }
          break;
        case kModifierOpSize:
        case kModifierOpFill:
          operation.applied_axes = operation.specified_axes & ~constrained_axes;
          if (operation.op == kModifierOpFill &&
              operation.layout_values[0] != 1.0 &&
              operation.applied_axes != 0 &&
              carrier_index != std::max(fill_propagation_start[0],
                                        fill_propagation_start[1])) {
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
    } else if (operation.op == kModifierOpAttachment ||
               operation.op == kModifierOpCallback ||
               operation.op == kModifierOpEvent) {
      operation.target_frame_index = target_frame_index;
      if (operation.introduces_frame) {
        plan->frame_child_constrained_axes[carrier_index] = constrained_axes;
        ++carrier_index;
      }
    } else if (operation.op == kModifierOpStyleNumber ||
               operation.op == kModifierOpStyleString) {
      const auto* wrap =
          (operation.style_id == kPropertyIDJustifyItems ||
           operation.style_id == kPropertyIDAlignItems)
              ? WrapStyleBeforeAlignment(plan->operations, position,
                                         operation.style_id)
              : nullptr;
      if (wrap) {
        const auto value = operation.value.ToLepusValue();
        if (!value.IsString() ||
            (value.StdString() != "start" && value.StdString() != "center" &&
             value.StdString() != "end")) {
          *error = "unsupported intrinsic alignment";
          *error_position = position;
          return false;
        }
        operation.target_frame_index = wrap->target_frame_index;
      } else if (IsParentDataStyle(operation.style_id) &&
                 plan->frame_count != 0) {
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
    const base::Vector<fml::RefPtr<ModifierElement>>& frames) {
  if (operation.target_frame_index == kNoModifierFrame) {
    return owner;
  }
  return frames[operation.target_frame_index].get();
}

Element* LayoutCarrierAt(
    size_t index, Element* owner,
    const base::Vector<fml::RefPtr<ModifierElement>>& frames) {
  return index < frames.size() ? frames[index].get() : owner;
}

lepus::Value CSSUnitValue(double value, const char* unit) {
  return lepus::Value(base::FormatString("%g%s", value, unit));
}

lepus::Value PreciseCSSPixelValue(double value) {
  return lepus::Value(base::FormatString(
      "%.*gpx", std::numeric_limits<double>::max_digits10, value));
}

void ApplyLayoutOperation(const ModifierOperation& operation,
                          Element* carrier) {
  const auto& values = operation.layout_values;
  switch (operation.op) {
    case kModifierOpSizeRange: {
      const auto& env = carrier->element_manager()->GetLynxEnvConfig();
      const auto set_bound = [&](CSSPropertyID id, double value, bool minimum) {
        if (value < 0) return;  // Unbounded maximum.
        double pixels = std::floor(
            std::min<double>(static_cast<float>(value) * env.DevicePixelRatio(),
                             std::numeric_limits<int>::max()) +
            0.5);
        if (pixels >= std::numeric_limits<int>::max()) {
          if (!minimum) return;
          pixels = 0;
        }
        carrier->SetStyle(
            id, PreciseCSSPixelValue(pixels / env.DevicePixelRatio()));
      };
      if (operation.specified_axes & kLayoutModifierAxisWidth) {
        set_bound(kPropertyIDMinWidth, values[0], true);
        set_bound(kPropertyIDMaxWidth, values[2], false);
      }
      if (operation.specified_axes & kLayoutModifierAxisHeight) {
        set_bound(kPropertyIDMinHeight, values[1], true);
        set_bound(kPropertyIDMaxHeight, values[3], false);
      }
      break;
    }
    case kModifierOpOffset: {
      const auto& env = carrier->element_manager()->GetLynxEnvConfig();
      const auto offset_value = [&env](double value) {
        // Round Dp in device pixels before CSS resolves the logical inset.
        const double pixels = std::clamp<double>(
            static_cast<float>(value) * env.DevicePixelRatio(),
            std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        return PreciseCSSPixelValue(std::floor(pixels + 0.5) /
                                    env.DevicePixelRatio());
      };
      carrier->SetStyle(kPropertyIDPosition, lepus::Value("relative"));
      carrier->SetStyle(kPropertyIDInsetInlineStart, offset_value(values[0]));
      carrier->SetStyle(kPropertyIDTop, offset_value(values[1]));
      break;
    }
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
    const base::Vector<fml::RefPtr<ModifierElement>>& frames, Element* owner) {
  if (plan.owner_uses_layout_box) {
    owner->SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
  }
  for (const auto& operation : plan.operations) {
    if (!IsLayoutOperation(operation.op)) {
      continue;
    }
    // Offset and range bounds apply inside their own pass-through boundary.
    ApplyLayoutOperation(
        operation, LayoutCarrierAt(operation.carrier_index +
                                       ((operation.op == kModifierOpOffset ||
                                         operation.op == kModifierOpSizeRange)
                                            ? 1
                                            : 0),
                                   owner, frames));
    if (operation.op == kModifierOpFill && operation.layout_values[0] == 1.0) {
      for (size_t index = 0; index < operation.carrier_index; ++index) {
        auto* carrier = LayoutCarrierAt(index, owner, frames);
        if (index >= operation.fill_propagation_start[0] &&
            (operation.applied_axes & kLayoutModifierAxisWidth) != 0) {
          carrier->SetStyle(kPropertyIDWidth, lepus::Value("100%"));
        }
        if (index >= operation.fill_propagation_start[1] &&
            (operation.applied_axes & kLayoutModifierAxisHeight) != 0) {
          carrier->SetStyle(kPropertyIDHeight, lepus::Value("100%"));
        }
      }
    }
  }
}

void ApplyLayoutChildConstraints(
    const ModifierFramePlan& plan,
    const base::Vector<fml::RefPtr<ModifierElement>>& frames, Element* owner) {
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
    const base::Vector<fml::RefPtr<ModifierElement>>& frames,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  Element* target = OperationTarget(operation, owner, frames);
  switch (operation.op) {
    case kModifierOpStyleNumber:
    case kModifierOpStyleString:
      target->SetStyle(operation.style_id, operation.value.ToLepusValue());
      break;
    case kModifierOpAttachment:
      if (operation.event_target) {
        target->SetModifierAttribute(
            kMinimumTouchTargetSizeAttribute,
            lepus::Value(operation.minimum_touch_target_px));
        target->SetModifierAttribute(
            kShareTouchTargetAttribute,
            lepus::Value(operation.share_touch_target));
      }
      break;
    case kModifierOpGraphicsLayer:
    case kModifierOpDrawBehind:
      // The following Element records the callback result during Draw.
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
    const base::Vector<fml::RefPtr<ModifierElement>>& frames,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  // Upstream Compose folds parent data from inner to outer, so the first
  // authored value wins. Our operations are in authored order: skip later
  // writes to those fields. Box producers emit both alignment axes and the
  // match-parent flag on every call, including defaults, so the first complete
  // Box configuration wins. Legacy CSS min/flex styles keep their existing
  // last-value rule. This mask is local to one complete application, so
  // replacement chains can change or remove every field without inheriting the
  // previous application's deduplication state.
  uint8_t applied_parent_data = 0;
  for (const auto& operation : plan.operations) {
    if (!IsLayoutOperation(operation.op)) {
      if (operation.op == kModifierOpStyleNumber ||
          operation.op == kModifierOpStyleString) {
        const auto flag = ComposeParentDataStyleFlag(operation.style_id);
        if ((applied_parent_data & flag) != 0) continue;
        applied_parent_data |= flag;
      }
      ApplyNonLayoutOperation(operation, owner, frames, registration_context,
                              deep_convert);
    }
  }
}

void SetWeightFill(Element* element, bool fill) {
  if (element->computed_css_style()
          ->GetLayoutComputedStyle()
          ->SetComposeWeightFill(fill)) {
    element->MarkLayoutDirtyLite();
  }
}

// Publish physical identities only after installing the complete chain. The
// runtime owns callback identities and their lifetime; Elements retain no
// parallel list of logical input nodes.
void BindModifierAttachments(const ModifierFramePlan& plan, Element* mount_root,
                             Element* content_element,
                             runtime::MTSRuntime* registration_context) {
  base::Vector<fml::RefPtr<ModifierElement>> frames;
  for (auto* target = mount_root; target != content_element;
       target = target->GetChildAt(0)) {
    frames.emplace_back(static_cast<ModifierElement*>(target));
  }
  for (const auto& operation : plan.operations) {
    if (operation.op == kModifierOpAttachment) {
      auto* target = OperationTarget(operation, content_element, frames);
      registration_context->CallClosure(operation.callback,
                                        lepus::Value(target->impl_id()));
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

void ClearModifierDrawCallbacks(Element* mount_root, Element* content_element) {
  // ValidateTopology guarantees a single-child Modifier chain ending at the
  // stable content Element. Include that Element: the innermost drawing hook
  // can bind directly to it. Do not visit its application-owned children.
  for (auto* target = mount_root;; target = target->GetChildAt(0)) {
    if (target->HasCustomDrawFunctions()) {
      target->SetCustomDrawFunctions({});
    }
    if (target == content_element) {
      break;
    }
  }
}

void BindModifierDrawCallbacks(const ModifierFramePlan& plan,
                               Element* mount_root, Element* content_element,
                               runtime::MTSRuntime* registration_context) {
  // Match the outer-to-inner frame indices assigned by BuildModifierFramePlan.
  // The stable content Element is not a frame; OperationTarget selects it when
  // the operation has kNoModifierFrame as its target.
  base::Vector<fml::RefPtr<ModifierElement>> frames;
  for (auto* target = mount_root; target != content_element;
       target = target->GetChildAt(0)) {
    frames.emplace_back(static_cast<ModifierElement*>(target));
  }

  for (const auto& operation : plan.operations) {
    if (!IsDrawOperation(operation.op)) {
      continue;
    }
    // Every hook introduces its own boundary, so no two hooks replace one
    // another on the same Element. Layer properties and content drawing run in
    // their respective Fragment phases, both with this Element's actual size.
    auto* target = OperationTarget(operation, content_element, frames);
    target->SetCustomDrawFunctions(
        operation.op == kModifierOpGraphicsLayer
            ? GraphicsLayerDrawFunctions(registration_context,
                                         operation.callback)
            : ComposeDrawBehindFunctions(registration_context,
                                         operation.callback));
  }
}

fml::RefPtr<Element> InstallModifierFramePlan(
    const ModifierFramePlan& plan, Element* owner, Element* current_mount_root,
    runtime::MTSRuntime* registration_context, bool deep_convert) {
  if (plan.frame_count == 0 && current_mount_root == owner) {
    const base::Vector<fml::RefPtr<ModifierElement>> frames;
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

  base::Vector<fml::RefPtr<ModifierElement>> frames;
  frames.reserve(plan.frame_count);
  for (size_t index = 0; index < plan.frame_count; ++index) {
    auto frame = CreateFrame(owner);
    // Pass-through boxes must not introduce implicit clipping around
    // transformed contents. Authored overflow styles are applied afterwards and
    // still win.
    if (plan.has_draw_callbacks || plan.has_offset || plan.has_wrap_content)
      frame->SetDefaultOverflow(true);
    frames.push_back(std::move(frame));
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

void ModifierElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);
  auto* style = computed_css_style()->GetLayoutComputedStyle();
  style->SetDisplay(starlight::DisplayType::kXBox);
  style->SetBoxSizing(starlight::BoxSizingType::kBorderBox);
  style->SetXBoxPropagateMinConstraints(true);
}

void ModifierElement::OnNodeAdded(Element* child) {
  Element::OnNodeAdded(child);
  if (!(child->dirty() & kDirtyCloned)) return;
  child->MarkStyleDirty();
  // Computed-style clones omit scalar positioning fields. Replay the existing
  // CSS on cloned inner carriers without installing a placement callback.
  const auto& styles = child->GetParsedStylesMap();
  for (auto id :
       {kPropertyIDPosition, kPropertyIDInsetInlineStart, kPropertyIDTop}) {
    const auto it = styles.find(id);
    if (it != styles.end()) {
      child->SetStyleInternal(child->ConvertRtlCSSPropertyID(id).second,
                              it->second);
    }
  }
}

void ModifierElement::SetupFragmentBehavior(Fragment* fragment) {
  // Fragment::CreateLayerIfNeeded decides whether a platform layer is needed.
  // Use the built-in view renderer type for generated Modifier boxes; the base
  // Element behavior instead resolves a platform component by tag.
  // Fragment dispatches DrawBehind/DrawForeground with either behavior, so
  // adding or removing draw callbacks does not affect this choice.
  fragment->SetBehavior(std::make_unique<ViewFragmentBehavior>(fragment));
}

ModifierElement::ModifierElement(ElementManager* manager)
    : Element(manager, BASE_STATIC_STRING(kElementViewTag)) {
  MarkAsComposeLayout();
  SetStyle(kPropertyIDDisplay, lepus::Value("-x-box"));
  SetStyle(kPropertyIDBoxSizing, lepus::Value("border-box"));
  // Modifier frames pass parent-imposed sizes through to their content,
  // including dimensions allocated by weight rather than an explicit size
  // modifier.
  SetStyle(kPropertyIDXBoxPropagateMinConstraints, lepus::Value(true));
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
  // The content Element keeps its identity while the surrounding Modifier
  // frames are replaced. The mount root is the outermost of those frames, or
  // the content Element itself when no frames are needed.
  auto content_element = handle->content_element();
  auto previous_mount_root = handle->mount_root();
  auto* manager = content_element->element_manager();

  ModifierFramePlan plan;
  size_t error_position = 0;
  if (!BuildModifierFramePlan(modifier_tail, &plan, &error, &error_position)) {
    return {false, error_position, std::move(error)};
  }
  if (plan.weight_fill.has_value() &&
      !content_element->EnableLayoutInElementMode()) {
    return {false, 0, "weight fill requires layout in Element"};
  }
  if (plan.has_size_range &&
      (!content_element->EnableLayoutInElementMode() ||
       !std::isfinite(manager->GetLynxEnvConfig().DevicePixelRatio()) ||
       manager->GetLynxEnvConfig().DevicePixelRatio() <= 0)) {
    return {false, 0,
            "size range requires layout in Element and positive density"};
  }
  if (plan.has_wrap_content && !content_element->EnableLayoutInElementMode()) {
    return {false, 0, "wrapContent requires layout in Element"};
  }
  if (plan.has_offset) {
    if (!content_element->EnableLayoutInElementMode()) {
      return {false, 0, "offset requires layout in Element"};
    }
    const double scale = manager->GetLynxEnvConfig().LayoutsUnitPerPx();
    for (const auto& operation : plan.operations) {
      if (operation.op == kModifierOpOffset &&
          (!std::isfinite(scale) || scale <= 0 ||
           std::abs(operation.layout_values[0] * scale) >
               std::numeric_limits<float>::max() ||
           std::abs(operation.layout_values[1] * scale) >
               std::numeric_limits<float>::max())) {
        return {false, 0, "offset exceeds the native layout unit range"};
      }
    }
  }
  // Reject unsupported plans before changing any active callbacks or nodes.
  if (plan.has_attachments && !registration_context) {
    return {false, 0, "Modifier attachment requires a registering runtime"};
  }
  if (plan.has_draw_callbacks) {
    if (!registration_context) {
      return {false, 0, "drawing modifiers require a registering runtime"};
    }
    if (!manager->IsFragmentLayerRenderModeOn()) {
      return {false, 0, "drawing modifiers require Fragment Layer rendering"};
    }
  }

  // Remove old bindings before replacing the chain, including any binding on
  // the content Element that will be reused by the new plan.
  ClearModifierDrawCallbacks(previous_mount_root.get(), content_element.get());

  auto mount_root = InstallModifierFramePlan(
      plan, content_element.get(), previous_mount_root.get(),
      registration_context, manager->GetEnableParallelElement());
  handle->SetMountRoot(std::move(mount_root));
  if (plan.has_attachments) {
    BindModifierAttachments(plan, handle->mount_root().get(),
                            content_element.get(), registration_context);
  }

  // Parent data follows the physical mount root regardless of authored Modifier
  // order. Clear the previous carrier when topology changes.
  if (previous_mount_root != handle->mount_root()) {
    SetWeightFill(previous_mount_root.get(), true);
  }
  SetWeightFill(handle->mount_root().get(), plan.weight_fill.value_or(true));

  if (plan.has_draw_callbacks) {
    BindModifierDrawCallbacks(plan, handle->mount_root().get(),
                              content_element.get(), registration_context);
    // Request layout for the newly installed chain. Redrawing an existing
    // layer through InvalidateElement does not go through this Apply path.
    content_element->RequestLayout();
  }

  return {true, 0, {}};
}

}  // namespace tasm
}  // namespace lynx
