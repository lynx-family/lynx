// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/lynx_adaptor/painting_context_clay.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/gesture_handler/arena/gesture_arena_manager.h"
#include "clay/ui/gesture_handler/handler/gesture_handler_test_utils.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/testing/ui_test.h"
#include "core/public/ui_operation_queue_interface.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

PointerEvent CreateDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.position = {x, y};
  return event;
}

class BaseViewTest : public UITest {};

TEST(SlideDirectionTest, MapsAngleRangesToGestureDirections) {
  EXPECT_EQ(GetSlideDirectionForAngleRange(0, 180),
            SlideDirection::kHorizontal | SlideDirection::kUp);
  EXPECT_EQ(GetSlideDirectionForAngleRange(10, 20), SlideDirection::kRight);
  EXPECT_EQ(GetSlideDirectionForAngleRange(80, 100), SlideDirection::kUp);
  EXPECT_EQ(GetSlideDirectionForAngleRange(-100, -80), SlideDirection::kDown);
  EXPECT_EQ(GetSlideDirectionForAngleRange(170, 180), SlideDirection::kLeft);
  EXPECT_EQ(GetSlideDirectionForAngleRange(20, 10), SlideDirection::kNone);
}

class CountingInvalidationView final : public BaseView {
 public:
  explicit CountingInvalidationView(PageView* page)
      : BaseView(-1, "counting_view", std::make_unique<RenderContainer>(),
                 page) {}

  void Invalidate() override { ++invalidation_count_; }

  int invalidation_count() const { return invalidation_count_; }

 private:
  int invalidation_count_ = 0;
};

TEST_F_UI(BaseViewTest, StableRasterAnimationStateDoesNotInvalidate) {
  page_->SetRasterAnimationEnabled(true);
  CountingInvalidationView view(page_.get());

  ASSERT_FALSE(
      view.render_object()->HasAnimation(ClayAnimationPropertyType::kOpacity));
  view.UpdateKeyframesRasterAnimation();

  EXPECT_EQ(view.invalidation_count(), 0);
}

