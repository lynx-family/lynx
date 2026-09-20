// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {

class TestPlatformRenderer : public PlatformRendererImpl {
 public:
  explicit TestPlatformRenderer(int id, bool compatible = false)
      : PlatformRendererImpl(
            id,
            compatible ? PlatformRendererType::kUnknown
                       : PlatformRendererType::kView,
            compatible ? base::String("custom-view") : base::String()) {}

  void SetElementParent(int id) { SetFragmentParentId(id); }
  bool removed_through_ui_owner() const { return removed_through_ui_owner_; }

  bool HasParent() const { return GetParent() != nullptr; }

  int remove_from_parent_count() const { return remove_from_parent_count_; }
  int removal_notifications() const { return removal_notifications_; }
  bool detached_when_notified() const { return detached_when_notified_; }

 protected:
  void OnUpdateDisplayList(DisplayList) override {}
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&) override {}
  void OnAddChild(PlatformRenderer*, int, bool) override {}
  void OnRemoveFromParent(bool update_ui_owner) override {
    ++remove_from_parent_count_;
    removed_through_ui_owner_ = update_ui_owner;
  }
  void OnRemovedFromParent() override {
    ++removal_notifications_;
    detached_when_notified_ = !HasParent();
  }
  void OnUpdateSubtreeProperties(const DisplayList&) override {}

 private:
  int remove_from_parent_count_ = 0;
  int removal_notifications_ = 0;
  bool detached_when_notified_ = false;
  bool removed_through_ui_owner_ = false;
};

}  // namespace

TEST(PlatformRendererImplTest, RemovalNotifiesForBothNativeAndCompatibleHosts) {
  for (bool compatible : {false, true}) {
    auto parent = fml::MakeRefCounted<TestPlatformRenderer>(1, compatible);
    auto child = fml::MakeRefCounted<TestPlatformRenderer>(2, compatible);
    child->SetElementParent(1);
    parent->AddChild(child);
    child->RemoveFromParent();
    EXPECT_EQ(child->removed_through_ui_owner(), compatible);
    EXPECT_EQ(child->removal_notifications(), 1);
    EXPECT_TRUE(child->detached_when_notified());
    child->RemoveFromParent();
    EXPECT_EQ(child->removal_notifications(), 1);
  }
}

TEST(PlatformRendererImplTest,
     MovingToAnotherParentNotifiesButLeavesChildAttached) {
  auto parent = fml::MakeRefCounted<TestPlatformRenderer>(1);
  auto next_parent = fml::MakeRefCounted<TestPlatformRenderer>(2);
  auto child = fml::MakeRefCounted<TestPlatformRenderer>(3);
  parent->AddChild(child);
  next_parent->AddChild(child);
  EXPECT_EQ(child->removal_notifications(), 1);
  EXPECT_TRUE(child->detached_when_notified());
  EXPECT_TRUE(child->HasParent());
  EXPECT_TRUE(parent->Children().empty());
  EXPECT_EQ(next_parent->Children().size(), 1u);
}

TEST(PlatformRendererImplTest, MemoryEstimateDoesNotIncludeChildObjects) {
  auto parent = fml::MakeRefCounted<TestPlatformRenderer>(1);
  auto child = fml::MakeRefCounted<TestPlatformRenderer>(2);
  const auto before = parent->GetMemoryUsageBytes();
  parent->AddChild(child);
  EXPECT_EQ(parent->GetMemoryUsageBytes(), before);
}

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
