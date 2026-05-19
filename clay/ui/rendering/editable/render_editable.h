// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_EDITABLE_RENDER_EDITABLE_H_
#define CLAY_UI_RENDERING_EDITABLE_RENDER_EDITABLE_H_

#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/graphics_context.h"
#include "clay/ui/common/editing_misc.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/editable/text_editing_controller.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/painter/text_painter.h"
#include "clay/ui/rendering/render_box.h"

namespace txt {
class Paragraph;
}

namespace clay {

class PaintingContext;
class TextInputControllerDelegate;

class RenderEditable : public RenderBox {
 public:
  RenderEditable();

  const char* GetName() const override { return "RenderEditable"; }
  void SetTextEditingController(TextEditingController* controller) {
    text_editing_controller_ = controller;
  }

  void SetTextEditingValue(const TextEditingValue& value) {
    text_editing_controller_->SetValue(value, false, true);
  }

  const TextEditingValue& GetTextEditingValue() {
    return text_editing_controller_->GetValue();
  }

  void SetMultiline(bool is_multiline) { is_multiline_ = is_multiline; }

  void SetParagraph(txt::Paragraph* paragraph) {
    painter_->SetParagraph(paragraph);
    MarkNeedsPaint();
  }

  TextRange GetWordBoundary(size_t offset) {
    return painter_->GetWordBoundary(offset);
  }

  void SetRoughTextLineHeight(float rough_text_height) {
    rough_text_height_ = rough_text_height;
    MarkNeedsPaint();
  }

  void SetPlaceholderLineHeight(float placeholder_line_height) {
    need_paint_placeholder_ = true;
    placeholder_line_height_ = placeholder_line_height;
    MarkNeedsPaint();
  }

  void Paint(PaintingContext& context, const FloatPoint& offset) override;

  void SetSelection(const TextRange& selection,
                    Affinity selection_affinity = Affinity::kDownstream);
  void SetSelection(int base, int extent,
                    Affinity selection_affinity = Affinity::kDownstream);
  void SelectWord(FloatPoint point);
  void SelectLine(FloatPoint point);

  bool IsRepaintBoundary() const override { return true; }

  // |point| is relative to self.
  void UpdateCaretByCoordinate(const FloatPoint& point, bool is_select = false);
  void MoveCaretUpDown(VerticalDirection direction,
                       bool key_combination = false);

  bool Disabled() { return disabled_; }
  void SetDisabled(bool disabled);

  void SetCaretDisplay(bool display);

  void SetDefaultLineHeight(float default_height) {
    default_line_height_ = default_height;
  }

  void SetTextAlign(TextAlignment text_align) { text_align_ = text_align; }
  void SetTextDirection(TextDirection text_direction) {
    text_direction_ = text_direction;
  }

  void SetMaxLines(uint32_t max_lines);

  float GetRoughTextLineHeight();

  float GetPlaceholderLineHeight();

  void SetCaretColor(std::optional<Color> color);
  void SetCaretFallbackColor(const Color& color);
  void SetCaretGradient(std::optional<Gradient> gradient);
  void SetCaretWidth(float width);
  void SetCaretHeight(float height);
  void SetCaretRadius(float radius);

  float CaretWidth() const;

  // compute caret position offset by glyph offset.
  FloatRect ComputeCaretRect();
  FloatRect ComputeCaretRectRelativeToCanvas();

  FloatRect ComputeComposingRect(TextRange composing_range);
  void SetTextInputControllerDelegate(TextInputControllerDelegate* controller) {
    controller_ = controller;
  }

  void EnsurePlaceholderCenterInVertical(const FloatRect& content,
                                         const FloatRect& caret);
  void EnsureCaretCenterInVertical(const FloatRect& content,
                                   const FloatRect& caret);
  void EnsureCaretInVisibleArea(const FloatRect& content, FloatRect* caret);
  void ClampContent(const FloatRect& content, const FloatRect& caret);

 private:
  void PaintCaret(GraphicsContext* context, const FloatRect& caret_rect);
  void PaintSelection(GraphicsContext* context,
                      const std::vector<TextBox>& boxes);
  void UpdateSelectionForMultiLine(const std::vector<TextBox>& boxes,
                                   std::vector<skity::Rect>& res);

  // |caret_rect| is inited as caret proto metrics with zero offset.
  // |caret_offset| is the code point offset from start of text.
  bool UpdateCaretRectUpstream(int caret_offset, FloatRect& caret_rect);
  bool UpdateCaretRectDownstream(int caret_offset, FloatRect& caret_rect);
  // |point| is relative to paragraph.
  void UpdateCaretByParagraph(const FloatPoint& point, bool is_select = false);

  void UpdateCaretRectBesideBox(FloatRect& caret_rect,
                                const FloatRect& box_nearby,
                                bool caret_after_box);

  float CaretVerticalPreserveSpace() const;

  // TODO(yulitao): Decoupling caret painting logic from editable.
  std::optional<Color> caret_color_;
  Color caret_fallback_color_ = Color::kBlack();
  std::optional<Gradient> caret_gradient_;
  std::optional<float> caret_width_;
  std::optional<float> caret_height_;
  std::optional<float> caret_radius_;
  float rough_text_height_ = 0.f;
  float placeholder_line_height_ = 0.f;
  bool display_caret_ = false;
  bool disabled_ = false;
  // To ensure caret is in the region of content region, all contents may should
  // translate some offsets.
  FloatPoint offset_for_caret_visible_;
  FloatPoint last_offset_for_caret_visible_;
  FloatPoint paint_offset_;
  FloatPoint offset_for_placeholder_visible_;
  FloatPoint last_offset_for_placeholder_visible_;
  bool need_paint_placeholder_ = false;

  float default_line_height_ = 0.f;
  uint32_t max_lines_ = std::numeric_limits<uint32_t>::max();

  TextEditingController* text_editing_controller_ = nullptr;
  std::unique_ptr<TextPainter> painter_;
  TextAlignment text_align_ = TextAlignment::kLeft;
  TextDirection text_direction_ = TextDirection::kLtr;

  fml::WeakPtrFactory<RenderEditable> weak_factory_;
  TextInputControllerDelegate* controller_ = nullptr;
  bool is_multiline_ = false;
};

class TextInputControllerDelegate {
 public:
  virtual void UpdateRemoteStateIfNeeded(const TextEditingValue&) = 0;

  virtual void PostPaint() = 0;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_EDITABLE_RENDER_EDITABLE_H_
