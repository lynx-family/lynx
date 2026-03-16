// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/pipeline/pipeline_context.h"

#include <memory>
#include <vector>

#include "base/include/fml/hash_combine.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/pipeline/pipeline_context_manager.h"
#include "core/renderer/pipeline/pipeline_context_unittest.h"
#include "core/renderer/pipeline/pipeline_lifecycle_observer.h"
#include "core/renderer/pipeline/pipeline_version.h"

namespace lynx {
namespace tasm {
namespace test {
class TestLifecycleObserver : public PipelineLifecycleObserver {
 public:
  TestLifecycleObserver() = default;
  ~TestLifecycleObserver() override = default;
  void OnLifecycleChanged(const Data& data) override {
    ++on_changed_count;
    data_ = data;
  }
  int on_changed_count = 0;
  Data data_;
};

class SelfRemovingObserver : public PipelineLifecycleObserver {
 public:
  explicit SelfRemovingObserver(PipelineContext* context) : context_(context) {}
  ~SelfRemovingObserver() override = default;

  void OnLifecycleChanged(const Data&) override {
    ++on_changed_count;
    context_->RemoveObserver(this);
  }

  int on_changed_count = 0;

 private:
  PipelineContext* context_ = nullptr;
};

class RemoveOtherObserver : public PipelineLifecycleObserver {
 public:
  RemoveOtherObserver(PipelineContext* context,
                      PipelineLifecycleObserver* target)
      : context_(context), target_(target) {}
  ~RemoveOtherObserver() override = default;

  void OnLifecycleChanged(const Data&) override {
    ++on_changed_count;
    context_->RemoveObserver(target_);
  }

  int on_changed_count = 0;

 private:
  PipelineContext* context_ = nullptr;
  PipelineLifecycleObserver* target_ = nullptr;
};

class RecordingObserver : public PipelineLifecycleObserver {
 public:
  void OnLifecycleChanged(const Data& data) override { events.push_back(data); }

  std::vector<Data> events;
};

class ReentrantAdvanceObserver : public PipelineLifecycleObserver {
 public:
  explicit ReentrantAdvanceObserver(PipelineContext* context)
      : context_(context) {}

  void OnLifecycleChanged(const Data& data) override {
    events.push_back(data);
    if (!reentered_ && data.cur_state == LifecycleState::kInStyleResolve) {
      reentered_ = true;
      context_->AdvanceLifecycleTo(LifecycleState::kAfterStyleResolve);
    }
  }

  std::vector<Data> events;

 private:
  PipelineContext* context_ = nullptr;
  bool reentered_ = false;
};

PipelineContextTest::PipelineContextTest()
    : options_(std::make_shared<PipelineOptions>()) {}

TEST_F(PipelineContextTest, TestPipelineContextConstructor01) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_unique<PipelineContext>(version);
  EXPECT_EQ(context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextConstructor02) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_shared<PipelineContext>(version);
  auto next_context = PipelineContext::Create(context->GetVersion(), true);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 2);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextConstructor03) {
  auto version = PipelineVersion(1, 2);
  auto context = std::make_shared<PipelineContext>(version);
  auto next_context = PipelineContext::Create(context->GetVersion(), false);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 3);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate01) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  EXPECT_EQ(context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate02) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  auto next_context = PipelineContext::Create(context->GetVersion(), true);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 1);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 1);
}

TEST_F(PipelineContextTest, TestPipelineContextCreate03) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), false);
  auto next_context = PipelineContext::Create(context->GetVersion(), false);
  EXPECT_EQ(next_context->GetVersion().GetMajor(), 0);
  EXPECT_EQ(next_context->GetVersion().GetMinor(), 2);
}

TEST_F(PipelineContextTest, TestPipelineContextResolve) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsResolveRequested());
  context->RequestResolve();
  EXPECT_TRUE(context->IsResolveRequested());
  context->ResetResolveRequested();
  EXPECT_FALSE(context->IsResolveRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextLayout) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsLayoutRequested());
  context->RequestLayout();
  EXPECT_TRUE(context->IsLayoutRequested());
  context->ResetLayoutRequested();
  EXPECT_FALSE(context->IsLayoutRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextFlush) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
  context->RequestFlushUIOperation();
  EXPECT_TRUE(context->IsFlushUIOperationRequested());
  context->ResetFlushUIOperationRequested();
  EXPECT_FALSE(context->IsFlushUIOperationRequested());
}

TEST_F(PipelineContextTest, TestPipelineContextReload) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);
  EXPECT_FALSE(context->IsReload());
  context->MarkReload(true);
  EXPECT_TRUE(context->IsReload());
  context->MarkReload(false);
  EXPECT_FALSE(context->IsReload());
}

