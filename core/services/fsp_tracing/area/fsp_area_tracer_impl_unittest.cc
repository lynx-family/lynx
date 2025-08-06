// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public
#include "core/services/fsp_tracing/area/fsp_area_tracer_impl.h"
#undef protected
#undef private

#include "base/include/geometry/point.h"
#include "base/include/geometry/size.h"
#include "core/services/fsp_tracing/base/fsp_snapshot.h"
#include "core/services/fsp_tracing/fsp_config.h"
#include "core/services/fsp_tracing/fsp_tracer.h"
#include "gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace timing {

class FSPAreaTracerImplTest : public testing::Test {
 public:
  FSPAreaTracerImplTest() {
    tracer_ =
        std::make_unique<FSPAreaTracer>(FSPAreaConfig(), [](FSPSnapshot&) {});
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

  std::unique_ptr<FSPAreaTracer> tracer_;
};

TEST_F(FSPAreaTracerImplTest, FillSnapshotImpl_Single) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAreaSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(1000, 1000);

  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {200, 300}), 1.0,
           123);

  EXPECT_EQ(snapshot.last_ui_sign, 123);
  EXPECT_EQ(snapshot.last_change_timestamp, 1.0);

  // Prepare the snapshot for the first fill.
  std::ostringstream inspection;
  FSPAreaTracer::DiffAreaSnapshots(snapshot, nullptr, FSPAreaConfig(),
                                   &inspection);

  auto expected_area = 200 * 300;
  EXPECT_LE(
      std::abs(static_cast<double>(snapshot.projection_area - expected_area) /
               static_cast<double>(expected_area)),
      0.05)
      << std::endl
      << "Expected area: " << expected_area << std::endl
      << "Snapshot area: " << snapshot.projection_area << std::endl
      << "Difference: "
      << std::abs(
             static_cast<double>(snapshot.projection_area - expected_area) /
             static_cast<double>(expected_area))
      << std::endl
      << "Inspection: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, FillSnapshotImpl_Multiple) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAreaSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(1000, 1000);

  auto rect1 = lynx::base::geometry::IntRect({10, 10}, {200, 300});
  auto rect2 = lynx::base::geometry::IntRect({50, 50}, {300, 400});
  FillRect(snapshot, rect1, 0.001, 123);
  FillRect(snapshot, rect2, 0.002, 124);

  EXPECT_EQ(snapshot.last_ui_sign, 124);
  EXPECT_EQ(snapshot.last_change_timestamp, 0.002);

  // Prepare the snapshot for the first fill.
  std::ostringstream inspection;
  FSPAreaTracer::DiffAreaSnapshots(snapshot, nullptr, FSPAreaConfig(),
                                   &inspection);

  EXPECT_EQ(snapshot.projection_area, 140747)
      << std::endl
      << "Inspection: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, FillSnapshotImpl_EmptyRect) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAreaSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(1000, 1000);

  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {100, 100}));

  // Prepare the snapshot for the first fill.
  FSPAreaTracer::DiffAreaSnapshots(snapshot, nullptr, FSPAreaConfig(), nullptr);
  auto original_area = snapshot.projection_area;

  // Second fill with an empty rect.
  FillRect(snapshot, lynx::base::geometry::IntRect({10, 10}, {0, 0}));

  std::ostringstream inspection;
  FSPAreaTracer::DiffAreaSnapshots(snapshot, nullptr, FSPAreaConfig(),
                                   &inspection);

  EXPECT_EQ(snapshot.projection_area, original_area)
      << std::endl
      << "Inspection: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, InspectSnapshotImpl) {
  auto snapshot_ptr = tracer_->CreateSnapshot();
  auto& snapshot = static_cast<FSPAreaSnapshot&>(*snapshot_ptr);
  snapshot.container_size = lynx::base::geometry::IntSize(1000, 1000);
  FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}));

  std::string inspection = tracer_->InspectSnapshotImpl(snapshot, nullptr);
  EXPECT_NE(inspection.find("<Initially_Stable>"), std::string::npos);
}

