// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <vector>

#include "clay/gfx/attributes_testing.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "clay/gfx/testing_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkString.h"
#include "third_party/skia/include/core/SkSurface.h"
namespace clay {
namespace testing {

static fml::RefPtr<DlImage> MakeTestImage(int w, int h, SkColor color) {
  sk_sp<SkSurface> surface;
  if (SkColorGetA(color) < 255) {
    surface = SkSurface::MakeRasterN32Premul(w, h);
  } else {
    SkImageInfo info =
        SkImageInfo::MakeN32(w, h, SkAlphaType::kOpaque_SkAlphaType);
    surface = SkSurface::MakeRaster(info);
  }
  SkCanvas* canvas = surface->getCanvas();
  canvas->drawColor(color);
  return DlImage::Make(surface->makeImageSnapshot());
}

static const fml::RefPtr<DlRuntimeEffect> kTestRuntimeEffect1 =
    DlRuntimeEffect::MakeSkia(
        SkRuntimeEffect::MakeForShader(
            SkString("vec4 main(vec2 p) { return vec4(0); }"))
            .effect);
static const fml::RefPtr<DlRuntimeEffect> kTestRuntimeEffect2 =
    DlRuntimeEffect::MakeSkia(
        SkRuntimeEffect::MakeForShader(
            SkString("vec4 main(vec2 p) { return vec4(1); }"))
            .effect);

static const fml::RefPtr<DlImage> kTestImage1 =
    MakeTestImage(10, 10, SK_ColorGREEN);
static const fml::RefPtr<DlImage> kTestAlphaImage1 =
    MakeTestImage(10, 10, SK_ColorTRANSPARENT);
// clang-format off
static const skity::Matrix kTestMatrix1 =
    skity::Matrix(2, 0, 0, 0,
                  0, 3, 0, 0,
                  0, 0, 1, 0,
                  10,  12, 0, 1);
static const skity::Matrix kTestMatrix2 =
// 4 0 15 0 7 17 0 0 1
    skity::Matrix(4, 0, 0, 0,
                  0, 7, 0, 0,
                  0, 0, 1, 0,
                  15, 17, 0, 1);
// clang-format on
static constexpr int kTestStopCount = 3;
static constexpr DlColor kTestColors[kTestStopCount] = {
    DlColor::kRed(),
    DlColor::kGreen(),
    DlColor::kBlue(),
};
static const DlColor kTestAlphaColors[kTestStopCount] = {
    DlColor::kBlue().withAlpha(0x7F),
    DlColor::kRed().withAlpha(0x2F),
    DlColor::kGreen().withAlpha(0xCF),
};
static constexpr float kTestStops[kTestStopCount] = {
    0.0f,
    0.7f,
    1.0f,
};
static constexpr float kTestStops2[kTestStopCount] = {
    0.0f,
    0.3f,
    1.0f,
};
static constexpr skity::Vec2 kTestPoints[2] = {
    skity::Vec2(5, 15),
    skity::Vec2(7, 18),
};
static constexpr skity::Vec2 kTestPoints2[2] = {
    skity::Vec2(100, 115),
    skity::Vec2(107, 118),
};
static const sk_sp<SkShader> kShaderA = SkShaders::Color(SK_ColorRED);
static const sk_sp<SkShader> kShaderB = SkShaders::Color(SK_ColorBLUE);
static const sk_sp<SkShader> kTestUnknownShader =
    SkShaders::Blend(SkBlendMode::kOverlay, kShaderA, kShaderB);
static const sk_sp<SkShader> kTestAlphaUnknownShader =
    SkShaders::Blend(SkBlendMode::kDstOut, kShaderA, kShaderB);

TEST(DisplayListColorSource, FromSkiaColorShader) {
  // We cannot read back the matrix parameter from a Skia LinearGradient
  // so we conservatively use an UnknownColorSource wrapper so as to not
  // lose any data. Note that the Skia Color shader end is read back from
  // the Skia asAGradient() method so while this type of color source
  // does not really need the matrix, we represent all of the gradient
  // sources using an unknown source.
  // Note that this shader should never really happen in practice as it
  // represents a degenerate gradient that collapsed to a single color.
  std::shared_ptr<ColorSource> source = ColorSource::MakeColor(Color::kBlue());
  ASSERT_EQ(source->type(), DlColorSourceType::kColor);

  ASSERT_EQ(source->asColor(), source.get());
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaImageShader) {
  std::shared_ptr<DlColorSource> source =
      ColorSource::MakeImage(kTestImage1, TileMode::kClamp, TileMode::kClamp,
                             ImageSampling::kLinear, &kTestMatrix1);
  DlImageColorSource dl_source(kTestImage1, DlTileMode::kClamp,
                               DlTileMode::kClamp, DlImageSampling::kLinear,
                               &kTestMatrix1);
  ASSERT_EQ(source->type(), DlColorSourceType::kImage);
  ASSERT_EQ(*source->asImage(), dl_source);
  ASSERT_TRUE(source->asImage()->image()->Equals(kTestImage1));
  ASSERT_TRUE(kTestImage1->Equals(source->asImage()->image()));
  ASSERT_EQ(source->asImage()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->asImage()->horizontal_tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asImage()->vertical_tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asImage()->sampling(), DlImageSampling::kLinear);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaLinearGradient) {
  // We cannot read back the matrix parameter from a Skia LinearGradient
  // so we conservatively use an UnknownColorSource wrapper so as to not
  // lose any data.
  const Color* colors = reinterpret_cast<const Color*>(kTestColors);
  std::shared_ptr<ColorSource> source = ColorSource::MakeLinear(
      skity::Vec2(kTestPoints[0].x, kTestPoints[0].y),
      skity::Vec2(kTestPoints[1].x, kTestPoints[1].y), kTestStopCount, colors,
      kTestStops, TileMode::kClamp);
  ASSERT_EQ(source->type(), DlColorSourceType::kLinearGradient);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), source.get());
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaRadialGradient) {
  // We cannot read back the matrix parameter from a Skia RadialGradient
  // so we conservatively use an UnknownColorSource wrapper so as to not
  // lose any data.
  const Color* colors = reinterpret_cast<const Color*>(kTestColors);
  std::shared_ptr<DlColorSource> source = ColorSource::MakeRadial(
      skity::Vec2(kTestPoints[0].x, kTestPoints[0].y), 10.0, kTestStopCount,
      colors, kTestStops, TileMode::kClamp);
  ASSERT_EQ(source->type(), DlColorSourceType::kRadialGradient);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), source.get());
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaConicalGradient) {
  // We cannot read back the matrix parameter from a Skia ConicalGradient
  // so we conservatively use an UnknownColorSource wrapper so as to not
  // lose any data.
  const Color* colors = reinterpret_cast<const Color*>(kTestColors);
  std::shared_ptr<DlColorSource> source = ColorSource::MakeConical(
      skity::Vec2(kTestPoints[0].x, kTestPoints[0].y), 10.0,
      skity::Vec2(kTestPoints[1].x, kTestPoints[1].y), 20.0, kTestStopCount,
      colors, kTestStops, TileMode::kClamp);
  ASSERT_EQ(source->type(), DlColorSourceType::kConicalGradient);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), source.get());
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaSweepGradient) {
  // We cannot read back the matrix parameter, nor the sweep parameters from a
  // Skia SweepGradient so we conservatively use an UnknownColorSource wrapper
  // so as to not lose any data.
  const Color* colors = reinterpret_cast<const Color*>(kTestColors);
  std::shared_ptr<DlColorSource> source = ColorSource::MakeSweep(
      skity::Vec2{kTestPoints[0].x, kTestPoints[0].y}, 0, 360, kTestStopCount,
      colors, kTestStops, TileMode::kClamp);
  ASSERT_EQ(source->type(), DlColorSourceType::kSweepGradient);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), source.get());
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, FromSkiaUnrecognizedShader) {
  std::shared_ptr<DlColorSource> source =
      std::make_shared<UnknownColorSource>(kTestUnknownShader);
  ASSERT_EQ(source->type(), DlColorSourceType::kUnknown);
  ASSERT_EQ(source->gr_object(), kTestUnknownShader);

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, ColorConstructor) {
  DlColorColorSource source(SK_ColorRED);
}

