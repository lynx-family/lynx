// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>

#include "base/include/auto_reset.h"
#include "base/include/value/base_string.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/element_utils.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/text_fragment_behavior.h"

namespace lynx {
namespace tasm {

TextElement::TextElement(ElementManager* manager, const base::String& tag)
    : Element(manager, tag) {
  is_text_ = true;
  if (element_manager_ == nullptr) {
    return;
  }
  SetDefaultOverflow(element_manager_->GetDefaultTextOverflow());

  if (Config::DefaultFontScale() != manager->GetLynxEnvConfig().FontScale() &&
      EnableLayoutInElementMode()) {
    // if fontScale is 1.0f, force to process fontSize
    // FIXME(linxs): maybe there is a better way to set FontSize bit
    property_bits_.Set(kPropertyIDFontSize);
  }
  element_manager_->IncreaseTextElementCount();
}

TextElement::~TextElement() {
  if (ShouldDestroy()) {
    element_manager_->DestroyText(this);
  }
}

void TextElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  Element::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultTextOverflow());
}

void TextElement::SetStyleInternal(CSSPropertyID id,
                                   const tasm::CSSValue& value) {
  Element::SetStyleInternal(id, value);

  if (id == kPropertyIDFontFamily) {
    if (!EnableLayoutInElementMode()) {
      EnqueueLayoutTask(
          [this, value]() { ResolveAndFlushFontFaces(value.AsString()); });
    } else {
      ResolveAndFlushFontFaces(value.AsString());
    }
  }
}

void TextElement::OnNodeAdded(Element* child) {
  child->ConvertToInlineElement();
  UpdateRenderRootElementIfNecessary(child);
  if (!child->is_raw_text()) {
    has_inline_child_ = true;
  }
}

bool TextElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  if (EnableLayoutInElementMode()) {
    return ProcessAttributeForLayoutInElement(key, value);
  } else if (ProcessAttributeForNormalLayoutMode(key, value)) {
    return true;
  }
  return Element::SetAttributeInternal(key, value);
}

void TextElement::ResetAttribute(const base::String& key) {
  if (!EnableLayoutInElementMode() && key.IsEqual(kTextOverflowAttr)) {
    RemoveStyleFromAttributes(kPropertyIDTextOverflow);
  }
  if (EnableLayoutInElementMode()) {
    ProcessAttributeForLayoutInElement(key, lepus::Value(), true);
    return;
  }
  Element::ResetAttribute(key);
}

const StyleMap* TextElement::PeekCommittedStylesFromAttributes() const {
  if (!committed_styles_from_attributes_.has_value()) {
    return nullptr;
  }
  return &*committed_styles_from_attributes_;
}

void TextElement::CacheCommittedStyleFromAttributes(CSSPropertyID id,
                                                    const CSSValue& value) {
  committed_styles_from_attributes_->insert_or_assign(id, value);
}

void TextElement::CacheCommittedStyleFromAttributes(CSSPropertyID id,
                                                    const lepus::Value& value) {
  UnitHandler::Process(id, value, *committed_styles_from_attributes_,
                       element_manager()->GetCSSParserConfigs());
}

void TextElement::RemoveCommittedStyleFromAttributes(CSSPropertyID id) {
  if (!committed_styles_from_attributes_.has_value()) {
    return;
  }
  committed_styles_from_attributes_->erase(id);
  if (committed_styles_from_attributes_->empty()) {
    committed_styles_from_attributes_.reset();
  }
}

