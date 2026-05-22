// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDERING_TEXT_RENDER_TEXT_H_
#define CLAY_UI_RENDERING_TEXT_RENDER_TEXT_H_

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/gradient.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/painter/text_painter.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

class TextPainter;

using SelectionChangedCallback = std::function<void(TextBox*, int, int)>;

class RenderText : public RenderBox {
 public:
  RenderText();
  virtual ~RenderText();

  const char* GetName() const override;

  void SetParagraph(txt::Paragraph* paragraph, const std::u16string& text);

  void SetGradient(const std::optional<Gradient>& gradient);
  void SetGradientShaderMap(
      std::map<int, std::shared_ptr<ColorSource>>&& gradient_shader_map,
      std::map<int, std::pair<size_t, size_t>>&& range_map);

  void SetTextStrokeMap(std::unordered_map<int, TextStroke>&& text_stroke_map);

  void Paint(PaintingContext& context, const FloatPoint& offset) override;

  TextPainter* GetPainter() { return painter_.get(); }

  void SetSelection(const FloatPoint& select_start_position,
                    const FloatPoint& select_end_position);
  void SetAllSelection();

  void PaintSelection(GraphicsContext* context);

  std::u16string GetSelectionString() const;

  const std::u16string& GetText() const { return text_; }

  std::vector<Point> GetPointsFromRangeSelection(int select_start,
                                                 int select_end) const;

  std::vector<int> GetSelectPosition() const {
    return std::vector<int>{std::min(select_start_, select_end_),
                            std::max(select_start_, select_end_)};
  }

  std::vector<FloatRect> GetTextLineRects(int start, int end);
  FloatRect GetTextBoundingRect(int start, int end,
                                const std::vector<FloatRect>& line_rect);

  void SetSelectionChangedListener(
      SelectionChangedCallback selection_changed_callback) {
    selection_changed_callback_ = selection_changed_callback;
  }

  bool IsCollapsed() { return select_start_ == select_end_; }

  std::optional<TextBox> GetEndTextPositionTopAndBottom() const;
  std::optional<TextBox> GetStartTextPositionTopAndBottom() const;

  void SetLineSpacingOffset(double offset) { line_spacing_offset_ = offset; }
  void SetTextPaintAlign(TextAlignment align) { text_paint_align_ = align; }

 protected:
  std::unique_ptr<TextPainter> painter_;
  txt::Paragraph* paragraph_;
  std::u16string text_;
  int select_start_ = -1;
  int select_end_ = -1;
  int pre_select_end_ = -1;
  double line_spacing_offset_ = 0;
  TextAlignment text_paint_align_ = TextAlignment::kLeft;

 private:
  void PaintText(GraphicsContext* graphics_context, const FloatPoint& offset);

  SelectionChangedCallback selection_changed_callback_;
};

}  // namespace clay

#endif  // CLAY_UI_RENDERING_TEXT_RENDER_TEXT_H_