TEST(DisplayListColorSource, ColorShared) {
  DlColorColorSource source(SK_ColorRED);
  ASSERT_NE(source.shared().get(), &source);
  ASSERT_EQ(*source.shared(), source);
}

TEST(DisplayListColorSource, ColorAsColor) {
  DlColorColorSource source(SK_ColorRED);
  ASSERT_NE(source.asColor(), nullptr);
  ASSERT_EQ(source.asColor(), &source);

  ASSERT_EQ(source.asImage(), nullptr);
  ASSERT_EQ(source.asLinearGradient(), nullptr);
  ASSERT_EQ(source.asRadialGradient(), nullptr);
  ASSERT_EQ(source.asConicalGradient(), nullptr);
  ASSERT_EQ(source.asSweepGradient(), nullptr);
  ASSERT_EQ(source.asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, ColorContents) {
  DlColorColorSource source(SK_ColorRED);
  ASSERT_EQ(source.color(), SK_ColorRED);
  ASSERT_EQ(source.is_opaque(), true);
  for (int i = 0; i < 255; i++) {
    SkColor alpha_color = SkColorSetA(SK_ColorRED, i);
    DlColorColorSource alpha_source(alpha_color);
    ASSERT_EQ(alpha_source.color(), alpha_color);
    ASSERT_EQ(alpha_source.is_opaque(), false);
  }
}

TEST(DisplayListColorSource, ColorEquals) {
  DlColorColorSource source1(SK_ColorRED);
  DlColorColorSource source2(SK_ColorRED);
  TestEquals(source1, source2);
}

TEST(DisplayListColorSource, ColorNotEquals) {
  DlColorColorSource source1(SK_ColorRED);
  DlColorColorSource source2(SK_ColorBLUE);
  TestNotEquals(source1, source2, "Color differs");
}

TEST(DisplayListColorSource, ImageConstructor) {
  DlImageColorSource source(kTestImage1, DlTileMode::kClamp, DlTileMode::kClamp,
                            DlImageSampling::kLinear, &kTestMatrix1);
}

TEST(DisplayListColorSource, ImageShared) {
  DlImageColorSource source(kTestImage1, DlTileMode::kClamp, DlTileMode::kClamp,
                            DlImageSampling::kLinear, &kTestMatrix1);
  ASSERT_NE(source.shared().get(), &source);
  ASSERT_EQ(*source.shared(), source);
}

TEST(DisplayListColorSource, ImageAsImage) {
  DlImageColorSource source(kTestImage1, DlTileMode::kClamp, DlTileMode::kClamp,
                            DlImageSampling::kLinear, &kTestMatrix1);
  ASSERT_NE(source.asImage(), nullptr);
  ASSERT_EQ(source.asImage(), &source);

  ASSERT_EQ(source.asColor(), nullptr);
  ASSERT_EQ(source.asLinearGradient(), nullptr);
  ASSERT_EQ(source.asRadialGradient(), nullptr);
  ASSERT_EQ(source.asConicalGradient(), nullptr);
  ASSERT_EQ(source.asSweepGradient(), nullptr);
}

TEST(DisplayListColorSource, ImageContents) {
  DlImageColorSource source(kTestImage1, DlTileMode::kRepeat,
                            DlTileMode::kMirror, DlImageSampling::kLinear,
                            &kTestMatrix1);
  ASSERT_EQ(source.image(), kTestImage1);
  ASSERT_EQ(source.horizontal_tile_mode(), DlTileMode::kRepeat);
  ASSERT_EQ(source.vertical_tile_mode(), DlTileMode::kMirror);
  ASSERT_EQ(source.sampling(), DlImageSampling::kLinear);
  ASSERT_EQ(source.matrix(), kTestMatrix1);
  ASSERT_EQ(source.is_opaque(), true);
}

TEST(DisplayListColorSource, AlphaImageContents) {
  DlImageColorSource source(kTestAlphaImage1, DlTileMode::kRepeat,
                            DlTileMode::kMirror, DlImageSampling::kLinear,
                            &kTestMatrix1);
  ASSERT_EQ(source.image(), kTestAlphaImage1);
  ASSERT_EQ(source.horizontal_tile_mode(), DlTileMode::kRepeat);
  ASSERT_EQ(source.vertical_tile_mode(), DlTileMode::kMirror);
  ASSERT_EQ(source.sampling(), DlImageSampling::kLinear);
  ASSERT_EQ(source.matrix(), kTestMatrix1);
  ASSERT_EQ(source.is_opaque(), false);
}

TEST(DisplayListColorSource, ImageEquals) {
  DlImageColorSource source1(kTestImage1, DlTileMode::kClamp,
                             DlTileMode::kMirror, DlImageSampling::kLinear,
                             &kTestMatrix1);
  DlImageColorSource source2(kTestImage1, DlTileMode::kClamp,
                             DlTileMode::kMirror, DlImageSampling::kLinear,
                             &kTestMatrix1);
  TestEquals(source1, source2);
}

TEST(DisplayListColorSource, ImageNotEquals) {
  DlImageColorSource source1(kTestImage1, DlTileMode::kClamp,
                             DlTileMode::kMirror, DlImageSampling::kLinear,
                             &kTestMatrix1);
  {
    DlImageColorSource source2(kTestAlphaImage1, DlTileMode::kClamp,
                               DlTileMode::kMirror, DlImageSampling::kLinear,
                               &kTestMatrix1);
    TestNotEquals(source1, source2, "Image differs");
  }
  {
    DlImageColorSource source2(kTestImage1, DlTileMode::kRepeat,
                               DlTileMode::kMirror, DlImageSampling::kLinear,
                               &kTestMatrix1);
    TestNotEquals(source1, source2, "hTileMode differs");
  }
  {
    DlImageColorSource source2(kTestImage1, DlTileMode::kClamp,
                               DlTileMode::kRepeat, DlImageSampling::kLinear,
                               &kTestMatrix1);
    TestNotEquals(source1, source2, "vTileMode differs");
  }
  {
    DlImageColorSource source2(kTestImage1, DlTileMode::kClamp,
                               DlTileMode::kMirror, DlImageSampling::kCubic,
                               &kTestMatrix1);
    TestNotEquals(source1, source2, "Sampling differs");
  }
  {
    DlImageColorSource source2(kTestImage1, DlTileMode::kClamp,
                               DlTileMode::kMirror, DlImageSampling::kLinear,
                               &kTestMatrix2);
    TestNotEquals(source1, source2, "Matrix differs");
  }
}

TEST(DisplayListColorSource, LinearGradientConstructor) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
}

