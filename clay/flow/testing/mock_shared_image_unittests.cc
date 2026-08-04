// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_shared_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

TEST(MockSharedImageTests, QuerySize) {
  auto shared_image =
      fml::MakeRefCounted<MockSharedImageBacking>(skity::Vec2(100, 100));

  EXPECT_EQ(shared_image->GetSize(), skity::Vec2(100, 100));
}

TEST(MockSharedImageTests, QueryTransformation) {
  auto shared_image =
      fml::MakeRefCounted<MockSharedImageBacking>(skity::Vec2(100, 100));

  EXPECT_EQ(shared_image->GetTransformation(), skity::Matrix());

  skity::Matrix y_flip_mat = skity::Matrix(1, 0, 0, 0, -1, 1, 0, 0, 1);
  shared_image->SetTransformation(y_flip_mat);
  EXPECT_EQ(shared_image->GetTransformation(), y_flip_mat);
}

namespace {

fml::RefPtr<SharedImageSinkManaged> CreateSink(
    SharedImageSink::BufferMode buffer_mode) {
  auto sink = fml::MakeRefCounted<SharedImageSinkManaged>(
      buffer_mode, [](skity::Vec2 size,
                      std::optional<SharedImageSink::GraphicsMemoryHandle>) {
        return fml::MakeRefCounted<MockSharedImageBacking>(size);
      });
  sink->SetFrameAvailableCallback([] {});
  return sink;
}

void ProduceFrame(const fml::RefPtr<SharedImageSinkManaged>& sink,
                  const std::optional<skity::Rect>& frame_damage,
                  bool force_acquire = false) {
  fml::RefPtr<SharedImageBacking> backing;
  if (force_acquire) {
    std::tie(backing, std::ignore) =
        sink->AcquireBackForced(skity::Vec2(100, 100));
  } else {
    std::tie(backing, std::ignore) = sink->AcquireBack(skity::Vec2(100, 100));
  }
  ASSERT_TRUE(backing);
  ASSERT_TRUE(sink->SwapBack(nullptr, frame_damage));
}

void ExpectFrameDamage(const fml::RefPtr<SharedImageBacking>& backing,
                       const skity::Rect& expected_damage) {
  ASSERT_TRUE(backing);
  const auto frame_damage = backing->GetFrameDamage();
  ASSERT_TRUE(frame_damage.has_value());
  EXPECT_EQ(*frame_damage, expected_damage);
}

}  // namespace

TEST(SharedImageSinkDamageTests, PreservesDamageForSequentialFrames) {
  auto sink = CreateSink(SharedImageSink::BufferMode::kDoubleBuffer);
  const auto first_damage = skity::Rect::MakeLTRB(0, 0, 20, 20);
  const auto second_damage = skity::Rect::MakeLTRB(40, 40, 60, 60);

  ProduceFrame(sink, first_damage);
  ExpectFrameDamage(sink->UpdateFront(nullptr), first_damage);

  ProduceFrame(sink, second_damage);
  ExpectFrameDamage(sink->UpdateFront(nullptr), second_damage);
}

TEST(SharedImageSinkDamageTests, AccumulatesDamageWhenUpdatingToLatest) {
  auto sink = CreateSink(SharedImageSink::BufferMode::kTripleBuffer);

  ProduceFrame(sink, skity::Rect::MakeWH(100, 100));
  ASSERT_TRUE(sink->UpdateFront(nullptr));

  ProduceFrame(sink, skity::Rect::MakeLTRB(0, 0, 20, 20));
  ProduceFrame(sink, skity::Rect::MakeLTRB(40, 40, 60, 60));
  ExpectFrameDamage(sink->UpdateFrontToLatest(nullptr),
                    skity::Rect::MakeLTRB(0, 0, 60, 60));
}

TEST(SharedImageSinkDamageTests, PreservesDamageWhenPendingBufferIsReused) {
  auto sink = CreateSink(SharedImageSink::BufferMode::kDoubleBuffer);

  ProduceFrame(sink, skity::Rect::MakeWH(100, 100));
  ASSERT_TRUE(sink->UpdateFront(nullptr));

  ProduceFrame(sink, skity::Rect::MakeLTRB(0, 0, 20, 20));
  ProduceFrame(sink, skity::Rect::MakeLTRB(40, 40, 60, 60), true);
  ExpectFrameDamage(sink->UpdateFrontToLatest(nullptr),
                    skity::Rect::MakeLTRB(0, 0, 60, 60));
}

TEST(SharedImageSinkDamageTests, UnknownSkippedDamageForcesFullRedraw) {
  auto sink = CreateSink(SharedImageSink::BufferMode::kTripleBuffer);

  ProduceFrame(sink, skity::Rect::MakeWH(100, 100));
  ASSERT_TRUE(sink->UpdateFront(nullptr));

  ProduceFrame(sink, skity::Rect::MakeLTRB(0, 0, 20, 20));
  ProduceFrame(sink, std::nullopt);
  auto latest = sink->UpdateFrontToLatest(nullptr);
  ASSERT_TRUE(latest);
  EXPECT_FALSE(latest->GetFrameDamage().has_value());
}

}  // namespace testing
}  // namespace clay
