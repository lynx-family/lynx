// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <utility>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
class TestBacking final : public clay::SharedImageBacking {
 public:
  TestBacking() : SharedImageBacking(PixelFormat::kNative8888, {1, 1}) {}
  BackingType GetType() const override { return BackingType::kMocking; }
  clay::GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<clay::SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
  fml::RefPtr<clay::SkityImageRepresentation> CreateSkityRepresentation(clay::GrContext*) override {
    return nullptr;
  }
};
class FailedFence final : public clay::FenceSync {
 public:
  bool ClientWait() override { return false; }
  Type GetType() const override { return Type::kClientWaitOnly; }
};
#define FENCE_CHECK(x)                                     \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)
void TestRepresentationFailure() {
  auto backing = fml::MakeRefCounted<TestBacking>();
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  backing->SetFenceSync(std::move(failure));
  for (int i = 0; i < 2; ++i) {
    auto representation = fml::MakeRefCounted<clay::MTLImageRepresentation>(nil, nil, backing);
    representation->ConsumeFence(backing->GetFenceSync());
    ClaySharedImageReadResult read{};
    ClaySharedImageWriteResult write{};
    FENCE_CHECK(!representation->BeginRead(&read) && !representation->EndRead());
    FENCE_CHECK(!representation->BeginWrite(&write) && !representation->EndWrite());
    auto returned = representation->ProduceFence();
    FENCE_CHECK(returned.get() == identity && !returned->ClientWait());
    backing->SetFenceSync(std::move(returned));
  }
  FENCE_CHECK(backing->GetFenceSync().get() == identity);
  std::puts("PASS actual Metal representation: failed read/write/end/release preserves original "
            "proof across recreation");
}

auto MakeSink(clay::SharedImageSink::BufferMode mode) {
  auto sink = fml::MakeRefCounted<clay::SharedImageSinkManaged>(
      mode, [](skity::Vec2, auto) { return fml::MakeRefCounted<TestBacking>(); });
  sink->SetFrameAvailableCallback([] {});
  return sink;
}

fml::RefPtr<clay::SharedImageRepresentation> MakeRepresentation(
    fml::RefPtr<clay::SharedImageBacking> backing) {
  return fml::MakeRefCounted<clay::MTLImageRepresentation>(nil, nil, backing);
}

void CheckReturnedFailure(clay::SharedImageBacking* backing, clay::FenceSync* identity) {
  auto fence = backing->GetFenceSync();
  FENCE_CHECK(fence.get() == identity);
  FENCE_CHECK(!fence->ClientWait());
  backing->SetFenceSync(std::move(fence));
}

void TestFailedReadRelease(bool destroy_accessor) {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  auto [back, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  FENCE_CHECK(sink->SwapBack(std::move(failure)));
  {
    clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
    auto front = reader.UpdateFront();
    FENCE_CHECK(front && front->GetBacking() == back.get());
    ClaySharedImageReadResult read{};
    FENCE_CHECK(!front->BeginRead(&read));
    if (!destroy_accessor) {
      reader.ReleaseFront();
      CheckReturnedFailure(back.get(), identity);
      // A second release must not overwrite the returned failure fence.
      reader.ReleaseFront();
      auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
      FENCE_CHECK(returned == back);
      FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
    }
  }
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == back);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);

  // Recreating the accessor must not turn the returned slot into usable data.
  clay::SharedImageSinkAccessor writer(sink, MakeRepresentation);
  auto [repr, writer_age] = writer.AcquireBack({1, 1});
  FENCE_CHECK(repr);
  ClaySharedImageWriteResult write{};
  FENCE_CHECK(!repr->BeginWrite(&write));
  FENCE_CHECK(!writer.SwapBack());
  CheckReturnedFailure(back.get(), identity);
}

void TestFailedReadAdvance() {
  auto sink = MakeSink(clay::SharedImageSink::kDoubleBuffer);
  auto [first, first_age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  FENCE_CHECK(sink->SwapBack(std::move(failure)));
  auto [second, second_age] = sink->AcquireBack({1, 1});
  FENCE_CHECK(second && second != first);
  FENCE_CHECK(sink->SwapBack(nullptr));
  clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == first.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == second.get());
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == first);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);
}

