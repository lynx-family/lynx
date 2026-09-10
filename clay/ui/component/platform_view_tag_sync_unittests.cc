// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// Host-side (mac / windows / headless) coverage for Clay platform-view tag
// routing. The host build links native_view_tags_empty.cc, so these assertions
// pin the "no platform-reserved tags, prefer built-in Clay views" behavior for
// those platforms. iOS / Android specific tag membership is compile-time
// selected in native_view_tags.cc / native_view_tags_android.cc and is
// asserted from the platform (iOS ObjC / Android) test targets instead.

#include <utility>

#include "clay/ui/component/image_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/component/view_registry.h"
#include "clay/ui/component/xelement_tag_mapping.h"
#include "clay/ui/platform/native_view_tags.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

// On mac / windows / headless the InternalPlatformViewTags set is empty, so no
// tag is force-routed to a platform (XElement / native) view.
TEST(ClayPlatformViewTagSyncTest, HostHasNoInternalPlatformViewTags) {
  EXPECT_TRUE(InternalPlatformViewTags().empty());
  EXPECT_TRUE(InternalPlatformViewShadowNodeTags().empty());
  EXPECT_TRUE(InternalPlatformViewWithoutShadowNodeTags().empty());

  // Known reserved tags on iOS/Android must NOT be reserved on the host build.
  for (const char* tag : {"x-input-ng", "x-input", "input", "textarea",
                          "x-textarea", "x-map-ng", "x-video-engine"}) {
    EXPECT_EQ(InternalPlatformViewTags().count(tag), 0u)
        << "unexpected host platform-reserved tag: " << tag;
  }
}

// With no platform-reserved tags, the host must not silently fall back to a
// native view for an unknown tag; it prefers the built-in Clay view path.
TEST(ClayPlatformViewTagSyncTest, HostDoesNotForceNativeFallback) {
  EXPECT_FALSE(ShouldCreateFallbackNativeViewDirectly());
}

TEST(ClayPlatformViewTagSyncTest, ResolvesConfiguredElementAliases) {
  const std::pair<const char*, const char*> mappings[] = {
      {"viewpager", "x-viewpager-ng"},
      {"viewpager-item", "x-viewpager-item-ng"},
      {"webview", "x-webview"},
      {"overlay", "x-overlay-ng"},
      {"refresh", "x-refresh-view"},
      {"refresh-header", "x-refresh-header"},
      {"blur-view", "x-blur-view"},
      {"x-foldview-ng", "scroll-coordinator"},
      {"x-foldview-header-ng", "scroll-coordinator-header"},
      {"x-foldview-slot-ng", "scroll-coordinator-slot"},
      {"x-foldview-slot-drag-ng", "scroll-coordinator-slot-drag"},
      {"x-foldview-toolbar-ng", "scroll-coordinator-toolbar"},
      {"input", "x-input-ng"},
      {"textarea", "x-textarea-ng"},
  };
  for (const auto& [standard_tag, xelement_tag] : mappings) {
    EXPECT_EQ(ResolveXElementTag(standard_tag), xelement_tag);
  }
  EXPECT_EQ(ResolveXElementTag("scroll-coordinator"), "scroll-coordinator");
  EXPECT_EQ(ResolveXElementTag("view"), "view");
}

class ClayXElementRegistrySyncTest : public UITest {
 protected:
  void UISetUp() override {
    view_context_ = std::make_unique<ViewContext>(page_.get(), nullptr);
  }

  void UITearDown() override {
    view_context_->ResetPageView();
    view_context_.reset();
  }

  std::unique_ptr<ViewContext> view_context_;
};

TEST_F_UI(ClayXElementRegistrySyncTest, MappingIsScopedToEachViewContext) {
  ASSERT_TRUE(view_context_->CreateView(1, "input"));
  ASSERT_NE(view_context_->GetViewById(1), nullptr);
  EXPECT_EQ(view_context_->GetViewById(1)->GetName(), "input");

  view_context_->SetEnableSyncXElementRegistry(true);
  ASSERT_TRUE(view_context_->CreateView(2, "input"));
  ASSERT_NE(view_context_->GetViewById(2), nullptr);
  EXPECT_EQ(view_context_->GetViewById(2)->GetName(), "input-ng");
}

TEST_F_UI(ClayXElementRegistrySyncTest, KeepsTagWhenXElementIsUnavailable) {
  view_context_->SetEnableSyncXElementRegistry(true);
  ASSERT_TRUE(view_context_->CreateView(1, "webview"));
  ASSERT_NE(view_context_->GetViewById(1), nullptr);
  EXPECT_EQ(view_context_->GetViewById(1)->GetName(), "view");
}

TEST_F_UI(ClayXElementRegistrySyncTest,
          CreatesCanonicalViewsFromLegacyFoldViewTags) {
  const std::pair<const char*, const char*> mappings[] = {
      {"x-foldview-ng", "scroll-coordinator"},
      {"x-foldview-header-ng", "scroll-coordinator-header"},
      {"x-foldview-slot-ng", "scroll-coordinator-slot"},
      {"x-foldview-slot-drag-ng", "scroll-coordinator-slot-drag"},
      {"x-foldview-toolbar-ng", "scroll-coordinator-toolbar"},
  };
  auto* registry = ViewRegistry::GetInstance();
  for (const auto& [legacy_tag, canonical_tag] : mappings) {
    registry->RegisterView(canonical_tag, GetViewCreator<ImageView>());
  }

  view_context_->SetEnableSyncXElementRegistry(true);
  int id = 1;
  for (const auto& [legacy_tag, canonical_tag] : mappings) {
    ASSERT_TRUE(view_context_->CreateView(id, legacy_tag)) << legacy_tag;
    auto* view = view_context_->GetViewById(id);
    ASSERT_NE(view, nullptr) << legacy_tag;
    EXPECT_EQ(view->GetName(), "image") << canonical_tag;
    ++id;
  }
}

}  // namespace
}  // namespace clay