TEST_F(PipelineContextTest, TestPipelineContextGetHash) {
  std::vector<std::unique_ptr<PipelineContext>> contexts{};
  for (int i = 0; i < 10; i++) {
    auto context =
        PipelineContext::Create(PipelineVersion::Create(), i % 2 == 0);
    EXPECT_NE(context, nullptr);
    contexts.push_back(std::move(context));
  }
  for (int i = 0; i < 10; i++) {
    EXPECT_NE(contexts[i], nullptr);
    auto seed = fml::HashCombine();
    fml::HashCombineSeed(seed, contexts[i].get(),
                         contexts[i]->GetVersion().GetMajor(),
                         contexts[i]->GetVersion().GetMinor());
    EXPECT_EQ(seed, contexts[i]->GetHash());
  }
}

TEST_F(PipelineContextTest, TestPipelineContextAdvanceLifecycle) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  EXPECT_NE(context, nullptr);
  context->SetOptions(options_);

  context->RequestResolve();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInStyleResolve));
  EXPECT_TRUE(context->observer_data_.is_state_executed);

  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kAfterStyleResolve));
  EXPECT_TRUE(context->observer_data_.is_state_executed);

  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInPerformLayout));
  EXPECT_FALSE(context->observer_data_.is_state_executed);

  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kAfterPerformLayout));
  EXPECT_FALSE(context->observer_data_.is_state_executed);

  context->RequestFlushUIOperation();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kUIOpFlush));
  EXPECT_TRUE(context->observer_data_.is_state_executed);

  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kStopped));
  EXPECT_TRUE(context->observer_data_.is_state_executed);
}

TEST_F(PipelineContextTest, TestPipelineContextObserver) {
  auto manager = std::make_unique<PipelineContextManager>(true);
  options_->pipeline_id = "test";
  options_->pipeline_origin = "TestPipelineContextObserver";
  auto observer = std::make_unique<TestLifecycleObserver>();
  manager->AddObserver(observer.get());
  auto* context = manager->CreateAndUpdateCurrentPipelineContext(options_);
  EXPECT_NE(context, nullptr);
  EXPECT_EQ(context->observers_.size(), 1);

  manager->RemoveObserver(observer.get());
  EXPECT_EQ(context->observers_.size(), 0);
}

TEST_F(PipelineContextTest, TestPipelineContextObserverSelfRemoveInCallback) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  ASSERT_NE(context, nullptr);
  context->SetOptions(options_);
  auto self_removing_observer =
      std::make_unique<SelfRemovingObserver>(context.get());
  auto normal_observer = std::make_unique<TestLifecycleObserver>();
  context->AddObserver(self_removing_observer.get());
  context->AddObserver(normal_observer.get());

  context->RequestResolve();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInStyleResolve));
  EXPECT_EQ(self_removing_observer->on_changed_count, 1);
  EXPECT_LE(context->observers_.size(), 1);
}

TEST_F(PipelineContextTest, TestPipelineContextObserverCleanupExpiredWeakPtr) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  ASSERT_NE(context, nullptr);
  context->SetOptions(options_);
  {
    auto observer = std::make_unique<TestLifecycleObserver>();
    context->AddObserver(observer.get());
  }
  EXPECT_EQ(context->observers_.size(), 1);

  context->RequestResolve();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInStyleResolve));
  EXPECT_EQ(context->observers_.size(), 0);
}

TEST_F(PipelineContextTest, TestPipelineContextObserverRemoveOtherInCallback) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  ASSERT_NE(context, nullptr);
  context->SetOptions(options_);
  auto target_observer = std::make_unique<TestLifecycleObserver>();
  auto remover_observer = std::make_unique<RemoveOtherObserver>(
      context.get(), target_observer.get());
  context->AddObserver(remover_observer.get());
  context->AddObserver(target_observer.get());

  context->RequestResolve();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInStyleResolve));
  EXPECT_EQ(remover_observer->on_changed_count, 1);
  EXPECT_EQ(context->observers_.size(), 1);
  auto target_count_after_first = target_observer->on_changed_count;

  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kAfterStyleResolve));
  EXPECT_EQ(target_observer->on_changed_count, target_count_after_first);
}

TEST_F(PipelineContextTest,
       TestPipelineContextObserverReentrantAdvancePreservesOuterData) {
  auto context = PipelineContext::Create(PipelineVersion::Create(), true);
  ASSERT_NE(context, nullptr);
  context->SetOptions(options_);
  auto reentrant_observer =
      std::make_unique<ReentrantAdvanceObserver>(context.get());
  auto recording_observer = std::make_unique<RecordingObserver>();
  context->AddObserver(reentrant_observer.get());
  context->AddObserver(recording_observer.get());

  context->RequestResolve();
  EXPECT_TRUE(context->AdvanceLifecycleTo(LifecycleState::kInStyleResolve));

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
