// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/matrix_clip_tracker.h"
#include "clay/gfx/skity_to_skia_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkPath.h"

namespace clay {
namespace testing {

TEST(DisplayListMatrixClipTracker, Constructor) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkM44 m44 = SkM44::Scale(4, 4);
  const SkRect local_cull_rect = SkRect::MakeLTRB(5, 5, 15, 15);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));

  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(m44));
}

TEST(DisplayListMatrixClipTracker, Constructor4x4) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  // clang-format off
  const SkM44 m44 = SkM44(4, 0, 0.5, 0,
                          0, 4, 0.5, 0,
                          0, 0, 4.0, 0,
                          0, 0, 0.0, 1);
  // clang-format on
  const SkRect local_cull_rect = SkRect::MakeLTRB(5, 5, 15, 15);

  MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                            ConvertSkM44ToMatrix(m44));

  ASSERT_TRUE(tracker.using_4x4_matrix());
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker.matrix_4x4(), ConvertSkM44ToMatrix(m44));
}

TEST(DisplayListMatrixClipTracker, TransformTo4x4) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  // clang-format off
  const SkM44 m44 = SkM44(4, 0, 0.5, 0,
                          0, 4, 0.5, 0,
                          0, 0, 4.0, 0,
                          0, 0, 0.0, 1);
  // clang-format on
  const SkRect local_cull_rect = SkRect::MakeLTRB(5, 5, 15, 15);

  MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                            ConvertSkMatrixToSkityMatrix(SkMatrix::I()));
  ASSERT_TRUE(tracker.using_4x4_matrix());

  tracker.transform(ConvertSkM44ToMatrix(m44));
  ASSERT_TRUE(tracker.using_4x4_matrix());
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker.matrix_4x4(), ConvertSkM44ToMatrix(m44));
}

TEST(DisplayListMatrixClipTracker, SetTo4x4) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  // clang-format off
  const SkM44 m44 = SkM44(4, 0, 0.5, 0,
                          0, 4, 0.5, 0,
                          0, 0, 4.0, 0,
                          0, 0, 0.0, 1);
  // clang-format on
  const SkRect local_cull_rect = SkRect::MakeLTRB(5, 5, 15, 15);

  MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                            ConvertSkMatrixToSkityMatrix(SkMatrix::I()));
  ASSERT_TRUE(tracker.using_4x4_matrix());

  tracker.setTransform(ConvertSkM44ToMatrix(m44));
  ASSERT_TRUE(tracker.using_4x4_matrix());
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker.matrix_4x4(), ConvertSkM44ToMatrix(m44));
}

TEST(DisplayListMatrixClipTracker, UpgradeTo4x4SaveAndRestore) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  // clang-format off
  const SkM44 m44 = SkM44(4, 0, 0.5, 0,
                          0, 4, 0.5, 0,
                          0, 0, 4.0, 0,
                          0, 0, 0.0, 1);
  // clang-format on
  const SkRect local_cull_rect = SkRect::MakeLTRB(5, 5, 15, 15);

  MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                            ConvertSkMatrixToSkityMatrix(SkMatrix::I()));
  ASSERT_TRUE(tracker.using_4x4_matrix());

  tracker.save();
  ASSERT_TRUE(tracker.using_4x4_matrix());

  tracker.transform(ConvertSkM44ToMatrix(m44));
  ASSERT_TRUE(tracker.using_4x4_matrix());
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker.matrix_4x4(), ConvertSkM44ToMatrix(m44));

  tracker.restore();
  ASSERT_TRUE(tracker.using_4x4_matrix());
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.local_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker.matrix_4x4(), ConvertSkM44ToMatrix(SkM44()));
}

TEST(DisplayListMatrixClipTracker, Translate) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkMatrix matrix = SkMatrix::Scale(4, 4);
  const SkM44 m44 = SkM44::Scale(4, 4);
  const SkMatrix translated_matrix =
      SkMatrix::Concat(matrix, SkMatrix::Translate(5, 1));
  const SkM44 translated_m44 = SkM44(translated_matrix);
  const SkRect local_cull_rect = SkRect::MakeLTRB(0, 4, 10, 14);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.translate(5, 1);

  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(translated_m44));
}