bool TextElement::ProcessAttributeForLayoutInElement(const base::String& key,
                                                     const lepus::Value& value,
                                                     bool is_reset) {
  if (key.IsEqual(kTextAttr)) {
    auto content = !is_reset ? ConvertTextContent(value) : base::String();
    if (content_.IsEqual(content)) {
      return false;
    }
    content_ = std::move(content);
    content_utf16_length_ =
        GetUtf16SizeFromUtf8(content_.c_str(), content_.length());
    if (!preparing_detached_measurement_) {
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  } else if (key.IsEqual(kTextMaxlineAttr)) {
    if (is_reset) {
      if (!text_props_ || !text_props_->text_max_line.has_value()) {
        return false;
      }
      text_props_->text_max_line.reset();
    } else {
      const int text_max_line =
          value.IsNumber() ? value.Number() : std::stoi(value.StdString());
      if (text_props_ && text_props_->text_max_line == text_max_line) {
        return false;
      }
      EnsureTextProps();
      text_props_->text_max_line = text_max_line;
    }
    if (!preparing_detached_measurement_) {
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  } else if (key.IsEqual(kTextLineHeightAlignmentAttr)) {
    if (is_reset) {
      if (!text_props_ || !text_props_->line_height_alignment.has_value()) {
        return false;
      }
      text_props_->line_height_alignment.reset();
    } else {
      if (!value.IsString()) {
        return false;
      }
      uint32_t alignment;
      const auto& string_value = value.StdString();
      if (string_value == "top") {
        alignment = static_cast<uint32_t>(LineHeightAlignment::kTop);
      } else if (string_value == "center") {
        alignment = static_cast<uint32_t>(LineHeightAlignment::kCenter);
      } else if (string_value == "bottom") {
        alignment = static_cast<uint32_t>(LineHeightAlignment::kBottom);
      } else {
        return false;
      }
      if (text_props_ && text_props_->line_height_alignment == alignment) {
        return false;
      }
      EnsureTextProps();
      text_props_->line_height_alignment = alignment;
    }
    if (!preparing_detached_measurement_) {
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  } else if (key.IsEqual(kTextLineHeightTrimAttr)) {
    if (is_reset) {
      if (!text_props_ || !text_props_->line_height_trim.has_value()) {
        return false;
      }
      text_props_->line_height_trim.reset();
    } else {
      if (!value.IsString()) {
        return false;
      }
      uint32_t trim;
      const auto& string_value = value.StdString();
      if (string_value == "none") {
        trim = kLineHeightTrimNone;
      } else if (string_value == "first-line-top") {
        trim = kLineHeightTrimFirstLineTop;
      } else if (string_value == "last-line-bottom") {
        trim = kLineHeightTrimLastLineBottom;
      } else if (string_value == "both") {
        trim = kLineHeightTrimBoth;
      } else {
        return false;
      }
      if (text_props_ && text_props_->line_height_trim == trim) {
        return false;
      }
      EnsureTextProps();
      text_props_->line_height_trim = trim;
    }
    if (!preparing_detached_measurement_) {
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  } else if (key.IsEqual(kTextLineHeightModeAttr)) {
    if (is_reset) {
      if (!text_props_ || !text_props_->line_height_mode.has_value()) {
        return false;
      }
      text_props_->line_height_mode.reset();
    } else {
      if (!value.IsString()) {
        return false;
      }
      uint32_t mode;
      const auto& string_value = value.StdString();
      if (string_value == "fixed") {
        mode = static_cast<uint32_t>(LineHeightMode::kFixed);
      } else if (string_value == "minimum") {
        mode = static_cast<uint32_t>(LineHeightMode::kMinimum);
      } else {
        return false;
      }
      if (text_props_ && text_props_->line_height_mode == mode) {
        return false;
      }
      EnsureTextProps();
      text_props_->line_height_mode = mode;
    }
    if (!preparing_detached_measurement_) {
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  }

  if (is_reset) {
    Element::ResetAttribute(key);
    return true;
  }
  return Element::SetAttributeInternal(key, value);
}

bool TextElement::ProcessAttributeForNormalLayoutMode(
    const base::String& key, const lepus::Value& value) {
  if (key.IsEqual(kTextOverflowAttr)) {
    CacheStyleFromAttributes(kPropertyIDTextOverflow, value);
    has_layout_only_props_ = false;
    return true;
  }

  if (key.IsEqual(kTextAttr) && !children().empty()) {
    // if setNativeProps with key "text" on TextElement, we need to update it's
    // children.
    auto* first_child = children().front().get();
    if (first_child->is_raw_text()) {
      auto* raw_text = static_cast<RawTextElement*>(first_child);
      raw_text->SetText(value);
    }
    return true;
  }
  return false;
}

void TextElement::ConvertToInlineElement() {
  if (tag_.IsEqual(kElementXTextTag)) {
    tag_ = BASE_STATIC_STRING(kElementXInlineTextTag);
  } else {
    tag_ = BASE_STATIC_STRING(kElementInlineTextTag);
  }
  data_model()->set_tag(tag_);
  UpdateTagToLayoutBundle();
  Element::ConvertToInlineElement();
}

void TextElement::MarkLayoutInElementTextMeasurerPropertyIfNeeded(
    CSSPropertyID id) {
  if (EnableLayoutInElementMode() && IsTextMeasurerWanted(id)) {
    property_bits_.Set(id);
  }
}

void TextElement::ReplayElementSpecificStyleSideEffect(CSSPropertyID id) {
  if (id != kPropertyIDFontFamily) {
    return;
  }

  const auto& resolved_values = computed_css_style()->GetResolvedValues();
  auto it = resolved_values.find(kPropertyIDFontFamily);
  if (it == resolved_values.end()) {
    return;
  }

  auto font_family = it->second.AsString();
  if (font_family.empty()) {
    return;
  }

  if (!EnableLayoutInElementMode()) {
    EnqueueLayoutTask(
        [this, font_family]() { ResolveAndFlushFontFaces(font_family); });
  } else {
    ResolveAndFlushFontFaces(font_family);
  }
}

void TextElement::ResolveAndFlushFontFaces(const base::String& font_family) {
  auto* fragment = GetRelatedCSSFragment();
  if (fragment) {
    fragment->ForEachUnresolvedFontFaceMap(
        [](const CSSFontFaceRuleMap& font_face_map, void* cb_data) {
          auto* self = static_cast<TextElement*>(cb_data);
          if (!font_face_map.empty()) {
            // FIXME(linxs): parse the font face according to font_family,
            // instead of flushing all font faces
            self->SetFontFaces(font_face_map);
          }
        },
        this);
    fragment->MarkFontFacesResolved(true);
  }
}

bool TextElement::ResolveStyleValue(CSSPropertyID id,
                                    const tasm::CSSValue& value) {
  bool has_processed = false;

  if (EnableLayoutInElementMode() && IsTextMeasurerWanted(id)) {
    if (computed_css_style()->SetValue(id, value)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = Element::ResolveStyleValue(id, value);
  }

  return has_processed;
}

bool TextElement::ResetCSSValue(CSSPropertyID id) {
  bool has_processed = false;
  if (EnableLayoutInElementMode()) {
    if (id == kPropertyIDFontSize) {
      // font-size has been reset to default value in WillResetCSSValue
      return false;
    }
    if (computed_css_style()->ResetValue(id)) {
      property_bits_.Set(id);
      has_processed = true;
    }
  } else {
    has_processed = Element::ResetCSSValue(id);
  }

  return has_processed;
}

void TextElement::DispatchLayoutBefore() {
  if (is_inline_element()) {
    return;
  }

  element_manager_->DispatchLayoutBefore(this);
}

void TextElement::PrepareForDetachedMeasurement() {
  base::AutoReset<bool> preparation_scope(&preparing_detached_measurement_,
                                          true);
  ConsumeAllAttributes();

  // Share CSS resolution (including units, variables and attribute styles),
  // but not the normal flush pipeline and its layout/animation side effects.
  auto style = style_resolver_.ResolveBaseStyle(nullptr, computed_css_style());
  for (const auto& [id, value] : computed_css_style()->GetResolvedValues()) {
    MarkLayoutInElementTextMeasurerPropertyIfNeeded(id);
  }
  for (const auto& [id, value] : style->GetResolvedValues()) {
    MarkLayoutInElementTextMeasurerPropertyIfNeeded(id);
  }
  if (slnode()) {
    slnode()->SetLayoutStyle(style->GetLayoutComputedStyle());
  }
  platform_css_style_ = std::move(style);
  // The platform cache is recreated for every measurement, including when the
  // default font size is used or a previously explicit font size was removed.
  property_bits_.Set(kPropertyIDFontSize);
  UpdateLayoutNodeFontSize(computed_css_style()->GetFontSize(),
                           computed_css_style()->GetRootFontSize());
  ReplayElementSpecificStyleSideEffect(kPropertyIDFontFamily);
  ClearCachedStylesFromAttributes();
}

void TextElement::DispatchLayoutAfter() {
  if (!EnableLayoutInElementMode()) {
    Element::DispatchLayoutAfter();
  } else if (!is_inline_element() && has_pending_layout_event_) {
    // Position-only updates must not resend an event for the same measurement.
    has_pending_layout_event_ = false;
    if (!EnableFragmentLayerRender()) {
      TextFragmentBehavior::DispatchLayoutEvent(
          this, text_layout_event_size_.width_,
          text_layout_event_size_.height_);
    }
  }
}

void TextElement::OnEventHandlersChanged(const base::String& name) {
  // TODO: Refresh inline geometry when tap bindings change without text layout.
  if (name.empty() || name.IsEquals("layout")) {
    UpdateLayoutEventState();
  }
}

void TextElement::UpdateLayoutEventState() {
  BASE_STATIC_STRING_DECL(kLayoutEvent, "layout");
  const bool has_layout_event = event_map().count(kLayoutEvent) > 0 ||
                                lepus_event_map().count(kLayoutEvent) > 0;
  const bool had_layout_event = has_layout_event_;
  has_layout_event_ = has_layout_event;
  if (EnableLayoutInElementMode() && !is_inline_element() && has_layout_event &&
      !had_layout_event) {
    // A new layout handler needs a fresh measured result before UI
    // installation.
    MarkLayoutDirtyLite();
  }
}

LayoutResult TextElement::Measure(float width, int32_t width_mode, float height,
                                  int32_t height_mode, bool final_measure) {
  if (is_inline_element()) {
    return LayoutResult(0, 0, 0);
  }

  return element_manager_->MeasureText(this, width, width_mode, height,
                                       height_mode);
}

void TextElement::Align() {
  if (is_inline_element() || !need_layout_children_) {
    return;
  }

  element_manager_->AlignText(this);
}

void TextElement::OnLayoutObjectCreated() {
  if (!is_inline_element()) {
    SetMeasureFunc(
        this, [](void* context, const starlight::Constraints& constraints,
                 bool final_measure) {
          TextElement* element = static_cast<TextElement*>(context);
          DCHECK(element);
          SLMeasureMode width_mode = constraints[starlight::kHorizontal].Mode();
          SLMeasureMode height_mode = constraints[starlight::kVertical].Mode();
          float width = IsSLIndefiniteMode(width_mode)
                            ? 0.f
                            : constraints[starlight::kHorizontal].Size();
          float height = IsSLIndefiniteMode(height_mode)
                             ? 0.f
                             : constraints[starlight::kVertical].Size();

          LayoutResult result = element->Measure(width, width_mode, height,
                                                 height_mode, final_measure);

          return FloatSize(result.width_, result.height_, result.baseline_);
        });

    SetAlignmentFunc(this, [](void* context) {
      TextElement* element = static_cast<TextElement*>(context);
      DCHECK(element);
      element->Align();
    });
  }
}

void TextElement::UpdateLayoutNodeFontSize(double cur_node_font_size,
                                           double root_node_font_size) {
  if (EnableLayoutInElementMode()) {
    property_bits_.Set(kPropertyIDFontSize);
    computed_css_style()->SetValue(
        kPropertyIDFontSize,
        CSSValue(cur_node_font_size, CSSValuePattern::NUMBER));
  } else {
    Element::UpdateLayoutNodeFontSize(cur_node_font_size, root_node_font_size);
  }
}

void TextElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<TextFragmentBehavior>(fragment));
}

void TextElement::SetInlineTextRanges(
    const std::vector<PlatformTextEventTargetRange>& ranges) {
  // Every entry is copied below; avoid zero-filling the range buffer first.
  inline_text_ranges_.resize<false>(ranges.size());
  std::copy(ranges.begin(), ranges.end(), inline_text_ranges_.begin());
  // Assigning character ranges does not replace the separate geometry cache.
  // If no ranges remain, discard the old rectangles now; otherwise measurement
  // will replace them after the new layout is available.
  if (inline_text_ranges_.empty()) {
    UpdateInlineTextGeometry({});
  }
}

void TextElement::UpdateInlineTextGeometry(
    const PlatformTextEventTargetRegions& regions) {
  // Each measurement replaces the whole result, including removed inline ids.
  inline_text_geometry_.clear();
  for (const auto& region : regions) {
    if (region.sign == impl_id() || region.width <= 0 || region.height <= 0 ||
        !std::isfinite(region.left) || !std::isfinite(region.top) ||
        !std::isfinite(region.width) || !std::isfinite(region.height)) {
      continue;
    }
    base::geometry::FloatRect rect({region.left, region.top},
                                   {region.width, region.height});
    auto target = std::find_if(
        inline_text_geometry_.begin(), inline_text_geometry_.end(),
        [&](const auto& item) { return item.sign == region.sign; });
    if (target == inline_text_geometry_.end()) {
      auto& geometry = inline_text_geometry_.emplace_back();
      geometry.sign = region.sign;
      geometry.bounds = rect;
      geometry.rects.push_back(rect);
    } else {
      auto& bounds = target->bounds;
      const float left = std::min(bounds.X(), rect.X());
      const float top = std::min(bounds.Y(), rect.Y());
      const float right = std::max(bounds.MaxX(), rect.MaxX());
      const float bottom = std::max(bounds.MaxY(), rect.MaxY());
      bounds = {{left, top}, {right - left, bottom - top}};
      target->rects.push_back(rect);
    }
  }
}

const TextElement::InlineTextGeometry* TextElement::GetInlineTextGeometry(
    int32_t sign) const {
  for (const auto& geometry : inline_text_geometry_) {
    if (geometry.sign == sign) {
      return &geometry;
    }
  }
  return nullptr;
}

void TextElement::SetTextBundle(intptr_t text_bundle) {
  auto* fragment = fragment_impl();
  if (fragment) {
    fragment->SetTextBundle(text_bundle);
  }
}

}  // namespace tasm
}  // namespace lynx
