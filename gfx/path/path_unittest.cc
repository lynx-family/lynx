// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <cmath>
#include <limits>
#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"
namespace lynx::gfx {
TEST(PathTest, ImmutableSourceAndIdentity) {
  auto a = Path::MakeRect(10, 20);
  auto b = Path::MakeRect(10, 20);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  EXPECT_NE(a->GetKey(), 0u);
  EXPECT_NE(a->GetKey(), b->GetKey());
  EXPECT_EQ(a->GetKey(), a->GetImpl().GetKey());
  auto view = std::get<PathView>(a->GetSourceView());
  EXPECT_EQ(view.header.width, 10);
  EXPECT_EQ(view.header.height, 20);
  PathImpl impl(a->GetImpl().GetSource());
  EXPECT_NE(impl.GetKey(), a->GetKey());
  EXPECT_EQ(std::get<PathView>(impl.GetSourceView()).header.width, 10);
}
TEST(PathTest, MovedSourceRemainsAValidEmptyValue) {
  auto path = Path::MakeSvgPathData("M0 0L1 1", 1);
  auto source = path->GetImpl().GetSource();
  PathImpl impl(std::move(source));
  EXPECT_EQ(std::get<SvgPathView>(impl.GetSourceView()).data, "M0 0L1 1");
  PathImpl moved_from(std::move(source));
  EXPECT_EQ(std::get<PathView>(moved_from.GetSourceView()).header.width, 0);
  EXPECT_EQ(std::get<PathView>(moved_from.GetSourceView()).header.command_count,
            0u);
}

TEST(PathTest, SvgPreservesBytesAndScale) {
  std::string source = "m1e2 3 A10 20 45 01-2.5 .3z\n";
  auto path = Path::MakeSvgPathData(source, 2.f);
  ASSERT_TRUE(path);
  auto a = std::get<SvgPathView>(path->GetSourceView());
  auto b = std::get<SvgPathView>(path->GetSourceView());
  EXPECT_EQ(a.data, source);
  EXPECT_EQ(&a.data, &b.data);
  EXPECT_EQ(a.source_to_layout_scale, 2);
  EXPECT_FALSE(Path::MakeSvgPathData(source, 0));
  EXPECT_FALSE(
      Path::MakeSvgPathData(source, std::numeric_limits<float>::quiet_NaN()));
  EXPECT_FALSE(Path::MakeSvgPathData(std::string("M0 0\0L1 1", 10), 1));
  EXPECT_FALSE(
      Path::MakeSvgPathData(std::string(Path::kMaxSvgBytes + 1, ' '), 1));
  EXPECT_TRUE(Path::MakeSvgPathData("", 1));
}
TEST(PathTest, SvgSharesComputedStringAcrossScaleAndSourceCopies) {
  base::String data("M0 0 L1 2 C3 4 5 6 7 8 Z");
  auto first = Path::MakeSvgPathData(data, 1);
  auto second = Path::MakeSvgPathData(data, 2);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  PathImpl copy(first->GetImpl().GetSource());
  const auto& a = std::get<SvgPathView>(first->GetSourceView()).data;
  const auto& b = std::get<SvgPathView>(second->GetSourceView()).data;
  const auto& c = std::get<SvgPathView>(copy.GetSourceView()).data;
  EXPECT_EQ(&a, &data.str());
  EXPECT_EQ(&b, &data.str());
  EXPECT_EQ(&c, &data.str());
  data = base::String();
  first = nullptr;
  second = nullptr;
  EXPECT_EQ(c, "M0 0 L1 2 C3 4 5 6 7 8 Z");
}
}  // namespace lynx::gfx