TEST_F_UI(BaseViewTest, DestroyUnregistersGestureArenaMember) {
  auto view = std::make_unique<View>(1, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Native, std::vector<std::string>{},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  view->SetGestureDetectorMap(detectors);

  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  ASSERT_TRUE(arena_manager->IsMemberExist(view->Sign()));

  view->Destroy();

  EXPECT_FALSE(arena_manager->IsMemberExist(view->Sign()));
}

TEST_F_UI(BaseViewTest, DownEventPrunesDestroyedGestureArenaMember) {
  auto destroyed_view = std::make_unique<View>(1, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Native, std::vector<std::string>{},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  destroyed_view->SetGestureDetectorMap(detectors);
  destroyed_view.reset();

  auto target_view = std::make_unique<View>(2, page_.get());
  HitTestResult hit_test_result{target_view->GetHitTestTargetWeakPtr()};
  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();

  arena_manager->SetActiveUIToArenaAtDownEvent(hit_test_result);

  EXPECT_FALSE(arena_manager->IsMemberExist(1));
  target_view->Destroy();
}

TEST_F_UI(BaseViewTest, DestroyDuringActiveGestureRemovesExpiredCandidate) {
  auto winner_view = std::make_unique<View>(1, page_.get());
  auto destroyed_view = std::make_unique<View>(2, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Default, std::vector<std::string>{},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  winner_view->SetGestureDetectorMap(detectors);
  destroyed_view->SetGestureDetectorMap(detectors);

  HitTestResult hit_test_result{winner_view->GetHitTestTargetWeakPtr(),
                                destroyed_view->GetHitTestTargetWeakPtr()};
  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  arena_manager->SetActiveUIToArenaAtDownEvent(hit_test_result);
  arena_manager->DispatchTouchEventToArena(CreateDownPointer(0, 0));

  destroyed_view->Destroy();
  destroyed_view.reset();

  PointerEvent move(PointerEvent::EventType::kMoveEvent);
  move.position = {1, 0};
  arena_manager->DispatchTouchEventToArena(move);

  EXPECT_TRUE(arena_manager->IsMemberExist(winner_view->Sign()));
  winner_view->Destroy();
}

TEST_F_UI(BaseViewTest, DestroyDuringGestureCallbackSkipsExpiredCandidate) {
  testing::MockEventDelegate delegate;
  page_->SetEventDelegate(&delegate);

  auto winner_view = std::make_unique<View>(1, page_.get());
  auto destroyed_view = std::make_unique<View>(2, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Default,
             std::vector<std::string>{GestureConstants::ON_TOUCHES_DOWN},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  winner_view->SetGestureDetectorMap(detectors);
  destroyed_view->SetGestureDetectorMap(detectors);

  EXPECT_CALL(delegate, OnGestureHandlerEvent(
                            ::testing::StrEq(GestureConstants::ON_TOUCHES_DOWN),
                            ::testing::Eq(2), ::testing::Eq(1), ::testing::_,
                            ::testing::_, ::testing::_, ::testing::_,
                            ::testing::_, ::testing::_))
      .Times(0);
  EXPECT_CALL(delegate, OnGestureHandlerEvent(
                            ::testing::StrEq(GestureConstants::ON_TOUCHES_DOWN),
                            ::testing::Eq(1), ::testing::Eq(1), ::testing::_,
                            ::testing::_, ::testing::_, ::testing::_,
                            ::testing::_, ::testing::_))
      .WillOnce([&](const std::string&, int, uint32_t, float, float, float,
                    float, int64_t, Value&) { destroyed_view->Destroy(); });

  HitTestResult hit_test_result{winner_view->GetHitTestTargetWeakPtr(),
                                destroyed_view->GetHitTestTargetWeakPtr()};
  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  arena_manager->SetActiveUIToArenaAtDownEvent(hit_test_result);
  arena_manager->DispatchTouchEventToArena(CreateDownPointer(0, 0));

  EXPECT_TRUE(arena_manager->IsMemberExist(winner_view->Sign()));
  EXPECT_FALSE(arena_manager->IsMemberExist(2));
  destroyed_view.reset();
  winner_view->Destroy();
  page_->SetEventDelegate(nullptr);
}

TEST_F_UI(BaseViewTest,
          DestroyCurrentMemberDuringGestureCallbackStopsRemainingHandlers) {
  testing::MockEventDelegate delegate;
  page_->SetEventDelegate(&delegate);

  auto winner_view = std::make_unique<View>(1, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Default,
             std::vector<std::string>{GestureConstants::ON_BEGIN},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  detectors.emplace(
      2, std::make_shared<GestureDetector>(
             2, GestureHandlerType::Pan,
             std::vector<std::string>{GestureConstants::ON_BEGIN},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  winner_view->SetGestureDetectorMap(detectors);

  EXPECT_CALL(delegate,
              OnGestureHandlerEvent(
                  ::testing::StrEq(GestureConstants::ON_BEGIN),
                  ::testing::Eq(1), ::testing::_, ::testing::_, ::testing::_,
                  ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce([&](const std::string&, int, uint32_t, float, float, float,
                    float, int64_t, Value&) {
        winner_view->Destroy();
        winner_view.reset();
      });

  HitTestResult hit_test_result{winner_view->GetHitTestTargetWeakPtr()};
  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  arena_manager->SetActiveUIToArenaAtDownEvent(hit_test_result);
  arena_manager->DispatchTouchEventToArena(CreateDownPointer(0, 0));

  EXPECT_FALSE(arena_manager->IsMemberExist(1));
  page_->SetEventDelegate(nullptr);
}

TEST_F_UI(BaseViewTest, DestroyDuringFlingRemovesExpiredCandidate) {
  auto winner_view = std::make_unique<View>(1, page_.get());
  auto destroyed_view = std::make_unique<View>(2, page_.get());
  GestureMap detectors;
  detectors.emplace(
      1, std::make_shared<GestureDetector>(
             1, GestureHandlerType::Default, std::vector<std::string>{},
             std::unordered_map<std::string, std::vector<uint32_t>>{}));
  winner_view->SetGestureDetectorMap(detectors);
  destroyed_view->SetGestureDetectorMap(detectors);

  HitTestResult hit_test_result{winner_view->GetHitTestTargetWeakPtr(),
                                destroyed_view->GetHitTestTargetWeakPtr()};
  auto* arena_manager =
      page_->GetGestureHandlerDispatcher()->gesture_arena_manager();
  arena_manager->SetActiveUIToArenaAtDownEvent(hit_test_result);
  arena_manager->DispatchTouchEventToArena(CreateDownPointer(0, 0));
  arena_manager->SetVelocity(1000, 0);
  PointerEvent up(PointerEvent::EventType::kUpEvent);
  arena_manager->DispatchTouchEventToArena(up);

  auto* animation_handler = page_->GetAnimationHandler();
  animation_handler->DoAnimationFrame(0);
  animation_handler->DoAnimationFrame(16);

  destroyed_view->Destroy();
  destroyed_view.reset();
  winner_view->SetShouldConsumeGesture(false);
  animation_handler->DoAnimationFrame(32);
  animation_handler->DoAnimationFrame(48);

  EXPECT_TRUE(arena_manager->IsMemberExist(winner_view->Sign()));
  winner_view->Destroy();
}

class ViewContextMemoryTest : public UITest {
 protected:
  class TestViewContext final : public ViewContext {
   public:
    using ViewContext::ViewContext;

    void CreateViewForTesting(int id) {
      auto* view = new View(id, page_view_);
      view->SetDestructListener([this](BaseView* view) {
        external_memory_report_candidate_ids_.erase(view->id());
        view_map_.erase(view->id());
      });
      view_map_[id] = view;
      ConsumeInitialAttributes(view);
    }

    size_t ExternalMemoryCandidateCountForTesting() const {
      return external_memory_report_candidate_ids_.size();
    }
  };

  void UISetUp() override {
    view_context_ = std::make_shared<TestViewContext>(page_.get(), nullptr);
  }

  void UITearDown() override {
    if (view_context_) {
      view_context_->ResetPageView();
    }
    view_context_.reset();
  }

  std::shared_ptr<TestViewContext> view_context_;
};

class ExternalMemoryEventDelegate final : public testing::MockEventDelegate {
 public:
  MOCK_METHOD(void, OnExternalMemoryReport, (int64_t, int64_t), (override));
};

class SnapshotUIOperationQueue final
    : public lynx::shell::UIOperationQueueInterface {
 public:
  void Enqueue(lynx::base::closure operation) override {
    operations_.emplace_back(std::move(operation));
  }

  void Flush() override {
    auto operations = std::move(operations_);
    operations_.clear();
    for (auto& operation : operations) {
      operation();
    }
  }

  size_t PendingOperationCount() const { return operations_.size(); }

 private:
  std::vector<lynx::base::closure> operations_;
};

TEST_F_UI(BaseViewTest, TreeManipulation) {
  int view_id = 0;
  std::unique_ptr<BaseView> root =
      std::make_unique<View>(view_id++, page_.get());
  View* childView1 = new View(view_id++, page_.get());
  root->AddChild(childView1);
  View* childView2 = new View(view_id++, page_.get());
  View* childView3 = new View(view_id++, page_.get());
  root->AddChild(childView3);
  root->AddChild(childView2, 1);
  EXPECT_EQ(root->child_count(), 3u);
  EXPECT_EQ(root->Parent(), nullptr);
  EXPECT_EQ(childView1->Parent(), root.get());
  EXPECT_EQ(childView2->Parent(), root.get());
  EXPECT_EQ(childView3->Parent(), root.get());

  root->RemoveChild(childView3);
  EXPECT_EQ(childView3->Parent(), nullptr);
  delete childView3;

  EXPECT_EQ(root->child_count(), 2u);
  root->DestroyAllChildren();
  root->Destroy();
  EXPECT_EQ(root->child_count(), 0u);
}

TEST_F_UI(ViewContextMemoryTest, ExternalMemoryTracksRemovedNodeCandidates) {
  ASSERT_TRUE(view_context_->CreateView(1, "page"));
  view_context_->CreateViewForTesting(2);
  view_context_->CreateViewForTesting(3);
  const int64_t unit_size = sizeof(BaseView);

  view_context_->AddView(2, 1, 0);
  view_context_->AddView(3, 2, 0);
  view_context_->UpdateNodeReadyPatching({}, {2}, true);
  auto snapshot = view_context_->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 3 * unit_size);
  EXPECT_EQ(snapshot.garbage_size, 0);

  view_context_->RemoveView(2, 1, false);
  view_context_->UpdateNodeReadyPatching({}, {2, 2, 3}, true);
  snapshot = view_context_->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 3 * unit_size);
  EXPECT_EQ(snapshot.garbage_size, 2 * unit_size);
  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 2u);
  EXPECT_EQ(view_context_->GetExternalMemorySnapshot().garbage_size,
            2 * unit_size);

  view_context_->AddView(2, 1, 0);
  view_context_->RemoveView(2, 1, false);
  view_context_->UpdateNodeReadyPatching({}, {2}, true);
  view_context_->AddView(2, 1, 0);
  EXPECT_EQ(view_context_->GetExternalMemorySnapshot().garbage_size, 0);

  view_context_->RemoveView(2, 1, false);
  view_context_->UpdateNodeReadyPatching({}, {2}, true);
  ASSERT_TRUE(view_context_->DestroyView(2));
  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 0u);
  snapshot = view_context_->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, unit_size);
  EXPECT_EQ(snapshot.garbage_size, 0);
}

TEST_F_UI(ViewContextMemoryTest, ExternalMemoryPrunesMissingNodeCandidates) {
  view_context_->UpdateNodeReadyPatching({}, {404, 405}, true);
  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 2u);

  auto snapshot = view_context_->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 0);
  EXPECT_EQ(snapshot.garbage_size, 0);
  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 0u);
}

TEST_F_UI(ViewContextMemoryTest,
          FeatureOffDoesNotTrackExternalMemoryCandidates) {
  view_context_->UpdateNodeReadyPatching({}, {404, 405}, false);
  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 0u);
}

