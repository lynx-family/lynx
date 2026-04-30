// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

PointerEvent CreateTouchDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kTouch;
  event.position = {x, y};
  return event;
}

PointerEvent CreateMouseDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.position = {x, y};
  return event;
}

}  // namespace

TEST(NativeViewTest, TextureRegistry) {
  int view_id = 0;
  fml::Thread thread("mock_ui_platform");
  thread.GetTaskRunner()->PostSyncTask([&] {
    std::unique_ptr<PageView> page_view =
        std::make_unique<PageView>(view_id++, nullptr, thread.GetTaskRunner());
    std::unique_ptr<NativeView> root = std::make_unique<NativeView>(
        view_id++, "test-platform", page_view.get());
    EXPECT_EQ(root->IsNativeViewAvailable(), false);
  });
}

class NativeViewHitTestTest : public UITest {};

TEST_F_UI(NativeViewHitTestTest, IgnoreUnhandledTouchSequenceInHitTest) {
  auto* background_view = new View(1, page_.get());
  auto* native_view = new NativeView(2, "test-platform", page_.get());
  page_->AddChild(background_view);
  page_->AddChild(native_view);

  page_->SetWidth(100.f);
  page_->SetHeight(100.f);

  background_view->SetWidth(100.f);
  background_view->SetHeight(100.f);

  native_view->SetWidth(100.f);
  native_view->SetHeight(100.f);

  background_view->OnLayoutUpdated();
  native_view->OnLayoutUpdated();

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              native_view->id());
    EXPECT_EQ(page_->GetHitTestingTargetNativeViewId(FloatPoint(50.f, 50.f),
                                                     native_view->id()),
              native_view->id());
    EXPECT_NE(page_->GetHitTestingTargetNativeViewId(FloatPoint(50.f, 50.f),
                                                     background_view->id()),
              background_view->id());
  }

  native_view->UpdateTouchDispatchState(false, /* action= */ 0);

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              background_view->id());
  }

  {
    FloatPoint relative_position;
    EXPECT_EQ(page_->GetTopViewToAcceptEvent(FloatPoint(50.f, 50.f),
                                             &relative_position),
              background_view);
    EXPECT_EQ(page_->GetHitTestingTargetNativeViewId(FloatPoint(50.f, 50.f),
                                                     native_view->id()),
              native_view->id());
    EXPECT_NE(page_->GetHitTestingTargetNativeViewId(FloatPoint(50.f, 50.f),
                                                     background_view->id()),
              background_view->id());
  }

  native_view->UpdateTouchDispatchState(true, /* action= */ 1);

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              native_view->id());
    EXPECT_EQ(page_->GetHitTestingTargetNativeViewId(FloatPoint(50.f, 50.f),
                                                     native_view->id()),
              native_view->id());
  }
}

class NativeViewFocusTest : public UITest {
 protected:
  void SetUpEditableScene() {
    background_view_ = new View(1, page_.get());
    native_view_ = new NativeView(2, "test-platform", page_.get());

    page_->AddChild(background_view_);
    page_->AddChild(native_view_);

    page_->SetWidth(100.f);
    page_->SetHeight(100.f);

    background_view_->SetWidth(100.f);
    background_view_->SetHeight(100.f);

    native_view_->SetWidth(40.f);
    native_view_->SetHeight(100.f);

    background_view_->OnLayoutUpdated();
    native_view_->OnLayoutUpdated();
  }

  View* background_view_ = nullptr;
  NativeView* native_view_ = nullptr;
};

TEST_F_UI(NativeViewFocusTest, SelfTapKeepsEditingFocus) {
  SetUpEditableScene();

  native_view_->MarkAsEditing();
  ASSERT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);

  DispatchTapEvent(FloatPoint(20.f, 50.f));

  EXPECT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);
}

TEST_F_UI(NativeViewFocusTest, IgnoreFocusTargetKeepsEditingFocus) {
  SetUpEditableScene();

  auto* ignore_focus_parent = new View(3, page_.get());
  auto* ignore_focus_child = new View(4, page_.get());
  page_->AddChild(ignore_focus_parent);
  ignore_focus_parent->AddChild(ignore_focus_child);

  ignore_focus_parent->SetX(40.f);
  ignore_focus_parent->SetWidth(60.f);
  ignore_focus_parent->SetHeight(100.f);
  ignore_focus_parent->SetAttribute("ignore-focus", Value(true));

  ignore_focus_child->SetWidth(60.f);
  ignore_focus_child->SetHeight(100.f);

  ignore_focus_parent->OnLayoutUpdated();
  ignore_focus_child->OnLayoutUpdated();

  native_view_->MarkAsEditing();
  ASSERT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);

  DispatchTapEvent(FloatPoint(70.f, 50.f));

  EXPECT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);
}

TEST_F_UI(NativeViewFocusTest, ExplicitFalseOverrideClearsEditingFocus) {
  SetUpEditableScene();

  auto* ignore_focus_parent = new View(3, page_.get());
  auto* override_child = new View(4, page_.get());
  page_->AddChild(ignore_focus_parent);
  ignore_focus_parent->AddChild(override_child);

  ignore_focus_parent->SetX(40.f);
  ignore_focus_parent->SetWidth(60.f);
  ignore_focus_parent->SetHeight(100.f);
  ignore_focus_parent->SetAttribute("ignore-focus", Value(true));

  override_child->SetWidth(60.f);
  override_child->SetHeight(100.f);
  override_child->SetAttribute("ignore-focus", Value(false));

  ignore_focus_parent->OnLayoutUpdated();
  override_child->OnLayoutUpdated();

  native_view_->MarkAsEditing();
  ASSERT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);

  DispatchTapEvent(FloatPoint(70.f, 50.f));

  EXPECT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), nullptr);
}

TEST_F_UI(NativeViewFocusTest, NormalTouchTargetClearsEditingFocus) {
  SetUpEditableScene();

  native_view_->MarkAsEditing();
  ASSERT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);

  DispatchTapEvent(FloatPoint(70.f, 50.f));

  EXPECT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), nullptr);
}

TEST_F_UI(NativeViewFocusTest, NormalMouseTargetClearsEditingFocus) {
  SetUpEditableScene();

  native_view_->MarkAsEditing();
  ASSERT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), native_view_);

  page_->DispatchPointerEvent({CreateMouseDownPointer(70.f, 50.f)});

  EXPECT_EQ(page_->GetFocusManager()->GetLeafFocusedNode(), nullptr);
}

}  // namespace clay
