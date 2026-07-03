// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

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
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&, bool) override {}
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

}  // namespace lynx::tasm
