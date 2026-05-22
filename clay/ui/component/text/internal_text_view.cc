// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/internal_text_view.h"

#include <algorithm>
#include <codecvt>
#include <limits>
#include <locale>
#include <utility>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/unicode_util.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"
#include "clay/ui/painter/gradient_factory.h"
#include "clay/ui/rendering/text/render_text.h"

namespace clay {

namespace {

namespace utils = attribute_utils;
static const Color kDefaultTextColor = Color(0xFF000000);
static constexpr float kDefaultFontSizeInDip = 14.f;
static constexpr TextAlignment kDefaultAlignment = TextAlignment::kLeft;
// Used for avoid unnecessary second layout under kAtMost mode.
// For example:
//  Given a text with actual width 100px, and it's measured with
// width 200px and measure mode kAtMost. After layout, max intrinsic width will
// be 100px. Thus second layout is needed because 100 < 200.
//  While given a text with width 200px, and layout width 100px at most. After
// layout, the max intrinsic width will be less than 100px, like "99.42".
// But actually there's no need to be layout again.
constexpr float kLayoutTolerance = 1.f;

class LayoutContextMeasure : public LayoutContext {
 public:
  // Width for text paragraph layout.
  float layout_width_ = 0.f;

  // Indicate the intrinsic width after layout using |layout_width_|.
  int measured_width_ = 0;
  // Indicate the height after layout using |layout_width_|.
  int measured_height_ = 0;
};

}  // namespace

InternalTextView::InternalTextView(int id, PageView* page_view)
    : InternalTextView(id, "internal-text", std::make_unique<RenderText>(),
                       page_view) {}

InternalTextView::InternalTextView(int id, const std::string& tag,
                                   std::unique_ptr<RenderObject> render_object,
                                   PageView* page_view)
    : BaseView(id, tag, std::move(render_object), page_view) {
  default_style_.text_color = kDefaultTextColor;
  default_style_.text_align = kDefaultAlignment;
  text_style_ = default_style_;
  UpdateDefaultFontSize();
}

InternalTextView::~InternalTextView() {
  RemoveGestureRecognizer(tap_recognizer_);
}

void InternalTextView::DidUpdateStyle() {
  update_flag_ = static_cast<UpdateFlag>(update_flag_ | kUpdateFlagStyle);
}

void InternalTextView::SetText(const std::string& text) {
  std::u16string text_u16 = UnicodeUtil::Utf8ToUtf16(text);
  if (text_u16 != text_) {
    text_ = std::move(text_u16);
    DidUpdateStyle();
  }
}

void InternalTextView::OnLayout(LayoutContext* context) {
  const float content_width = LayoutWidth();
  float layout_width = content_width;
  auto* context_measure = static_cast<LayoutContextMeasure*>(context);
  if (context_measure) {
    layout_width = context_measure->layout_width_;
  }

  const bool force_rebuild = update_flag_ != kUpdateFlagNone;
  const bool should_relayout =
      force_rebuild || prev_layout_width_ != layout_width;

  if (force_rebuild) {
    auto builder =
        std::make_unique<TextParagraphBuilder>(use_skia_, text_style_);
    int max_length = text_.size();
    if (max_length_.has_value() && *max_length_ > 0) {
      max_length = max_length_.value();
    }
    builder->PushStyle(text_style_.value());
    builder->AddText(text_.substr(0, max_length));
    builder->Pop();
    paragraph_ = Build(std::move(builder));
  }

  if (should_relayout) {
    paragraph_->Layout(layout_width);
    GetRenderText()->SetParagraph(paragraph_.get(), text_);
    if (text_style_->text_gradient.has_value()) {
      GetRenderText()->SetGradient(text_style_->text_gradient);
    } else {
      GetRenderText()->SetGradient(std::nullopt);
    }
  }

  if (context_measure) {
    context_measure->measured_width_ =
        std::ceil(paragraph_->GetMaxIntrinsicWidth());
    context_measure->measured_height_ = std::ceil(paragraph_->GetHeight());
  }
  update_flag_ = kUpdateFlagNone;
  prev_layout_width_ = layout_width;
}

void InternalTextView::Measure(const MeasureConstraint& constraint,
                               MeasureResult& result) {
  if (!constraint.IsValid()) {
    FML_DLOG(WARNING) << "Invalid measure metrics.";
    result.width = 0;
    result.height = 0;
    return;
  }

  LayoutContextMeasure context;
  switch (constraint.width_mode) {
    case TextMeasureMode::kIndefinite:
      context.layout_width_ = std::numeric_limits<float>::infinity();
      break;
    case TextMeasureMode::kDefinite:
    case TextMeasureMode::kAtMost:
      context.layout_width_ = *constraint.width;
      break;
  }

  Layout(&context);

  if (constraint.width_mode == TextMeasureMode::kIndefinite) {
    if (text_style_ && text_style_->text_align.value_or(TextAlignment::kLeft) !=
                           TextAlignment::kLeft) {
      // Do second layout if the width mode is indefinite and the alignment is
      // not left. Because in this case, the paragraph_ won't have the actual
      // line and nothing will be painted.
      context.layout_width_ = context.measured_width_;
      Layout(&context);
    }
  }

  if (constraint.width_mode == TextMeasureMode::kAtMost &&
      context.measured_width_ + kLayoutTolerance < context.layout_width_) {
    context.layout_width_ = context.measured_width_;
    // Do second layout if actual text width is less than constraints.
    // For example, given at most 200px width and actually 100px is needed,
    // use 100px to layout again. Otherwise text align will be problem.
    // Note: Here maybe some optimizations to avoid second layout.
    Layout(&context);
  }

  if (constraint.width_mode == TextMeasureMode::kDefinite) {
    result.width = *constraint.width;
  } else if (constraint.width_mode == TextMeasureMode::kIndefinite) {
    result.width = std::ceil(context.measured_width_);
  } else {
    result.width =
        std::min(std::ceil(static_cast<float>(context.measured_width_)),
                 *constraint.width);
  }

  auto desired_height = context.measured_height_;
  switch (constraint.height_mode) {
    case TextMeasureMode::kIndefinite: {
      result.height = desired_height;
      break;
    }
    case TextMeasureMode::kDefinite: {
      result.height = *constraint.height;
      break;
    }
    case TextMeasureMode::kAtMost: {
      result.height = std::min(std::ceil(static_cast<float>(desired_height)),
                               *constraint.height);
      break;
    }
  }
}

RenderText* InternalTextView::GetRenderText() {
  return static_cast<RenderText*>(render_object_.get());
}

bool InternalTextView::ShouldCreateStyle() {
  if (text_style_) {
    return false;
  }
  text_style_ = std::make_optional<TextStyle>();
  return true;
}

void InternalTextView::SetEnableFontScaling(bool enabled) {}

void InternalTextView::SetFontSize(float font_size) {
  if (ShouldCreateStyle() || text_style_->font_size != font_size) {
    text_style_->font_size = font_size;
    DidUpdateStyle();
  }
}

void InternalTextView::SetLineHeight(float line_height) {
  if (line_height <= 0) {
    line_height = 1.0f;
  }
  if (ShouldCreateStyle() || text_style_->line_height != line_height) {
    text_style_->line_height = line_height;
    DidUpdateStyle();
  }
}

void InternalTextView::SetLineSpacing(float line_spacing) {
  if (ShouldCreateStyle() || text_style_->line_spacing != line_spacing) {
    text_style_->line_spacing = line_spacing;
    DidUpdateStyle();
  }
}

void InternalTextView::SetLetterSpacing(float letter_spacing) {
  if (ShouldCreateStyle() || text_style_->letter_spacing != letter_spacing) {
    text_style_->letter_spacing = letter_spacing;
    DidUpdateStyle();
  }
}
void InternalTextView::SetTextAlign(TextAlignment text_align) {
  if (ShouldCreateStyle() || text_style_->text_align != text_align) {
    text_style_->text_align = text_align;
    DidUpdateStyle();
  }
}
void InternalTextView::SetFontWeight(FontWeight font_weight) {
  if (ShouldCreateStyle() || text_style_->font_weight != font_weight) {
    text_style_->font_weight = font_weight;
    DidUpdateStyle();
  }
}
void InternalTextView::SetFontStyle(FontStyle font_style) {
  if (ShouldCreateStyle() || text_style_->font_style != font_style) {
    text_style_->font_style = font_style;
    DidUpdateStyle();
  }
}
void InternalTextView::SetTextColor(const Color& text_color) {
  if (ShouldCreateStyle() || text_style_->text_color != text_color) {
    text_style_->text_color = text_color;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextBackgroundColor(const Color& color) {
  if (ShouldCreateStyle() || text_style_->background_color != color) {
    text_style_->background_color = color;
    DidUpdateStyle();
  }
}

void InternalTextView::SetFontFamily(const std::string& font_family) {
  if (ShouldCreateStyle() || text_style_->font_family != font_family) {
    text_style_->font_family = font_family;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextDecoration(
    const TextDecoration& text_decoration) {
  if (ShouldCreateStyle() || text_style_->text_decoration != text_decoration) {
    text_style_->text_decoration = text_decoration;
    DidUpdateStyle();
  }
}

void InternalTextView::AppendTextShadow(Shadow&& text_shadow) {
  if (!text_style_) {
    text_style_ = std::make_optional<TextStyle>();
  }
  if (!text_style_->text_shadows) {
    text_style_->text_shadows = std::vector<Shadow>();
  }

  text_style_->text_shadows->emplace_back(std::move(text_shadow));
  DidUpdateStyle();
}

void InternalTextView::SetTextShadows(std::vector<Shadow>&& text_shadows) {
  if (ShouldCreateStyle() || text_style_->text_shadows != text_shadows) {
    text_style_->text_shadows = std::move(text_shadows);
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextGradient(const Gradient& gradient) {
  if (ShouldCreateStyle() || text_style_->text_gradient != gradient) {
    text_style_->text_gradient = gradient;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextMaxLine(uint32_t max_lines) {
  if (ShouldCreateStyle() || text_style_->max_lines != max_lines) {
    text_style_->max_lines = max_lines;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextMaxLength(uint32_t max_length) {
  if (ShouldCreateStyle()) {
    max_length_ = max_length;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextOverflow(TextOverflow overflow) {
  if (ShouldCreateStyle() || text_style_->overflow != overflow) {
    text_style_->overflow = overflow;
    DidUpdateStyle();
  }
}

void InternalTextView::SetTextEllipsis(std::u16string ellipsis) {
  if (ShouldCreateStyle() || text_style_->ellipsis != ellipsis) {
    text_style_->ellipsis = std::move(ellipsis);
    DidUpdateStyle();
  }
}

void InternalTextView::UpdateDefaultFontSize() {
  const float prev = default_style_.font_size.value_or(0.f);
  default_style_.font_size = GetDefaultFontSize();
  if (prev != default_style_.font_size.value()) {
    DidUpdateStyle();
  }
}

void InternalTextView::AddTapUpListener(
    std::function<void(const PointerEvent& down_event)> func) {
  auto tap_recognizer =
      std::make_unique<TapGestureRecognizer>(page_view()->gesture_manager());
  tap_recognizer_ = tap_recognizer.get();
  tap_recognizer->SetTapUpCallback(std::move(func));
  AddGestureRecognizer(std::move(tap_recognizer));
}

float InternalTextView::GetDefaultFontSize() const {
  return FromLogical(kDefaultFontSizeInDip);
}

#ifndef NDEBUG
std::string InternalTextView::ToString() const {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  std::stringstream ss;
  ss << BaseView::ToString();
  ss << " text_=(" << lynx::base::U16StringToU8(text_) << ")";
  return ss.str();
#pragma GCC diagnostic pop
}
#endif

}  // namespace clay
