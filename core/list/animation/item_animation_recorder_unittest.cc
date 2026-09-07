// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <functional>
#include <unordered_map>
#include <utility>

#include "core/list/animation/animation_target.h"
#include "core/list/animation/animation_types.h"

#define private public
#include "core/list/animation/item_animation_recorder.h"
#undef private

#include "core/list/testing/mock_animation_target.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

enum class ProcessType {
  kDisappeared,
  kAppeared,
  kPersistent,
  kChanged,
  kNone,
};

struct ProcessResult {
  ProcessType type{ProcessType::kNone};
  ItemLayoutInfo pre_layout_info;
  ItemLayoutInfo post_layout_info;
};

class RecordingProcessCallback final
    : public ItemAnimationRecorder::ProcessCallback {
 public:
  using OnProcessed = std::function<void(AnimationTarget*, ProcessType)>;

  void ProcessDisappeared(AnimationTarget* target,
                          const ItemLayoutInfo& pre_layout_info) override {
    Record(target, ProcessType::kDisappeared, pre_layout_info, {});
  }

  void ProcessAppeared(AnimationTarget* target,
                       const ItemLayoutInfo& post_layout_info) override {
    Record(target, ProcessType::kAppeared, {}, post_layout_info);
  }

  void ProcessPersistent(AnimationTarget* target,
                         const ItemLayoutInfo& pre_layout_info,
                         const ItemLayoutInfo& post_layout_info) override {
    Record(target, ProcessType::kPersistent, pre_layout_info, post_layout_info);
  }

  void ProcessChanged(AnimationTarget* target,
                      const ItemLayoutInfo& pre_layout_info,
                      const ItemLayoutInfo& post_layout_info) override {
    Record(target, ProcessType::kChanged, pre_layout_info, post_layout_info);
  }

  void ProcessNone(AnimationTarget* target) override {
    Record(target, ProcessType::kNone, {}, {});
  }

  void Clear() { results.clear(); }

  std::unordered_map<AnimationTarget*, ProcessResult> results;
  OnProcessed on_processed;

 private:
  void Record(AnimationTarget* target, ProcessType type,
              ItemLayoutInfo pre_layout_info, ItemLayoutInfo post_layout_info) {
    results.insert_or_assign(target,
                             ProcessResult{type, std::move(pre_layout_info),
                                           std::move(post_layout_info)});
    if (on_processed) {
      on_processed(target, type);
    }
  }
};