TEST(DisplayListColorSource, LinearGradientShared) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->shared().get(), source.get());
  ASSERT_EQ(*source->shared().get(), *source.get());
}

TEST(DisplayListColorSource, LinearGradientAsLinear) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), source.get());

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, LinearGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asLinearGradient()->start_point(), kTestPoints[0]);
  ASSERT_EQ(source->asLinearGradient()->end_point(), kTestPoints[1]);
  ASSERT_EQ(source->asLinearGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asLinearGradient()->colors()[i], kTestColors[i]);
    ASSERT_EQ(source->asLinearGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asLinearGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asLinearGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), true);
}

TEST(DisplayListColorSource, AlphaLinearGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestAlphaColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asLinearGradient()->start_point(), kTestPoints[0]);
  ASSERT_EQ(source->asLinearGradient()->end_point(), kTestPoints[1]);
  ASSERT_EQ(source->asLinearGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asLinearGradient()->colors()[i], kTestAlphaColors[i]);
    ASSERT_EQ(source->asLinearGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asLinearGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asLinearGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), false);
}

TEST(DisplayListColorSource, LinearGradientEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  TestEquals(*source1, *source2);
}

TEST(DisplayListColorSource, LinearGradientNotEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeLinear(
      kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints2[0], kTestPoints[1], kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Point 0 differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints2[1], kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Point 1 differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints[1], 2, kTestColors, kTestStops,  //
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stop count differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints[1], kTestStopCount, kTestAlphaColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Colors differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors,
        kTestStops2, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stops differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kMirror, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Tile Mode differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeLinear(
        kTestPoints[0], kTestPoints[1], kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix2);
    TestNotEquals(*source1, *source2, "Matrix differs");
  }
}

