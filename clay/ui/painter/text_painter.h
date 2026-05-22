// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_TEXT_PAINTER_H_
#define CLAY_UI_PAINTER_TEXT_PAINTER_H_

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/graphics_context.h"
#include "clay/third_party/txt/src/txt/paragraph.h"
#include "clay/third_party/txt/src/txt/placeholder_run.h"
#include "clay/ui/common/editing_misc.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/painter/gradient.h"

namespace clay {

struct TextBox {
  FloatRect rect;
  // TODO(Xietong): text direction

  explicit TextBox(const FloatRect& _rect) : rect(_rect) {}
  float GetLeft() { return rect.x(); }
  float GetTop() { return rect.y(); }
  float GetBottom() { return rect.y() + rect.height(); }
  float GetRight() { return rect.x() + rect.width(); }
};

struct TextStroke {
  double width = 0;
  Color stroke_color;
  Color fill_color;
};

using TextPosWithAffinity = std::pair<size_t, Affinity>;

using RectHeightStyle = txt::Paragraph::RectHeightStyle;
using RectWidthStyle = txt::Paragraph::RectWidthStyle;

class TextPainter {
 public:
  TextPainter();
  ~TextPainter();

  void SetWidth(float width);
  void SetHeight(float height);

  void SetParagraph(txt::Paragraph* paragraph);

  void SetGradient(const std::optional<Gradient>& gradient);
  void SetGradientShaderMap(
      std::map<int, std::shared_ptr<ColorSource>>&& gradient_shader_map,
      std::map<int, std::pair<size_t, size_t>>&& range_map);

  void SetTextStrokeMap(std::unordered_map<int, TextStroke>&& text_stroke_map);

  std::vector<TextBox> GetRectsForPlaceholders();
  std::vector<TextBox> GetRectsForRange(
      int start, int end,
      RectHeightStyle height_style = RectHeightStyle::kTight,
      RectWidthStyle width_style = RectWidthStyle::kTight);
  TextPosWithAffinity GetGlyphPositionAtCoordinate(float x, float y);
  txt::Paragraph* GetParagraph() { return paragraph_; }

  // Returns the line height for glyph position. If no line match, return zero.
  double GetLineHeightForPosition(size_t position,
                                  Affinity affinity = Affinity::kDownstream,
                                  size_t* line_number = nullptr);

  // Return the line height up or down the line of provided position.
  // If no line match, return zero.
  double GetUpDownLineHeightForPosition(TextPosWithAffinity pos_with_affinity,
                                        VerticalDirection direction);

  TextRange GetLineRangeForPosition(size_t position);

  bool CanPaint() const { return paragraph_ != nullptr; }
  void Paint(GraphicsContext* context, double x_offset = 0,
             double y_offset = 0);

  // Finds the first and last glyphs that define a word containing the glyph at
  // index offset.
  TextRange GetWordBoundary(size_t offset);

  double GetLongestLine() { return paragraph_->GetLongestLine(); }

  std::vector<FloatRect> GetTextLineRects(int start, int end);

 private:
  size_t GetTextSize();
  void UpdateGradientIfNeeded(GraphicsContext* context);
  void UpdateTextFillPaint();
  void UpdateTextStrokePaint();

  float width_ = 0.f;
  float height_ = 0.f;

  txt::Paragraph* paragraph_ = nullptr;
  bool is_gradient_dirty_ = true;
  std::map<int, std::shared_ptr<ColorSource>> gradient_shader_map_;
  std::map<int, std::pair<size_t, size_t>> gradient_shader_range_map_;
  std::unordered_map<int, TextStroke> text_stroke_map_;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_TEXT_PAINTER_H_
