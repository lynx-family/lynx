// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/scheduler/scheduler_state_machine.h"
#include "gtest/gtest.h"

namespace clay {
namespace {
using State = SchedulerStateMachine;
using Action = State::Action;

void RequestUI(State& state) {
  state.SetNeedsUIBeginFrame();
  state.NotifyReadyToSendUIBeginFrame();
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
