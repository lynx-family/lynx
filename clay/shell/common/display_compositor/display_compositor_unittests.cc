// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/display_compositor.h"
#include "clay/shell/common/display_compositor/test_support.h"
#include "gtest/gtest.h"
namespace clay::display {
using testing_support::ManualRunner;
using testing_support::Publication;
using testing_support::Record;
using testing_support::Signal;
struct TargetState {
  std::vector<std::shared_ptr<Signal>> reads;
  int presents = 0, imports_released = 0;
  bool retry = false, fail_swap = false, detached = false, destroyed = false;
  bool omit_output = false, alias_output = false;
};
class Target : public DisplayTarget {
 public:
  explicit Target(std::shared_ptr<TargetState> state) : state_(state) {}
  ~Target() override { state_->destroyed = true; }
  void Detach() override { state_->detached = true; }
  DisplaySubmission Present(const CompositorFrame& frame) override {
    EXPECT_FALSE(state_->detached);
    if (state_->retry) return {nullptr, nullptr, false, true};
    ++state_->presents;
    auto signal = std::make_shared<Signal>();
    signal->status = CompletionStatus::kPending;
    state_->reads.push_back(signal);
    auto resources = std::shared_ptr<void>(
        new int(1),
        [state = state_, texture = std::weak_ptr<const OutputTexture>(
                             frame.result->outputs[0].texture_ref)](void* p) {
          EXPECT_FALSE(texture.expired());
          ++state->imports_released;
          delete static_cast<int*>(p);
        });
    std::shared_ptr<const OutputTexture> output;
    if (!state_->omit_output) {
      output = state_->alias_output ? frame.result->outputs[0].texture_ref
                                    : std::make_shared<const OutputTexture>(
                                          Record(signal), frame.frame_id);
    }
    return {signal, resources, !state_->fail_swap, false, std::move(output)};
  }

