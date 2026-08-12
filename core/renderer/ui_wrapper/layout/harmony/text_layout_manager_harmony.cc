// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/harmony/text_layout_manager_harmony.h"

#include <utility>
#include <vector>

#include "core/base/harmony/props_constant.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "core/style/color.h"
#include "core/style/default_computed_style.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/inline_placeholder_shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/text_shadow_node.h"

namespace lynx {
namespace tasm {

namespace {

void SyncComputedTextStyle(TextElement* element,
                           harmony::ShadowNode* shadow_node) {
  if (element == nullptr || shadow_node == nullptr) {
    return;
  }

  auto* style = element->computed_css_style();
  if (style == nullptr) {
    return;
  }

  const auto& text_attributes = style->GetTextAttributes();
  if (text_attributes.has_value()) {
    const auto& property_bits = element->property_bits();
    if (property_bits.Has(kPropertyIDColor)) {
      if (text_attributes->text_gradient.has_value() &&
          text_attributes->text_gradient->IsArray()) {
        shadow_node->OnPropsUpdate(harmony::kColor,
                                   *text_attributes->text_gradient);
      } else {
        const uint32_t color =
            text_attributes->color.has_value()
                ? *text_attributes->color
                : starlight::DefaultColor::DEFAULT_TEXT_COLOR;
        shadow_node->OnPropsUpdate(harmony::kColor, lepus::Value(color));
      }
    }
    shadow_node->OnPropsUpdate(harmony::kFontSize,
                               lepus::Value(style->GetFontSize()));
    if (property_bits.Has(kPropertyIDLineHeight)) {
      shadow_node->OnPropsUpdate(
          harmony::kLineHeight,
          lepus::Value(text_attributes->computed_line_height));
    }
    if (property_bits.Has(kPropertyIDFontFamily)) {
      shadow_node->OnPropsUpdate(
          harmony::kFontFamily,
          lepus::Value(text_attributes->font_family.str()));
    }
    if (property_bits.Has(kPropertyIDFontWeight)) {
      shadow_node->OnPropsUpdate(
          harmony::kFontWeight,
          lepus::Value(static_cast<int32_t>(text_attributes->font_weight)));
    }
    if (property_bits.Has(kPropertyIDFontStyle)) {
      shadow_node->OnPropsUpdate(
          harmony::kFontStyle,
          lepus::Value(static_cast<int32_t>(text_attributes->font_style)));
    }
    if (property_bits.Has(kPropertyIDLetterSpacing)) {
      shadow_node->OnPropsUpdate(harmony::kLetterSpacing,
                                 lepus::Value(text_attributes->letter_spacing));
    }
    shadow_node->OnPropsUpdate(
        harmony::kWordBreak,
        lepus::Value(static_cast<int32_t>(text_attributes->word_break)));
  }

  if (!shadow_node->IsTextShadowNode()) {
    return;
  }

  auto direction = style->GetDirection();
  direction = direction == starlight::DirectionType::kRtl ||
                      direction == starlight::DirectionType::kLynxRtl
                  ? starlight::DirectionType::kRtl
                  : starlight::DirectionType::kLtr;
  shadow_node->OnPropsUpdate(harmony::kDirection,
                             lepus::Value(static_cast<int32_t>(direction)));
  if (text_attributes.has_value()) {
    const auto& property_bits = element->property_bits();
    if (property_bits.Has(kPropertyIDTextAlign)) {
      shadow_node->OnPropsUpdate(
          harmony::kTextAlign,
          lepus::Value(static_cast<int32_t>(text_attributes->text_align)));
    }
    if (property_bits.Has(kPropertyIDWhiteSpace)) {
      shadow_node->OnPropsUpdate(
          harmony::kWhiteSpace,
          lepus::Value(static_cast<int32_t>(text_attributes->white_space)));
    }
    if (property_bits.Has(kPropertyIDTextOverflow)) {
      shadow_node->OnPropsUpdate(
          harmony::kTextOverflow,
          lepus::Value(static_cast<int32_t>(text_attributes->text_overflow)));
    }
  }

  const auto* text_props = element->text_props();
  const int32_t max_line =
      text_props != nullptr && text_props->text_max_line.has_value()
          ? *text_props->text_max_line
          : starlight::DefaultComputedStyle::DEFAULT_TEXT_MAX_LINE;
  shadow_node->OnPropsUpdate(harmony::kTextMaxLine, lepus::Value(max_line));
}

void SyncInlinePlaceholderStyle(Element* element,
                                harmony::ShadowNode* shadow_node) {
  if (element == nullptr || shadow_node == nullptr ||
      !shadow_node->IsPlaceholder()) {
    return;
  }
  static_cast<harmony::InlinePlaceholderShadowNode*>(shadow_node)
      ->SetIsInlineImage(element->is_image());

  auto* style = element->computed_css_style();
  if (style == nullptr) {
    return;
  }
  const auto& text_attributes = style->GetTextAttributes();
  if (!text_attributes.has_value()) {
    return;
  }

  auto vertical_align = lepus::CArray::Create();
  vertical_align->emplace_back(
      static_cast<int32_t>(text_attributes->vertical_align));
  vertical_align->emplace_back(text_attributes->vertical_align_length);
  shadow_node->OnPropsUpdate(harmony::kVerticalAlign,
                             lepus::Value(std::move(vertical_align)));
}

bool HasInlineImage(Element* element) {
  if (element == nullptr) {
    return false;
  }
  for (auto* child = element->first_render_child(); child != nullptr;
       child = child->next_render_sibling()) {
    if (child->is_image() && child->is_inline_element()) {
      return true;
    }
    if ((child->is_text() || child->is_wrapper()) && HasInlineImage(child)) {
      return true;
    }
  }
  return false;
}

}  // namespace

harmony::ShadowNode* TextLayoutManagerHarmony::GetOrCreateShadowNode(
    Element* element, NodeIdSet& current_node_ids) {
  if (node_owner_ == nullptr || element == nullptr) {
    return nullptr;
  }

  const int32_t id = element->impl_id();
  auto* node = node_owner_->FindShadowNodeBySign(id);
  if (node != nullptr) {
    if (owned_node_ids_.count(id) != 0) {
      current_node_ids.insert(id);
    }
    return node;
  }

  PropBundleHarmony props;
  node_owner_->CreateShadowNode(id, element->GetTag().str(), &props,
                                element->is_inline_element());
  node = node_owner_->FindShadowNodeBySign(id);
  if (node != nullptr) {
    owned_node_ids_.insert(id);
    current_node_ids.insert(id);
  }
  return node;
}

harmony::TextShadowNode* TextLayoutManagerHarmony::GetOrCreateTextShadowNode(
    Element* element, NodeIdSet& current_node_ids) {
  auto* node = GetOrCreateShadowNode(element, current_node_ids);
  if (node == nullptr || !node->IsTextShadowNode()) {
    return nullptr;
  }
  return static_cast<harmony::TextShadowNode*>(node);
}

void TextLayoutManagerHarmony::SyncTextSubtree(Element* element,
                                               harmony::ShadowNode* shadow_node,
                                               NodeIdSet& current_node_ids) {
  if (element == nullptr || shadow_node == nullptr || node_owner_ == nullptr) {
    return;
  }

  if (element->is_raw_text()) {
    const auto& content = static_cast<RawTextElement*>(element)->content();
    shadow_node->OnPropsUpdate(harmony::kTextAttr, lepus::Value(content.str()));
  } else if (element->is_text()) {
    auto* text_element = static_cast<TextElement*>(element);
    SyncComputedTextStyle(text_element, shadow_node);
    const auto& content = text_element->content();
    shadow_node->OnPropsUpdate(harmony::kTextAttr, lepus::Value(content.str()));
  }

  std::vector<std::pair<Element*, harmony::ShadowNode*>> children;
  auto collect_children = [&](auto&& self, Element* parent) -> void {
    for (auto* child = parent->first_render_child(); child != nullptr;
         child = child->next_render_sibling()) {
      auto* child_node = GetOrCreateShadowNode(child, current_node_ids);
      if (child_node != nullptr) {
        children.emplace_back(child, child_node);
      } else if (child->is_wrapper()) {
        self(self, child);
      }
    }
  };
  collect_children(collect_children, element);

  const auto current_children = shadow_node->GetChildren();
  for (auto* child : current_children) {
    shadow_node->RemoveChild(child);
  }
  for (const auto& [child_element, child_node] : children) {
    SyncInlinePlaceholderStyle(child_element, child_node);
    shadow_node->AddChild(child_node, -1);
    if (child_element->is_text() || child_element->is_raw_text()) {
      SyncTextSubtree(child_element, child_node, current_node_ids);
    }
  }
}

void TextLayoutManagerHarmony::DispatchLayoutBefore(Element* element) {
  if (element == nullptr || !element->is_text()) {
    return;
  }

  NodeIdSet current_node_ids;
  auto* text_node = GetOrCreateTextShadowNode(element, current_node_ids);
  if (text_node == nullptr) {
    return;
  }

  SyncTextSubtree(element, text_node, current_node_ids);
  // Alignment resolves visible placeholder bounds after wrapping and
  // truncation. The renderer uses those bounds to draw inline images.
  static_cast<TextElement*>(element)->set_need_layout_children(
      HasInlineImage(element));

  const int32_t root_id = element->impl_id();
  auto previous_it = text_tree_node_ids_.find(root_id);
  if (previous_it != text_tree_node_ids_.end()) {
    NodeIdSet removed_node_ids;
    for (int32_t id : previous_it->second) {
      if (current_node_ids.count(id) == 0) {
        removed_node_ids.insert(id);
      }
    }
    DestroyNodes(removed_node_ids, root_id);
  }
  text_tree_node_ids_[root_id] = std::move(current_node_ids);
  text_node->OnLayoutBefore();
}

LayoutResult TextLayoutManagerHarmony::Measure(Element* element, float width,
                                               int width_mode, float height,
                                               int height_mode) {
  NodeIdSet unused_node_ids;
  auto* text_node = GetOrCreateTextShadowNode(element, unused_node_ids);
  if (text_node == nullptr) {
    return {};
  }
  return text_node->Measure(
      width, static_cast<harmony::MeasureMode>(width_mode), height,
      static_cast<harmony::MeasureMode>(height_mode), true);
}

void TextLayoutManagerHarmony::Align(Element* element) {
  NodeIdSet unused_node_ids;
  auto* text_node = GetOrCreateTextShadowNode(element, unused_node_ids);
  if (text_node != nullptr) {
    text_node->Align();
  }
}

void TextLayoutManagerHarmony::DestroyNodes(const NodeIdSet& node_ids,
                                            int32_t root_id) {
  if (node_owner_ == nullptr || node_ids.empty()) {
    return;
  }

  for (int32_t id : node_ids) {
    if (id != root_id && owned_node_ids_.erase(id) != 0) {
      node_owner_->DestroyNode(id);
    }
  }
  if (node_ids.count(root_id) != 0 && owned_node_ids_.erase(root_id) != 0) {
    node_owner_->DestroyNode(root_id);
  }
}

void TextLayoutManagerHarmony::RemoveNodeIdsFromTextTrees(
    const NodeIdSet& node_ids) {
  for (auto it = text_tree_node_ids_.begin();
       it != text_tree_node_ids_.end();) {
    for (int32_t id : node_ids) {
      it->second.erase(id);
    }
    if (it->second.empty()) {
      it = text_tree_node_ids_.erase(it);
    } else {
      ++it;
    }
  }
}

void TextLayoutManagerHarmony::Destroy(Element* element) {
  if (element == nullptr || node_owner_ == nullptr) {
    return;
  }

  const int32_t id = element->impl_id();
  NodeIdSet node_ids;
  auto tree_it = text_tree_node_ids_.find(id);
  if (tree_it != text_tree_node_ids_.end()) {
    node_ids = std::move(tree_it->second);
    text_tree_node_ids_.erase(tree_it);
  } else if (owned_node_ids_.count(id) != 0) {
    node_ids.insert(id);
  }

  DestroyNodes(node_ids, id);
  RemoveNodeIdsFromTextTrees(node_ids);
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
TextLayoutManagerHarmony::GetTextBundle(int32_t id) {
  return node_owner_ != nullptr ? node_owner_->GetExtraBundle(id) : nullptr;
}

}  // namespace tasm
}  // namespace lynx
