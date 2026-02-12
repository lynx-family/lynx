// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <iostream>
#include <memory>
#include <string>

#include "clay/common/thread_host.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/image/image.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/style/length.h"
#include "clay/testing/canvas_test.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/painter/painter_graphics_test.h"
#include "clay/ui/rendering/render_image.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkM44.h"
#include "third_party/skia/include/core/SkMaskFilter.h"
#include "third_party/skia/include/gpu/GrContextOptions.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"

namespace clay {
namespace testing {

using clay::testing::MockCanvas;

class ImagePainterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    SkISize image_size = SkISize::Make(32, 32);
    bitmap_.allocPixels(::SkImageInfo::MakeN32(
        image_size.width(), image_size.width(), kOpaque_SkAlphaType));
    bitmap_.eraseColor(SK_ColorGREEN);
    bitmap_.setImmutable();
    render_image_ = std::make_unique<RenderImage>();

    render_image_->SetLeft(10);
    render_image_->SetTop(10);
    render_image_->SetWidth(100);
    render_image_->SetHeight(100);
    render_image_->SetRepeat(ImageRepeat::kNoRepeat);
    image_painter_ = std::make_unique<ImagePainter>(render_image_.get());

    image_ = bitmap_.asImage();

    raw_image_ = Image::CreateImage("", nullptr, nullptr,
                                    std::weak_ptr<Image::Notifier>(), nullptr,
                                    nullptr, false, true);
    render_image_->SetImage(std::make_unique<ImageResource>(raw_image_));
    render_image_->image_resource_->SetTestImage(GraphicsImage::Make(image_));
  }

  void SetImageData(ImageData& image_data) {
    image_data.image_resource = render_image_->image_resource_.get();
    image_data.mode = render_image_->mode_;
    image_data.repeat = render_image_->repeat_;
    image_data.has_cap_insets = render_image_->has_cap_insets_;
    image_data.cap_insets = render_image_->cap_insets_;
    image_data.cap_insets_scale = render_image_->cap_insets_scale_;
    image_data.drop_shadow_offset_x = render_image_->drop_shadow_offset_x_;
    image_data.drop_shadow_offset_y = render_image_->drop_shadow_offset_y_;
    image_data.drop_shadow_color = render_image_->drop_shadow_color_;
  }

  void TearDown() override {}

  FloatPoint Offset() const { return FloatPoint(10, 10); }

  // For debug
  void PrintDrawCalls() {
    for (auto& draw_call : graphics_owner_.mock_canvas().draw_calls()) {
      std::cout << draw_call << std::endl;
    }
  }

  SkBitmap bitmap_;
  // not owned.
  sk_sp<SkImage> image_ = nullptr;
  std::shared_ptr<Image> raw_image_;
  std::unique_ptr<RenderImage> render_image_;
  std::unique_ptr<ImagePainter> image_painter_;
  PainterGraphicsTest graphics_owner_;
};

TEST_F(ImagePainterTest, FillMode) {
  ImageData image_data;
  render_image_->SetMode(FillMode::kScaleToFill);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  render_image_->SetMode(FillMode::kAspectFit);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  render_image_->SetMode(FillMode::kAspectFill);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  render_image_->SetMode(FillMode::kCenter);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());

  constexpr SkRect src = SkRect::MakeLTRB(0, 0, 32, 32);
  constexpr SkRect dst = SkRect::MakeLTRB(0, 0, 100, 100);
  constexpr SkRect dst_center = SkRect::MakeLTRB(34, 34, 66, 66);

  SkPaint paint;
  paint.setAntiAlias(false);
  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_center,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
  };

  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(ImagePainterTest, ImageRepeat) {
  ImageData image_data;
  render_image_->SetWidth(64);
  render_image_->SetHeight(32);
  render_image_->SetMode(FillMode::kCenter);
  render_image_->SetRepeat(ImageRepeat::kNoRepeat);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  render_image_->SetRepeat(ImageRepeat::kRepeat);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  render_image_->SetRepeat(ImageRepeat::kRepeatX);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());

  constexpr SkRect src = SkRect::MakeLTRB(0, 0, 32, 32);
  constexpr SkRect clipRect = SkRect::MakeLTRB(0, 0, 64, 32);
  constexpr SkRect dst_1 = SkRect::MakeLTRB(16, 0, 48, 32);
  constexpr SkRect dst_2 = SkRect::MakeLTRB(-16, 0, 16, 32);
  constexpr SkRect dst_3 = SkRect::MakeLTRB(48, 0, 80, 32);

  SkPaint paint;
  paint.setAntiAlias(false);
  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_1,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
      MockCanvas::DrawCall{
          1, MockCanvas::ClipRectData{clipRect, SkClipOp::kIntersect,
                                      MockCanvas::kHard_ClipEdgeStyle}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_2,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_1,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_3,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}},
      MockCanvas::DrawCall{0, MockCanvas::SaveData{1}},
      MockCanvas::DrawCall{
          1, MockCanvas::ClipRectData{clipRect, SkClipOp::kIntersect,
                                      MockCanvas::kHard_ClipEdgeStyle}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_2,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_1,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src, dst_3,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint, paint}},
      MockCanvas::DrawCall{1, MockCanvas::RestoreData{0}}};
  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls(), expected_draw_calls);
}

