// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_HARMONY_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_HARMONY_H_

#include <native_drawing/drawing_text_typography.h>

#include <cstdint>
#include <limits>
#include <list>
#include <memory>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/line_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/text_event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {

class TextBoxHarmony {
 public:
  explicit TextBoxHarmony(OH_Drawing_TextBox* text_box) : text_box_(text_box) {}
  ~TextBoxHarmony() { OH_Drawing_TypographyDestroyTextBox(text_box_); }
  uint32_t GetCount() const { return OH_Drawing_GetSizeOfTextBox(text_box_); }

  float GetLeft(uint32_t idx) {
    return OH_Drawing_GetLeftFromTextBox(text_box_, idx);
  }

  float GetTop(uint32_t idx) {
    return OH_Drawing_GetTopFromTextBox(text_box_, idx);
  }

  float GetRight(uint32_t idx) {
    return OH_Drawing_GetRightFromTextBox(text_box_, idx);
  }

  float GetBottom(uint32_t idx) {
    return OH_Drawing_GetBottomFromTextBox(text_box_, idx);
  }

 private:
  OH_Drawing_TextBox* text_box_;
};

class PositionAndAffinityHarmony {
 public:
  explicit PositionAndAffinityHarmony(OH_Drawing_PositionAndAffinity* pa)
      : position_and_affinity_(pa) {}

  size_t GetPosition() {
    return OH_Drawing_GetPositionFromPositionAndAffinity(
        position_and_affinity_);
  }

  int GetAffinity() {
    return OH_Drawing_GetAffinityFromPositionAndAffinity(
        position_and_affinity_);
  };

 private:
  OH_Drawing_PositionAndAffinity* position_and_affinity_;
};

class ParagraphHarmony : public fml::RefCountedThreadSafeStorage {
 public:
  explicit ParagraphHarmony(
      OH_Drawing_Typography* para,
      std::shared_ptr<FontCollectionHarmony>& font_collection,
      std::list<std::shared_ptr<TextEventTarget>>&& event_target_roots,
      std::vector<int32_t>&& placeholder_signs,
      std::optional<std::list<fml::RefPtr<ShaderEffect>>>&& effects)
      : paragraph_(para),
        font_collection_(font_collection),
        event_target_roots_(std::move(event_target_roots)),
        placeholder_signs_(std::move(placeholder_signs)),
        effects_(std::move(effects)) {}

  ~ParagraphHarmony() override {
    if (paragraph_) {
      //      OH_Drawing_DestroyLineMetrics(line_metrics_);
      OH_Drawing_DestroyTypography(paragraph_);
    }
  }

  void ReleaseSelf() const override { delete this; }

  OH_Drawing_Typography* GetRawStruct() const { return paragraph_; }

  void Layout(double max_width) const {
    OH_Drawing_TypographyLayout(paragraph_, max_width);
    //    line_metrics_ = OH_Drawing_TypographyGetLineMetrics(paragraph_);
  }

  void Draw(OH_Drawing_Canvas* canvas, double x, double y) const;

  float GetMaxWidth() const {
    return static_cast<float>(OH_Drawing_TypographyGetMaxWidth(paragraph_));
  }

  float GetHeight() const {
    return static_cast<float>(OH_Drawing_TypographyGetHeight(paragraph_));
  }

  float GetBaseline() const {
    return static_cast<float>(
        OH_Drawing_TypographyGetAlphabeticBaseline(paragraph_));
  }

  float GetLongestLine() const {
    return static_cast<float>(OH_Drawing_TypographyGetLongestLine(paragraph_));
  }

  float GetMinIntrinsicWidth() const {
    return static_cast<float>(
        OH_Drawing_TypographyGetMinIntrinsicWidth(paragraph_));
  }

  float GetMaxIntrinsicWidth() const {
    return static_cast<float>(
        OH_Drawing_TypographyGetMaxIntrinsicWidth(paragraph_));
  }

  float GetAlphabeticBaseline() const {
    return static_cast<float>(
        OH_Drawing_TypographyGetAlphabeticBaseline(paragraph_));
  }

  float GetIdeographicBaseline() const {
    return static_cast<float>(
        OH_Drawing_TypographyGetIdeographicBaseline(paragraph_));
  }

  bool DidExceedMaxLines() const {
    return OH_Drawing_TypographyDidExceedMaxLines(paragraph_);
  }

  uint32_t GetLineCount() const {
    return OH_Drawing_TypographyGetLineCount(paragraph_);
  }

  float GetLineHeight(int32_t line_id) const {
    return static_cast<float>(
        OH_Drawing_TypographyGetLineHeight(paragraph_, line_id));
  }

  float GetLineWidth(int32_t line_id) const {
    return static_cast<float>(
        OH_Drawing_TypographyGetLineWidth(paragraph_, line_id));
  }