void TestFailedWriteRelease(bool destroy_accessor) {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  auto [back, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  back->SetFenceSync(std::move(failure));
  {
    clay::SharedImageSinkAccessor writer(sink, MakeRepresentation);
    auto [repr, writer_age] = writer.AcquireBack({1, 1});
    FENCE_CHECK(repr && repr->GetBacking() == back.get());
    ClaySharedImageWriteResult write{};
    FENCE_CHECK(!repr->BeginWrite(&write));
    if (!destroy_accessor) {
      FENCE_CHECK(!writer.SwapBack());
      CheckReturnedFailure(back.get(), identity);
      // The accessor must have relinquished the back representation.
      FENCE_CHECK(!writer.SwapBack());
      auto [unavailable, unavailable_age, status] = sink->TryAcquireBack({1, 1});
      FENCE_CHECK(!unavailable);
      FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kFullAttach);
    }
  }
  // No pending front would make single-buffer UpdateFront time out here.
  clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == back.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  reader.ReleaseFront();
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == back);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  CheckReturnedFailure(returned.get(), identity);
}

void TestSuccessfulHandoff() {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  id<MTLCommandQueue> queue = [device newCommandQueue];
  FENCE_CHECK(device && queue);
  auto factory = [device, queue](fml::RefPtr<clay::SharedImageBacking> backing)
      -> fml::RefPtr<clay::SharedImageRepresentation> {
    return fml::MakeRefCounted<clay::MTLImageRepresentation>(device, queue, backing);
  };
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  clay::SharedImageSinkAccessor writer(sink, factory);
  clay::SharedImageSinkAccessor reader(sink, factory);
  for (int i = 0; i < 2; ++i) {
    auto [back, age] = writer.AcquireBack({1, 1});
    FENCE_CHECK(back);
    // No pixel access is needed to check a successful fence/slot handoff.
    FENCE_CHECK(writer.SwapBack());
    FENCE_CHECK(!writer.SwapBack());
    auto front = reader.UpdateFront();
    FENCE_CHECK(front && front->GetBacking() == back->GetBacking());
    reader.ReleaseFront();
    auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
    FENCE_CHECK(returned.get() == back->GetBacking());
    FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
    auto fence = returned->GetFenceSync();
    FENCE_CHECK(fence && fence->ClientWait());
    returned->SetFenceSync(std::move(fence));
  }
}

