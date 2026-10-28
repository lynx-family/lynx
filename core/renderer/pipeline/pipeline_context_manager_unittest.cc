// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/pipeline/pipeline_context_manager_unittest.h"

#include <memory>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/pipeline/pipeline_context_manager.h"
#include "core/renderer/pipeline/pipeline_lifecycle_observer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
class TestLifecycleObserver : public PipelineLifecycleObserver {
 public:
  void OnLifecycleChanged(const Data& data) override {
    ++on_changed_count;
    last_data = data;
  }

  int on_changed_count = 0;
  Data last_data;
};

class RemoveByManagerObserver : public PipelineLifecycleObserver {
 public:
  RemoveByManagerObserver(PipelineContextManager* manager,
                          PipelineLifecycleObserver* target)
      : manager_(manager), target_(target) {}

  void OnLifecycleChanged(const Data&) override {
    ++on_changed_count;
    manager_->RemoveObserver(target_);
  }

  int on_changed_count = 0;

 private:
  PipelineContextManager* manager_ = nullptr;
  PipelineLifecycleObserver* target_ = nullptr;
};

class SelfRemovingByManagerObserver : public PipelineLifecycleObserver {
 public:
  explicit SelfRemovingByManagerObserver(PipelineContextManager* manager)
      : manager_(manager) {}

  void OnLifecycleChanged(const Data&) override {
    ++on_changed_count;
    manager_->RemoveObserver(this);
  }

  int on_changed_count = 0;

 private:
  PipelineContextManager* manager_ = nullptr;
};

class RecordingObserver : public PipelineLifecycleObserver {
 public:
  void OnLifecycleChanged(const Data& data) override { events.push_back(data); }

  std::vector<Data> events;
};

class ReentrantAdvanceObserver : public PipelineLifecycleObserver {
 public:
  ReentrantAdvanceObserver(PipelineContextManager* manager,
                           PipelineContext* context)
      : manager_(manager), context_(context) {}

  void OnLifecycleChanged(const Data& data) override {
    events.push_back(data);
    if (!reentered_ && data.cur_state == LifecycleState::kInStyleResolve) {
      reentered_ = true;
      manager_->AdvanceLifecycleTo(context_,
                                   LifecycleState::kAfterStyleResolve);
    }
  }

  std::vector<Data> events;

 private:
  PipelineContextManager* manager_ = nullptr;
  PipelineContext* context_ = nullptr;
  bool reentered_ = false;
};

class AddByManagerObserver : public PipelineLifecycleObserver {
 public:
  AddByManagerObserver(PipelineContextManager* manager,
                       PipelineLifecycleObserver* target)
      : manager_(manager), target_(target) {}

  void OnLifecycleChanged(const Data&) override {
    ++on_changed_count;
    manager_->AddObserver(target_);
  }

  int on_changed_count = 0;

 private:
  PipelineContextManager* manager_ = nullptr;
  PipelineLifecycleObserver* target_ = nullptr;
};

TEST_F(PipelineContextManagerTest, TestPipelineContextManagerCreate) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto context = manager->CreateAndUpdateCurrentPipelineContext(options);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
  auto next_options = std::make_shared<PipelineOptions>();
  context = manager->CreateAndUpdateCurrentPipelineContext(next_options, true);
  EXPECT_EQ(context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextManagerTest,
       TestPipelineContextManagerGetCurrentContext) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto current_context = manager->GetCurrentPipelineContext();
  EXPECT_EQ(context, current_context);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);

  context->RequestResolve();
  EXPECT_TRUE(context->IsResolveRequested());
  context->ResetResolveRequested();
  EXPECT_FALSE(context->IsResolveRequested());
  context->RequestLayout();
  EXPECT_TRUE(context->IsLayoutRequested());
  context->ResetLayoutRequested();
  EXPECT_FALSE(context->IsLayoutRequested());
  context->RequestFlushUIOperation();
  EXPECT_TRUE(context->IsFlushUIOperationRequested());
  context->ResetFlushUIOperationRequested();
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
  context->MarkReload(true);
  EXPECT_TRUE(context->IsReload());
  context->MarkReload(false);
  EXPECT_FALSE(context->IsReload());

  auto next_options = std::make_shared<PipelineOptions>();
  auto next_context =
      manager->CreateAndUpdateCurrentPipelineContext(next_options, false);
  EXPECT_NE(next_context, current_context);
  EXPECT_EQ(next_context, manager->GetCurrentPipelineContext());
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextManagerTest,
       TestPipelineContextManagerGetContextByVersion) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(true);
  std::vector<const PipelineContext*> contexts{};
  for (int i = 0; i < 10; i++) {
    auto context =
        manager->CreateAndUpdateCurrentPipelineContext(options, i % 2 == 0);
    contexts.push_back(context);
  }
  for (int i = 0; i < 10; i++) {
    auto context =
        manager->GetPipelineContextByVersion(contexts[i]->GetVersion());
    EXPECT_EQ(context, contexts[i]);
    EXPECT_EQ(context->GetVersion().GetMajor(),
              contexts[i]->GetVersion().GetMajor());
    EXPECT_EQ(context->GetVersion().GetMinor(),
              contexts[i]->GetVersion().GetMinor());
  }
}