TEST(DisplayListColorSource, RadialGradientConstructor) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
}

TEST(DisplayListColorSource, RadialGradientShared) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->shared().get(), source.get());
  ASSERT_EQ(*source->shared().get(), *source.get());
}

TEST(DisplayListColorSource, RadialGradientAsRadial) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), source.get());

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, RadialGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asRadialGradient()->center(), kTestPoints[0]);
  ASSERT_EQ(source->asRadialGradient()->radius(), 10.0);
  ASSERT_EQ(source->asRadialGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asRadialGradient()->colors()[i], kTestColors[i]);
    ASSERT_EQ(source->asRadialGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asRadialGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asRadialGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), true);
}

TEST(DisplayListColorSource, AlphaRadialGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestAlphaColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asRadialGradient()->center(), kTestPoints[0]);
  ASSERT_EQ(source->asRadialGradient()->radius(), 10.0);
  ASSERT_EQ(source->asRadialGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asRadialGradient()->colors()[i], kTestAlphaColors[i]);
    ASSERT_EQ(source->asRadialGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asRadialGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asRadialGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), false);
}

TEST(DisplayListColorSource, RadialGradientEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  TestEquals(*source1, *source2);
}

TEST(DisplayListColorSource, RadialGradientNotEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeRadial(
      kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints2[0], 10.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Center differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 20.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Radius differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 10.0, 2, kTestColors, kTestStops,  //
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stop count differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 10.0, kTestStopCount, kTestAlphaColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Colors differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops2,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stops differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kMirror, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Tile Mode differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeRadial(
        kTestPoints[0], 10.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix2);
    TestNotEquals(*source1, *source2, "Matrix differs");
  }
}