  TextBoxHarmony GetRectsForPlaceholders() const {
    return TextBoxHarmony{
        OH_Drawing_TypographyGetRectsForPlaceholders(paragraph_)};
  }

  PositionAndAffinityHarmony GetPositionAndAffinity(double x, double y) const {
    return PositionAndAffinityHarmony(
        OH_Drawing_TypographyGetGlyphPositionAtCoordinate(paragraph_, x, y));
  }

  LineMetricsHarmony GetLineMetrics(size_t idx) {
    if (idx < GetLineCount()) {
      OH_Drawing_TypographyGetLineMetricsAt(paragraph_, idx, &line_metrics_);
    }
    return LineMetricsHarmony{&line_metrics_};
  }

  TextBoxHarmony GetRectsForRange(size_t start, size_t end) const {
    return TextBoxHarmony{OH_Drawing_TypographyGetRectsForRange(
        paragraph_, start, end, RECT_HEIGHT_STYLE_TIGHT,
        RECT_WIDTH_STYLE_TIGHT)};
  }

  void SetMeasuredSize(float width, float height, float baseline) {
    measured_width_ = width;
    measured_height_ = height;
    measured_baseline_ = baseline;
  }

  void SetText(const std::string& text) { text_ = text; }

  float GetMeasuredWidth() const { return measured_width_; }

  float GetMeasuredHeight() const { return measured_height_; }

  float GetMeasuredBaseline() const { return measured_baseline_; }

  void SetTranslateLeftOffset(float offset) { translate_left_offset_ = offset; }

  float GetTranslateLeftOffset() { return translate_left_offset_; }

  const std::vector<int32_t>& GetPlaceholders() const {
    return placeholder_signs_;
  }

  void GenerateEventRect(std::shared_ptr<TextEventTarget>& event_target) const {
    std::stack<std::shared_ptr<TextEventTarget>> target_stack;
    target_stack.push(event_target);
    while (!target_stack.empty()) {
      auto top_target = target_stack.top();
      TextBoxHarmony text_box =
          GetRectsForRange(top_target->Start(), top_target->End());
      for (uint32_t i = 0; i < text_box.GetCount(); ++i) {
        top_target->AddRect(text_box.GetLeft(i), text_box.GetTop(i),
                            text_box.GetRight(i), text_box.GetBottom(i));
      }
      target_stack.pop();
      for (auto child : top_target->Children()) {
        target_stack.push(child);
      }
    }
  }

  EventTarget* HitTest(float point[2]) {
    if (!has_generate_event_rects_) {
      for (auto event_target : event_target_roots_) {
        GenerateEventRect(event_target);
      }
      has_generate_event_rects_ = true;
    }
    float point_in_text_layout[2] = {point[0] - translate_left_offset_,
                                     point[1]};

    for (auto event_target : event_target_roots_) {
      EventTarget* ret = event_target->HitTest(point_in_text_layout);
      if (ret != nullptr) {
        return ret;
      }
    }

    return nullptr;
  }
  void SetEventTargetParent(EventTarget* parent) {
    for (auto event_target : event_target_roots_) {
      event_target->SetParent(parent);
    }
  }

  void SetIndent(float indent) {
    const float indents[2] = {indent, 0};
    OH_Drawing_TypographySetIndents(paragraph_, 2, indents);
  }

  float GetIndent() {
    return OH_Drawing_TypographyGetIndentsWithIndex(paragraph_, 0);
  }

  const std::string& GetText() const { return text_; }

 private:
  OH_Drawing_Typography* paragraph_;
  // should keep the shared font_collection live with paragraph
  // FIXME(linxs): the shared font_collection can be used as global
  std::shared_ptr<FontCollectionHarmony> font_collection_{nullptr};
  OH_Drawing_LineMetrics line_metrics_ = {};
  float measured_width_{.0f};
  float measured_height_{.0f};
  float measured_baseline_{.0f};
  float translate_left_offset_{0.f};
  std::list<std::shared_ptr<TextEventTarget>> event_target_roots_;
  std::vector<int32_t> placeholder_signs_;
  bool has_generate_event_rects_{false};
  std::optional<std::list<fml::RefPtr<ShaderEffect>>> effects_;
  std::string text_;
};

class PlaceholderHarmony {
 public:
  explicit PlaceholderHarmony(
      float width = 50, float height = 50,
      OH_Drawing_PlaceholderVerticalAlignment vertical_align_type =
          ALIGNMENT_ABOVE_BASELINE,
      float baseline_offset = 0.f)
      : placeholder_span_({width, height, vertical_align_type,
                           TEXT_BASELINE_ALPHABETIC, baseline_offset}) {}

  OH_Drawing_PlaceholderSpan* GetRawStruct() { return &placeholder_span_; }

 private:
  OH_Drawing_PlaceholderSpan placeholder_span_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_PARAGRAPH_HARMONY_H_
