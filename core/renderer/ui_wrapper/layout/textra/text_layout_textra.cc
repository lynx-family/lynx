// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/textra/text_layout_textra.h"

#include <memory>
#include <utility>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/starlight/types/layout_constraints.h"
#include "core/renderer/ui_wrapper/layout/textra/text_layout_api.h"

namespace lynx {
namespace tasm {

namespace {

class TextraInlineView : public text::InlineView {
 public:
  explicit TextraInlineView(Element* child)
      : child_(static_cast<FiberElement*>(child)) {}
  ~TextraInlineView() override = default;
  text::MeasureResult Measure(const text::MeasureParams& params) override {
    starlight::Constraints constraints;
    constraints[starlight::kHorizontal] = starlight::OneSideConstraint(
        params.width, static_cast<SLMeasureMode>(params.width_mode));
    constraints[starlight::kVertical] = starlight::OneSideConstraint(
        params.height, static_cast<SLMeasureMode>(params.height_mode));

    FloatSize size =
        child_->slnode()->UpdateMeasureByPlatform(constraints, true);
    return {size.width_, size.height_, size.baseline_};
  }
  void Align(float x, float y) override {
    child_->slnode()->AlignmentByPlatform(y, x);
  }

  void HideView() override {
    // FIXME(linxs): need a better way to hide the view
    starlight::Constraints constraints;
    constraints[starlight::kHorizontal] =
        starlight::OneSideConstraint(0, SLMeasureMode::SLMeasureModeDefinite);
    constraints[starlight::kVertical] =
        starlight::OneSideConstraint(0, SLMeasureMode::SLMeasureModeDefinite);
    child_->slnode()->UpdateMeasureByPlatform(constraints, true);
  }

 private:
  FiberElement* child_;
};

class TextraParagraphListener : public text::ParagraphListener {
 public:
  explicit TextraParagraphListener(Element* element) : element_(element) {}
  ~TextraParagraphListener() override = default;
  void MarkParagraphDirty() override {
    if (element_) {
      element_->MarkLayoutDirty();
    }
  }

