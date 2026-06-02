// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/element_tree_serializer.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/runtime/lepus/json_parser.h"

namespace lynx {
namespace tasm {

namespace {

bool IsSerializableAttributeValue(const lepus::Value& value) {
  return !value.IsJSFunction() && !value.IsNil() && !value.IsUndefined();
}

lepus::Value BuildInlineStyleValue(Element* node) {
  auto style = lepus::Dictionary::Create();
  auto append_inline_styles = [&style](const RawLepusStyleMap& styles) {
    for (const auto& [key, value] : styles) {
      if (!CSSProperty::IsPropertyValid(key) ||
          !IsSerializableAttributeValue(value)) {
        continue;
      }
      style->SetValue(CSSProperty::GetPropertyName(key), value);
    }
  };

  if (node->GetCurrentRawInlineStyles().has_value()) {
    append_inline_styles(*node->GetCurrentRawInlineStyles());
  }
  if (node->GetCurrentRawImportantInlineStyles().has_value()) {
    append_inline_styles(*node->GetCurrentRawImportantInlineStyles());
  }

  if (style->size() == 0) {
    return lepus::Value();
  }
  return lepus::Value(std::move(style));
}

void AppendRectIfNeeded(const lepus::DictionaryPtr& target,
                        const base::String& key, const float* rect,
                        size_t size) {
  if (rect == nullptr || size < 4) {
    return;
  }

  auto result = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  result->SetValue(kX, rect[0]);
  result->SetValue(kY, rect[1]);
  result->SetValue(kWidth, rect[2]);
  result->SetValue(kHeight, rect[3]);
  target->SetValue(key, std::move(result));
}

bool IsVirtualOrWrapper(Element* node) {
  return node != nullptr && (node->is_virtual() || node->is_wrapper());
}

bool HasUIPrimitive(Element* node) {
  return node != nullptr && node->HasElementContainer() &&
         node->HasUIPrimitive();
}

Element* ResolveVirtualPositionSource(Element* node) {
  while (IsVirtualOrWrapper(node)) {
    node = node->parent();
  }
  return node;
}

bool GetRectToScreen(Element* node, float* position) {
  auto* element_manager = node->element_manager();
  if (element_manager == nullptr ||
      element_manager->painting_context() == nullptr) {
    return false;
  }
  element_manager->painting_context()->GetAbsolutePosition(node->impl_id(),
                                                           position);
  return position[2] >= 0.f && position[3] >= 0.f;
}

void AppendPositionRects(const lepus::DictionaryPtr& position, Element* node) {
  if (node == nullptr) {
    return;
  }

  if (IsVirtualOrWrapper(node)) {
    AppendPositionRects(position, ResolveVirtualPositionSource(node));
    return;
  }

  if (HasUIPrimitive(node)) {
    BASE_STATIC_STRING_DECL(kFrameInRoot, "frameInRoot");
    BASE_STATIC_STRING_DECL(kFrameInScreen, "frameInScreen");
    auto frame_in_root = node->GetRectToLynxView();
    AppendRectIfNeeded(position, kFrameInRoot, frame_in_root.data(),
                       frame_in_root.size());
    float frame_in_screen[4] = {0.f, 0.f, -1.f, -1.f};
    if (GetRectToScreen(node, frame_in_screen)) {
      AppendRectIfNeeded(position, kFrameInScreen, frame_in_screen, 4);
    }
    return;
  }

  // TODO(songshourui.null): Support layout-only node position by sharing the
  // DevTool box model path instead of duplicating offset calculation here.
}

lepus::Value BuildAttributesValue(Element* node) {
  auto attributes = lepus::Dictionary::Create();
  if (node->data_model() == nullptr) {
    return lepus::Value(std::move(attributes));
  }

  for (const auto& [key, value] : node->data_model()->attributes()) {
    if (key == AttributeHolder::kIdSelectorAttrName ||
        !IsSerializableAttributeValue(value)) {
      continue;
    }
    attributes->SetValue(key, value);
  }

  auto inline_style = BuildInlineStyleValue(node);
  if (!inline_style.IsNil()) {
    BASE_STATIC_STRING_DECL(kStyle, "style");
    attributes->SetValue(kStyle, std::move(inline_style));
  }

  if (!node->GetIdSelector().empty()) {
    BASE_STATIC_STRING_DECL(kId, "id");
    attributes->SetValue(kId, node->GetIdSelector());
  }

  if (!node->classes().empty()) {
    auto classes = lepus::CArray::Create();
    for (const auto& clazz : node->classes()) {
      classes->emplace_back(clazz);
    }
    BASE_STATIC_STRING_DECL(kClass, "class");
    attributes->SetValue(kClass, std::move(classes));
  }

  for (const auto& [key, value] : node->dataset()) {
    if (!IsSerializableAttributeValue(value)) {
      continue;
    }
    attributes->SetValue(base::String("data-" + key.str()), value);
  }

  return lepus::Value(std::move(attributes));
}

lepus::Value BuildPositionValue(Element* node) {
  auto position = lepus::Dictionary::Create();
  AppendPositionRects(position, node);
  if (position->size() == 0) {
    return lepus::Value();
  }
  return lepus::Value(std::move(position));
}

void AppendEventHandlers(lepus::CArray* events,
                         const EventMap& event_handlers) {
  BASE_STATIC_STRING_DECL(kBTS, "BTS");
  BASE_STATIC_STRING_DECL(kMTS, "MTS");
  for (const auto& [name, handler] : event_handlers) {
    if (!handler) {
      continue;
    }

    auto event = lepus::Dictionary::Create();
    BASE_STATIC_STRING_DECL(kEventName, "eventName");
    BASE_STATIC_STRING_DECL(kType, "type");
    BASE_STATIC_STRING_DECL(kRuntime, "runtime");
    BASE_STATIC_STRING_DECL(kFunction, "function");
    const base::String& runtime = handler->is_js_event() ? kBTS : kMTS;
    event->SetValue(kEventName, name);
    event->SetValue(kType, handler->type());
    event->SetValue(kRuntime, runtime);
    if (!handler->function().empty()) {
      event->SetValue(kFunction, handler->function());
    }

    events->emplace_back(std::move(event));
  }
}

lepus::Value BuildEventsValue(Element* node) {
  auto events = lepus::CArray::Create();
  AppendEventHandlers(events.get(), node->event_map());
  AppendEventHandlers(events.get(), node->global_bind_event_map());
  AppendEventHandlers(events.get(), node->lepus_event_map());
  if (events->size() == 0) {
    return lepus::Value();
  }
  return lepus::Value(std::move(events));
}

lepus::Value BuildDebugInfoValue(Element* node) {
  auto debug_info = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kNodeIndex, "nodeIndex");
  debug_info->SetValue(kNodeIndex, node->NodeIndex());

