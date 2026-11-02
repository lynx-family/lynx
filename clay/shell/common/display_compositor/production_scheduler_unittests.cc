// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "clay/shell/common/display_compositor/production_opportunity.h"
#include "clay/shell/common/scheduler/scheduler.h"
#include "clay/shell/common/scheduler/scheduler_state_machine.h"
#include "gtest/gtest.h"

namespace clay {
namespace {
using State = SchedulerStateMachine;
using Action = State::Action;

fml::TimePoint At(int64_t ticks) { return fml::TimePoint::FromTicks(ticks); }

TEST(ProductionOpportunity, RequiresClockAndExpiresAtDeadline) {
  display::ProductionOpportunity opportunity;
  EXPECT_EQ(opportunity.serial(), 0u);
  EXPECT_FALSE(opportunity.CanProduce(At(10)));
  opportunity.Begin(At(20));
  EXPECT_EQ(opportunity.serial(), 1u);
  EXPECT_TRUE(opportunity.CanProduce(At(19)));
  EXPECT_FALSE(opportunity.CanProduce(At(20)));
  EXPECT_FALSE(opportunity.CanProduce(At(21)));
}

TEST(ProductionOpportunity, ClosedCycleRejectsLateWorkUntilNextClock) {
  display::ProductionOpportunity opportunity;
  opportunity.Begin(At(20));
  opportunity.Close();
  EXPECT_FALSE(opportunity.CanProduce(At(11)));
  EXPECT_FALSE(opportunity.CanProduce(At(19)));
  opportunity.Begin(At(40));
  EXPECT_EQ(opportunity.serial(), 2u);
  EXPECT_TRUE(opportunity.CanProduce(At(21)));
}

TEST(ProductionOpportunity, NoWorkDoesNotSpendOpportunityOrExtendDeadline) {
  display::ProductionOpportunity opportunity;
  opportunity.Begin(At(20));
  EXPECT_TRUE(opportunity.CanProduce(At(10)));
  EXPECT_TRUE(opportunity.CanProduce(At(15)));
  EXPECT_EQ(opportunity.serial(), 1u);
  EXPECT_FALSE(opportunity.CanProduce(At(20)));
  opportunity.Begin(At(40));
  EXPECT_TRUE(opportunity.CanProduce(At(21)));
}

TEST(ProductionOpportunity, ResetRevokesOldBindingBudget) {
  display::ProductionOpportunity opportunity;
  opportunity.Begin(At(20));
  opportunity.Reset();
  EXPECT_EQ(opportunity.serial(), 0u);
  EXPECT_FALSE(opportunity.CanProduce(At(10)));
  opportunity.Begin(At(40));
  EXPECT_EQ(opportunity.serial(), 1u);
  EXPECT_TRUE(opportunity.CanProduce(At(21)));
}

class GatedSchedulerClient : public SchedulerClient {
 public:
  void ScheduledActionBeginFrame() override { ++begin_frames; }
  void ScheduledActionCommit() override {
    ++commits;
    scheduler->NotifyReadyToActivate();
  }
  void ScheduledActionActivePendingTree() override { ++active_generation; }
  void ScheduledActionRasterInvalidate() override {}
  void ScheduledActionUploadImage() override {}
  void ScheduledActionDraw() override {
    drawn_generations.push_back(active_generation);
    if (draw_has_work) opportunity.Close();
    scheduler->NotifyActiveTreeHasBeenDrawn();
  }
  bool CanDraw() const override { return opportunity.CanProduce(now); }

  display::ProductionOpportunity opportunity;
  fml::TimePoint now = At(10);
  Scheduler* scheduler = nullptr;
  bool draw_has_work = true;
  int begin_frames = 0;
  int commits = 0;
  int active_generation = 0;
  std::vector<int> drawn_generations;
};

void Activate(Scheduler& scheduler) {
  scheduler.SetMeaningfulLayout(true);
  scheduler.SetOutputSurfaceValid(true);
}

TEST(ProductionScheduler, GatedDrawSurvivesUntilNextOpportunity) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  client.opportunity.Begin(At(20));
  scheduler.NotifyReadyToPerformRasterFrame();
  ASSERT_EQ(client.drawn_generations.size(), 1u);
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations.size(), 1u);
  client.now = At(21);
  client.opportunity.Begin(At(40));
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations.size(), 2u);
}