TEST(DisplayListColorSource, ConicalGradientConstructor) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
}

TEST(DisplayListColorSource, ConicalGradientShared) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->shared().get(), source.get());
  ASSERT_EQ(*source->shared().get(), *source.get());
}

TEST(DisplayListColorSource, ConicalGradientAsConical) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), source.get());

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, ConicalGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asConicalGradient()->start_center(), kTestPoints[0]);
  ASSERT_EQ(source->asConicalGradient()->start_radius(), 10.0);
  ASSERT_EQ(source->asConicalGradient()->end_center(), kTestPoints[1]);
  ASSERT_EQ(source->asConicalGradient()->end_radius(), 20.0);
  ASSERT_EQ(source->asConicalGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asConicalGradient()->colors()[i], kTestColors[i]);
    ASSERT_EQ(source->asConicalGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asConicalGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asConicalGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), true);
}

TEST(DisplayListColorSource, AlphaConicalGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount,
      kTestAlphaColors, kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asConicalGradient()->start_center(), kTestPoints[0]);
  ASSERT_EQ(source->asConicalGradient()->start_radius(), 10.0);
  ASSERT_EQ(source->asConicalGradient()->end_center(), kTestPoints[1]);
  ASSERT_EQ(source->asConicalGradient()->end_radius(), 20.0);
  ASSERT_EQ(source->asConicalGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asConicalGradient()->colors()[i], kTestAlphaColors[i]);
    ASSERT_EQ(source->asConicalGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asConicalGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asConicalGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), false);
}

TEST(DisplayListColorSource, ConicalGradientEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  TestEquals(*source1, *source2);
}