void TestDetachedSwapBack(bool recreate_accessor) {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  sink->SetFrameAvailableCallback(nullptr);
  auto [backing, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  // Rejection still transfers the fence to the current slot.
  FENCE_CHECK(!sink->SwapBack(std::move(failure)));
  CheckReturnedFailure(backing.get(), identity);
  FENCE_CHECK(!sink->SwapBack(nullptr));
  CheckReturnedFailure(backing.get(), identity);
  int notifications = 0;
  auto writer = std::make_unique<clay::SharedImageSinkAccessor>(sink, MakeRepresentation);
  // Blocking acquisition rejects a detached sink; the retained current slot
  // remains available through the nonblocking acquisition API.
  auto [repr, writer_age, writer_status] = writer->TryAcquireBack({1, 1});
  FENCE_CHECK(writer_status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  FENCE_CHECK(repr && repr->GetBacking() == backing.get());
  ClaySharedImageWriteResult write{};
  FENCE_CHECK(!repr->BeginWrite(&write));
  FENCE_CHECK(!writer->SwapBack());
  CheckReturnedFailure(backing.get(), identity);
  if (recreate_accessor) {
    writer.reset();
    CheckReturnedFailure(backing.get(), identity);
    writer = std::make_unique<clay::SharedImageSinkAccessor>(sink, MakeRepresentation);
    auto [recreated, recreated_age, recreated_status] = writer->TryAcquireBack({1, 1});
    FENCE_CHECK(recreated_status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
    FENCE_CHECK(recreated && recreated->GetBacking() == backing.get());
    FENCE_CHECK(!recreated->BeginWrite(&write));
  }
  sink->SetFrameAvailableCallback([&] { ++notifications; });
  // EndWrite fails, but the reattached sink must accept the retained slot.
  FENCE_CHECK(!writer->SwapBack());
  FENCE_CHECK(notifications == 1);
  FENCE_CHECK(!writer->SwapBack());
  FENCE_CHECK(notifications == 1);
  clay::SharedImageSinkAccessor reader(sink, MakeRepresentation);
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == backing.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  reader.ReleaseFront();
  CheckReturnedFailure(backing.get(), identity);
  std::puts(
      "PASS detached swap: rejection, null retry, accessor retention/recreation and reattach");
}

void TestRawAcquisitionFailure() {
  auto sink = MakeSink(clay::SharedImageSink::kSingleBuffer);
  auto [backing, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  backing->SetFenceSync(std::move(failure));
  const ClaySize size{1, 1};
  auto sink_ref = reinterpret_cast<ClaySharedImageSinkRef>(sink.get());
  ClaySharedImageRef output = nullptr;
  uint32_t output_age = 123;
  for (int i = 0; i < 2; ++i) {
    FENCE_CHECK(!ClaySharedImageSinkTryAcquireBack(sink_ref, &size, &output, &output_age));
    FENCE_CHECK(output == nullptr && output_age == 123);
    CheckReturnedFailure(backing.get(), identity);
  }
  // Retries keep the same slot. An explicitly replaced dependency can recover.
  backing->SetFenceSync(nullptr);
  FENCE_CHECK(ClaySharedImageSinkTryAcquireBack(sink_ref, &size, &output, &output_age));
  FENCE_CHECK(output == reinterpret_cast<ClaySharedImageRef>(backing.get()));
  FENCE_CHECK(output_age == 0);
  std::puts("PASS raw C acquisition: failed wait preserves dependency and outputs across retries");
}

void TestFrontRepresentationFailure(clay::SharedImageSink::UpdateFrontMode mode) {
  auto sink = fml::MakeRefCounted<clay::SharedImageSinkManaged>(
      clay::SharedImageSink::kSingleBuffer,
      [](skity::Vec2, auto) { return fml::MakeRefCounted<TestBacking>(); }, mode);
  sink->SetFrameAvailableCallback([] {});
  auto [backing, age] = sink->AcquireBack({1, 1});
  auto failure = std::make_unique<FailedFence>();
  auto* identity = failure.get();
  FENCE_CHECK(sink->SwapBack(std::move(failure)));
  bool fail_creation = true;
  clay::SharedImageSinkAccessor reader(sink, [&](auto back) {
    return fail_creation ? fml::RefPtr<clay::SharedImageRepresentation>()
                         : MakeRepresentation(back);
  });
  FENCE_CHECK(!reader.UpdateFront());
  CheckReturnedFailure(backing.get(), identity);
  // The failed reader must not strand the only slot. Return it to the reader
  // with the exact dependency after the producer reacquires it.
  auto [returned, returned_age, status] = sink->TryAcquireBack({1, 1});
  FENCE_CHECK(returned == backing);
  FENCE_CHECK(status == clay::SharedImageSink::AcquireBackStatus::kSuccess);
  FENCE_CHECK(sink->SwapBack(returned->GetFenceSync()));
  fail_creation = false;
  auto front = reader.UpdateFront();
  FENCE_CHECK(front && front->GetBacking() == backing.get());
  ClaySharedImageReadResult read{};
  FENCE_CHECK(!front->BeginRead(&read));
  reader.ReleaseFront();
  CheckReturnedFailure(backing.get(), identity);
  std::puts(
      "PASS front accessor: representation allocation failure returns slot and preserves fence");
}

int main() {
  @autoreleasepool {
    TestRawAcquisitionFailure();
    TestFrontRepresentationFailure(clay::SharedImageSink::UpdateFrontMode::kLatest);
    TestFrontRepresentationFailure(clay::SharedImageSink::UpdateFrontMode::kSequence);
    TestRepresentationFailure();
    TestFailedReadRelease(false);
    TestFailedReadRelease(true);
    TestFailedReadAdvance();
    TestFailedWriteRelease(false);
    TestFailedWriteRelease(true);
    TestSuccessfulHandoff();
    TestDetachedSwapBack(false);
    TestDetachedSwapBack(true);
    std::puts("PASS actual Metal accessor: failed read/write returns slots, "
              "sequence advances, destruction cleans up, failure proof survives");
  }
}