TEST(ProductionScheduler, EmptyDrawDoesNotBlockWorkInSameCycle) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  client.draw_has_work = false;
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  ASSERT_EQ(client.drawn_generations.size(), 1u);
  EXPECT_TRUE(client.opportunity.CanProduce(client.now));
  client.draw_has_work = true;
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations.size(), 2u);
  EXPECT_FALSE(client.opportunity.CanProduce(client.now));
}

TEST(ProductionScheduler, LateTreeWaitsWithoutLosingActivation) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  ASSERT_EQ(client.drawn_generations, std::vector<int>({0}));
  scheduler.SetNeedsUIBeginFrame();
  scheduler.NotifyReadyToSendUIBeginFrame();
  ASSERT_EQ(client.begin_frames, 1);
  scheduler.NotifyReadyToCommit(false);
  ASSERT_EQ(client.commits, 1);
  ASSERT_EQ(client.active_generation, 1);
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0}));
  client.now = At(21);
  client.opportunity.Begin(At(40));
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0, 1}));
}

TEST(ProductionScheduler, ResetDropsOldDrawWithoutSpendingNewBudget) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  scheduler.ResetForNewProduction();
  client.opportunity.Reset();
  client.opportunity.Begin(At(20));
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  EXPECT_TRUE(client.opportunity.CanProduce(client.now));
  scheduler.SetNeedsRasterBeginFrame();
  scheduler.NotifyReadyToPerformRasterFrame();
  EXPECT_EQ(client.drawn_generations.size(), 1u);
}

TEST(ProductionScheduler, ContentRequestWaitsForSharedTick) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  scheduler.SetNeedsContentBeginFrame();
  EXPECT_FALSE(scheduler.HasPendingUIFrame());
  scheduler.SetMeaningfulLayout(true);
  EXPECT_TRUE(client.drawn_generations.empty());
  scheduler.OnBeginRasterFrame();
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0}));
  EXPECT_FALSE(scheduler.HasPendingUIFrame());
}

TEST(ProductionScheduler, ContentAllowsDeadlineAndEarlyTreeWins) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  scheduler.SetNeedsUIBeginFrame();
  scheduler.NotifyReadyToSendUIBeginFrame();
  scheduler.SetNeedsContentBeginFrame();
  ASSERT_TRUE(scheduler.BeginRasterFrameDeadline());
  scheduler.OnBeginRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  scheduler.NotifyReadyToCommit(false);
  EXPECT_EQ(client.drawn_generations, std::vector<int>({1}));
}

TEST(ProductionScheduler, DeadlineDrawsRetainedTreeAndLateTreeWaits) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  scheduler.SetNeedsUIBeginFrame();
  scheduler.NotifyReadyToSendUIBeginFrame();
  scheduler.SetNeedsContentBeginFrame();
  ASSERT_TRUE(scheduler.BeginRasterFrameDeadline());
  scheduler.OnBeginRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  scheduler.OnReachRasterFrameDeadline();
  ASSERT_EQ(client.drawn_generations, std::vector<int>({0}));
  scheduler.NotifyReadyToCommit(false);
  scheduler.OnBeginRasterFrame();
  EXPECT_EQ(client.active_generation, 1);
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0}));
  client.now = At(21);
  client.opportunity.Begin(At(40));
  scheduler.OnBeginRasterFrame();
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0, 1}));
}

TEST(ProductionScheduler, ClearExpiredDeadlineWaitsForSharedTick) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  client.opportunity.Begin(At(20));
  client.now = At(20);
  scheduler.SetNeedsContentBeginFrame();
  scheduler.OnBeginRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  scheduler.SetNeedsUIBeginFrame();
  scheduler.NotifyReadyToSendUIBeginFrame();
  ASSERT_TRUE(scheduler.BeginRasterFrameDeadline());

  client.now = At(21);
  client.opportunity.Begin(At(40));
  scheduler.ClearRasterFrameDeadline();
  EXPECT_FALSE(scheduler.IsInsideRasterFrameDeadline());
  EXPECT_TRUE(client.drawn_generations.empty());
  scheduler.OnBeginRasterFrame();
  EXPECT_EQ(client.drawn_generations, std::vector<int>({0}));
}

TEST(ProductionScheduler, RebindDropsContentRequest) {
  GatedSchedulerClient client;
  Scheduler scheduler(&client);
  client.scheduler = &scheduler;
  Activate(scheduler);
  scheduler.SetNeedsContentBeginFrame();
  scheduler.ResetForNewProduction();
  client.opportunity.Reset();
  client.opportunity.Begin(At(20));
  scheduler.OnBeginRasterFrame();
  EXPECT_TRUE(client.drawn_generations.empty());
  EXPECT_TRUE(client.opportunity.CanProduce(client.now));
}

