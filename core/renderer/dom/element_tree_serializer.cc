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
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/runtime/lepus/json_parser.h"

namespace lynx {
namespace tasm {

namespace {

bool IsValidRect(float, float, float width, float height) {
  return width >= 0.f && height >= 0.f;
}

bool IsValidRect(const float* rect, size_t size) {
  return rect != nullptr && size >= 4 &&
         IsValidRect(rect[0], rect[1], rect[2], rect[3]);
}

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
  if (!IsValidRect(rect, size)) {
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

void AppendOffsetRectIfNeeded(const lepus::DictionaryPtr& target,
                              const base::String& key,
                              const std::vector<float>& anchor_rect,
                              float offset_x, float offset_y, float width,
                              float height) {
  if (!IsValidRect(anchor_rect.data(), anchor_rect.size())) {
    return;
  }

  const float x = anchor_rect[0] + offset_x;
  const float y = anchor_rect[1] + offset_y;
  if (!IsValidRect(x, y, width, height)) {
    return;
  }

  auto result = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kX, "x");
  BASE_STATIC_STRING_DECL(kY, "y");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  result->SetValue(kX, x);
  result->SetValue(kY, y);
  result->SetValue(kWidth, width);
  result->SetValue(kHeight, height);
  target->SetValue(key, std::move(result));
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
    auto frame_in_screen = node->GetRectToScreen();
    AppendRectIfNeeded(position, kFrameInScreen, frame_in_screen.data(),
                       frame_in_screen.size());
    return;
  }

  // TODO(songshourui.null): Unify this lightweight layout-only and virtual node
  // position resolution with the DevTool box model path.
  auto* layout_object = node->GetLayoutObject();
  if (layout_object == nullptr) {
    return;
  }

  float offset_x = 0.f;
  float offset_y = 0.f;
  auto* anchor = node;
  while (anchor != nullptr && !HasUIPrimitive(anchor)) {
    auto* current_layout_object = anchor->GetLayoutObject();
    if (current_layout_object != nullptr) {
      offset_x +=
          current_layout_object->GetBorderBoundLeftFromParentPaddingBound();
      offset_y +=
          current_layout_object->GetBorderBoundTopFromParentPaddingBound();
    }
    do {
      anchor = anchor->parent();
    } while (anchor != nullptr && anchor->is_wrapper());
  }

  if (anchor == nullptr) {
    return;
  }

  auto* anchor_layout_object = anchor->GetLayoutObject();
  if (anchor_layout_object != nullptr) {
    offset_x += anchor_layout_object->GetLayoutBorderLeftWidth();
    offset_y += anchor_layout_object->GetLayoutBorderTopWidth();
  }

  const float width = layout_object->GetBorderBoundWidth();
  const float height = layout_object->GetBorderBoundHeight();
  BASE_STATIC_STRING_DECL(kFrameInRoot, "frameInRoot");
  BASE_STATIC_STRING_DECL(kFrameInScreen, "frameInScreen");
  AppendOffsetRectIfNeeded(position, kFrameInRoot, anchor->GetRectToLynxView(),
                           offset_x, offset_y, width, height);
  AppendOffsetRectIfNeeded(position, kFrameInScreen, anchor->GetRectToScreen(),
                           offset_x, offset_y, width, height);
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

std::string ResolveEntryName(Element* node) {
  for (auto* current = node; current != nullptr; current = current->parent()) {
    const auto& entry_name = current->entry_name();
    if (!entry_name.empty()) {
      return entry_name.str();
    }
  }
  return "";
}

std::shared_ptr<PageConfig> ResolvePageConfig(Element* node) {
  auto* element_manager = node->element_manager();
  if (element_manager == nullptr) {
    return nullptr;
  }

  auto* delegate = element_manager->element_manager_delegate();
  auto entry_name = ResolveEntryName(node);
  if (delegate != nullptr && !entry_name.empty()) {
    auto entry_config = delegate->GetPageConfigForEntry(entry_name);
    if (entry_config != nullptr) {
      return entry_config;
    }
  }
  return element_manager->GetConfig();
}

lepus::Value BuildDebugInfoValue(Element* node) {
  auto debug_info = lepus::Dictionary::Create();
  BASE_STATIC_STRING_DECL(kNodeIndex, "nodeIndex");
  debug_info->SetValue(kNodeIndex, node->NodeIndex());

  auto config = ResolvePageConfig(node);
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
