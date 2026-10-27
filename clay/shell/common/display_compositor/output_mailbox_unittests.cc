// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/output_mailbox.h"
#include "clay/shell/common/display_compositor/test_support.h"
#include "gtest/gtest.h"

namespace clay::display {
using testing_support::Record;
using testing_support::Signal;

TEST(OutputMailbox, SubmittedPendingWriteTransfersWithoutCpuReadinessQuery) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto texture = std::make_shared<OutputTexture>(Record(signal), 1);
  ASSERT_TRUE(mailbox.Publish(generation, texture));
  auto host = mailbox.TakeLatest();
  ASSERT_TRUE(host.texture);
  EXPECT_EQ(host.texture, texture);
  EXPECT_TRUE(host.open);
  EXPECT_EQ(host.generation, generation);
  EXPECT_EQ(signal->polls, 0u);
  EXPECT_TRUE(host.texture->ready()->EnqueueWait());
  EXPECT_EQ(signal->status, CompletionStatus::kPending);
  EXPECT_FALSE(mailbox.TakeLatest().texture);
}

TEST(OutputMailbox, MissingSubmissionDependencyCannotReachHost) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  EXPECT_FALSE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(Record(nullptr), 1)));
  EXPECT_FALSE(mailbox.TakeLatest().texture);
}

TEST(OutputMailbox, LateCompletionCannotReplaceNewerOutput) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto newer = std::make_shared<OutputTexture>(Record(), 2);
  EXPECT_TRUE(mailbox.Publish(generation, newer));
  EXPECT_FALSE(mailbox.Publish(generation,
                               std::make_shared<OutputTexture>(Record(), 1)));
  EXPECT_EQ(mailbox.TakeLatest().texture, newer);
  EXPECT_FALSE(mailbox.TakeLatest().texture);
}

TEST(OutputMailbox, DetachRejectsPublicationAndReattachRejectsOldGeneration) {
  OutputMailbox mailbox;
  auto old_generation = mailbox.Open();
  auto texture = std::make_shared<OutputTexture>(Record(), 1);
  EXPECT_TRUE(mailbox.Publish(old_generation, texture));
  mailbox.Close(old_generation);
  auto closed = mailbox.TakeLatest();
  EXPECT_FALSE(closed.texture);
  EXPECT_FALSE(closed.open);
  EXPECT_EQ(closed.generation, old_generation);
  EXPECT_FALSE(mailbox.Publish(old_generation, texture));
  auto generation = mailbox.Open();
  EXPECT_GT(generation, old_generation);
  EXPECT_FALSE(mailbox.Publish(old_generation, texture));
  EXPECT_TRUE(mailbox.Publish(generation, texture));
  mailbox.Close(old_generation);
  auto host = mailbox.TakeLatest();
  EXPECT_EQ(host.texture, texture);
  EXPECT_TRUE(host.open);
  EXPECT_EQ(host.generation, generation);
}

TEST(OutputMailbox, ResizeRevokesOldSizeWithoutReleasingHostLease) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto record = Record();
  EXPECT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 1)));
  auto host = mailbox.TakeLatest().texture;
  mailbox.Resize(200, 100);
  auto resized = mailbox.TakeLatest();
  EXPECT_FALSE(resized.texture);
  EXPECT_TRUE(resized.open);
  EXPECT_EQ(resized.generation, generation);
  EXPECT_FALSE(mailbox.Publish(generation, host));
  EXPECT_FALSE(record->CanReuse());
  host.reset();
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, HostGpuReadsOutliveMailboxAndCpuLease) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto record = Record();
  ASSERT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 1)));
  auto host = mailbox.TakeLatest().texture;
  ASSERT_TRUE(host);
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  record->EndRead(record->BeginRead(), first);
  record->EndRead(record->BeginRead(), second);
  mailbox.Close(generation);
  host.reset();
  EXPECT_FALSE(record->CanDestroy());
  first->status = CompletionStatus::kComplete;
  EXPECT_FALSE(record->CanDestroy());
  second->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, ResizeBackToOriginalSizeStillRejectsOldPublication) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  mailbox.Resize(100, 200);
  auto old_size_generation = mailbox.size_generation();
  ASSERT_TRUE(mailbox.Publish(generation,
                              std::make_shared<OutputTexture>(Record(), 1),
                              old_size_generation));
  auto host = mailbox.TakeLatest();
  ASSERT_TRUE(host.texture);
  mailbox.Resize(200, 100);
  mailbox.Resize(100, 200);
  auto resized = mailbox.TakeLatest();
  EXPECT_FALSE(resized.texture);
  EXPECT_TRUE(resized.open);
  EXPECT_EQ(resized.generation, host.generation);
  EXPECT_GT(resized.size_generation, host.size_generation);
  EXPECT_EQ(resized.size_generation, mailbox.size_generation());
  auto texture = std::make_shared<OutputTexture>(Record(), 2);
  EXPECT_FALSE(mailbox.Publish(generation, texture, old_size_generation));
  EXPECT_TRUE(mailbox.Publish(generation, texture, resized.size_generation));
}

