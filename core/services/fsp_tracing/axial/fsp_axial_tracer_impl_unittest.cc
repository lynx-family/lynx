// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public
#include "core/services/fsp_tracing/axial/fsp_axial_tracer_impl.h"
#undef protected
#undef private

#include "base/include/geometry/point.h"
#include "base/include/geometry/size.h"
#include "gtest/gtest.h"
#include "core/services/fsp_tracing/base/fsp_snapshot.h"
#include "core/services/fsp_tracing/fsp_config.h"
#include "core/services/fsp_tracing/fsp_tracer.h"

namespace lynx {
namespace tasm {
namespace timing {

class FSPAxialTracerImplTest : public testing::Test {
 public:
  FSPAxialTracerImplTest() {
    tracer_ =
        std::make_unique<FSPAxialTracer>(FSPAxialConfig(), [](FSPSnapshot&) {});
  }

 protected:
  void FillRect(FSPSnapshot& snapshot,
                const lynx::base::geometry::IntRect& rect,
                double timestamp = 0.001, uint64_t ui_sign = 123) {
    FSPContentInfo info;
    info.rect = rect;
    info.change_timestamp = timestamp;
    info.ui_sign = ui_sign;
    tracer_->FillSnapshotImpl(snapshot, info);
  }

  std::unique_ptr<FSPAxialTracer> tracer_;
};

TEST_F(FSPAxialTracerImplTest, FillSnapshotImpl_Single) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAxialSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(100, 100);

  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {20, 20}), 1.0,
           123);

  EXPECT_EQ(snapshot.last_ui_sign, 123);
  EXPECT_EQ(snapshot.last_change_timestamp, 1.0);

  size_t expected_x_start = 10 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_x_end = 30 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_y_start = 10 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;
  size_t expected_y_end = 30 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;

  for (size_t i = 0; i < FSPAxialSnapshot::X_PROJECTIONS_LEN; ++i) {
    EXPECT_EQ(snapshot.x_projections[i],
              i >= expected_x_start && i <= expected_x_end);
  }
  for (size_t i = 0; i < FSPAxialSnapshot::Y_PROJECTIONS_LEN; ++i) {
    EXPECT_EQ(snapshot.y_projections[i],
              i >= expected_y_start && i <= expected_y_end);
  }
}

TEST_F(FSPAxialTracerImplTest, FillSnapshotImpl_MultipleOverlapping) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAxialSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(100, 100);

  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {20, 20}), 1.0,
           123);
  FillRect(snapshot, lynx::base::geometry::IntRect({25, 25}, {20, 20}), 2.0,
           124);

  EXPECT_EQ(snapshot.last_ui_sign, 124);
  EXPECT_EQ(snapshot.last_change_timestamp, 2.0);

  size_t expected_x_start1 = 10 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_x_end1 = 30 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_y_start1 = 10 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;
  size_t expected_y_end1 = 30 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;

  size_t expected_x_start2 = 25 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_x_end2 = 45 * FSPAxialSnapshot::X_PROJECTIONS_LEN / 100;
  size_t expected_y_start2 = 25 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;
  size_t expected_y_end2 = 45 * FSPAxialSnapshot::Y_PROJECTIONS_LEN / 100;

  for (size_t i = 0; i < FSPAxialSnapshot::X_PROJECTIONS_LEN; ++i) {
    bool in_range1 = i >= expected_x_start1 && i <= expected_x_end1;
    bool in_range2 = i >= expected_x_start2 && i <= expected_x_end2;
    EXPECT_EQ(snapshot.x_projections[i], in_range1 || in_range2);
  }
  for (size_t i = 0; i < FSPAxialSnapshot::Y_PROJECTIONS_LEN; ++i) {
    bool in_range1 = i >= expected_y_start1 && i <= expected_y_end1;
    bool in_range2 = i >= expected_y_start2 && i <= expected_y_end2;
    EXPECT_EQ(snapshot.y_projections[i], in_range1 || in_range2);
  }
}

TEST_F(FSPAxialTracerImplTest, FillSnapshotImpl_EmptyRect) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAxialSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(100, 100);
  auto original_x = snapshot.x_projections;
  auto original_y = snapshot.y_projections;

  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {0, 0}));

  EXPECT_EQ(snapshot.x_projections, original_x);
  EXPECT_EQ(snapshot.y_projections, original_y);
}

TEST_F(FSPAxialTracerImplTest, InspectSnapshotImpl) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAxialSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(100, 100);
  FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {100, 100}));

  std::string inspection = tracer_->InspectSnapshotImpl(snapshot, nullptr);
  EXPECT_NE(inspection.find("<Initially_Stable>"), std::string::npos);

  auto prev_snapshot_ptr = tracer_->CreateSnapshot();
  auto& prev_snapshot = static_cast<FSPAxialSnapshot&>(*prev_snapshot_ptr);
  prev_snapshot.container_size = lynx::base::geometry::IntSize(100, 100);
  FillRect(prev_snapshot, lynx::base::geometry::IntRect({0, 0}, {50, 50}), 1.0);
  snapshot.last_change_timestamp = 2.0;

  inspection = tracer_->InspectSnapshotImpl(snapshot, &prev_snapshot);
  EXPECT_EQ(inspection.find("<Initially_Stable>"), std::string::npos);
  EXPECT_NE(inspection.find("Diff:"), std::string::npos);
}