 private:
  Element* element_;
};

}  // namespace

TextLayoutTextra::TextLayoutTextra(intptr_t api)
    : api_(reinterpret_cast<text::TextLayoutAPI*>(api)) {}

TextLayoutTextra::~TextLayoutTextra() {
  if (!paragraphs_.empty()) {
    // If paragraphs_ is not empty, it means some TextElements were leaked or
    // not properly destroyed before TextLayoutTextra destruction. Calling
    // api_->DestroyParagraph here might cause a crash if the underlying text
    // engine is already shut down. So we just log a warning and skip
    // destruction.
    // LOGE("TextLayoutTextra::~TextLayoutTextra: paragraphs_ is not empty!
    // Count: "
    //      << paragraphs_.size());
  }
}

void TextLayoutTextra::ApplyTextStyle(TextElement* text_element) {
  const CSSIDBitset& property_bits = text_element->property_bits();

  // process font-size by default
  float font_size =
      static_cast<float>(text_element->computed_css_style()->GetFontSize());
  paragraph_builder_->SetTextStyle(kTextPropFontSize, &(font_size),
                                   sizeof(float));

  auto& text_attributes =
      text_element->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value()) {
    for (CSSPropertyID id : property_bits) {
      switch (id) {
        case kPropertyIDFontWeight: {
          int fontWeight = static_cast<int>(text_attributes->font_weight);
          paragraph_builder_->SetTextStyle(kTextPropFontWeight, &(fontWeight),
                                           sizeof(int));
          break;
        }
        case kPropertyIDColor: {
          if (text_attributes->text_gradient.has_value() &&
              text_attributes->text_gradient->IsArray()) {
            // TODO: gradient
          } else {
            int color = static_cast<int>(
                text_attributes->color.has_value()
                    ? *text_attributes->color
                    : starlight::DefaultColor::DEFAULT_TEXT_COLOR);
            paragraph_builder_->SetTextStyle(kTextPropColor, &(color),
                                             sizeof(int));
          }
          break;
        }
        case kPropertyIDBackgroundColor: {
          // TODO: background color
          break;
        }
        case kPropertyIDTextShadow: {
          // TODO: text shadow
          break;
        }
        case kPropertyIDFontStyle: {
          int font_style = static_cast<int>(text_attributes->font_style);
          paragraph_builder_->SetTextStyle(kTextPropFontStyle, &(font_style),
                                           sizeof(int));
          break;
        }
        case kPropertyIDFontFamily: {
          auto family = text_attributes->font_family;
          paragraph_builder_->SetTextStyle(kTextPropFontFamily,
                                           const_cast<char*>(family.c_str()),
                                           family.length());
          EnsureParagraphListener(text_element);
          break;
        }
        case kPropertyIDLetterSpacing: {
          float letter_spacing = text_attributes->letter_spacing;
          paragraph_builder_->SetTextStyle(kTextPropLetterSpacing,
                                           &(letter_spacing), sizeof(float));
          break;
        }
        case kPropertyIDTextDecoration: {
          //[type,style,color]
          int decoration[3] = {0};

          if (text_attributes->underline_decoration) {
            decoration[0] = 1;  // 1 indicate is underline
          }
          if (text_attributes->line_through_decoration) {
            decoration[0] = 2;
          }

          decoration[1] =
              static_cast<int>(text_attributes->text_decoration_style);

          if (text_attributes->text_decoration_color.has_value() &&
              text_attributes->text_decoration_color != 0) {
            decoration[2] =
                static_cast<int>(*text_attributes->text_decoration_color);
          }
          paragraph_builder_->SetTextStyle(kTextPropTextDecoration,
                                           &(decoration), sizeof(int) * 3);
          break;
        }
        default:
          break;
      }
    }
  }
}

void TextLayoutTextra::ApplyParagraphStyle(TextElement* text_element) {
  const CSSIDBitset& props_set = text_element->property_bits();
  TextProps* text_props = text_element->text_props();
  auto* computed_css_style = text_element->computed_css_style();
  if (text_props) {
    if (text_props->text_max_line) {
      auto text_max_line = *text_props->text_max_line;
      paragraph_builder_->SetParagraphStyle(kTextPropTextMaxLine,
                                            &(text_max_line), sizeof(int));
    }
  }
  if (props_set.Has(kPropertyIDTextOverflow)) {
    auto text_overflow = static_cast<int>(
        computed_css_style->GetTextAttributes()->text_overflow);
    paragraph_builder_->SetParagraphStyle(kTextPropTextOverflow,
                                          &(text_overflow), sizeof(int));
  }
  if (props_set.Has(kPropertyIDLineHeight)) {
    float line_height =
        computed_css_style->GetTextAttributes()->computed_line_height;
    paragraph_builder_->SetParagraphStyle(kTextPropLineHeight, &(line_height),
                                          sizeof(float));
  }
  if (props_set.Has(kPropertyIDWhiteSpace)) {
    auto white_space =
        static_cast<int>(computed_css_style->GetTextAttributes()->white_space);
    paragraph_builder_->SetParagraphStyle(kTextPropWhiteSpace, &(white_space),
                                          sizeof(int));
  }
  if (props_set.Has(kPropertyIDTextAlign)) {
    auto text_align =
        static_cast<int>(computed_css_style->GetTextAttributes()->text_align);
    paragraph_builder_->SetParagraphStyle(kTextPropTextAlign, &(text_align),
                                          sizeof(int));
  }
}

void TextLayoutTextra::BuildParagraphRecursively(Element* element,
                                                 bool& has_inline_view) {
  if (element->is_text()) {
    // no raw-text case
    TextElement* text_element = static_cast<TextElement*>(element);
    ApplyTextStyle(text_element);
    auto element_content = text_element->content();
    if (!element_content.empty()) {
      paragraph_builder_->AddText(element_content.c_str(),
                                  element_content.length());
    }
  }

  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    ProcessChildStyleAndProps(child, has_inline_view);
  }
}