TEST(OutputMailbox, EmptySnapshotDistinguishesOpenAndClosedMailbox) {
  OutputMailbox mailbox;
  auto initial = mailbox.TakeLatest();
  EXPECT_FALSE(initial.texture);
  EXPECT_FALSE(initial.open);
  auto generation = mailbox.Open();
  auto open = mailbox.TakeLatest();
  EXPECT_FALSE(open.texture);
  EXPECT_TRUE(open.open);
  EXPECT_EQ(open.generation, generation);
  mailbox.Close(generation);
  auto closed = mailbox.TakeLatest();
  EXPECT_FALSE(closed.texture);
  EXPECT_FALSE(closed.open);
  EXPECT_EQ(closed.generation, open.generation);
  EXPECT_EQ(closed.size_generation, open.size_generation);
}

TEST(OutputMailbox, TakeLatestTransfersLeaseWithoutRetainingConsumedOutput) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto record = Record();
  auto texture = std::make_shared<OutputTexture>(record, 1);
  std::weak_ptr<const OutputTexture> weak = texture;
  ASSERT_TRUE(mailbox.Publish(generation, texture));
  texture.reset();
  auto host = mailbox.TakeLatest();
  ASSERT_TRUE(host.texture);
  EXPECT_FALSE(mailbox.TakeLatest().texture);
  EXPECT_FALSE(weak.expired());
  EXPECT_FALSE(record->CanReuse());
  host.texture.reset();
  EXPECT_TRUE(weak.expired());
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, NewPendingOutputReleasesReplacedLease) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto old_record = Record();
  auto old = std::make_shared<OutputTexture>(old_record, 1);
  std::weak_ptr<const OutputTexture> weak = old;
  ASSERT_TRUE(mailbox.Publish(generation, old));
  old.reset();
  EXPECT_FALSE(old_record->CanReuse());
  auto latest = std::make_shared<OutputTexture>(Record(), 2);
  ASSERT_TRUE(mailbox.Publish(generation, latest));
  EXPECT_TRUE(weak.expired());
  EXPECT_TRUE(old_record->CanReuse());
  EXPECT_EQ(mailbox.TakeLatest().texture, latest);
}

TEST(OutputMailbox, ConsumedVersionRejectsLateAndDuplicatePublication) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  ASSERT_TRUE(mailbox.Publish(generation,
                              std::make_shared<OutputTexture>(Record(), 5)));
  ASSERT_TRUE(mailbox.TakeLatest().texture);
  EXPECT_FALSE(mailbox.Publish(generation,
                               std::make_shared<OutputTexture>(Record(), 4)));
  EXPECT_FALSE(mailbox.Publish(generation,
                               std::make_shared<OutputTexture>(Record(), 5)));
  EXPECT_FALSE(mailbox.TakeLatest().texture);
  ASSERT_TRUE(mailbox.Publish(generation,
                              std::make_shared<OutputTexture>(Record(), 6)));
  auto latest = mailbox.TakeLatest();
  ASSERT_TRUE(latest.texture);
  EXPECT_EQ(latest.texture->content_version(), 6u);
}

TEST(OutputMailbox, ResizePreservesAcceptedVersionWatermark) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  mailbox.Resize(100, 200);
  ASSERT_TRUE(mailbox.Publish(generation,
                              std::make_shared<OutputTexture>(Record(), 10)));
  ASSERT_TRUE(mailbox.TakeLatest().texture);
  mailbox.Resize(200, 100);
  mailbox.Resize(100, 200);
  auto size_generation = mailbox.size_generation();
  EXPECT_FALSE(mailbox.Publish(generation,
                               std::make_shared<OutputTexture>(Record(), 9),
                               size_generation));
  EXPECT_FALSE(mailbox.Publish(generation,
                               std::make_shared<OutputTexture>(Record(), 10),
                               size_generation));
  EXPECT_TRUE(mailbox.Publish(generation,
                              std::make_shared<OutputTexture>(Record(), 11),
                              size_generation));
}