TEST_F_UI(ViewContextMemoryTest,
          ClayNodeReadyPatchingCompletesInOneUIOperationFlush) {
  auto queue = std::make_shared<SnapshotUIOperationQueue>();
  lynx::tasm::PaintingContextClay painting_context(view_context_.get());
  painting_context.SetUIOperationQueue(queue);
  auto platform_ref = painting_context.GetPlatformRef();

  queue->Enqueue([platform_ref]() {
    platform_ref->UpdateNodeReadyPatching({}, {404, 405}, true);
  });
  queue->Flush();

  EXPECT_EQ(view_context_->ExternalMemoryCandidateCountForTesting(), 2u);
  EXPECT_EQ(queue->PendingOperationCount(), 0u);
}

TEST_F_UI(ViewContextMemoryTest, PendingReportSurvivesPageReset) {
  auto task_runner = testing::TestTaskRunner::Create();
  auto page = std::make_unique<PageView>(0, nullptr, task_runner);
  auto view_context = std::make_shared<TestViewContext>(page.get(), nullptr);
  ExternalMemoryEventDelegate delegate;
  page->SetEventDelegate(&delegate);

  EXPECT_CALL(delegate, OnExternalMemoryReport(0, 0)).Times(1);
  view_context->RequestExternalMemoryReport(1000);
  view_context->ResetPageView();
  task_runner->AdvanceBy(fml::TimeDelta::FromMilliseconds(999));
  task_runner->AdvanceBy(fml::TimeDelta::FromMilliseconds(1));

  view_context.reset();
  page.reset();
}