void TextLayoutTextra::HandleInlineImageProps(Element* element) {
  text::ImageProps props{};
  auto* image_element = static_cast<ImageElement*>(element);
  float width = starlight::NLengthToFakeLayoutUnit(
                    image_element->slnode()->GetCSSStyle()->GetWidth())
                    .ClampIndefiniteToZero()
                    .ToFloat();
  float height = starlight::NLengthToFakeLayoutUnit(
                     image_element->slnode()->GetCSSStyle()->GetHeight())
                     .ClampIndefiniteToZero()
                     .ToFloat();
  props.width = width;
  props.height = height;
  int margin_left = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginLeft())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_top = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginTop())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_right = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginRight())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_bottom = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginBottom())
          .ClampIndefiniteToZero()
          .ToFloat());
  props.margin_left = margin_left;
  props.margin_top = margin_top;
  props.margin_right = margin_right;
  props.margin_bottom = margin_bottom;

  auto computed_css_style = image_element->computed_css_style();
  if (computed_css_style->HasBorderRadius()) {
    // only support: left,top,right,bottom border-radius for such mode
    const auto resolve_length =
        [](const starlight::NLength& length) -> std::pair<float, int> {
      float value = .0f;
      int type = 0;
      if (length.NumericLength().ContainsPercentage()) {
        value = static_cast<float>(length.NumericLength().GetPercentagePart() /
                                   100.f);
        type = static_cast<int>(starlight::PlatformLengthUnit::PERCENTAGE);
      } else if (length.NumericLength().ContainsFixedValue()) {
        value = static_cast<float>(length.NumericLength().GetFixedPart());
        type = static_cast<int>(starlight::PlatformLengthUnit::NUMBER);
      }
      return {value, type};
    };
    const auto top_left =
        resolve_length(computed_css_style->GetSimpleBorderTopLeftRadius());
    const auto& top_right =
        resolve_length(computed_css_style->GetSimpleBorderTopLeftRadius());
    const auto& bottom_left =
        resolve_length(computed_css_style->GetSimpleBorderTopLeftRadius());
    const auto& bottom_right =
        resolve_length(computed_css_style->GetSimpleBorderTopLeftRadius());
    props.radius.top_left = top_left.first;
    props.radius.top_left_type = top_left.second;
    props.radius.top_right = top_right.first;
    props.radius.top_right_type = top_right.second;
    props.radius.bottom_left = bottom_left.first;
    props.radius.bottom_left_type = bottom_left.second;
    props.radius.bottom_right = bottom_right.first;
    props.radius.bottom_right_type = bottom_right.second;
  }

  const auto& text_attributes =
      image_element->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value() &&
      text_attributes->vertical_align !=
          starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN) {
    text::VerticalAlign align;
    align.vertical_align = static_cast<int>(text_attributes->vertical_align);
    align.vertical_align_length = text_attributes->vertical_align_length;
    paragraph_builder_->SetPlaceHolderStyle(kTextPropVerticalAlign, &align,
                                            sizeof(text::VerticalAlign));
  }

  paragraph_builder_->SetPlaceHolderStyle(kPropImageProps, &props,
                                          sizeof(text::ImageProps));
  paragraph_builder_->AddImage(image_element->src().c_str(),
                               image_element->src().length());
}

void TextLayoutTextra::HandleInlineViewProps(Element* element) {
  auto* child = static_cast<FiberElement*>(element);
  auto inline_view = std::make_unique<TextraInlineView>(child);
  const auto& text_attributes =
      child->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value() &&
      text_attributes->vertical_align !=
          starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN) {
    text::VerticalAlign vertical_align;
    vertical_align.vertical_align =
        static_cast<int>(text_attributes->vertical_align);
    vertical_align.vertical_align_length =
        text_attributes->vertical_align_length;
    paragraph_builder_->SetPlaceHolderStyle(
        kTextPropVerticalAlign, &vertical_align, sizeof(text::VerticalAlign));
  }
  paragraph_builder_->AddInlineView(std::move(inline_view));
}