TEST(DisplayListColorSource, ConicalGradientNotEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeConical(
      kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
      kTestStops, DlTileMode::kClamp, &kTestMatrix1);
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints2[0], 10.0, kTestPoints[1], 20.0, kTestStopCount,
        kTestColors, kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Start Center differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 15.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Start Radius differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints2[1], 20.0, kTestStopCount,
        kTestColors, kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "End Center differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 25.0, kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "End Radius differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 20.0, 2, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stop count differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount,
        kTestAlphaColors, kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Colors differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
        kTestStops2, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stops differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kMirror, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Tile Mode differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeConical(
        kTestPoints[0], 10.0, kTestPoints[1], 20.0, kTestStopCount, kTestColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix2);
    TestNotEquals(*source1, *source2, "Matrix differs");
  }
}

TEST(DisplayListColorSource, SweepGradientConstructor) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
}

TEST(DisplayListColorSource, SweepGradientShared) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->shared().get(), source.get());
  ASSERT_EQ(*source->shared().get(), *source.get());
}

TEST(DisplayListColorSource, SweepGradientAsSweep) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_NE(source->asSweepGradient(), nullptr);
  ASSERT_EQ(source->asSweepGradient(), source.get());

  ASSERT_EQ(source->asColor(), nullptr);
  ASSERT_EQ(source->asImage(), nullptr);
  ASSERT_EQ(source->asLinearGradient(), nullptr);
  ASSERT_EQ(source->asRadialGradient(), nullptr);
  ASSERT_EQ(source->asConicalGradient(), nullptr);
  ASSERT_EQ(source->asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, SweepGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asSweepGradient()->center(), kTestPoints[0]);
  ASSERT_EQ(source->asSweepGradient()->start(), 10.0);
  ASSERT_EQ(source->asSweepGradient()->end(), 20.0);
  ASSERT_EQ(source->asSweepGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asSweepGradient()->colors()[i], kTestColors[i]);
    ASSERT_EQ(source->asSweepGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asSweepGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asSweepGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), true);
}

TEST(DisplayListColorSource, AlphaSweepGradientContents) {
  std::shared_ptr<DlColorSource> source = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestAlphaColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  ASSERT_EQ(source->asSweepGradient()->center(), kTestPoints[0]);
  ASSERT_EQ(source->asSweepGradient()->start(), 10.0);
  ASSERT_EQ(source->asSweepGradient()->end(), 20.0);
  ASSERT_EQ(source->asSweepGradient()->stop_count(), kTestStopCount);
  for (int i = 0; i < kTestStopCount; i++) {
    ASSERT_EQ(source->asSweepGradient()->colors()[i], kTestAlphaColors[i]);
    ASSERT_EQ(source->asSweepGradient()->stops()[i], kTestStops[i]);
  }
  ASSERT_EQ(source->asSweepGradient()->tile_mode(), DlTileMode::kClamp);
  ASSERT_EQ(source->asSweepGradient()->matrix(), kTestMatrix1);
  ASSERT_EQ(source->is_opaque(), false);
}

TEST(DisplayListColorSource, SweepGradientEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  TestEquals(*source1, *source2);
}

TEST(DisplayListColorSource, SweepGradientNotEquals) {
  std::shared_ptr<DlColorSource> source1 = DlColorSource::MakeSweep(
      kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
      DlTileMode::kClamp, &kTestMatrix1);
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints2[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Center differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 15.0, 20.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Start Angle differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 25.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "End Angle differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 20.0, 2, kTestColors, kTestStops,  //
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stop count differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestAlphaColors,
        kTestStops, DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Colors differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops2,
        DlTileMode::kClamp, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Stops differ");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kMirror, &kTestMatrix1);
    TestNotEquals(*source1, *source2, "Tile Mode differs");
  }
  {
    std::shared_ptr<DlColorSource> source2 = DlColorSource::MakeSweep(
        kTestPoints[0], 10.0, 20.0, kTestStopCount, kTestColors, kTestStops,
        DlTileMode::kClamp, &kTestMatrix2);
    TestNotEquals(*source1, *source2, "Matrix differs");
  }
}

TEST(DisplayListColorSource, UnknownConstructor) {
  DlUnknownColorSource source(kTestUnknownShader);
}