TEST_F_UI(ViewContextMemoryTest, PendingReportSkipsDestroyedPage) {
  auto task_runner = testing::TestTaskRunner::Create();
  auto page = std::make_unique<PageView>(0, nullptr, task_runner);
  auto view_context = std::make_shared<TestViewContext>(page.get(), nullptr);
  ExternalMemoryEventDelegate delegate;
  page->SetEventDelegate(&delegate);

  EXPECT_CALL(delegate, OnExternalMemoryReport(::testing::_, ::testing::_))
      .Times(0);
  view_context->RequestExternalMemoryReport(1000);
  page.reset();
  task_runner->AdvanceBy(fml::TimeDelta::FromMilliseconds(1000));

  view_context.reset();
}

TEST_F_UI(BaseViewTest, HitTest) {
  //     0     100     200 250    450 600   800
  //     |---------------|
  //     |     View1     |
  // 200 |       |-------------------|------|
  // 300 |-------|    View3          |      |
  // 350         |         |--------||      |
  //             |         |  View4 ||      |
  //             |         |--------||      |
  //             |                   |      |
  // 700         |-------------------|      |
  //             |                          |
  //             |           View2          |
  //             |                          |
  // 1000        |--------------------------|
  //

  std::unique_ptr<BaseView> root = std::make_unique<View>(0, page_.get());
  // View type doesn't matter. All views in the region will be added in.
  BaseView* View1 = new View(1, page_.get());
  BaseView* View2 = new View(2, page_.get());
  BaseView* View3 = new View(3, page_.get());
  BaseView* View4 = new View(4, page_.get());
  root->AddChild(View1);
  root->AddChild(View2);
  View2->AddChild(View3);
  View3->AddChild(View4);
  EXPECT_EQ(root->child_count(), 2u);

  root->SetX(0.f);
  root->SetY(0.f);
  root->SetWidth(1000.f);
  root->SetHeight(1000.f);

  View1->SetX(0.f);
  View1->SetY(0.f);
  View1->SetWidth(200.f);
  View1->SetHeight(300.f);

  View2->SetX(100.f);
  View2->SetY(200.f);
  View2->SetWidth(800.f);
  View2->SetHeight(800.f);

  View3->SetX(0.f);
  View3->SetY(0.f);
  View3->SetWidth(500.f);
  View3->SetHeight(500.f);

  View4->SetX(150.f);
  View4->SetY(100.f);
  View4->SetWidth(200.f);
  View4->SetHeight(200.f);

  View3->OnLayoutUpdated();
  View4->OnLayoutUpdated();

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(300, 100), hit_test_result);
    // root
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 1);
  }

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(150, 350), hit_test_result);
    // root / view2 / view3
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 3);
  }

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(650, 750), hit_test_result);
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 2);
    int list[] = {2, 0};
    int index = 0;
    for (auto it = hit_test_result.begin(); it != hit_test_result.end(); ++it) {
      EXPECT_EQ(static_cast<BaseView*>(it->get())->id(), list[index]);
      index++;
    }
  }

  root->DestroyAllChildren();
  root->Destroy();
}

