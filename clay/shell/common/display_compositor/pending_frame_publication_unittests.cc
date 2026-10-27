// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/pending_frame_publication.h"
#include "clay/shell/common/display_compositor/test_support.h"
#include "gtest/gtest.h"

namespace clay::display {
using testing_support::Binding;
using testing_support::ManualRunner;
using testing_support::Publication;
using testing_support::Record;
using testing_support::Signal;
class PendingPublicationTest : public testing::Test {
 protected:
  std::shared_ptr<PendingFramePublication> Make(RasterPublication p) {
    return std::make_shared<PendingFramePublication>(
        std::move(p), mailbox, runner, [&] { ++wakes; },
        [&](bool success) {
          ++completions;
          published = success;
        });
  }
  fml::RefPtr<ManualRunner> runner = fml::MakeRefCounted<ManualRunner>();
  std::shared_ptr<FrameMailbox> mailbox =
      std::make_shared<FrameMailbox>(Binding());
  int wakes = 0, completions = 0;
  bool published = false;
};

TEST_F(PendingPublicationTest, ReadyOutputPublishesWithoutWaiting) {
  auto publication = Make(Publication());
  publication->Start();
  EXPECT_FALSE(publication->is_pending());
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(completions, 1);
  EXPECT_TRUE(published);
  EXPECT_EQ(wakes, 1);
  EXPECT_EQ(mailbox->TakeLatest().result->render_version, 1u);
}

TEST_F(PendingPublicationTest, NewWriteKeepsOldReadyOutputUntilCompletion) {
  auto old = Publication();
  std::weak_ptr<const OutputTexture> old_texture =
      old.result->outputs[0].texture_ref;
  ASSERT_NE(mailbox->Publish(std::move(old)),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto publication = Make(Publication(2, 1, signal));
  publication->Start();
  EXPECT_TRUE(publication->is_pending());
  EXPECT_FALSE(old_texture.expired());
  EXPECT_EQ(completions, 0);
  ASSERT_EQ(runner->polls.size(), 1u);
  runner->Poll();
  EXPECT_FALSE(old_texture.expired());
  EXPECT_EQ(completions, 0);
  ASSERT_EQ(runner->polls.size(), 1u);
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(old_texture.expired());
  EXPECT_FALSE(publication->is_pending());
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(completions, 1);
  EXPECT_TRUE(published);
  EXPECT_EQ(mailbox->TakeLatest().result->render_version, 2u);
}

TEST_F(PendingPublicationTest, ConsumerCanTakeOldOutputWhileNewWriteIsPending) {
  ASSERT_NE(mailbox->Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto publication = Make(Publication(2, 1, signal));
  publication->Start();
  auto consumer = mailbox->TakeLatest();
  ASSERT_TRUE(consumer);
  EXPECT_EQ(consumer.result->render_version, 1u);
  EXPECT_FALSE(mailbox->TakeLatest());
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(mailbox->TakeLatest().result->render_version, 2u);
  EXPECT_EQ(consumer.result->render_version, 1u);
}

TEST_F(PendingPublicationTest,
       CancellationReleasesPendingLeaseWithoutPublishing) {
  ASSERT_NE(mailbox->Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto p = Publication(2, 1, signal);
  std::weak_ptr<const OutputTexture> texture = p.result->outputs[0].texture_ref;
  auto publication = Make(std::move(p));
  publication->Start();
  publication->Cancel();
  EXPECT_FALSE(publication->is_pending());
  EXPECT_TRUE(texture.expired());
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(completions, 0);
  EXPECT_EQ(mailbox->TakeLatest().result->render_version, 1u);
  EXPECT_NE(mailbox->Publish(Publication(2)),
            FrameMailbox::PublishResult::kRejected);
}

TEST_F(PendingPublicationTest, FailedWriteReportsErrorAndKeepsReadyOutput) {
  ASSERT_NE(mailbox->Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto publication = Make(Publication(2, 1, signal));
  publication->Start();
  signal->status = CompletionStatus::kFailed;
  runner->Poll();
  EXPECT_FALSE(publication->is_pending());
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_EQ(completions, 1);
  EXPECT_FALSE(published);
  EXPECT_EQ(mailbox->TakeFailure(), PipelineError::kProducerFailure);
  EXPECT_EQ(mailbox->TakeLatest().result->render_version, 1u);
}

TEST_F(PendingPublicationTest, ClosedMailboxRejectsLateCompletion) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto publication = Make(Publication(1, 1, signal));
  publication->Start();
  mailbox->Close();
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_FALSE(publication->is_pending());
  EXPECT_FALSE(mailbox->TakeLatest());
  EXPECT_EQ(wakes, 0);
  EXPECT_FALSE(published);
}

TEST_F(PendingPublicationTest, AllSegmentsBecomeReadyBeforeAtomicReplacement) {
  ASSERT_NE(mailbox->Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto p = Publication(2);
  auto scene = std::make_shared<PageSceneSnapshot>(*p.scene);
  scene->ordered_segments.push_back({2, {}});
  auto result = std::make_shared<RasterResult>(*p.result);
  result->outputs.push_back(
      {2, 2, std::make_shared<OutputTexture>(Record(signal), 2), {}, {}});
  auto publication = Make({scene, result});
  publication->Start();
  EXPECT_TRUE(publication->is_pending());
  EXPECT_EQ(completions, 0);
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_TRUE(published);
  auto latest = mailbox->TakeLatest();
  ASSERT_TRUE(latest);
  EXPECT_EQ(latest.result->render_version, 2u);
  EXPECT_EQ(latest.result->outputs.size(), 2u);
}
}  // namespace clay::display