TEST(OutputMailbox, UnchangedResizePreservesPendingOutputAndSizeEpoch) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  mailbox.Resize(100, 200);
  auto size_generation = mailbox.size_generation();
  auto texture = std::make_shared<OutputTexture>(Record(), 1);
  ASSERT_TRUE(mailbox.Publish(generation, texture, size_generation));
  mailbox.Resize(100, 200);
  auto host = mailbox.TakeLatest();
  EXPECT_EQ(host.texture, texture);
  EXPECT_EQ(host.size_generation, size_generation);
}

TEST(OutputMailbox, ChangedResizeDropsPendingOutputEvenWhenItsSizeFits) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto record = Record();
  ASSERT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 1)));
  auto size_generation = mailbox.size_generation();
  mailbox.Resize(100, 200);
  auto resized = mailbox.TakeLatest();
  EXPECT_FALSE(resized.texture);
  EXPECT_GT(resized.size_generation, size_generation);
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, ResizeReleasesCpuLeaseButPreservesPendingGpuWrite) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto record = Record(signal);
  ASSERT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 1)));
  mailbox.Resize(200, 100);
  EXPECT_FALSE(mailbox.TakeLatest().texture);
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
  signal->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, OpenDropsPendingOutputAndResetsAcceptedVersion) {
  OutputMailbox mailbox;
  auto old_generation = mailbox.Open();
  auto record = Record();
  ASSERT_TRUE(mailbox.Publish(old_generation,
                              std::make_shared<OutputTexture>(record, 10)));
  auto generation = mailbox.Open();
  EXPECT_GT(generation, old_generation);
  EXPECT_TRUE(record->CanReuse());
  EXPECT_FALSE(mailbox.TakeLatest().texture);
  EXPECT_FALSE(mailbox.Publish(old_generation,
                               std::make_shared<OutputTexture>(Record(), 100)));
  auto first = std::make_shared<OutputTexture>(Record(), 1);
  EXPECT_TRUE(mailbox.Publish(generation, first));
  EXPECT_EQ(mailbox.TakeLatest().texture, first);
}

TEST(OutputMailbox, PendingReplacementDoesNotRevokeHostCurrentLease) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto record = Record();
  ASSERT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 1)));
  auto current = mailbox.TakeLatest().texture;
  ASSERT_TRUE(current);
  auto next_write = std::make_shared<Signal>();
  next_write->status = CompletionStatus::kPending;
  auto next = std::make_shared<OutputTexture>(Record(next_write), 2);
  ASSERT_TRUE(mailbox.Publish(generation, next));
  EXPECT_FALSE(record->CanReuse());
  EXPECT_EQ(current->content_version(), 1u);
  EXPECT_EQ(mailbox.TakeLatest().texture, next);
  EXPECT_FALSE(record->CanReuse());
  current.reset();
  EXPECT_TRUE(record->CanReuse());
}

TEST(OutputMailbox, SubmittedReplacementAdvancesPendingBeforeGpuCompletion) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto current_record = Record();
  auto pending_record = Record();
  auto writing_record = Record();
  ASSERT_TRUE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(current_record, 1)));
  auto current = mailbox.TakeLatest().texture;
  ASSERT_TRUE(current);
  auto host_read = current_record->BeginRead();
  ASSERT_NE(host_read, 0u);
  auto host_completion = std::make_shared<Signal>();
  host_completion->status = CompletionStatus::kPending;
  current_record->EndRead(host_read, host_completion);
  ASSERT_TRUE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(pending_record, 2)));

  // A is the host's current read, B is unconsumed, and C is independently
  // writable. C replaces B once its GPU commands and dependency are submitted.
  EXPECT_FALSE(current_record->BeginWrite());
  EXPECT_FALSE(pending_record->BeginWrite());
  ASSERT_TRUE(writing_record->BeginWrite());
  auto write = std::make_shared<Signal>();
  write->status = CompletionStatus::kPending;
  writing_record->EndWrite(write);
  auto replacement = std::make_shared<OutputTexture>(writing_record, 3);
  ASSERT_TRUE(mailbox.Publish(generation, replacement));
  EXPECT_EQ(write->polls, 0u);
  EXPECT_FALSE(current_record->CanReuse());
  EXPECT_TRUE(pending_record->CanReuse());
  EXPECT_FALSE(writing_record->CanReuse());
  EXPECT_EQ(current->content_version(), 1u);

  auto acquired = mailbox.TakeLatest();
  ASSERT_EQ(acquired.texture, replacement);
  EXPECT_TRUE(acquired.texture->ready()->EnqueueWait());
  EXPECT_FALSE(mailbox.TakeLatest().texture);
  acquired.texture.reset();
  replacement.reset();
  EXPECT_FALSE(writing_record->CanReuse());
  write->status = CompletionStatus::kComplete;
  EXPECT_TRUE(writing_record->CanReuse());
  current.reset();
  EXPECT_FALSE(current_record->CanReuse());
  host_completion->status = CompletionStatus::kComplete;
  EXPECT_TRUE(current_record->CanReuse());
}