TEST_F_UI(BaseViewTest, EventThroughUsesNearestExplicitAncestorValue) {
  struct TestCase {
    bool page_event_through;
    std::optional<bool> parent_event_through;
    std::optional<bool> child_event_through;
    bool expected_page;
    bool expected_parent;
    bool expected_child;
  };
  const TestCase test_cases[] = {
      {false, std::nullopt, std::nullopt, false, false, false},
      {true, std::nullopt, std::nullopt, true, true, true},
      {true, false, std::nullopt, true, false, false},
      {false, true, std::nullopt, false, true, true},
      {true, false, true, true, false, true},
  };

  for (const auto& test_case : test_cases) {
    page_->SetEventThrough(test_case.page_event_through);
    auto current_parent = std::make_unique<View>(1, page_.get());
    auto current_child = std::make_unique<View>(2, page_.get());
    page_->AddChild(current_parent.get());
    current_parent->AddChild(current_child.get());
    current_parent->SetBound(0, 0, 100, 100);
    current_child->SetBound(0, 0, 100, 100);
    if (test_case.parent_event_through.has_value()) {
      current_parent->SetEventThrough(*test_case.parent_event_through);
    }
    if (test_case.child_event_through.has_value()) {
      current_child->SetEventThrough(*test_case.child_event_through);
    }

    HitTestResult result;
    ASSERT_TRUE(page_->HitTest(CreateDownPointer(50, 50), result));
    ASSERT_EQ(result.size(), 3u);

    auto it = result.begin();
    EXPECT_EQ(it->get(), current_child.get());
    EXPECT_EQ((*it)->ShouldPassEventToNative(), test_case.expected_child);
    ++it;
    EXPECT_EQ(it->get(), current_parent.get());
    EXPECT_EQ((*it)->ShouldPassEventToNative(), test_case.expected_parent);
    ++it;
    EXPECT_EQ(it->get(), page_.get());
    EXPECT_EQ((*it)->ShouldPassEventToNative(), test_case.expected_page);

    current_parent->RemoveChild(current_child.get());
    page_->RemoveChild(current_parent.get());
  }
}

