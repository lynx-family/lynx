// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"

#include <functional>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::tasm {
namespace {

class TestPlatformRenderer : public PlatformRendererImpl {
 public:
  explicit TestPlatformRenderer(int id)
      : PlatformRendererImpl(id, PlatformRendererType::kView, base::String()) {}

  std::function<void()> on_remove;

  bool HasParent() const { return GetParent() != nullptr; }

  int remove_from_parent_count() const { return remove_from_parent_count_; }
  int host_operation_count() const { return host_operation_count_; }
  const float* layout_frame() const { return layout_frame_; }

 protected:
  void OnUpdateDisplayList(DisplayList) override { ++host_operation_count_; }
  void OnUpdateAttributes(const fml::RefPtr<PropBundle>&) override {
    ++host_operation_count_;
  }
  void OnAddChild(PlatformRenderer*, int, bool) override {
    ++host_operation_count_;
  }
  void OnRemoveFromParent(bool) override {
    ++host_operation_count_;
    ++remove_from_parent_count_;
    if (on_remove) {
      on_remove();
    }
  }
  void OnUpdateSubtreeProperties(const DisplayList&) override {
    ++host_operation_count_;
  }

 private:
  int remove_from_parent_count_ = 0;
  int host_operation_count_ = 0;
};

}  // namespace

TEST(PlatformRendererImplTest, LayoutMetricsAreCachedWithoutHostOperations) {
  auto renderer = fml::MakeRefCounted<TestPlatformRenderer>(1);
  float metrics[4] = {1.f, 2.f, 3.f, 4.f};
  EXPECT_FALSE(renderer->HasLayoutMetrics());

  renderer->UpdateLayoutMetrics(10.f, 20.f, 30.f, 40.f, metrics, metrics,
                                metrics);
  metrics[0] = 99.f;
  EXPECT_TRUE(renderer->HasLayoutMetrics());
  EXPECT_FLOAT_EQ(renderer->layout_frame()[0], 10.f);
  EXPECT_FLOAT_EQ(renderer->layout_frame()[3], 40.f);
  for (int i = 0; i < 4; ++i) {
    EXPECT_FLOAT_EQ(renderer->GetLayoutPaddings()[i], i + 1.f);
    EXPECT_FLOAT_EQ(renderer->GetLayoutMargins()[i], i + 1.f);
    EXPECT_FLOAT_EQ(renderer->GetLayoutBorders()[i], i + 1.f);
  }

  renderer->UpdateLayoutMetrics(50.f, 60.f, 70.f, 80.f, nullptr, nullptr,
                                nullptr);
  EXPECT_FLOAT_EQ(renderer->layout_frame()[0], 50.f);
  EXPECT_FLOAT_EQ(renderer->layout_frame()[3], 80.f);
  for (int i = 0; i < 4; ++i) {
    EXPECT_FLOAT_EQ(renderer->GetLayoutPaddings()[i], 0.f);
    EXPECT_FLOAT_EQ(renderer->GetLayoutMargins()[i], 0.f);
    EXPECT_FLOAT_EQ(renderer->GetLayoutBorders()[i], 0.f);
  }
  EXPECT_EQ(renderer->host_operation_count(), 0);

  renderer->UpdateDisplayList(DisplayList());
  EXPECT_GT(renderer->host_operation_count(), 0);
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

TEST(PlatformRendererImplTest, RetainsParentAcrossReentrantRemoval) {
  auto parent = fml::MakeRefCounted<TestPlatformRenderer>(1);
  auto child = fml::MakeRefCounted<TestPlatformRenderer>(2);
  parent->AddChild(child);
  child->on_remove = [&] {
    parent = nullptr;
    // The native parent relationship is still needed after the host callback.
    EXPECT_TRUE(child->HasParent());
  };

  child->RemoveFromParent();

  EXPECT_FALSE(child->HasParent());
  EXPECT_EQ(child->remove_from_parent_count(), 1);
}

}  // namespace lynx::tasm
