// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#include "core/services/fsp_tracing/fsp_tracer.h"
#undef protected

#include "core/services/fsp_tracing/area/fsp_area_tracer_impl.h"
#include "core/services/fsp_tracing/axial/fsp_axial_tracer_impl.h"
#include "core/services/fsp_tracing/base/fsp_tracer_impl.h"
#include "gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace timing {

class FSPTracerImplForTest
    : public FSPTracerImpl<FSPTracerImplForTest, FSPAreaConfig> {
 public:
  using FSPTracerImpl<FSPTracerImplForTest, FSPAreaConfig>::FSPTracerImpl;
  using FSPTracerImpl<FSPTracerImplForTest, FSPAreaConfig>::SwapCurrentSnapshot;

  bool capture_snapshot_impl_called() const {
    return capture_snapshot_impl_called_;
  }
  const lynx::base::geometry::IntSize& last_container_size() const {
    return last_container_size_;
  }
  int fill_snapshot_impl_call_count() const {
    return fill_snapshot_impl_call_count_;
  }
  const FSPContentInfo& last_content_info() const { return last_content_info_; }

 private:
  friend class FSPTracerImpl<FSPTracerImplForTest, FSPAreaConfig>;

  bool capture_snapshot_impl_called_{false};
  lynx::base::geometry::IntSize last_container_size_;
  int fill_snapshot_impl_call_count_{0};
  FSPContentInfo last_content_info_{};

  std::unique_ptr<FSPSnapshot> CreateSnapshot() override {
    return std::make_unique<FSPSnapshot>();
  }

  FSPResult CaptureSnapshotImpl(
      FSPSnapshot& snapshot,
      lynx::base::geometry::IntSize container_size) override {
    capture_snapshot_impl_called_ = true;
    last_container_size_ = container_size;
    return FSPResult();
  }

  void FillSnapshotImpl(FSPSnapshot& snapshot,
                        const FSPContentInfo& content_info) override {
    fill_snapshot_impl_call_count_++;
    last_content_info_ = content_info;
  }

  std::string InspectSnapshotImpl(FSPSnapshot& current,
                                  FSPSnapshot* previous) override {
    return "";
  }
};

TEST(FSPTracerImplTest, SwapCurrentSnapshot) {
  FSPTracerImplForTest tracer{FSPAreaConfig(), FSPTracer::SnapshotCallback()};
  auto& snapshot1 = tracer.SwapCurrentSnapshot();
  auto& snapshot2 = tracer.SwapCurrentSnapshot();
  auto& snapshot3 = tracer.SwapCurrentSnapshot();
  auto& snapshot4 = tracer.SwapCurrentSnapshot();
  ASSERT_NE(std::addressof(snapshot1), std::addressof(snapshot2));
  ASSERT_EQ(std::addressof(snapshot1), std::addressof(snapshot3));
  ASSERT_EQ(std::addressof(snapshot2), std::addressof(snapshot4));
}

TEST(FSPTracerImplTest, CaptureSnapshot) {
  FSPTracerImplForTest tracer{FSPAreaConfig(), FSPTracer::SnapshotCallback()};
  ASSERT_FALSE(tracer.capture_snapshot_impl_called());
  auto container_size = lynx::base::geometry::IntSize(100, 200);
  tracer.CaptureSnapshot(container_size);
  ASSERT_TRUE(tracer.capture_snapshot_impl_called());
  ASSERT_EQ(container_size.Width(), tracer.last_container_size().Width());
  ASSERT_EQ(container_size.Height(), tracer.last_container_size().Height());
}

TEST(FSPTracerImplTest, FillSnapshot) {
  FSPTracerImplForTest tracer{FSPAreaConfig(), FSPTracer::SnapshotCallback()};
  ASSERT_EQ(0, tracer.fill_snapshot_impl_call_count());
  // FillSnapshot asserts that we are filling the current snapshot.
  auto& snapshot = tracer.SwapCurrentSnapshot();
  FSPContentInfo content_info{};
  content_info.ui_sign = 123;
  content_info.change_timestamp = 456.789;
  content_info.rect =
      lynx::base::geometry::IntRect(lynx::base::geometry::Point<int>(10, 20),
                                    lynx::base::geometry::Size<int>(30, 40));
  tracer.FillSnapshot(snapshot, content_info);
  ASSERT_EQ(1, tracer.fill_snapshot_impl_call_count());
  const auto& last_info = tracer.last_content_info();
  ASSERT_EQ(content_info.ui_sign, last_info.ui_sign);
  ASSERT_DOUBLE_EQ(content_info.change_timestamp, last_info.change_timestamp);
  ASSERT_EQ(content_info.rect, last_info.rect);

  tracer.FillSnapshot(snapshot, content_info);
  ASSERT_EQ(2, tracer.fill_snapshot_impl_call_count());
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