TEST_F_UI(BaseViewTest, EventThroughControlsNativeEventTargetByInheritance) {
  auto parent = std::make_unique<View>(1, page_.get());
  auto child = std::make_unique<View>(2, page_.get());
  page_->AddChild(parent.get());
  parent->AddChild(child.get());
  parent->SetBound(0, 0, 100, 100);
  child->SetBound(0, 0, 100, 100);

  FloatPoint relative_position;
  page_->SetEventThrough(true);
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      nullptr);

  parent->SetEventThrough(false);
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      child.get());

  child->SetEventThrough(true);
  EXPECT_EQ(
      page_->GetTopViewToAcceptEvent(FloatPoint(50, 50), &relative_position),
      parent.get());

  parent->RemoveChild(child.get());
  page_->RemoveChild(parent.get());
}

class BaseViewWithChildrenTest : public UITest {
 protected:
  void UISetUp() override {
    for (int i = 0; i <= 6; i++) {
      nodeList.push_back(std::make_unique<View>(i, page_.get()));
    }

    nodeList[0]->AddChild(nodeList[1].get());
    nodeList[0]->AddChild(nodeList[2].get());
    nodeList[1]->AddChild(nodeList[3].get());
    nodeList[1]->AddChild(nodeList[4].get());
    nodeList[1]->AddChild(nodeList[5].get());
    nodeList[2]->AddChild(nodeList[6].get());
  }

  void UITearDown() override { nodeList.clear(); }

  bool ChildrenPaintingOrderIsDirtyForTesting(BaseView* view) {
    return !view->children_.empty() && view->sorted_children_.empty();
  }

  const std::vector<BaseView*>& GetSortedChildrenForTesting(BaseView* view) {
    view->RebuildSortedChildrenIfNeeded();
    return view->sorted_children_;
  }

  std::vector<std::unique_ptr<BaseView>> nodeList;
};