TEST_F(FSPAxialTracerImplTest, DiffAxialSnapshots_Base) {
  FSPAxialConfig config;
  config.acceptable_pixel_difference_per_ms = 2;
  config.acceptable_pixel_difference_per_snapshot = 50;
  config.minimum_completion_rate_x = 0.2;
  config.minimum_completion_rate_y = 0.2;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}),
           0.001);

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 0.002);

  // Stable if two snapshots are the same.
  std::ostringstream inspection;
  EXPECT_TRUE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                 &inspection))
      << "DiffAxialSnapshots: " << inspection.str();

  FSPAxialSnapshot incomplete;
  incomplete.container_size = {1000, 1000};
  FillRect(incomplete, lynx::base::geometry::IntRect({0, 0}, {150, 150}),
           0.002);

  // Unstable if the previous snapshot is not complete.
  inspection.str("");
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(incomplete, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots: " << inspection.str();
}

TEST_F(FSPAxialTracerImplTest, DiffAxialSnapshots_Stable) {
  FSPAxialConfig config;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {990, 990}), 1.0);

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 2.0);

  std::ostringstream inspection;
  EXPECT_TRUE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                 &inspection))
      << "DiffAxialSnapshots_Stable: " << inspection.str();
}

TEST_F(FSPAxialTracerImplTest, DiffAxialSnapshots_FailCompletionRate) {
  FSPAxialConfig config;
  config.minimum_completion_rate_x = 0.9;
  config.minimum_completion_rate_y = 0.9;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {990, 990}), 1.0);

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {800, 800}), 2.0);

  std::ostringstream inspection;
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots_FailCompletionRate: " << inspection.str();
}

TEST_F(FSPAxialTracerImplTest,
       DiffAxialSnapshots_FailAcceptablePixelDifferencePerSnapshot) {
  FSPAxialConfig config;
  config.acceptable_pixel_difference_per_snapshot = 5;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {990, 990}), 1.0);

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  config.acceptable_pixel_difference_per_snapshot = 5;
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 2.0);
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {990, 990}), 1.0);

  std::ostringstream inspection;
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots_FailAcceptablePixelDifferencePerSnapshot: "
      << inspection.str();
}

TEST_F(FSPAxialTracerImplTest, DiffAxialSnapshots_FailShortTimeInterval) {
  FSPAxialConfig config;
  config.minimum_diff_interval_ms = 2.0;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {999, 999}), 0.001);

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 0.002);

  std::ostringstream inspection;
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots_FailShortTimeInterval: " << inspection.str();
}

TEST_F(FSPAxialTracerImplTest,
       DiffAxialSnapshots_FailPixelDifferencePerMsDimension) {
  FSPAxialConfig config;
  config.acceptable_pixel_difference_per_ms = 5;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {980, 980}),
           1.0);  // 20px change in 2ms = 10px/ms

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 1.002);
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {980, 980}),
           1.0);  // 20px change in 2ms = 10px/ms

  std::ostringstream inspection;
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots_FailPixelDifferencePerMsDimension: "
      << inspection.str();
}

TEST_F(FSPAxialTracerImplTest,
       DiffAxialSnapshots_FailPixelDifferencePerMsContent) {
  FSPAxialConfig config;
  config.acceptable_pixel_difference_per_ms = 5;

  FSPAxialSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {500, 500}),
           1.0);  // Large content change

  // Prepare the previous snapshot for the first snapshot.
  FSPAxialTracer::DiffAxialSnapshots(previous, nullptr, config, nullptr);

  FSPAxialSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 1.002);

  std::ostringstream inspection;
  EXPECT_FALSE(FSPAxialTracer::DiffAxialSnapshots(current, &previous, config,
                                                  &inspection))
      << "DiffAxialSnapshots_FailPixelDifferencePerMsContent: "
      << inspection.str();
}

TEST_F(FSPAxialTracerImplTest, CaptureSnapshot) {
  // Config
  FSPAxialConfig config;
  config.minimum_diff_interval_ms = 1.0;
  config.minimum_completion_rate_x = 0.2;
  config.minimum_completion_rate_y = 0.2;
  config.acceptable_pixel_difference_per_snapshot = 50;
  config.acceptable_pixel_difference_per_ms = 2;  // Threshold to be crossed
  tracer_->config_ = config;

  // Step 1: Initial stable snapshot
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {201, 201}),
                   0.001, 123);
  };

  auto result1 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_TRUE(result1.is_stable)
      << "First snapshot should be stable." << std::endl
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 2: Unstable due to large content change
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {500, 500}),
                   0.002, 123);
  };
  auto result2 = tracer_->CaptureSnapshot({1000, 1000});

  EXPECT_FALSE(result2.is_stable)
      << "Second snapshot should be unstable." << std::endl
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 3: Stable with no content change
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {500, 500}),
                   0.003, 123);
  };
  auto result3 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_TRUE(result3.is_stable)
      << "Third snapshot should be stable again." << std::endl
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 4: Unstable with less than completion rate content
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {1, 1}),
                   0.004, 123);
  };

  tracer_->CaptureSnapshot({1000, 1000});
  auto result4 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_FALSE(result4.is_stable)
      << "Fourth snapshot should be unstable." << std::endl
      << "Inspection: " << tracer_->InspectCurrentSnapshot();
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