TEST(OutputMailbox, HostCanConsumePreviousOutputWhileNextWriteIsRecorded) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto current_record = Record();
  auto pending_record = Record();
  auto writing_record = Record();
  ASSERT_TRUE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(current_record, 1)));
  auto current = mailbox.TakeLatest().texture;
  ASSERT_TRUE(current);
  auto host_read = current_record->BeginRead();
  ASSERT_NE(host_read, 0u);
  auto host_completion = std::make_shared<Signal>();
  host_completion->status = CompletionStatus::kPending;
  current_record->EndRead(host_read, host_completion);
  ASSERT_TRUE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(pending_record, 2)));
  ASSERT_TRUE(writing_record->BeginWrite());

  auto acquired = mailbox.TakeLatest();
  ASSERT_TRUE(acquired.texture);
  EXPECT_EQ(acquired.texture->content_version(), 2u);
  current = std::move(acquired.texture);
  EXPECT_FALSE(current_record->CanReuse());
  EXPECT_FALSE(pending_record->CanReuse());
  host_completion->status = CompletionStatus::kComplete;
  EXPECT_TRUE(current_record->CanReuse());

  auto write = std::make_shared<Signal>();
  write->status = CompletionStatus::kPending;
  writing_record->EndWrite(write);
  auto replacement = std::make_shared<OutputTexture>(writing_record, 3);
  ASSERT_TRUE(mailbox.Publish(generation, replacement));
  acquired = mailbox.TakeLatest();
  ASSERT_TRUE(acquired.texture);
  EXPECT_EQ(acquired.texture, replacement);
  EXPECT_TRUE(acquired.texture->ready()->EnqueueWait());
  EXPECT_EQ(write->polls, 0u);
  current = std::move(acquired.texture);
  EXPECT_TRUE(pending_record->CanReuse());
}

TEST(OutputMailbox, FailedDependencyCannotAuthorizeHostSampling) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  auto pending = std::make_shared<OutputTexture>(Record(), 1);
  ASSERT_TRUE(mailbox.Publish(generation, pending));
  auto failed = std::make_shared<Signal>();
  failed->status = CompletionStatus::kFailed;
  auto record = Record(failed);
  ASSERT_TRUE(
      mailbox.Publish(generation, std::make_shared<OutputTexture>(record, 2)));
  auto acquired = mailbox.TakeLatest();
  ASSERT_TRUE(acquired.texture);
  EXPECT_EQ(acquired.texture->content_version(), 2u);
  EXPECT_EQ(failed->polls, 0u);
  auto use = record->BeginRead();
  ASSERT_NE(use, 0u);
  EXPECT_FALSE(acquired.texture->ready()->EnqueueWait());
  record->CancelRead(use);
  acquired.texture->Retire();
  acquired.texture.reset();
  EXPECT_FALSE(record->CanReuse());
}

TEST(OutputMailbox, ReplacementIsVisibleBeforeOldStorageIsReleasedOutsideLock) {
  OutputMailbox mailbox;
  auto generation = mailbox.Open();
  bool released = false;
  std::shared_ptr<const OutputTexture> observed;
  auto storage = std::shared_ptr<void>(new int(1), [&](void* value) {
    auto acquisition = mailbox.TakeLatest();
    EXPECT_TRUE(acquisition.open);
    observed = std::move(acquisition.texture);
    released = true;
    delete static_cast<int*>(value);
  });
  ImageSpec image;
  image.width = 100;
  image.height = 200;
  image.valid_rect = {0, 0, 100, 200};
  auto record = std::make_shared<ResourceRecord>(ResourceKey{1, 1, 1, 1}, image,
                                                 80000, std::move(storage));
  ASSERT_TRUE(record->BeginWrite());
  record->EndWrite(std::make_shared<Signal>());
  ASSERT_TRUE(mailbox.Publish(
      generation, std::make_shared<OutputTexture>(std::move(record), 1)));
  ASSERT_FALSE(released);
  auto replacement = std::make_shared<OutputTexture>(Record(), 2);
  EXPECT_TRUE(mailbox.Publish(generation, replacement));
  EXPECT_TRUE(released);
  EXPECT_EQ(observed, replacement);
}
}  // namespace clay::display