TEST_F_UI(BaseViewWithChildrenTest, PaintOrder) {
  const auto translate_z = [](float value) {
    lynx::gfx::TransformOperations result;
    result.AppendTranslate({}, {}, {value, lynx::gfx::LengthUnit::kNumber});
    return result;
  };
  // Initial state.
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted1 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), false);
  EXPECT_EQ(sorted1[0], nodeList[3].get());
  EXPECT_EQ(sorted1[1], nodeList[4].get());
  EXPECT_EQ(sorted1[2], nodeList[5].get());

  // Set z-index = 5 for node 3.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted2 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted2[0], nodeList[4].get());
  EXPECT_EQ(sorted2[1], nodeList[5].get());
  EXPECT_EQ(sorted2[2], nodeList[3].get());

  // Set z-index = 5 for node 3 again.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), false);

  // Set translate-z = 1 for node 4.
  auto transform3 = translate_z(1.0f);
  nodeList[4]->SetProperty(ClayAnimationPropertyType::kTransform, transform3,
                           false);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted3 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted3[0], nodeList[5].get());
  EXPECT_EQ(sorted3[1], nodeList[3].get());
  EXPECT_EQ(sorted3[2], nodeList[4].get());

  // Insert new child to node 1.
  BaseView* obj = new View(7, page_.get());
  nodeList[1]->AddChild(obj);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted4 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted4[0], nodeList[5].get());
  EXPECT_EQ(sorted4[1], obj);
  EXPECT_EQ(sorted4[2], nodeList[3].get());
  EXPECT_EQ(sorted4[3], nodeList[4].get());

  // Set z-index = 10 for obj.
  obj->SetPaintingOrder(10);
  const auto& sorted5 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted5[0], nodeList[5].get());
  EXPECT_EQ(sorted5[1], nodeList[3].get());
  EXPECT_EQ(sorted5[2], obj);
  EXPECT_EQ(sorted5[3], nodeList[4].get());

  // Set translate-z = 1 for obj.
  auto transform6 = translate_z(1.0f);
  obj->SetProperty(ClayAnimationPropertyType::kTransform, transform6, false);
  const auto& sorted6 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted6[0], nodeList[5].get());
  EXPECT_EQ(sorted6[1], nodeList[3].get());
  EXPECT_EQ(sorted6[2], nodeList[4].get());
  EXPECT_EQ(sorted6[3], obj);

  // Set translate-z = 0 for obj and node 4.
  auto transform7 = translate_z(0.0f);
  nodeList[4]->SetProperty(ClayAnimationPropertyType::kTransform, transform7,
                           false);
  obj->SetProperty(ClayAnimationPropertyType::kTransform, transform7, false);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted7 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted7[0], nodeList[4].get());
  EXPECT_EQ(sorted7[1], nodeList[5].get());
  EXPECT_EQ(sorted7[2], nodeList[3].get());
  EXPECT_EQ(sorted7[3], obj);

  // Remove node 3.
  nodeList[1]->RemoveChild(nodeList[3].get());
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted8 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted8[0], nodeList[4].get());
  EXPECT_EQ(sorted8[1], nodeList[5].get());
  EXPECT_EQ(sorted8[2], obj);

  // Update root node‘s painting order shouldn't trigger a crash.
  auto transform8 = translate_z(1.0f);
  auto* root = nodeList[0].get();
  root->SetProperty(ClayAnimationPropertyType::kTransform, transform8, false);
  root->SetPaintingOrder(1);
}

TEST_F_UI(BaseViewTest, OnBoundChange) {
  class MockBaseView : public BaseView {
   public:
    using BaseView::BaseView;
    MOCK_METHOD(void, OnBoundsChanged,
                (const FloatRect& old_bounds, const FloatRect& new_bounds),
                (override));
  };

  MockBaseView mock_view(std::make_unique<RenderContainer>(), page_.get());

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetBound(0, 0, 100, 100);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(FloatRect(0, 0, 100, 100),
                                         FloatRect(0, 0, 200, 300)))
      .Times(1);
  mock_view.SetBound(0, 0, 200, 300);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetX(10);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetY(10);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(0);
  mock_view.SetBound(10, 10, 200, 300);
}

}  // namespace clay
