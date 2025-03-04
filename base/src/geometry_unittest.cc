// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/geometry/point.h"
#include "base/include/geometry/rect.h"
#include "base/include/geometry/size.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace geometry {
namespace {

TEST(GeometryTest, PointInitAndAccess) {
  FloatPoint float_point_0;
  EXPECT_EQ(0.f, float_point_0.X());
  EXPECT_EQ(0.f, float_point_0.Y());

  FloatPoint float_point_1(3.4, 2.8);
  EXPECT_EQ(3.4f, float_point_1.X());
  EXPECT_EQ(2.8f, float_point_1.Y());

  float_point_0.SetX(3.4);
  float_point_0.SetY(2.8);
  EXPECT_EQ(3.4f, float_point_0.X());
  EXPECT_EQ(2.8f, float_point_0.Y());
}

TEST(GeometryTest, PointMove) {
  FloatPoint float_point_0(3.4, 2.8);
  FloatPoint float_point_1(-1.2, 0.4);

  float_point_0.Move(0.8, -1.3);
  EXPECT_FLOAT_EQ(4.2, float_point_0.X());
  EXPECT_FLOAT_EQ(1.5, float_point_0.Y());

  float_point_0.MoveBy(float_point_1);
  EXPECT_FLOAT_EQ(3.0, float_point_0.X());
  EXPECT_FLOAT_EQ(1.9, float_point_0.Y());
}

TEST(GeometryTest, PointOperation) {
  FloatPoint float_point_0(3.4f, 2.8f);
  FloatPoint float_point_1(3.4f, 2.8f);
  FloatPoint float_point_2(3.4f, 2.f);
  FloatPoint float_point_3(1.f, 2.8f);
  FloatPoint float_point_4(1.f, -2.f);

  EXPECT_TRUE(float_point_0 == float_point_1);
  EXPECT_FALSE(float_point_0 == float_point_2);
  EXPECT_FALSE(float_point_0 == float_point_3);
  EXPECT_FALSE(float_point_0 == float_point_4);

  EXPECT_FALSE(float_point_0 != float_point_1);
  EXPECT_TRUE(float_point_0 != float_point_2);
  EXPECT_TRUE(float_point_0 != float_point_3);
  EXPECT_TRUE(float_point_0 != float_point_4);

  FloatPoint float_point_5 = float_point_0 + float_point_4;
  EXPECT_FLOAT_EQ(4.4f, float_point_5.X());
  EXPECT_FLOAT_EQ(0.8f, float_point_5.Y());
  float_point_0 += float_point_4;
  EXPECT_EQ(float_point_5, float_point_0);
  EXPECT_EQ(float_point_1, float_point_5 - float_point_4);
}

TEST(GeometryTest, SizeInitAndAccess) {
  FloatSize float_size_0;
  EXPECT_EQ(0.f, float_size_0.Width());
  EXPECT_EQ(0.f, float_size_0.Height());
  EXPECT_TRUE(float_size_0.IsEmpty());

  FloatSize float_size_1(3.4, 2.8);
  EXPECT_EQ(3.4f, float_size_1.Width());
  EXPECT_EQ(2.8f, float_size_1.Height());
  EXPECT_FALSE(float_size_1.IsEmpty());

  float_size_0.SetWidth(3.4);
  float_size_0.SetHeight(2.8);
  EXPECT_EQ(3.4f, float_size_0.Width());
  EXPECT_EQ(2.8f, float_size_0.Height());
  EXPECT_FALSE(float_size_0.IsEmpty());

  float_size_1.SetWidth(0.f);
  float_size_1.SetHeight(0.f);
  EXPECT_TRUE(float_size_1.IsEmpty());
}

TEST(GeometryTest, SizeOperation) {
  FloatSize float_size_0(3.4f, 2.8f);
  FloatSize float_size_1(3.4f, 2.8f);
  FloatSize float_size_2(3.4f, 2.f);
  FloatSize float_size_3(1.f, 2.8f);
  FloatSize float_size_4(1.f, -2.f);

  EXPECT_TRUE(float_size_0 == float_size_1);
  EXPECT_FALSE(float_size_0 == float_size_2);
  EXPECT_FALSE(float_size_0 == float_size_3);
  EXPECT_FALSE(float_size_0 == float_size_4);

  EXPECT_FALSE(float_size_0 != float_size_1);
  EXPECT_TRUE(float_size_0 != float_size_2);
  EXPECT_TRUE(float_size_0 != float_size_3);
  EXPECT_TRUE(float_size_0 != float_size_4);

  FloatSize float_size_5 = float_size_0 + float_size_4;
  EXPECT_FLOAT_EQ(4.4f, float_size_5.Width());
  EXPECT_FLOAT_EQ(0.8f, float_size_5.Height());
  float_size_0 += float_size_4;
  EXPECT_EQ(float_size_5, float_size_0);
  EXPECT_EQ(float_size_1, float_size_5 - float_size_4);
}

TEST(GeometryTest, SizeExpand) {
  FloatSize float_size_0(3.4, 2.8);
  FloatSize float_size_1(3.3, 0.4);
  FloatSize float_size_2(4.6, 0.4);
  FloatSize float_size_3(3.3, 3.8);
  FloatSize float_size_4(4.3, 5.8);

  EXPECT_EQ(float_size_0, float_size_0.ExpandedTo(float_size_1));
  EXPECT_EQ(FloatSize(4.6, 2.8), float_size_0.ExpandedTo(float_size_2));
  EXPECT_EQ(FloatSize(3.4, 3.8), float_size_0.ExpandedTo(float_size_3));
  EXPECT_EQ(FloatSize(4.3, 5.8), float_size_0.ExpandedTo(float_size_4));

  float_size_2.Expand(float_size_3.Width(), float_size_3.Height());
  EXPECT_FLOAT_EQ(7.9f, float_size_2.Width());
  EXPECT_FLOAT_EQ(4.2f, float_size_2.Height());
}

TEST(GeometryTest, RectInitAndAccess) {
  FloatRect float_rect_0;
  FloatRect float_rect_1(FloatPoint(3.3, 0.4), FloatSize(4.3, 2.8));
  EXPECT_TRUE(float_rect_0.IsEmpty());
  EXPECT_FALSE(float_rect_1.IsEmpty());
  EXPECT_EQ(FloatPoint(0, 0), float_rect_0.GetLocation());
  EXPECT_EQ(FloatSize(0, 0), float_rect_0.GetSize());
  EXPECT_EQ(FloatPoint(3.3, 0.4), float_rect_1.GetLocation());
  EXPECT_EQ(FloatSize(4.3, 2.8), float_rect_1.GetSize());
  EXPECT_FLOAT_EQ(7.6, float_rect_1.MaxX());
  EXPECT_FLOAT_EQ(3.2, float_rect_1.MaxY());

  float_rect_0.SetSize(FloatSize(4.3, 2.8));
  float_rect_0.SetLocation(FloatPoint(3.3, 0.4));
  EXPECT_EQ(FloatPoint(3.3, 0.4), float_rect_0.GetLocation());
  EXPECT_EQ(FloatSize(4.3, 2.8), float_rect_0.GetSize());
}

TEST(GeometryTest, RectContains) {
  FloatRect float_rect_0(FloatPoint(3.3, 0.4), FloatSize(4.3, 2.8));
  EXPECT_TRUE(float_rect_0.Contains(4.5, 1.2));
  EXPECT_FALSE(float_rect_0.Contains(1.2, 1.2));
  EXPECT_FALSE(float_rect_0.Contains(10, 1.2));
  EXPECT_FALSE(float_rect_0.Contains(4.5, 4.2));
  EXPECT_FALSE(float_rect_0.Contains(10, 4.2));
  EXPECT_FALSE(float_rect_0.Contains(10, 0));
  EXPECT_FALSE(float_rect_0.Contains(4.5, 0));
}

TEST(GeometryTest, RectIntersectedSize) {
  FloatRect float_rect_0(FloatPoint(3.3, 0.4), FloatSize(4.3, 2.8));
  FloatRect float_rect_1(FloatPoint(3.4, 0.8), FloatSize(4.3, 2.8));
  FloatRect float_rect_2(FloatPoint(1.2, 1.2), FloatSize(4.3, 2.8));
  FloatRect float_rect_3(FloatPoint(1.2, 100.2), FloatSize(4.3, 2.8));
  FloatRect float_rect_4(FloatPoint(-100.f, 1.2), FloatSize(4.3, 2.8));
  EXPECT_TRUE(float_rect_0.IsIntersectedWith(float_rect_1));
  EXPECT_TRUE(float_rect_0.IsIntersectedWith(float_rect_2));
  EXPECT_FALSE(float_rect_0.IsIntersectedWith(float_rect_3));
  EXPECT_FALSE(float_rect_0.IsIntersectedWith(float_rect_4));
}

TEST(GeometryTest, RectIntersect) {
  IntRect int_rect_0(IntPoint(3, 0), IntSize(4, 5));
  IntRect int_rect_1(IntPoint(3, 0), IntSize(4, 5));
  IntRect int_rect_2(IntPoint(1, 4), IntSize(4, 5));
  IntRect int_rect_3(IntPoint(1, 100), IntSize(4, 5));
  IntRect int_rect_4(IntPoint(-100, 1), IntSize(4, 5));
  IntRect int_rect_5(IntPoint(4, 3), IntSize(1, 1));
  IntRect int_rect_6(IntPoint(1, -2), IntSize(4, 5));

  IntRect int_rect = int_rect_0;
  int_rect.Intersect(int_rect_1);

  EXPECT_EQ(3, int_rect.X());
  EXPECT_EQ(0, int_rect.Y());
  EXPECT_EQ(7, int_rect.MaxX());
  EXPECT_EQ(5, int_rect.MaxY());

  int_rect = int_rect_0;
  int_rect.Intersect(int_rect_2);

  EXPECT_EQ(3, int_rect.X());
  EXPECT_EQ(4, int_rect.Y());
  EXPECT_EQ(5, int_rect.MaxX());
  EXPECT_EQ(5, int_rect.MaxY());

  int_rect = int_rect_0;
  int_rect.Intersect(int_rect_3);

  EXPECT_TRUE(int_rect.IsEmpty());

  int_rect = int_rect_0;
  int_rect.Intersect(int_rect_4);

  EXPECT_TRUE(int_rect.IsEmpty());

  int_rect = int_rect_0;
  int_rect.Intersect(int_rect_5);

  EXPECT_EQ(4, int_rect.X());
  EXPECT_EQ(3, int_rect.Y());
  EXPECT_EQ(5, int_rect.MaxX());
  EXPECT_EQ(4, int_rect.MaxY());

  int_rect = int_rect_0;
  int_rect.Intersect(int_rect_6);

  EXPECT_EQ(3, int_rect.X());
  EXPECT_EQ(0, int_rect.Y());
  EXPECT_EQ(5, int_rect.MaxX());
  EXPECT_EQ(3, int_rect.MaxY());
}

}  // namespace
}  // namespace geometry
}  // namespace base
}  // namespace lynx