// Verifies that the recorder stores and updates PRE and POST layout snapshots
// by target address.
TEST(ItemAnimationRecorderTest, RecordsAndQueriesSnapshots) {
  ItemAnimationRecorder recorder;
  MockAnimationTarget first_target("same-key");
  MockAnimationTarget second_target("same-key");
  ItemLayoutInfo first_pre{0.f, 0.f, 10.f, 10.f};
  ItemLayoutInfo latest_first_pre{20.f, 30.f, 30.f, 40.f};
  ItemLayoutInfo first_post{40.f, 50.f, 50.f, 60.f};

  // 1. A new recorder contains no target layout information.
  EXPECT_TRUE(recorder.records_.empty());

  // 2. Recording PRE repeatedly for the same target updates the snapshot
  // without adding another record.
  recorder.RecordPreItemLayoutInfo(&first_target, first_pre);
  recorder.RecordPreItemLayoutInfo(&first_target, latest_first_pre);
  ASSERT_EQ(recorder.records_.size(), 1u);
  auto first_iter = recorder.records_.find(&first_target);
  ASSERT_NE(first_iter, recorder.records_.end());
  EXPECT_TRUE(first_iter->second.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_FALSE(first_iter->second.HasFlag(ItemAnimationRecord::kPost));
  EXPECT_FALSE(first_iter->second.HasFlag(ItemAnimationRecord::kChanged));
  EXPECT_FLOAT_EQ(first_iter->second.pre_info.left_, 20.f);
  EXPECT_FLOAT_EQ(first_iter->second.pre_info.top_, 30.f);

  // 3. The POST snapshot is merged into the record that already contains PRE.
  recorder.RecordPostLayoutInfo(&first_target, first_post);
  ASSERT_EQ(recorder.records_.size(), 1u);
  EXPECT_TRUE(first_iter->second.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_TRUE(first_iter->second.HasFlag(ItemAnimationRecord::kPost));
  EXPECT_FLOAT_EQ(first_iter->second.post_info.left_, 40.f);
  EXPECT_FLOAT_EQ(first_iter->second.post_info.top_, 50.f);

  // 4. The recorder distinguishes targets by address, so equal item keys do not
  // merge different objects.
  recorder.RecordPreItemLayoutInfo(&second_target, first_pre);
  EXPECT_EQ(recorder.records_.size(), 2u);
  EXPECT_NE(recorder.records_.find(&first_target), recorder.records_.end());
  EXPECT_NE(recorder.records_.find(&second_target), recorder.records_.end());
}

// Verifies that Process() dispatches Disappeared, Appeared, and Persistent
// callbacks according to each PRE/POST snapshot combination.
TEST(ItemAnimationRecorderTest, ProcessesPrePostCombinations) {
  ItemAnimationRecorder recorder;
  RecordingProcessCallback callback;
  MockAnimationTarget disappeared_target("disappeared");
  MockAnimationTarget appeared_target("appeared");
  MockAnimationTarget persistent_target("persistent");
  ItemLayoutInfo pre_info{0.f, 0.f, 10.f, 10.f};
  ItemLayoutInfo post_info{20.f, 30.f, 30.f, 40.f};
  ItemLayoutInfo unchanged_post_info = pre_info;

  // 1. Create PRE-only, POST-only, and combined PRE/POST records.
  recorder.RecordPreItemLayoutInfo(&disappeared_target, pre_info);
  recorder.RecordPostLayoutInfo(&appeared_target, post_info);
  recorder.RecordPreItemLayoutInfo(&persistent_target, pre_info);
  recorder.RecordPostLayoutInfo(&persistent_target, unchanged_post_info);

  // 2. Process() consumes the records present at entry. Query callback results
  // by target rather than depending on unordered_map iteration order.
  recorder.Process(callback);
  ASSERT_EQ(callback.results.size(), 3u);
  ASSERT_NE(callback.results.find(&disappeared_target), callback.results.end());
  ASSERT_NE(callback.results.find(&appeared_target), callback.results.end());
  ASSERT_NE(callback.results.find(&persistent_target), callback.results.end());
  EXPECT_EQ(callback.results.at(&disappeared_target).type,
            ProcessType::kDisappeared);
  EXPECT_EQ(callback.results.at(&appeared_target).type, ProcessType::kAppeared);
  EXPECT_EQ(callback.results.at(&persistent_target).type,
            ProcessType::kPersistent);

  // 3. Each callback receives the corresponding layout-stage snapshot, and no
  // records remain after Process() when callbacks do not add any.
  EXPECT_FLOAT_EQ(
      callback.results.at(&disappeared_target).pre_layout_info.right_, 10.f);
  EXPECT_FLOAT_EQ(callback.results.at(&appeared_target).post_layout_info.left_,
                  20.f);
  EXPECT_FLOAT_EQ(
      callback.results.at(&persistent_target).pre_layout_info.right_, 10.f);
  EXPECT_FLOAT_EQ(
      callback.results.at(&persistent_target).post_layout_info.right_, 10.f);
  // A PRE+POST record without kChanged is always dispatched as Persistent.
  // ItemAnimator decides from the layout positions whether a move animation is
  // actually needed.
  EXPECT_FALSE(
      callback.results.at(&persistent_target)
          .pre_layout_info.PositionChanged(
              callback.results.at(&persistent_target).post_layout_info));
  EXPECT_TRUE(recorder.records_.empty());
}

// Verifies that a Changed flag dispatches ProcessChanged only when both PRE and
// POST snapshots are present.
TEST(ItemAnimationRecorderTest, ProcessesChangedAndNoneRecords) {
  ItemAnimationRecorder recorder;
  RecordingProcessCallback callback;
  MockAnimationTarget changed_target("changed");
  MockAnimationTarget changed_only_target("changed-only");
  ItemLayoutInfo pre_info{0.f, 0.f, 10.f, 10.f};
  ItemLayoutInfo post_info{20.f, 30.f, 30.f, 40.f};

  // 1. Give the first target PRE, POST, and Changed flags.
  recorder.RecordPreItemLayoutInfo(&changed_target, pre_info);
  recorder.RecordPostLayoutInfo(&changed_target, post_info);
  recorder.GetOrCreateRecord(&changed_target)
      .AddFlag(ItemAnimationRecord::kChanged);
  EXPECT_TRUE(recorder.records_.at(&changed_target)
                  .HasFlag(ItemAnimationRecord::kChanged));

  // 2. Give the second target only a Changed flag and no layout snapshot.
  ItemAnimationRecord& changed_only_record =
      recorder.GetOrCreateRecord(&changed_only_target);
  changed_only_record.AddFlag(ItemAnimationRecord::kChanged);
  EXPECT_TRUE(changed_only_record.HasFlag(ItemAnimationRecord::kChanged));
  EXPECT_FALSE(changed_only_record.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_FALSE(changed_only_record.HasFlag(ItemAnimationRecord::kPost));

  // 3. The complete record dispatches Changed, while the record without PRE and
  // POST dispatches None.
  recorder.Process(callback);
  ASSERT_EQ(callback.results.size(), 2u);
  EXPECT_EQ(callback.results.at(&changed_target).type, ProcessType::kChanged);
  EXPECT_EQ(callback.results.at(&changed_only_target).type, ProcessType::kNone);
  EXPECT_FLOAT_EQ(callback.results.at(&changed_target).pre_layout_info.right_,
                  10.f);
  EXPECT_FLOAT_EQ(callback.results.at(&changed_target).post_layout_info.left_,
                  20.f);
  EXPECT_TRUE(recorder.records_.empty());
}

// Verifies that a record added reentrantly by a Process() callback remains for
// the next pass. Process() swaps records_ into the local processing_records map
// at entry and iterates only that map. A POST record added by a callback enters
// the now-empty records_, so it neither joins the current pass nor gets removed
// when that pass ends.
TEST(ItemAnimationRecorderTest, KeepsReentrantRecordsForNextProcess) {
  ItemAnimationRecorder recorder;
  RecordingProcessCallback callback;
  MockAnimationTarget target("item");
  ItemLayoutInfo pre_info{0.f, 0.f, 10.f, 10.f};
  ItemLayoutInfo post_info{20.f, 30.f, 30.f, 40.f};
  recorder.RecordPreItemLayoutInfo(&target, pre_info);

  // 1. While processing a PRE-only record, add a new POST record for the same
  // target from the callback.
  bool has_recorded_post = false;
  callback.on_processed = [&](AnimationTarget* processed_target,
                              ProcessType type) {
    if (!has_recorded_post && processed_target == &target &&
        type == ProcessType::kDisappeared) {
      has_recorded_post = true;
      recorder.RecordPostLayoutInfo(&target, post_info);
    }
  };
  recorder.Process(callback);

  // 2. The first Process() consumes only the original PRE record. The POST
  // record added by the callback remains stored.
  ASSERT_EQ(callback.results.size(), 1u);
  EXPECT_EQ(callback.results.at(&target).type, ProcessType::kDisappeared);
  ASSERT_EQ(recorder.records_.size(), 1u);
  auto next_iter = recorder.records_.find(&target);
  ASSERT_NE(next_iter, recorder.records_.end());
  EXPECT_FALSE(next_iter->second.HasFlag(ItemAnimationRecord::kPre));
  EXPECT_TRUE(next_iter->second.HasFlag(ItemAnimationRecord::kPost));

  // 3. The next Process() consumes the POST record added by the callback.
  callback.Clear();
  callback.on_processed = nullptr;
  recorder.Process(callback);
  ASSERT_EQ(callback.results.size(), 1u);
  EXPECT_EQ(callback.results.at(&target).type, ProcessType::kAppeared);
  EXPECT_FLOAT_EQ(callback.results.at(&target).post_layout_info.left_, 20.f);
  EXPECT_TRUE(recorder.records_.empty());
}

}  // namespace
}  // namespace list
}  // namespace lynx
