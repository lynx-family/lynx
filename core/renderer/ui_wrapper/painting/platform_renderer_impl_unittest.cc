// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

#include "core/renderer/dom/fragment/display_list_builder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {

class TestPlatformRenderer : public PlatformRendererImpl {
 public:
  explicit TestPlatformRenderer(int id)
      : PlatformRendererImpl(id, PlatformRendererType::kView, base::String()) {}

  bool HasParent() const { return GetParent() != nullptr; }

  int remove_from_parent_count() const { return remove_from_parent_count_; }

 protected:
  void OnUpdateDisplayList(DisplayList) override {}
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&) override {}
  void OnAddChild(PlatformRenderer*, int, bool) override {}
  void OnRemoveFromParent(bool) override { ++remove_from_parent_count_; }
  void OnUpdateSubtreeProperties(const DisplayList&) override {}

 private:
  int remove_from_parent_count_ = 0;
};

}  // namespace

TEST(PlatformRendererImplTest, ClearsChildParentWhenParentIsReleased) {
  auto parent = fml::MakeRefCounted<TestPlatformRenderer>(1);
  auto child = fml::MakeRefCounted<TestPlatformRenderer>(2);

  parent->AddChild(child);
  ASSERT_TRUE(child->HasParent());

  parent = nullptr;

  ASSERT_FALSE(child->HasParent());
  child->RemoveFromParent();
  EXPECT_EQ(child->remove_from_parent_count(), 0);
}

TEST(PlatformRendererImplTest, ClipPathPropertyOwnsOnlyTheCurrentResource) {
  auto renderer = fml::MakeRefCounted<TestPlatformRenderer>(1);
  auto first = gfx::Path::MakeRect(10, 20);
  auto second = gfx::Path::MakeRect(30, 40);
  auto update = [&](fml::RefPtr<gfx::Path> path) {
    DisplayListBuilder builder;
    builder.ClipPathProperty(std::move(path), gfx::PathFillRule::kEvenOdd, 3,
                             4);
    renderer->UpdateDisplayList(builder.Build());
  };
  update(first);
  EXPECT_EQ(renderer->GetClipPath(), first.get());
  EXPECT_FALSE(first->HasOneRef());
  EXPECT_EQ(renderer->GetClipPathFillRule(), gfx::PathFillRule::kEvenOdd);
  EXPECT_EQ(renderer->GetClipPathOffset()[0], 3);
  EXPECT_EQ(renderer->GetClipPathOffset()[1], 4);
  update(second);
  EXPECT_TRUE(first->HasOneRef());
  EXPECT_FALSE(second->HasOneRef());
  EXPECT_EQ(renderer->GetClipPath(), second.get());
  renderer->UpdateDisplayList(DisplayList());
  EXPECT_EQ(renderer->GetClipPath(), second.get());
  update(nullptr);
  EXPECT_EQ(renderer->GetClipPath(), nullptr);
  EXPECT_TRUE(second->HasOneRef());
  update(first);
  renderer = nullptr;
  EXPECT_TRUE(first->HasOneRef());
}

}  // namespace lynx::tasm