TEST(DisplayListColorSource, UnknownShared) {
  DlUnknownColorSource source(kTestUnknownShader);
  ASSERT_NE(source.shared().get(), &source);
  ASSERT_EQ(*source.shared(), source);
}

TEST(DisplayListColorSource, UnknownAsNone) {
  DlUnknownColorSource source(kTestUnknownShader);
  ASSERT_EQ(source.asColor(), nullptr);
  ASSERT_EQ(source.asImage(), nullptr);
  ASSERT_EQ(source.asLinearGradient(), nullptr);
  ASSERT_EQ(source.asRadialGradient(), nullptr);
  ASSERT_EQ(source.asConicalGradient(), nullptr);
  ASSERT_EQ(source.asSweepGradient(), nullptr);
  ASSERT_EQ(source.asRuntimeEffect(), nullptr);
}

TEST(DisplayListColorSource, UnknownContents) {
  DlUnknownColorSource source(kTestUnknownShader);
  ASSERT_EQ(source.gr_object(), kTestUnknownShader);
  // Blend shaders always return false for is_opaque.
  // See: https://bugs.chromium.org/p/skia/issues/detail?id=13046
  ASSERT_EQ(source.is_opaque(), false);
}

TEST(DisplayListColorSource, AlphaUnknownContents) {
  DlUnknownColorSource source(kTestAlphaUnknownShader);
  ASSERT_EQ(source.gr_object(), kTestAlphaUnknownShader);
  ASSERT_EQ(source.is_opaque(), false);
}

TEST(DisplayListColorSource, UnknownEquals) {
  DlUnknownColorSource source1(kTestUnknownShader);
  DlUnknownColorSource source2(kTestUnknownShader);
  TestEquals(source1, source2);
}

TEST(DisplayListColorSource, UnknownNotEquals) {
  DlUnknownColorSource source1(kTestUnknownShader);
  DlUnknownColorSource source2(kTestAlphaUnknownShader);
  TestNotEquals(source1, source2, "SkShader differs");
}

TEST(DisplayListColorSource, RuntimeEffect) {
  std::shared_ptr<DlRuntimeEffectColorSource> source1 =
      DlColorSource::MakeRuntimeEffect(
          kTestRuntimeEffect1, {}, std::make_shared<std::vector<uint8_t>>());
  std::shared_ptr<DlRuntimeEffectColorSource> source2 =
      DlColorSource::MakeRuntimeEffect(
          kTestRuntimeEffect2, {}, std::make_shared<std::vector<uint8_t>>());
  std::shared_ptr<DlRuntimeEffectColorSource> source3 =
      DlColorSource::MakeRuntimeEffect(
          nullptr, {}, std::make_shared<std::vector<uint8_t>>());

  ASSERT_EQ(source1->type(), DlColorSourceType::kRuntimeEffect);
  ASSERT_EQ(source1->asRuntimeEffect(), source1.get());
  ASSERT_NE(source2->asRuntimeEffect(), source1.get());

  ASSERT_EQ(source1->asImage(), nullptr);
  ASSERT_EQ(source1->asColor(), nullptr);
  ASSERT_EQ(source1->asLinearGradient(), nullptr);
  ASSERT_EQ(source1->asRadialGradient(), nullptr);
  ASSERT_EQ(source1->asConicalGradient(), nullptr);
  ASSERT_EQ(source1->asSweepGradient(), nullptr);

  ASSERT_NE(source1->gr_object(), nullptr);
  ASSERT_EQ(source3->gr_object(), nullptr);

  TestEquals(source1, source1);
  TestEquals(source3, source3);
  TestNotEquals(source1, source2, "SkRuntimeEffect differs");
  TestNotEquals(source2, source3, "SkRuntimeEffect differs");
}

TEST(DisplayListColorSource, RuntimeEffectWithNullSampler) {
  std::shared_ptr<DlRuntimeEffectColorSource> source1 =
      DlColorSource::MakeRuntimeEffect(
          kTestRuntimeEffect1, {nullptr},
          std::make_shared<std::vector<uint8_t>>());

  ASSERT_EQ(source1->gr_object(), nullptr);
}

}  // namespace testing
}  // namespace clay