void TextLayoutTextra::ProcessChildStyleAndProps(Element* element,
                                                 bool& has_inline_view) {
  auto* child = static_cast<FiberElement*>(element);
  if (child->is_raw_text()) {
    RawTextElement* rawText = static_cast<RawTextElement*>(child);
    paragraph_builder_->AddText(rawText->content().c_str(),
                                rawText->content().length());
  } else if (child->is_text()) {
    // inline text
    paragraph_builder_->PushTextStyle();
    BuildParagraphRecursively(child, has_inline_view);
    paragraph_builder_->PopTextStyle();

  } else if (child->is_image()) {
    paragraph_builder_->PushTextStyle();
    HandleInlineImageProps(child);
    paragraph_builder_->PopTextStyle();
  } else if (child->is_view()) {
    paragraph_builder_->PushTextStyle();
    HandleInlineViewProps(child);
    paragraph_builder_->PopTextStyle();
    has_inline_view = true;
  } else if (child->is_wrapper()) {
    for (auto* wrap_child = child->first_render_child(); wrap_child;
         wrap_child = wrap_child->next_render_sibling()) {
      ProcessChildStyleAndProps(wrap_child, has_inline_view);
    }
  }
}

LayoutResult TextLayoutTextra::Measure(Element* element, float width,
                                       int width_mode, float height,
                                       int height_mode) {
  if (!api_) {
    return LayoutResult{0, 0, 0};
  }
  auto paragraph_iter = paragraphs_.find(element->impl_id());
  if (paragraph_iter == paragraphs_.end()) {
    return LayoutResult{0, 0, 0};
  }
  auto* paragraph = paragraph_iter->second;
  text::MeasureParams measure_param = {
      width, static_cast<text::LayoutMode>(width_mode), height,
      static_cast<text::LayoutMode>(height_mode)};
  text::MeasureResult result =
      api_->MeasureParagraph(paragraph, std::move(measure_param));

  // update text bundle
  auto* text_element = static_cast<TextElement*>(element);
  text::Page* page = api_->GetPage(paragraph);
  text_element->SetTextBundle(reinterpret_cast<intptr_t>(page));

  return {result.width, result.height, result.baseline};
}

void TextLayoutTextra::Align(Element* element) {
  auto paragraph_iter = paragraphs_.find(element->impl_id());
  if (paragraph_iter == paragraphs_.end()) {
    return;
  }
  auto* paragraph = paragraph_iter->second;
  api_->AlignParagraph(paragraph, 0, 0);
}

void TextLayoutTextra::EnsureParagraphListener(Element* element) {
  if (!element || !paragraph_builder_) {
    return;
  }

  int32_t id = element->impl_id();
  auto it = paragraph_listeners_.find(id);
  text::ParagraphListener* listener = nullptr;
  if (it == paragraph_listeners_.end()) {
    auto new_listener = std::make_unique<TextraParagraphListener>(element);
    listener = new_listener.get();
    paragraph_listeners_.emplace(id, std::move(new_listener));
  } else {
    listener = it->second.get();
  }

  paragraph_builder_->SetParagraphListener(listener);
}

void TextLayoutTextra::DispatchLayoutBefore(Element* element) {
  if (!element->is_text()) {
    return;
  }
  auto* text_element = static_cast<TextElement*>(element);

  // create a new ParagraphBuilder
  paragraph_builder_ = api_->CreateParagraphBuilder();

  // Apply paragraph element's styles & props
  ApplyParagraphStyle(text_element);

  // Apply inline element's styles
  paragraph_builder_->PushTextStyle();
  bool has_inline_view = false;
  BuildParagraphRecursively(text_element, has_inline_view);
  paragraph_builder_->PopTextStyle();
  text_element->set_need_layout_children(has_inline_view);

  auto paragraph = paragraph_builder_->BuildParagraph();
  auto it_para = paragraphs_.find(element->impl_id());
  if (it_para != paragraphs_.end()) {
    api_->DestroyParagraph(it_para->second);
    it_para->second = paragraph;
  } else {
    paragraphs_.emplace(element->impl_id(), paragraph);
  }

  // release build builder in the end
  if (paragraph_builder_ != nullptr) {
    api_->DestroyParagraphBuilder(paragraph_builder_);
    paragraph_builder_ = nullptr;
  }
}

void TextLayoutTextra::Destroy(Element* element) {
  auto it_para = paragraphs_.find(element->impl_id());
  if (it_para != paragraphs_.end()) {
    api_->DestroyParagraph(it_para->second);
    paragraphs_.erase(it_para);
  }

  auto it_listener = paragraph_listeners_.find(element->impl_id());
  if (it_listener != paragraph_listeners_.end()) {
    paragraph_listeners_.erase(it_listener);
  }
}

}  // namespace tasm
}  // namespace lynx
