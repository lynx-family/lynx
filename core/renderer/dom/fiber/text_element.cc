// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/text_element.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "base/include/string/string_utils.h"
#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/text_fragment_behavior.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {

namespace {
constexpr char kPunctuationTrimRuleAttr[] = "punctuation-trim-rule";

bool ParsePunctuationTrimRatio(const lepus::Value& rule,
                               const base::String& key, float& output) {
  if (!rule.Contains(key)) {
    return true;
  }
  const auto value = rule.GetProperty(key);
  if (!value.IsNumber()) {
    return false;
  }
  const double ratio = value.Number();
  if (!(ratio >= 0.0 && ratio <= 1.0)) {
    return false;
  }
  output = static_cast<float>(ratio);
  return true;
}

base::Vector<PunctuationTrimRule> ParsePunctuationTrimRules(
    const lepus::Value& value) {
  base::Vector<PunctuationTrimRule> rules;
  if (!value.IsObject()) {
    return rules;
  }
  ForEachLepusValue(value, [&](const lepus::Value& key,
                               const lepus::Value& value) {
    const auto unicode = base::U8StringToU32(key.StringView());
    if (unicode.size() != 1 || unicode[0] > 0x10ffff ||
        (unicode[0] >= 0xd800 && unicode[0] <= 0xdfff) || !value.IsObject()) {
      return;
    }
    BASE_STATIC_STRING_DECL(kType, "type");
    const auto type = value.GetProperty(kType);
    if (!type.IsString()) {
      return;
    }
    PunctuationTrimRule rule{};
    rule.unicode = unicode[0];
    const auto& name = type.StdString();
    if (name == "open") {
      rule.type = PunctuationTrimRule::Type::kOpen;
    } else if (name == "close") {
      rule.type = PunctuationTrimRule::Type::kClose;
    } else if (name == "middle") {
      rule.type = PunctuationTrimRule::Type::kMiddle;
    } else {
      return;
    }
    BASE_STATIC_STRING_DECL(kDefault, "default");
    BASE_STATIC_STRING_DECL(kEdge, "edge");
    BASE_STATIC_STRING_DECL(kAdjacent, "adjacent");
    if (ParsePunctuationTrimRatio(value, kDefault, rule.default_ratio) &&
        ParsePunctuationTrimRatio(value, kEdge, rule.edge_ratio) &&
        ParsePunctuationTrimRatio(value, kAdjacent, rule.adjacent_ratio)) {
      rules.push_back(rule);
    }
  });
  // Canonicalize rule order without instantiating a sort for this record type.
  static_assert(std::is_trivially_copyable_v<PunctuationTrimRule>);
  if (rules.size() > 1) {
    std::qsort(
        rules.data(), rules.size(), sizeof(PunctuationTrimRule),
        [](const void* lhs, const void* rhs) {
          const auto a = static_cast<const PunctuationTrimRule*>(lhs)->unicode;
          const auto b = static_cast<const PunctuationTrimRule*>(rhs)->unicode;
          return static_cast<int>(a) - static_cast<int>(b);
        });
  }
  return rules;
}
}  // namespace

TextElement::TextElement(ElementManager* manager, const base::String& tag)
    : FiberElement(manager, tag) {
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
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  SetDefaultOverflow(manager->GetDefaultTextOverflow());
}

void TextElement::SetStyleInternal(CSSPropertyID id,
                                   const tasm::CSSValue& value) {
  FiberElement::SetStyleInternal(id, value);

  if (id == kPropertyIDFontFamily) {
    if (!EnableLayoutInElementMode()) {
      EnqueueLayoutTask(
          [this, value]() { ResolveAndFlushFontFaces(value.AsString()); });
    } else {
      ResolveAndFlushFontFaces(value.AsString());
    }
  }
}

void TextElement::OnNodeAdded(FiberElement* child) {
  child->ConvertToInlineElement();
  UpdateRenderRootElementIfNecessary(child);
  if (!child->is_raw_text()) {
    has_inline_child_ = true;
  }
}

base::String TextElement::ConvertContent(const lepus::Value value) {
  auto result = value.String();
  if (result.empty()) {
    if (value.IsInt32()) {
      result = base::String(std::to_string(value.Int32()));
    } else if (value.IsInt64()) {
      result = base::String(std::to_string(value.Int64()));
    } else if (value.IsNumber()) {
      std::stringstream stream;
      stream << value.Number();
      result = stream.str();
    } else if (value.IsNaN()) {
      BASE_STATIC_STRING_DECL(kNaN, "NaN");
      result = kNaN;
    } else if (value.IsNil()) {
      BASE_STATIC_STRING_DECL(kNull, "null");
      result = kNull;
    } else if (value.IsUndefined()) {
      BASE_STATIC_STRING_DECL(kUndefined, "undefined");
      result = kUndefined;
    }
  }
  return result;
}

void TextElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  bool processed = EnableLayoutInElementMode()
                       ? ProcessAttributeForLayoutInElement(key, value)
                       : ProcessAttributeForNormalLayoutMode(key, value);
  if (!processed) {
    FiberElement::SetAttributeInternal(key, value);
  }
}

void TextElement::ResetAttribute(const base::String& key) {
  if (!EnableLayoutInElementMode() && key.IsEqual(kTextOverflowAttr)) {
    RemoveStyleFromAttributes(kPropertyIDTextOverflow);
  }
  if (!EnableLayoutInElementMode() ||
      !ProcessAttributeForLayoutInElement(key, lepus::Value(), true)) {
    FiberElement::ResetAttribute(key);
  }
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
    content_ = !is_reset ? ConvertContent(value) : base::String();
    content_utf16_length_ =
        GetUtf16SizeFromUtf8(content_.c_str(), content_.length());
    MarkLayoutDirty();
    element_container_->InvalidateForRedraw();
    return true;
  }

  if (key.IsEqual(kTextMaxlineAttr)) {
    EnsureTextProps();
    if (is_reset) {
      text_props_->text_max_line.reset();
    } else {
      text_props_->text_max_line =
          value.IsNumber() ? value.Number() : std::stoi(value.StdString());
    }
    MarkLayoutDirty();
    element_container_->InvalidateForRedraw();
    return true;
  }

  if (key.IsEqual(kPunctuationTrimRuleAttr)) {
    auto rules = ParsePunctuationTrimRules(value);
    if (text_props_ ? text_props_->punctuation_trim_rules != rules
                    : !rules.empty()) {
      EnsureTextProps();
      text_props_->punctuation_trim_rules = std::move(rules);
      MarkLayoutDirty();
      element_container_->InvalidateForRedraw();
    }
    return true;
  }
  return false;
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
    auto* first_child = static_cast<FiberElement*>(children().front().get());
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
  FiberElement::ConvertToInlineElement();
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
    has_processed = FiberElement::ResetCSSValue(id);
  }

  return has_processed;
}

void TextElement::DispatchLayoutBefore() {
  if (is_inline_element()) {
    return;
  }

  element_manager_->DispatchLayoutBefore(this);
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
    FiberElement::UpdateLayoutNodeFontSize(cur_node_font_size,
                                           root_node_font_size);
  }
}

void TextElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<TextFragmentBehavior>(fragment));
}

void TextElement::SetTextBundle(intptr_t text_bundle) {
  auto* fragment = fragment_impl();
  if (fragment) {
    fragment->SetTextBundle(text_bundle);
  }
}

}  // namespace tasm
}  // namespace lynx