TEST_F(PipelineContextManagerTest, TestPipelineOptionsHeldByContext) {
  auto options = std::make_shared<PipelineOptions>();
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  EXPECT_NE(context, nullptr);
  auto* next_context = manager->CreateAndUpdateCurrentPipelineContext(options);
  EXPECT_EQ(context, next_context);
  EXPECT_EQ(context->GetVersion(), next_context->GetVersion());
}

TEST_F(PipelineContextManagerTest, ObserverPersistAcrossContexts) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto observer = std::make_unique<TestLifecycleObserver>();
  manager->AddObserver(observer.get());

  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);

  auto next_options = std::make_shared<PipelineOptions>();
  auto* next_context =
      manager->CreateAndUpdateCurrentPipelineContext(next_options);
  next_context->RequestResolve();
  EXPECT_TRUE(manager->AdvanceLifecycleTo(next_context,
                                          LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 2);
  EXPECT_EQ(observer->last_data.pipeline_version, next_context->GetVersion());
}

TEST_F(PipelineContextManagerTest, ObserverAttachToExistingContext) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto observer = std::make_unique<TestLifecycleObserver>();

  manager->AddObserver(observer.get());
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest, ObserverRemoveStopsCallbacks) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto observer = std::make_unique<TestLifecycleObserver>();
  manager->AddObserver(observer.get());
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);

  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);

  manager->RemoveObserver(observer.get());
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest,
       ObserverDataDeduplicationAndIdempotentRemove) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto observer = std::make_unique<TestLifecycleObserver>();
  manager->AddObserver(observer.get());
  manager->AddObserver(observer.get());
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);

  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);

  manager->RemoveObserver(observer.get());
  manager->RemoveObserver(observer.get());
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest,
       ObserverDataDeduplicationAfterContextCreated) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto observer = std::make_unique<TestLifecycleObserver>();

  manager->AddObserver(observer.get());
  manager->AddObserver(observer.get());

  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest, ObserverRemoveOtherInCallbackViaManager) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto target_observer = std::make_unique<TestLifecycleObserver>();
  auto remover_observer = std::make_unique<RemoveByManagerObserver>(
      manager.get(), target_observer.get());

  manager->AddObserver(remover_observer.get());
  manager->AddObserver(target_observer.get());
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(remover_observer->on_changed_count, 1);
  EXPECT_EQ(target_observer->on_changed_count, 1);

  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(target_observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest, ObserverRemoveSelfInCallbackViaManager) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto self_removing_observer =
      std::make_unique<SelfRemovingByManagerObserver>(manager.get());
  auto normal_observer = std::make_unique<TestLifecycleObserver>();

  manager->AddObserver(self_removing_observer.get());
  manager->AddObserver(normal_observer.get());
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(self_removing_observer->on_changed_count, 1);
  EXPECT_EQ(normal_observer->on_changed_count, 1);
  EXPECT_EQ(manager->observers_.size(), 1u);

  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(self_removing_observer->on_changed_count, 1);
  EXPECT_EQ(normal_observer->on_changed_count, 2);
}

TEST_F(PipelineContextManagerTest, ObserverCleanupExpiredWeakPtrInManager) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  {
    auto expired_observer = std::make_unique<TestLifecycleObserver>();
    manager->AddObserver(expired_observer.get());
  }
  EXPECT_EQ(manager->observers_.size(), 1u);

  auto live_observer = std::make_unique<TestLifecycleObserver>();
  manager->AddObserver(live_observer.get());
  EXPECT_EQ(manager->observers_.size(), 1u);

  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(live_observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest, ObserverCleanupExpiredWeakPtrOnNotify) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  {
    auto expired_observer = std::make_unique<TestLifecycleObserver>();
    manager->AddObserver(expired_observer.get());
  }
  EXPECT_EQ(manager->observers_.size(), 1u);

  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(manager->observers_.size(), 0u);
}

TEST_F(PipelineContextManagerTest,
       ObserverAddedInCallbackDoesNotJoinCurrentNotification) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto target_observer = std::make_unique<TestLifecycleObserver>();
  auto adder_observer = std::make_unique<AddByManagerObserver>(
      manager.get(), target_observer.get());

  manager->AddObserver(adder_observer.get());
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));
  EXPECT_EQ(adder_observer->on_changed_count, 1);
  EXPECT_EQ(target_observer->on_changed_count, 0);

  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(target_observer->on_changed_count, 1);
}

TEST_F(PipelineContextManagerTest,
       ObserverReentrantAdvancePreservesOuterDataViaManager) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  auto options = std::make_shared<PipelineOptions>();
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options);
  auto reentrant_observer =
      std::make_unique<ReentrantAdvanceObserver>(manager.get(), context);
  auto recording_observer = std::make_unique<RecordingObserver>();

  manager->AddObserver(reentrant_observer.get());
  manager->AddObserver(recording_observer.get());
  context->RequestResolve();
  EXPECT_TRUE(
      manager->AdvanceLifecycleTo(context, LifecycleState::kInStyleResolve));

  ASSERT_EQ(recording_observer->events.size(), 2u);
  EXPECT_EQ(recording_observer->events[0].prev_state,
            LifecycleState::kInStyleResolve);
  EXPECT_EQ(recording_observer->events[0].cur_state,
            LifecycleState::kAfterStyleResolve);
  EXPECT_EQ(recording_observer->events[1].prev_state,
            LifecycleState::kInactive);
  EXPECT_EQ(recording_observer->events[1].cur_state,
            LifecycleState::kInStyleResolve);
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