  auto* element_manager = node->element_manager();
  auto config =
      element_manager == nullptr ? nullptr : element_manager->GetConfig();
  if (config != nullptr) {
    const auto debug_metadata_url = config->GetDebugMetadataUrl();
    if (!debug_metadata_url.empty()) {
      BASE_STATIC_STRING_DECL(kDebugMetadataUrl, "debugMetadataUrl");
      debug_info->SetValue(kDebugMetadataUrl, base::String(debug_metadata_url));
    }
  }
  return lepus::Value(std::move(debug_info));
}

}  // namespace

lepus::Value ElementTreeSerializer::ToLepusValue(Element* root) {
  if (root == nullptr) {
    return lepus::Value();
  }

  auto result = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kNodeId, "nodeId");
  BASE_STATIC_STRING_DECL(kTag, "tag");
  BASE_STATIC_STRING_DECL(kAttributes, "attributes");
  BASE_STATIC_STRING_DECL(kPosition, "position");
  BASE_STATIC_STRING_DECL(kEvents, "events");
  BASE_STATIC_STRING_DECL(kDebugInfo, "debugInfo");
  BASE_STATIC_STRING_DECL(kChildren, "children");
  BASE_STATIC_STRING_DECL(kId, "id");
  result->SetValue(kNodeId, root->impl_id());
  result->SetValue(kTag, root->GetTag());
  result->SetValue(kAttributes, BuildAttributesValue(root));
  result->SetValue(kId, root->GetIdSelector());

  auto position = BuildPositionValue(root);
  if (!position.IsNil()) {
    result->SetValue(kPosition, std::move(position));
  }

  auto events = BuildEventsValue(root);
  if (!events.IsNil()) {
    result->SetValue(kEvents, std::move(events));
  }

  result->SetValue(kDebugInfo, BuildDebugInfoValue(root));

  auto children = lepus::CArray::Create();
  for (const auto& child : root->children()) {
    children->emplace_back(ToLepusValue(child.get()));
  }
  result->SetValue(kChildren, std::move(children));
  return lepus::Value(std::move(result));
}

std::string ElementTreeSerializer::ToJSONString(Element* root) {
  if (root == nullptr) {
    return "";
  }
  return lepus::lepusValueToJSONString(ToLepusValue(root), true);
}

}  // namespace tasm
}  // namespace lynx
