// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "clay/ui/component/image_view.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator.h"
#include "clay/ui/component/scroll_coordinator/scroll_coordinator_slot_drag.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view_registry.h"
#include "clay/ui/component/xelement_tag_mapping.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

TEST(ScrollCoordinatorRegistryTest, RegistersCanonicalAndMapsLegacyTags) {
  auto* registry = ViewRegistry::GetInstance();
  for (const char* tag :
       {"scroll-coordinator", "scroll-coordinator-header",
        "scroll-coordinator-toolbar", "scroll-coordinator-slot",
        "scroll-coordinator-slot-drag"}) {
    EXPECT_TRUE(registry->HasView(tag)) << tag;
  }
  const std::pair<const char*, const char*> legacy_mappings[] = {
      {"x-foldview-ng", "scroll-coordinator"},
      {"x-foldview-header-ng", "scroll-coordinator-header"},
      {"x-foldview-toolbar-ng", "scroll-coordinator-toolbar"},
      {"x-foldview-slot-ng", "scroll-coordinator-slot"},
      {"x-foldview-slot-drag-ng", "scroll-coordinator-slot-drag"},
  };
  for (const auto& [legacy_tag, canonical_tag] : legacy_mappings) {
    EXPECT_FALSE(registry->HasView(legacy_tag)) << legacy_tag;
    EXPECT_EQ(ResolveXElementTag(legacy_tag), canonical_tag);
  }
}

class ScrollCoordinatorTest : public UITest {
 protected:
  void UISetUp() override {
    header_ = std::make_unique<ScrollCoordinatorHeader>(-1, page_.get());
    tool_bar_ = std::make_unique<ScrollCoordinatorToolbar>(-1, page_.get());
    scroll_coordinator_ = std::make_unique<ScrollCoordinator>(-1, page_.get());
    scroll_view_ = std::make_unique<ScrollView>(-1, ScrollDirection::kVertical,
                                                page_.get());
    content_view_ = std::make_unique<ImageView>(-1, page_.get());

    auto slot = new ScrollCoordinatorSlot(-1, page_.get());
    page_->AddChild(scroll_coordinator_.get());
    scroll_coordinator_->GetScrollView()->BaseView::AddChild(header_.get());
    slot->BaseView::AddChild(scroll_view_.get());
    scroll_coordinator_->GetScrollView()->BaseView::AddChild(slot);
    scroll_coordinator_->BaseView::AddChild(tool_bar_.get());
    scroll_view_->BaseView::AddChild(content_view_.get());

    /*
        +------------------------------+  --- --- ---
        |           toolbar            |  50   |   |
        +- - - - - - - - - - - - - - - +  ---  |   |
        |            header            |      200  |
        |                              |       |  500
        +------------------------------+      ---  |
        |                              |       |   |
        |                              |       |   |
        |          scroll_view         |      450  |
        +------------------------------+       |  ---
        |                              |       |
                                               |
        |                              |       |
        +- - - - - - - - - - - - - - - +      ---
    */
    // Set non-zero top for these views to verify the layout
    header_->SetBound(0, 50, 300, 200);
    tool_bar_->SetBound(0, 50, 300, 50);
    scroll_view_->SetBound(0, 0, 300, 450);
    slot->SetBound(0, 0, 300, 450);
    content_view_->SetBound(0, 0, 300, 1000);
    scroll_coordinator_->SetBound(0, 0, 300, 500);

    scroll_view_->SetEnableNestedScroll(true);

    Layout();
  }

  void UITearDown() override {
    header_.reset();
    tool_bar_.reset();
    scroll_coordinator_.reset();
    scroll_view_.reset();
    content_view_.reset();
  }

  std::unique_ptr<ScrollCoordinatorHeader> header_;
  std::unique_ptr<ScrollCoordinatorToolbar> tool_bar_;
  std::unique_ptr<ScrollCoordinator> scroll_coordinator_;
  std::unique_ptr<ScrollView> scroll_view_;
  std::unique_ptr<ImageView> content_view_;
};

TEST_F_UI(ScrollCoordinatorTest, RegistryCreatesCanonicalTypes) {
  auto* registry = ViewRegistry::GetInstance();
  std::unique_ptr<BaseView> coordinator(
      registry->CreateView(-1, "scroll-coordinator", page_.get()));
  std::unique_ptr<BaseView> header(
      registry->CreateView(-1, "scroll-coordinator-header", page_.get()));
  std::unique_ptr<BaseView> toolbar(
      registry->CreateView(-1, "scroll-coordinator-toolbar", page_.get()));
  std::unique_ptr<BaseView> slot(
      registry->CreateView(-1, "scroll-coordinator-slot", page_.get()));
  std::unique_ptr<BaseView> slot_drag(
      registry->CreateView(-1, "scroll-coordinator-slot-drag", page_.get()));

  ASSERT_NE(coordinator, nullptr);
  ASSERT_NE(header, nullptr);
  ASSERT_NE(toolbar, nullptr);
  ASSERT_NE(slot, nullptr);
  ASSERT_NE(slot_drag, nullptr);
  EXPECT_TRUE(coordinator->Is<ScrollCoordinator>());
  EXPECT_TRUE(header->Is<ScrollCoordinatorHeader>());
  EXPECT_TRUE(toolbar->Is<ScrollCoordinatorToolbar>());
  EXPECT_TRUE(slot->Is<ScrollCoordinatorSlot>());
  EXPECT_TRUE(slot_drag->Is<ScrollCoordinatorSlotDrag>());
}

TEST_F_UI(ScrollCoordinatorTest, Layout) {
  EXPECT_EQ(header_->Top(), 0);
  EXPECT_EQ(tool_bar_->Top(), 0);
  EXPECT_EQ(scroll_view_->AbsoluteLocationWithScroll().y(), 200);
}

TEST_F_UI(ScrollCoordinatorTest, DragOnHeader) {
  DispatchDragEvent({150, 100}, {150, -100});
  // For now, we cannot pass the drag to the scroll view when dragging on the
  // header.
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 150);
}

TEST_F_UI(ScrollCoordinatorTest, DragOnContent) {
  DispatchDragEvent({150, 300}, {150, 160});
  // First the scroll coordinator handles the drag event until it is fully
  // collapsed.
  EXPECT_GT(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 100);
  EXPECT_LT(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 150);
  EXPECT_EQ(scroll_view_->GetScrollOffset().y(), 0);

  // The drag event is passed to the scroll view.
  DispatchDragEvent({150, 300}, {150, 0});
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 150);
  EXPECT_GT(scroll_view_->GetScrollOffset().y(), 200);

  // The fling animation can pass to the scroll coordinator continuously.
  DispatchDragEvent({150, 300}, {150, 450}, true, 10, 1);
  DoAnimation();
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 0);
  EXPECT_EQ(scroll_view_->GetScrollOffset().y(), 0);
}

TEST_F_UI(ScrollCoordinatorTest, Method_setFoldExpanded) {
  scroll_coordinator_->setFoldExpanded(
      CreateLynxModuleValues({"offset", "smooth"}, {Value(50), Value(false)}),
      [](auto, auto) {});
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 50);

  scroll_coordinator_->setFoldExpanded(
      CreateLynxModuleValues({"offset", "smooth"}, {Value(100), Value(true)}),
      [](auto, auto) {});
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 50);
  for (int i = 0; i < 20; i++) {
    DoAnimation(20);
  }
  EXPECT_EQ(scroll_coordinator_->GetScrollView()->GetScrollOffset().y(), 100);
}

}  // namespace clay