void RequestUI(State& state) {
  state.SetNeedsUIBeginFrame();
  state.NotifyReadyToSendUIBeginFrame();
}

TEST(ProductionScheduler, PendingUIIncludesSentCommitAndUndrawnTree) {
  State state;
  state.SetMeaningfulLayoutState(State::MeaningfulLayoutState::FINISH);
  state.SetOutputSurfaceState(State::OutputSurfaceState::ACTIVE);
  EXPECT_FALSE(state.HasPendingUIFrame());
  RequestUI(state);
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.WillSendUIBeginFrame();
  EXPECT_FALSE(state.NeedsUIBeginFrame());
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.NotifyReadyToCommit();
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.WillCommit(false);
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.NotifyReadyToActivate();
  state.WillActivate();
  EXPECT_FALSE(state.HasPendingTree());
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.NotifyActiveTreeHasBeenDrawn();
  EXPECT_FALSE(state.HasPendingUIFrame());
}

TEST(ProductionScheduler, EmptyUICommitDoesNotKeepSuccessorPending) {
  State state;
  state.SetMeaningfulLayoutState(State::MeaningfulLayoutState::FINISH);
  state.SetOutputSurfaceState(State::OutputSurfaceState::ACTIVE);
  RequestUI(state);
  state.WillSendUIBeginFrame();
  state.NotifyReadyToCommit();
  EXPECT_TRUE(state.HasPendingUIFrame());
  state.WillCommit(true);
  EXPECT_FALSE(state.HasPendingUIFrame());
}

TEST(ProductionScheduler, RebindDoesNotWaitForDiscardedUIResponse) {
  State state;
  state.SetMeaningfulLayoutState(State::MeaningfulLayoutState::FINISH);
  state.SetOutputSurfaceState(State::OutputSurfaceState::ACTIVE);
  RequestUI(state);
  ASSERT_EQ(state.NextAction(), Action::BEGIN_FRAME);
  state.WillSendUIBeginFrame();
  // Closing the old binding discards its queued tree response.
  state.SetOutputSurfaceState(State::OutputSurfaceState::LOST);
  RequestUI(state);
  EXPECT_NE(state.NextAction(), Action::BEGIN_FRAME);
  state.ResetForNewProduction();
  state.SetOutputSurfaceState(State::OutputSurfaceState::ACTIVE);
  RequestUI(state);
  EXPECT_EQ(state.NextAction(), Action::BEGIN_FRAME);
}

TEST(ProductionScheduler, RebindDropsPendingTreeAndRedraw) {
  State state;
  state.SetMeaningfulLayoutState(State::MeaningfulLayoutState::FINISH);
  state.SetOutputSurfaceState(State::OutputSurfaceState::ACTIVE);
  RequestUI(state);
  state.WillSendUIBeginFrame();
  state.NotifyReadyToCommit();
  state.WillCommit(false);
  state.NotifyReadyToActivate();
  state.SetNeedsRasterBeginFrame();
  state.NotifyReadyToPerformRasterFrame();
  ASSERT_TRUE(state.HasPendingTree());
  ASSERT_TRUE(state.RedrawPending());
  state.ResetForNewProduction();
  EXPECT_FALSE(state.HasPendingTree());
  EXPECT_FALSE(state.RedrawPending());
  EXPECT_EQ(state.NextAction(), Action::NONE);
  RequestUI(state);
  EXPECT_EQ(state.NextAction(), Action::BEGIN_FRAME);
}

TEST(ProductionScheduler, RebindPreservesUploadsAndVisibility) {
  State state;
  state.SetMeaningfulLayoutState(State::MeaningfulLayoutState::FINISH);
  state.SetVisible(false);
  state.OnUploadTaskRegistered();
  state.ResetForNewProduction();
  EXPECT_FALSE(state.Visible());
  EXPECT_EQ(state.NextAction(), Action::UPLOAD_IMAGE);
  RequestUI(state);
  EXPECT_NE(state.NextAction(), Action::BEGIN_FRAME);
  state.SetVisible(true);
  EXPECT_EQ(state.NextAction(), Action::BEGIN_FRAME);
}

TEST(ProductionScheduler, RebindDoesNotInventMeaningfulLayout) {
  State state;
  state.ResetForNewProduction();
  RequestUI(state);
  EXPECT_EQ(state.NextAction(), Action::NONE);
}
}  // namespace
}  // namespace clay