TEST(DisplayListMatrixClipTracker, Scale) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkMatrix matrix = SkMatrix::Scale(4, 4);
  const SkM44 m44 = SkM44::Scale(4, 4);
  const SkMatrix scaled_matrix =
      SkMatrix::Concat(matrix, SkMatrix::Scale(5, 2.5));
  const SkM44 scaled_m44 = SkM44(scaled_matrix);
  const SkRect local_cull_rect = SkRect::MakeLTRB(1, 2, 3, 6);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.scale(5, 2.5);

  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(scaled_m44));
}

TEST(DisplayListMatrixClipTracker, Skew) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkMatrix matrix = SkMatrix::Scale(4, 4);
  const SkM44 m44 = SkM44::Scale(4, 4);
  const SkMatrix skewed_matrix =
      SkMatrix::Concat(matrix, SkMatrix::Skew(.25, 0));
  const SkM44 skewed_m44 = SkM44(skewed_matrix);
  const SkRect local_cull_rect = SkRect::MakeLTRB(1.25, 5, 13.75, 15);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.skew(.25, 0);

  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(skewed_m44));
}

TEST(DisplayListMatrixClipTracker, Rotate) {
  const skity::Rect cull_rect = skity::Rect::MakeLTRB(20, 20, 60, 60);
  const skity::Matrix matrix = skity::Matrix::Scale(4, 4);
  skity::Matrix rotated_matrix = matrix;
  rotated_matrix.PreConcat(skity::Matrix::RotateDeg(90));
  const skity::Rect local_cull_rect = skity::Rect::MakeLTRB(5, -15, 15, -5);

  MatrixClipTracker tracker1(cull_rect, matrix);
  tracker1.rotate(90);

  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), cull_rect);
  // tracker1.local_cull_rect() and tracker1.matrix_4x4() have some precision
  // problems. So we cannot simply use ASSERT_EQ to compare them.
  auto tracker1_local_cull_rect = tracker1.local_cull_rect();
  tracker1_local_cull_rect.Round();
  ASSERT_EQ(tracker1_local_cull_rect, local_cull_rect);
  ASSERT_EQ(tracker1.matrix_4x4(), rotated_matrix);
}

TEST(DisplayListMatrixClipTracker, Transform2DAffine) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkMatrix matrix = SkMatrix::Scale(4, 4);
  const SkM44 m44 = SkM44::Scale(4, 4);

  const SkMatrix transformed_matrix =
      SkMatrix::Concat(matrix, SkMatrix::MakeAll(2, 0, 5,  //
                                                 0, 2, 6,  //
                                                 0, 0, 1));
  const SkM44 transformed_m44 = SkM44(transformed_matrix);
  const SkRect local_cull_rect = SkRect::MakeLTRB(0, -0.5, 5, 4.5);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.transform2DAffine(2, 0, 5,  //
                             0, 2, 6);
  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(transformed_m44));
}

TEST(DisplayListMatrixClipTracker, TransformFullPerspectiveUsing3x3Matrix) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkMatrix matrix = SkMatrix::Scale(4, 4);
  const SkM44 m44 = SkM44::Scale(4, 4);

  const SkMatrix transformed_matrix =
      SkMatrix::Concat(matrix, SkMatrix::MakeAll(2, 0, 5,  //
                                                 0, 2, 6,  //
                                                 0, 0, 1));
  const SkM44 transformed_m44 = SkM44(transformed_matrix);
  const SkRect local_cull_rect = SkRect::MakeLTRB(0, -0.5, 5, 4.5);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.transformFullPerspective(2, 0, 0, 5,  //
                                    0, 2, 0, 6,  //
                                    0, 0, 1, 0,  //
                                    0, 0, 0, 1);
  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(transformed_m44));
}