 private:
  std::shared_ptr<TargetState> state_;
};
class DisplayTest : public testing::Test {
 protected:
  void SetUp() override {
    compositor = std::make_shared<DisplayCompositor>(
        runner, std::make_unique<Target>(state), WindowKey{1, 1},
        [&] { ++requests; },
        [&](auto&, auto feedback) {
          ++callbacks;
          EXPECT_NE(feedback.status, PresentationFeedback::Status::kPresented);
        },
        [&](auto failure) { error = failure; });
  }
  void TearDown() override {
    if (compositor) compositor->Stop();
    for (auto& read : state->reads) read->status = CompletionStatus::kComplete;
    while (!runner->polls.empty()) runner->Poll();
    compositor.reset();
    runner->Poll();
  }
  std::shared_ptr<const CompositorFrame> Frame(
      uint64_t version = 1,
      std::shared_ptr<Signal> signal = std::make_shared<Signal>()) {
    return std::make_shared<const CompositorFrame>(CompositorFrame{
        version, {1, 1}, Publication(version, 1, signal).result});
  }
  bool Compose() {
    return compositor->Compose(fml::TimePoint::Now() +
                               fml::TimeDelta::FromMilliseconds(50));
  }
  void ExpectCurrentFrame(const std::shared_ptr<const CompositorFrame>& input) {
    const auto& current = compositor->current();
    ASSERT_TRUE(current);
    EXPECT_NE(current, input);
    EXPECT_EQ(current->frame_id, input->frame_id);
    EXPECT_EQ(current->window, input->window);
    EXPECT_EQ(current->result->binding, input->result->binding);
    EXPECT_EQ(current->result->render_version, input->result->render_version);
    EXPECT_EQ(current->result->layout_version, input->result->layout_version);
    EXPECT_EQ(current->result->segment_plan_version,
              input->result->segment_plan_version);
    EXPECT_EQ(current->result->config, input->result->config);
    ASSERT_EQ(current->result->outputs.size(), 1u);
    const auto& output = current->result->outputs[0];
    EXPECT_EQ(output.content_version, current->frame_id);
    EXPECT_EQ(output.properties.x, 0);
    EXPECT_EQ(output.properties.y, 0);
    for (const auto& entry : input->result->outputs)
      EXPECT_NE(output.texture_ref->resource(), entry.texture_ref->resource());
  }
  fml::RefPtr<ManualRunner> runner = fml::MakeRefCounted<ManualRunner>();
  std::shared_ptr<TargetState> state = std::make_shared<TargetState>();
  std::shared_ptr<DisplayCompositor> compositor;
  int requests = 0, callbacks = 0;
  PipelineError error = PipelineError::kNone;
};
TEST_F(DisplayTest, WakeAndRetirementDoNotPresentOutsideAnOpportunity) {
  compositor->Accept(Frame());
  EXPECT_EQ(state->presents, 0);
  EXPECT_EQ(requests, 1);
  Compose();
  EXPECT_EQ(state->presents, 1);
  compositor->Accept(Frame(2));
  runner->Poll();
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, PendingProducerDefersWithoutCpuWait) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  EXPECT_EQ(state->presents, 0);
  signal->status = CompletionStatus::kComplete;
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, PendingOutputKeepsThePreviouslyPresentedFrame) {
  auto previous = Frame();
  compositor->Accept(previous);
  ASSERT_FALSE(Compose());
  ExpectCurrentFrame(previous);
  auto displayed = compositor->current();

  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto next = Frame(2, signal);
  compositor->Accept(next);
  EXPECT_TRUE(Compose());
  EXPECT_EQ(compositor->current(), displayed);
  EXPECT_EQ(state->presents, 1);
  EXPECT_EQ(callbacks, 1);

  signal->status = CompletionStatus::kComplete;
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(next);
  EXPECT_EQ(state->presents, 2);
}
TEST_F(DisplayTest, NoNewOutputKeepsCurrentWithoutRepeatedSubmission) {
  auto frame = Frame();
  compositor->Accept(frame);
  ASSERT_FALSE(Compose());
  const auto requests_after_present = requests;

  EXPECT_FALSE(Compose());
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(frame);
  EXPECT_EQ(state->presents, 1);
  EXPECT_EQ(state->reads.size(), 1u);
  EXPECT_EQ(callbacks, 1);
  EXPECT_EQ(requests, requests_after_present);
}
TEST_F(DisplayTest, ReadyOutputDoesNotWaitForPreviousDisplayCompletion) {
  compositor->Accept(Frame());
  ASSERT_FALSE(Compose());
  ASSERT_EQ(state->reads.size(), 1u);
  ASSERT_EQ(state->reads[0]->status, CompletionStatus::kPending);

  auto next = Frame(2);
  compositor->Accept(next);
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(next);
  EXPECT_EQ(state->presents, 2);
  EXPECT_EQ(state->reads[0]->status, CompletionStatus::kPending);
  EXPECT_EQ(state->reads.size(), 2u);
}
TEST_F(DisplayTest, NewReadyOutputSupersedesAnOlderPendingOutput) {
  compositor->Accept(Frame());
  ASSERT_FALSE(Compose());

  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto pending = Frame(2, signal);
  compositor->Accept(pending);
  EXPECT_TRUE(Compose());

  auto ready = Frame(3);
  compositor->Accept(ready);
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(ready);
  EXPECT_EQ(state->presents, 2);
  EXPECT_EQ(signal->status, CompletionStatus::kPending);
  signal->status = CompletionStatus::kComplete;
}
TEST_F(DisplayTest, ReplacementCandidateReleasesUnconsumedRasterInput) {
  auto first = Frame();
  auto record = first->result->outputs[0].texture_ref->resource();
  std::weak_ptr<const OutputTexture> input =
      first->result->outputs[0].texture_ref;
  compositor->Accept(std::move(first));
  EXPECT_FALSE(record->CanReuse());
  compositor->Accept(Frame(2));
  EXPECT_TRUE(input.expired());
  EXPECT_TRUE(record->CanReuse());
  EXPECT_EQ(state->presents, 0);
}
TEST_F(DisplayTest, CompletedDisplayReleasesRasterInputAndKeepsComposedOutput) {
  auto frame = Frame();
  auto record = frame->result->outputs[0].texture_ref->resource();
  std::weak_ptr<const OutputTexture> input =
      frame->result->outputs[0].texture_ref;
  compositor->Accept(frame);
  ASSERT_FALSE(Compose());
  ExpectCurrentFrame(frame);
  auto displayed = compositor->current();
  auto output_record = displayed->result->outputs[0].texture_ref->resource();
  frame.reset();
  EXPECT_FALSE(input.expired());
  EXPECT_FALSE(record->CanReuse());
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(input.expired());
  EXPECT_TRUE(record->CanReuse());
  EXPECT_EQ(compositor->current(), displayed);
  EXPECT_FALSE(output_record->CanReuse());
}
TEST_F(DisplayTest, ScreenshotLeaseKeepsOnlyTheCapturedComposedOutput) {
  auto first = Frame();
  auto input_record = first->result->outputs[0].texture_ref->resource();
  compositor->Accept(first);
  ASSERT_FALSE(Compose());
  auto capture = compositor->current();
  auto output_record = capture->result->outputs[0].texture_ref->resource();
  first.reset();
  auto next = Frame(2);
  compositor->Accept(next);
  ASSERT_FALSE(Compose());
  ExpectCurrentFrame(next);
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(input_record->CanReuse());
  EXPECT_EQ(capture->frame_id, 1u);
  EXPECT_FALSE(output_record->CanReuse());
  capture.reset();
  EXPECT_TRUE(output_record->CanReuse());
}
TEST_F(DisplayTest, MissingComposedOutputDoesNotReplaceTheCurrentFrame) {
  compositor->Accept(Frame());
  ASSERT_FALSE(Compose());
  auto previous = compositor->current();
  state->omit_output = true;
  compositor->Accept(Frame(2));
  EXPECT_FALSE(Compose());
  EXPECT_EQ(error, PipelineError::kDisplayFailure);
  EXPECT_EQ(compositor->current(), previous);
  EXPECT_EQ(callbacks, 1);
}
TEST_F(DisplayTest, AliasedRasterInputCannotBecomeTheComposedCurrentOutput) {
  state->alias_output = true;
  compositor->Accept(Frame());
  EXPECT_FALSE(Compose());
  EXPECT_EQ(error, PipelineError::kDisplayFailure);
  EXPECT_FALSE(compositor->current());
  EXPECT_EQ(callbacks, 0);
}
TEST_F(DisplayTest, ProducerReadinessWakesTheCurrentDisplayOpportunity) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  EXPECT_EQ(requests, 1);
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(requests, 2);
  EXPECT_EQ(state->presents, 0);
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest,
       PendingProducerCoalescesPollsWithoutRepeatedComposeRequests) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  EXPECT_TRUE(Compose());
  ASSERT_EQ(runner->polls.size(), 1u);
  runner->Poll();
  EXPECT_EQ(requests, 1);
  EXPECT_EQ(state->presents, 0);
  EXPECT_EQ(runner->polls.size(), 1u);
}
TEST_F(DisplayTest, ReadinessPollStopsWhenTheDisplayDeadlineExpires) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  EXPECT_TRUE(compositor->Compose(fml::TimePoint::Now() -
                                  fml::TimeDelta::FromMilliseconds(1)));
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(requests, 1);
  EXPECT_EQ(state->presents, 0);
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, ReadinessPollObservesTheReplacementCandidate) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  compositor->InvalidateCandidate();
  compositor->Accept(Frame(2));
  runner->Poll();
  EXPECT_EQ(requests, 3);
  EXPECT_EQ(state->presents, 0);
  EXPECT_FALSE(Compose());
  ASSERT_TRUE(compositor->current());
  EXPECT_EQ(compositor->current()->frame_id, 2u);
}
TEST_F(DisplayTest, ReadinessPollStopsAfterBackgrounding) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  compositor->SetEnabled(false);
  runner->Poll();
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(requests, 1);
  signal->status = CompletionStatus::kComplete;
  compositor->SetEnabled(true);
  EXPECT_EQ(requests, 2);
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, ReadinessPollDoesNotRetainStoppedTarget) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  compositor->Stop();
  compositor.reset();
  EXPECT_TRUE(state->destroyed);
  runner->Poll();
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(requests, 1);
}
TEST_F(DisplayTest, ProducerFailureWakesFailureHandlingWithoutPresenting) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  compositor->Accept(Frame(1, signal));
  EXPECT_TRUE(Compose());
  signal->status = CompletionStatus::kFailed;
  runner->Poll();
  EXPECT_EQ(requests, 2);
  EXPECT_EQ(state->presents, 0);
  EXPECT_FALSE(Compose());
  EXPECT_EQ(error, PipelineError::kProducerFailure);
}
TEST_F(DisplayTest, TwoCreditsBoundSubmissions) {
  compositor->Accept(Frame(1));
  Compose();
  compositor->Accept(Frame(2));
  Compose();
  compositor->Accept(Frame(3));
  EXPECT_TRUE(Compose());
  EXPECT_EQ(state->presents, 2);
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  Compose();
  EXPECT_EQ(state->presents, 3);
}
TEST_F(DisplayTest, ExpiredDeadlineSkipsSubmission) {
  compositor->Accept(Frame());
  EXPECT_TRUE(compositor->Compose(fml::TimePoint::Now() -
                                  fml::TimeDelta::FromMilliseconds(1)));
  EXPECT_EQ(state->presents, 0);
}
TEST_F(DisplayTest, FailedProducerRetiresAllocation) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kFailed;
  auto frame = Frame(1, signal);
  auto record = frame->result->outputs[0].texture_ref->resource();
  compositor->Accept(frame);
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 0);
  frame.reset();
  EXPECT_FALSE(record->CanReuse());
}
TEST_F(DisplayTest, OldWindowGenerationIsRejected) {
  auto frame = std::make_shared<CompositorFrame>(*Frame());
  frame->window.generation = 2;
  compositor->Accept(frame);
  Compose();
  EXPECT_EQ(state->presents, 0);
}
TEST_F(DisplayTest, StopRetainsTargetUntilGpuReadFinishes) {
  compositor->Accept(Frame());
  Compose();
  compositor->Stop();
  compositor.reset();
  EXPECT_FALSE(state->destroyed);
  EXPECT_TRUE(state->detached);
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(state->destroyed);
  EXPECT_EQ(state->imports_released, 1);
}
TEST_F(DisplayTest, NoCommandRetryCancelsResourceRead) {
  auto frame = Frame();
  auto record = frame->result->outputs[0].texture_ref->resource();
  state->retry = true;
  compositor->Accept(frame);
  EXPECT_TRUE(Compose());
  compositor->Stop();
  frame.reset();
  EXPECT_TRUE(record->CanReuse());
}
TEST_F(DisplayTest, RetryCoalescesAsynchronousRequestWithoutPresenting) {
  auto frame = Frame();
  state->retry = true;
  compositor->Accept(frame);
  EXPECT_EQ(requests, 1);
  EXPECT_TRUE(Compose());
  EXPECT_TRUE(Compose());
  ASSERT_EQ(runner->polls.size(), 1u);
  EXPECT_EQ(requests, 1);
  EXPECT_EQ(state->presents, 0);
  EXPECT_TRUE(state->reads.empty());
  EXPECT_EQ(callbacks, 0);

  state->retry = false;
  runner->Poll();
  EXPECT_EQ(requests, 2);
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(state->presents, 0);
  EXPECT_TRUE(state->reads.empty());
  EXPECT_EQ(callbacks, 0);
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(frame);
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, RetryPollRequestsOpportunityForLatestCandidate) {
  state->retry = true;
  compositor->Accept(Frame(1));
  EXPECT_TRUE(Compose());
  ASSERT_EQ(runner->polls.size(), 1u);
  auto latest = Frame(2);
  compositor->Accept(latest);
  EXPECT_EQ(requests, 2);

  state->retry = false;
  runner->Poll();
  EXPECT_EQ(requests, 3);
  EXPECT_EQ(state->presents, 0);
  EXPECT_FALSE(compositor->current());
  EXPECT_FALSE(Compose());
  ExpectCurrentFrame(latest);
  EXPECT_EQ(state->presents, 1);
}
TEST_F(DisplayTest, RetryPollStopsWithoutWakeOrSubmissionAfterStop) {
  state->retry = true;
  compositor->Accept(Frame());
  EXPECT_TRUE(Compose());
  ASSERT_EQ(runner->polls.size(), 1u);
  compositor->Stop();
  EXPECT_TRUE(state->detached);

  state->retry = false;
  runner->Poll();
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(requests, 1);
  EXPECT_EQ(state->presents, 0);
  EXPECT_TRUE(state->reads.empty());
  EXPECT_EQ(callbacks, 0);
  EXPECT_FALSE(Compose());
  EXPECT_FALSE(compositor->current());
}
TEST_F(DisplayTest, BackgroundRetainsCandidateWithoutSubmission) {
  compositor->SetEnabled(false);
  compositor->Accept(Frame());
  EXPECT_FALSE(Compose());
  EXPECT_EQ(state->presents, 0);
  compositor->SetEnabled(true);
  Compose();
  EXPECT_EQ(callbacks, 1);
}
TEST_F(DisplayTest, RepeatedFrameSamplingRegistersIndependentGpuUses) {
  auto frame = Frame();
  auto record = frame->result->outputs[0].texture_ref->resource();
  compositor->Accept(frame);
  Compose();
  compositor->Accept(frame);
  Compose();
  compositor->Stop();
  frame.reset();
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_FALSE(record->CanReuse());
  state->reads[1]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(record->CanReuse());
}
TEST_F(DisplayTest, FailedSwapKeepsGpuLeasesUntilCompletionAndStopsWork) {
  auto frame = Frame();
  auto record = frame->result->outputs[0].texture_ref->resource();
  state->fail_swap = true;
  compositor->Accept(frame);
  Compose();
  EXPECT_EQ(error, PipelineError::kDisplayFailure);
  EXPECT_FALSE(compositor->current());
  EXPECT_EQ(callbacks, 0);
  frame.reset();
  EXPECT_FALSE(record->CanReuse());
  compositor->Accept(Frame(2));
  Compose();
  EXPECT_EQ(state->presents, 1);
  state->reads[0]->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(record->CanReuse());
}
TEST_F(DisplayTest, OlderCompletionFailureDoesNotClearTheNewerOutputSnapshot) {
  compositor->Accept(Frame());
  ASSERT_FALSE(Compose());
  auto next = Frame(2);
  compositor->Accept(next);
  ASSERT_FALSE(Compose());
  state->reads[0]->status = CompletionStatus::kFailed;
  runner->Poll();
  EXPECT_EQ(error, PipelineError::kDisplayFailure);
  ExpectCurrentFrame(next);
}
TEST_F(DisplayTest, FailedReadRetiresStorageAndClearsCurrent) {
  auto frame = Frame();
  auto record = frame->result->outputs[0].texture_ref->resource();
  compositor->Accept(frame);
  Compose();
  state->reads[0]->status = CompletionStatus::kFailed;
  runner->Poll();
  EXPECT_EQ(error, PipelineError::kDisplayFailure);
  EXPECT_FALSE(compositor->current());
  frame.reset();
  EXPECT_FALSE(record->CanReuse());
  EXPECT_TRUE(record->CanDestroy());
}
}  // namespace clay::display