TEST_F(FSPAreaTracerImplTest, DiffAreaSnapshots_Base) {
  FSPAreaConfig config;
  config.acceptable_difference_per_ms = 2;
  config.acceptable_difference_per_snapshot = 50;
  config.minimum_completion_rate = 0.2;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}),
           0.001);

  // Prepare the previous snapshot for the first snapshot.
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 0.002);

  // Stable if two snapshots are the same.
  std::ostringstream inspection;
  EXPECT_TRUE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots: " << inspection.str();

  FSPAreaSnapshot incomplete;
  incomplete.container_size = {1000, 1000};
  FillRect(incomplete, lynx::base::geometry::IntRect({0, 0}, {150, 150}),
           0.002);

  // Unstable if the previous snapshot is not complete.
  inspection.str("");
  EXPECT_FALSE(FSPAreaTracer::DiffAreaSnapshots(incomplete, &previous, config,
                                                &inspection))
      << "DiffAreaSnapshots: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, DiffAreaSnapshots_Stable) {
  FSPAreaConfig config;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};

  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {500, 500}), 0.001);
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};

  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {500, 501}), 0.002);

  std::ostringstream inspection;
  EXPECT_TRUE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots_Stable: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, DiffAreaSnapshots_FailShortTimeInterval) {
  FSPAreaConfig config;
  config.minimum_diff_interval_ms = 2.0;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};
  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {999, 999}), 0.001);

  // Prepare the previous snapshot for the first snapshot.
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};
  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {1000, 1000}), 0.002);

  std::ostringstream inspection;
  EXPECT_FALSE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots_FailShortTimeInterval: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, DiffAreaSnapshots_FailCompletionRate) {
  FSPAreaConfig config;
  config.minimum_completion_rate = 0.8;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};

  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {800, 800}), 0.001);
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};

  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {700, 700}), 0.002);

  std::ostringstream inspection;
  EXPECT_FALSE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots_FailCompletionRate: " << inspection.str();
}

TEST_F(FSPAreaTracerImplTest,
       DiffAreaSnapshots_FailAcceptableDifferencePerSnapshot) {
  FSPAreaConfig config;
  config.acceptable_difference_per_snapshot = 1;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};

  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {500, 500}), 0.001);
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};

  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {600, 600}), 0.002);

  std::ostringstream inspection;
  EXPECT_FALSE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots_FailAcceptableDifferencePerSnapshot: "
      << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, DiffAreaSnapshots_FailAcceptableDifferencePerMs) {
  FSPAreaConfig config;
  config.acceptable_difference_per_ms = 500;

  FSPAreaSnapshot previous;
  previous.container_size = {1000, 1000};

  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {500, 500}), 0.001);
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FSPAreaSnapshot current;
  current.container_size = {1000, 1000};

  FillRect(previous, lynx::base::geometry::IntRect({0, 0}, {500, 500}), 0.002);
  FSPAreaTracer::DiffAreaSnapshots(previous, nullptr, config, nullptr);

  FillRect(current, lynx::base::geometry::IntRect({0, 0}, {600, 600}), 0.003);

  std::ostringstream inspection;
  EXPECT_FALSE(
      FSPAreaTracer::DiffAreaSnapshots(current, &previous, config, &inspection))
      << "DiffAreaSnapshots_FailAcceptableDifferencePerMs: "
      << inspection.str();
}

TEST_F(FSPAreaTracerImplTest, CaptureSnapshot) {
  // Config
  FSPAreaConfig config;
  config.minimum_completion_rate = 0.2;
  config.acceptable_difference_per_snapshot = 50000;
  config.acceptable_difference_per_ms = 20000;
  tracer_->config_ = config;

  // Step 1: Initial stable snapshot
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {500, 500}),
                   0.001, 123);
  };
  auto result1 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_TRUE(result1.is_stable)
      << "First snapshot should be stable.\n"
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 2: Unstable due to large area change
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {700, 700}),
                   0.002, 124);
  };
  auto result2 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_FALSE(result2.is_stable)
      << "Second snapshot should be unstable.\n"
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 3: Stable again with no content change
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {700, 700}),
                   0.003, 125);
  };
  auto result3 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_TRUE(result3.is_stable)
      << "Third snapshot should be stable again.\n"
      << "Inspection: " << tracer_->InspectCurrentSnapshot();

  // Step 4: Unstable with less than completion rate content
  tracer_->callback_ = [this](FSPSnapshot& snapshot) {
    this->FillRect(snapshot, lynx::base::geometry::IntRect({0, 0}, {100, 100}),
                   0.004, 126);
  };
  auto result4 = tracer_->CaptureSnapshot({1000, 1000});
  EXPECT_FALSE(result4.is_stable)
      << "Fourth snapshot should be unstable due to completion rate.\n"
      << "Inspection: " << tracer_->InspectCurrentSnapshot();
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