TEST(DisplayListMatrixClipTracker, TransformFullPerspectiveUsing4x4Matrix) {
  const SkRect cull_rect = SkRect::MakeLTRB(20, 20, 60, 60);
  const SkM44 m44 = SkM44::Scale(4, 4);

  const SkM44 transformed_m44 = SkM44(m44, SkM44(2, 0, 0, 5,  //
                                                 0, 2, 0, 6,  //
                                                 0, 0, 1, 7,  //
                                                 0, 0, 0, 1));
  const SkRect local_cull_rect = SkRect::MakeLTRB(0, -0.5, 5, 4.5);

  MatrixClipTracker tracker1(ConvertSkRectToSkityRect(cull_rect),
                             ConvertSkM44ToMatrix(m44));
  tracker1.transformFullPerspective(2, 0, 0, 5,  //
                                    0, 2, 0, 6,  //
                                    0, 0, 1, 7,  //
                                    0, 0, 0, 1);
  ASSERT_TRUE(tracker1.using_4x4_matrix());
  ASSERT_EQ(tracker1.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(tracker1.local_cull_rect(),
            ConvertSkRectToSkityRect(local_cull_rect));
  ASSERT_EQ(tracker1.matrix_4x4(), ConvertSkM44ToMatrix(transformed_m44));
}

TEST(DisplayListMatrixClipTracker, ClipDifference) {
  SkRect cull_rect = SkRect::MakeLTRB(20, 20, 40, 40);

  auto non_reducing = [&cull_rect](const SkRect& diff_rect,
                                   const std::string& label) {
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      tracker.clipRect(ConvertSkRectToSkityRect(diff_rect),
                       SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect))
          << label;
    }
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      const SkRRect diff_rrect = SkRRect::MakeRect(diff_rect);
      tracker.clipRRect(ConvertSkRRectToSkityRRect(diff_rrect),
                        SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect))
          << label << " (RRect)";
    }
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      const SkPath diff_path = SkPath().addRect(diff_rect);
      tracker.clipPath(diff_path, SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect))
          << label << " (RRect)";
    }
  };

  auto reducing = [&cull_rect](const SkRect& diff_rect,
                               const SkRect& result_rect,
                               const std::string& label) {
    ASSERT_TRUE(result_rect.isEmpty() || cull_rect.contains(result_rect));
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      tracker.clipRect(ConvertSkRectToSkityRect(diff_rect),
                       SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(),
                ConvertSkRectToSkityRect(result_rect))
          << label;
    }
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      const SkRRect diff_rrect = SkRRect::MakeRect(diff_rect);
      tracker.clipRRect(ConvertSkRRectToSkityRRect(diff_rrect),
                        SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(),
                ConvertSkRectToSkityRect(result_rect))
          << label << " (RRect)";
    }
    {
      MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                                skity::Matrix());
      const SkPath diff_path = SkPath().addRect(diff_rect);
      tracker.clipPath(diff_path, SkClipOp::kDifference, false);
      ASSERT_EQ(tracker.device_cull_rect(),
                ConvertSkRectToSkityRect(result_rect))
          << label << " (RRect)";
    }
  };

  // Skim the corners and edge
  non_reducing(SkRect::MakeLTRB(10, 10, 20, 20), "outside UL corner");
  non_reducing(SkRect::MakeLTRB(20, 10, 40, 20), "Above");
  non_reducing(SkRect::MakeLTRB(40, 10, 50, 20), "outside UR corner");
  non_reducing(SkRect::MakeLTRB(40, 20, 50, 40), "Right");
  non_reducing(SkRect::MakeLTRB(40, 40, 50, 50), "outside LR corner");
  non_reducing(SkRect::MakeLTRB(20, 40, 40, 50), "Below");
  non_reducing(SkRect::MakeLTRB(10, 40, 20, 50), "outside LR corner");
  non_reducing(SkRect::MakeLTRB(10, 20, 20, 40), "Left");

  // Overlap corners
  non_reducing(SkRect::MakeLTRB(15, 15, 25, 25), "covering UL corner");
  non_reducing(SkRect::MakeLTRB(35, 15, 45, 25), "covering UR corner");
  non_reducing(SkRect::MakeLTRB(35, 35, 45, 45), "covering LR corner");
  non_reducing(SkRect::MakeLTRB(15, 35, 25, 45), "covering LL corner");

  // Overlap edges, but not across an entire side
  non_reducing(SkRect::MakeLTRB(20, 15, 39, 25), "Top edge left-biased");
  non_reducing(SkRect::MakeLTRB(21, 15, 40, 25), "Top edge, right biased");
  non_reducing(SkRect::MakeLTRB(35, 20, 45, 39), "Right edge, top-biased");
  non_reducing(SkRect::MakeLTRB(35, 21, 45, 40), "Right edge, bottom-biased");
  non_reducing(SkRect::MakeLTRB(20, 35, 39, 45), "Bottom edge, left-biased");
  non_reducing(SkRect::MakeLTRB(21, 35, 40, 45), "Bottom edge, right-biased");
  non_reducing(SkRect::MakeLTRB(15, 20, 25, 39), "Left edge, top-biased");
  non_reducing(SkRect::MakeLTRB(15, 21, 25, 40), "Left edge, bottom-biased");

  // Slice all the way through the middle
  non_reducing(SkRect::MakeLTRB(25, 15, 35, 45), "Vertical interior slice");
  non_reducing(SkRect::MakeLTRB(15, 25, 45, 35), "Horizontal interior slice");

  // Slice off each edge
  reducing(SkRect::MakeLTRB(20, 15, 40, 25),  //
           SkRect::MakeLTRB(20, 25, 40, 40),  //
           "Slice off top");
  reducing(SkRect::MakeLTRB(35, 20, 45, 40),  //
           SkRect::MakeLTRB(20, 20, 35, 40),  //
           "Slice off right");
  reducing(SkRect::MakeLTRB(20, 35, 40, 45),  //
           SkRect::MakeLTRB(20, 20, 40, 35),  //
           "Slice off bottom");
  reducing(SkRect::MakeLTRB(15, 20, 25, 40),  //
           SkRect::MakeLTRB(25, 20, 40, 40),  //
           "Slice off left");

  // cull rect contains diff rect
  non_reducing(SkRect::MakeLTRB(21, 21, 39, 39), "Contained, non-covering");

  // cull rect equals diff rect
  reducing(cull_rect, SkRect::MakeEmpty(), "Perfectly covering");

  // diff rect contains cull rect
  reducing(SkRect::MakeLTRB(15, 15, 45, 45), SkRect::MakeEmpty(), "Smothering");
}