TEST_F(ImagePainterTest, Blur) {
  // Normal blur will not be recorded in single displaylist, in fact, blur will
  // be applied in layer tree

  /*
      ImageData image_data;

      render_image_->SetBlurRadius(0.1f);
      SetImageData(image_data);
      image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

      render_image_->SetBlurRadius(0.0f);
      SetImageData(image_data);
      image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

      auto picture = graphics_owner_.mock_context().FinishRecording();
      picture->picture()->RenderTo(&graphics_owner_.mock_canvas());

      // 4 draw_calls for FIRST PaintImage and another draw_call for SECOND:
      EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls().size(), 5u);

      MockCanvas::DrawImageRectData blur_image_draw_call =
          std::get<MockCanvas::DrawImageRectData>(
              graphics_owner_.mock_canvas().draw_calls()[2].data);
      // Draws the blur_image (did not create a new image).
      EXPECT_TRUE(blur_image_draw_call.image_unique_id == image_->uniqueID());

      blur_image_draw_call = std::get<MockCanvas::DrawImageRectData>(
          graphics_owner_.mock_canvas().draw_calls()[4].data);
      // Draws the original image.
      EXPECT_TRUE(blur_image_draw_call.image_unique_id == image_->uniqueID());
      */
}

TEST_F(ImagePainterTest, NinePatchImage) {
  ImageData image_data;
  render_image_->SetWidth(100);
  render_image_->SetHeight(100);

  std::array<Length, 4> cap_insets_vec = {
      Length(5, LengthUnit::kNum),
      Length(10, LengthUnit::kNum),
      Length(0.25, LengthUnit::kPercent),
      Length(0.5, LengthUnit::kPercent),
  };
  render_image_->SetCapInsets(cap_insets_vec);
  SetImageData(image_data);
  image_painter_->PaintImage(&graphics_owner_.mock_context(), image_data);

  auto picture = graphics_owner_.mock_context().FinishRecording();
  picture->picture()->raw()->playback(&graphics_owner_.mock_canvas());

  // 9 draw_calls for individually drawing nine-patch image:
  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls().size(), 9u);

  constexpr SkRect src1 = SkRect::MakeLTRB(0, 0, 16, 5);
  constexpr SkRect src2 = SkRect::MakeLTRB(16, 0, 22, 5);
  constexpr SkRect src3 = SkRect::MakeLTRB(22, 0, 32, 5);
  constexpr SkRect src4 = SkRect::MakeLTRB(0, 5, 16, 24);
  constexpr SkRect src5 = SkRect::MakeLTRB(16, 5, 22, 24);
  constexpr SkRect src6 = SkRect::MakeLTRB(22, 5, 32, 24);
  constexpr SkRect src7 = SkRect::MakeLTRB(0, 24, 16, 32);
  constexpr SkRect src8 = SkRect::MakeLTRB(16, 24, 22, 32);
  constexpr SkRect src9 = SkRect::MakeLTRB(22, 24, 32, 32);

  constexpr SkRect dst1 = SkRect::MakeLTRB(0, 0, 16, 5);
  constexpr SkRect dst2 = SkRect::MakeLTRB(16, 0, 90, 5);
  constexpr SkRect dst3 = SkRect::MakeLTRB(90, 0, 100, 5);
  constexpr SkRect dst4 = SkRect::MakeLTRB(0, 5, 16, 92);
  constexpr SkRect dst5 = SkRect::MakeLTRB(16, 5, 90, 92);
  constexpr SkRect dst6 = SkRect::MakeLTRB(90, 5, 100, 92);
  constexpr SkRect dst7 = SkRect::MakeLTRB(0, 92, 16, 100);
  constexpr SkRect dst8 = SkRect::MakeLTRB(16, 92, 90, 100);
  constexpr SkRect dst9 = SkRect::MakeLTRB(90, 92, 100, 100);

  const auto expected_draw_calls = std::vector{
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src1, dst1,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src2, dst2,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src3, dst3,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src4, dst4,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src5, dst5,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src6, dst6,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src7, dst7,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src8, dst8,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
      MockCanvas::DrawCall{0,
                           MockCanvas::DrawImageRectData{
                               image_->uniqueID(), src9, dst9,
                               SkSamplingOptions(SkFilterMode::kLinear),
                               SkCanvas::kFast_SrcRectConstraint}},
  };

  EXPECT_EQ(graphics_owner_.mock_canvas().draw_calls(), expected_draw_calls);
}

}  // namespace testing
}  // namespace clay
