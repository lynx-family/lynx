// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/public/path.h"

#include <limits>
#include <thread>
#include <utility>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::gfx {

TEST(PathTest, KeepsPrimitiveGeometryAndUniqueIdentity) {
  auto rect = Path::MakeRect(20, 30);
  auto other = Path::MakeRect(20, 30);
  ASSERT_TRUE(rect);
  ASSERT_TRUE(other);
  EXPECT_NE(rect->GetKey(), other->GetKey());
  EXPECT_EQ(rect->GetHeader().kind, PathKind::kRect);
  EXPECT_TRUE(rect->GetCommands().empty());
  const float radii[] = {20, 10, 20, 10, 20, 10, 20, 10};
  auto round = Path::MakeRoundedRect(20, 30, radii);
  ASSERT_TRUE(round);
  EXPECT_EQ(round->GetHeader().kind, PathKind::kRoundedRect);
  EXPECT_FLOAT_EQ(round->GetHeader().radii[0], 10);
  EXPECT_FLOAT_EQ(round->GetHeader().radii[1], 5);
}

TEST(PathTest, PublishedGeometrySurvivesBuilderReuseAndThreadHandoff) {
  PathBuilder builder;
  builder.Begin(1, 2);
  builder.QuadraticTo(-5, 40, 10, 15);
  builder.CubicTo(20, -10, 25, 50, 30, 20);
  builder.End(true);
  auto path = builder.Build();
  ASSERT_TRUE(path);
  const uint64_t key = path->GetKey();
  builder.Begin(100, 200);
  builder.LineTo(300, 400);
  ASSERT_TRUE(builder.Build());
  std::thread consumer([retained = std::move(path), key] {
    EXPECT_EQ(retained->GetKey(), key);
    EXPECT_EQ(retained->GetCommands().size(), 4u);
    EXPECT_EQ(retained->GetPoints().size(), 6u);
    EXPECT_FLOAT_EQ(retained->GetHeader().x, -5);
    EXPECT_FLOAT_EQ(retained->GetHeader().y, -10);
    EXPECT_FLOAT_EQ(retained->GetHeader().width, 35);
    EXPECT_FLOAT_EQ(retained->GetHeader().height, 60);
  });
  consumer.join();
  EXPECT_FALSE(path);
}

TEST(PathTest, ValidatesCommandsAndFiniteGeometry) {
  const auto nan = std::numeric_limits<float>::quiet_NaN();
  EXPECT_FALSE(Path::MakeRect(nan, 10));
  EXPECT_FALSE(Path::MakeRect(-1, 10));
  EXPECT_TRUE(Path::MakeRect(0, 10)->IsEmpty());
  EXPECT_FALSE(Path::MakeCommands({PathVerb::kLine}, {{0, 0}}));
  EXPECT_FALSE(Path::MakeCommands({PathVerb::kBegin}, {{0, 0}}));
  EXPECT_FALSE(
      Path::MakeCommands({PathVerb::kBegin, PathVerb::kEnd}, {{nan, 0}}));
  EXPECT_FALSE(
      Path::MakeCommands({PathVerb::kBegin, PathVerb::kEnd}, {{0, 0}, {1, 1}}));
  EXPECT_FALSE(Path::MakeCommands({static_cast<PathVerb>(99)}, {}));
  PathBuilder builder;
  builder.LineTo(1, 2);
  EXPECT_FALSE(builder.Build());
  builder.Begin(1, 2);
  builder.LineTo(3, 4);
  auto recovered = builder.Build();
  ASSERT_TRUE(recovered);
  EXPECT_EQ(recovered->GetCommands().back(), PathVerb::kEnd);
  auto empty = builder.Build();
  ASSERT_TRUE(empty);
  EXPECT_TRUE(empty->IsEmpty());
}

}  // namespace lynx::gfx