TEST(DisplayListMatrixClipTracker, ClipPathWithInvertFillType) {
  SkRect cull_rect = SkRect::MakeLTRB(0, 0, 100.0, 100.0);
  MatrixClipTracker builder(ConvertSkRectToSkityRect(cull_rect),
                            skity::Matrix());
  SkPath clip = SkPath().addCircle(10.2, 11.3, 2).addCircle(20.4, 25.7, 2);
  clip.setFillType(SkPathFillType::kInverseWinding);
  builder.clipPath(clip, SkClipOp::kIntersect, false);

  ASSERT_EQ(builder.local_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
  ASSERT_EQ(builder.device_cull_rect(), ConvertSkRectToSkityRect(cull_rect));
}

TEST(DisplayListMatrixClipTracker, DiffClipPathWithInvertFillType) {
  SkRect cull_rect = SkRect::MakeLTRB(0, 0, 100.0, 100.0);
  MatrixClipTracker tracker(ConvertSkRectToSkityRect(cull_rect),
                            skity::Matrix());

  SkPath clip = SkPath().addCircle(10.2, 11.3, 2).addCircle(20.4, 25.7, 2);
  clip.setFillType(SkPathFillType::kInverseWinding);
  SkRect clip_bounds = SkRect::MakeLTRB(8.2, 9.3, 22.4, 27.7);
  tracker.clipPath(clip, SkClipOp::kDifference, false);

  ASSERT_EQ(tracker.local_cull_rect(), ConvertSkRectToSkityRect(clip_bounds));
  ASSERT_EQ(tracker.device_cull_rect(), ConvertSkRectToSkityRect(clip_bounds));
}

}  // namespace testing
}  // namespace clay
