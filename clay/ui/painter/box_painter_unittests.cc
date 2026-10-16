// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/geometry/float_point.h"
#include "clay/testing/canvas_test.h"
#include "clay/ui/painter/box_painter.h"
#include "clay/ui/painter/painter_graphics_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

using clay::testing::MockCanvas;

class BoxPainterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    render_box_ = std::make_unique<RenderBox>();
    render_box_->SetLeft(0);
    render_box_->SetTop(0);
    render_box_->SetWidth(100);
    render_box_->SetHeight(100);
  }

  std::unique_ptr<RenderBox> render_box_;
  PainterGraphicsTest graphics_owner_;
};

TEST_F(BoxPainterTest, EmptyBox) {
  EXPECT_FALSE(render_box_->HasBackground());
  EXPECT_FALSE(render_box_->HasBorder());
  std::unique_ptr<BoxPainter> box_painter =
      std::make_unique<BoxPainter>(render_box_.get());
  box_painter->Paint(&graphics_owner_.mock_context(), FloatPoint());
  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());
  EXPECT_TRUE(graphics_owner_.mock_canvas().draw_calls().empty());
}

TEST_F(BoxPainterTest, Background) {
  BackgroundData bg_data;
  EXPECT_TRUE(Color::Parse("blue", &bg_data.background_color));
  render_box_->SetBackgroundData(bg_data);
  EXPECT_TRUE(render_box_->HasBackground());
  std::unique_ptr<BoxPainter> box_painter =
      std::make_unique<BoxPainter>(render_box_.get());
  box_painter->Paint(&graphics_owner_.mock_context(), FloatPoint());
  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());

  SkPaint paint;
  paint.setColor(SK_ColorBLUE);
  paint.setAntiAlias(true);
  constexpr SkRect rect = SkRect::MakeLTRB(0, 0, 100, 100);
  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0, MockCanvas::DrawRectData{rect, paint}}};

  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(BoxPainterTest, SimpleBorder) {
  BordersData border_data;
  border_data.width_top_ = 10.f;
  border_data.width_bottom_ = 10.f;
  border_data.width_right_ = 10.f;
  border_data.width_left_ = 10.f;
  border_data.color_top_ = SK_ColorBLUE;
  border_data.color_right_ = SK_ColorBLUE;
  border_data.color_bottom_ = SK_ColorBLUE;
  border_data.color_left_ = SK_ColorBLUE;
  render_box_->SetBorders(border_data);
  EXPECT_TRUE(render_box_->HasBorder());
  EXPECT_FALSE(render_box_->HasBackground());
  std::unique_ptr<BoxPainter> box_painter =
      std::make_unique<BoxPainter>(render_box_.get());
  box_painter->Paint(&graphics_owner_.mock_context(), FloatPoint());
  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());

  SkPaint paint;
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(border_data.width_top_);
  paint.setAntiAlias(true);
  paint.setColor(SK_ColorBLUE);
  constexpr SkRect rect = SkRect::MakeLTRB(5, 5, 95, 95);
  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0, MockCanvas::DrawRectData{rect, paint}}};

  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(BoxPainterTest, BorderPainterRestoresClip) {
  auto* canvas = graphics_owner_.mock_context().Canvas();
  const auto original_clip = canvas->GetGrCanvas()->getDeviceClipBounds();
  const int save_count = canvas->GetSaveCount();
  const FloatRect rect(10, 10, 20, 20);
  for (auto style : {BorderStyleType::kDashed, BorderStyleType::kDotted}) {
    BordersData border_data;
    border_data.width_top_ = border_data.width_bottom_ =
        border_data.width_right_ = border_data.width_left_ = 4.f;
    border_data.style_top_ = border_data.style_bottom_ =
        border_data.style_right_ = border_data.style_left_ = style;
    border_data.color_top_ = border_data.color_bottom_ =
        border_data.color_right_ = border_data.color_left_ = SK_ColorBLUE;
    OutlineData outline_data;
    outline_data.width_ = 3.f;
    outline_data.offset_ = 2.f;
    outline_data.color_ = SK_ColorBLUE;
    outline_data.style_ = style;
    BorderPainter painter;
    painter.SetOuterRect(rect);
    painter.SetBorder(border_data, rect);
    painter.SetOutline(outline_data, rect);

    painter.PaintBorders(canvas, rect, border_data);
    EXPECT_EQ(canvas->GetSaveCount(), save_count);
    EXPECT_EQ(canvas->GetGrCanvas()->getDeviceClipBounds(), original_clip);
    painter.PaintOutline(canvas, outline_data);
    EXPECT_EQ(canvas->GetSaveCount(), save_count);
    EXPECT_EQ(canvas->GetGrCanvas()->getDeviceClipBounds(), original_clip);
  }
}

TEST(BorderPainterTest, ReconfigureWithoutRadius) {
  const FloatRect rect(0, 0, 100, 80);
  BordersData borders;
  borders.width_top_ = borders.width_right_ = borders.width_bottom_ =
      borders.width_left_ = 5.f;
  borders.radius_x_top_left_ = borders.radius_y_top_left_ = 20.f;
  BorderPainter painter;
  painter.SetOuterRect(rect);
  painter.SetBorder(borders, rect);
  ASSERT_TRUE(painter.OuterRect().IsRounded());

  borders.radius_x_top_left_ = borders.radius_y_top_left_ = 0.f;
  painter.SetBorder(borders, rect);
  EXPECT_FALSE(painter.OuterRect().IsRounded());
  EXPECT_FALSE(painter.GetBackgroundRoundedRect(rect).IsRounded());
}

}  // namespace clay
