// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "clay/gfx/shared_image/mtl_fence_sync.h"
#include "clay/gfx/shared_image/tests/api_contract.h"

#define FENCE_CHECK(x)                                     \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)
@interface FenceTestBuffer : NSObject
@property(nonatomic) MTLCommandBufferStatus status;
@property(nonatomic) NSUInteger waits;
@end
@implementation FenceTestBuffer
- (void)waitUntilCompleted {
  ++_waits;
  _status = MTLCommandBufferStatusError;
}
@end
static unsigned handle_destructions;
@interface FenceTestHandle : NSObject
@end
@implementation FenceTestHandle
- (void)dealloc {
  ++handle_destructions;
}
@end
@interface FenceTestEvent : NSObject
@end
@implementation FenceTestEvent
- (MTLSharedEventHandle*)newSharedEventHandle {
  return (MTLSharedEventHandle*)[FenceTestHandle new];
}
@end
static ClayFenceSyncRef Ref(clay::FenceSync* fence) {
  return reinterpret_cast<ClayFenceSyncRef>(fence);
}
static void StatusAndOwnership() {
  for (bool shared : {false, true}) {
    for (NSUInteger status = MTLCommandBufferStatusNotEnqueued;
         status <= MTLCommandBufferStatusError; ++status) {
      FenceTestBuffer* buffer = [FenceTestBuffer new];
      buffer.status = static_cast<MTLCommandBufferStatus>(status);
      std::unique_ptr<clay::FenceSync> fence;
      if (shared)
        fence =
            std::make_unique<clay::MTLSharedEventFenceSync>((id<MTLCommandBuffer>)buffer, nil, 7);
      else
        fence = std::make_unique<clay::MTLCompleteFenceSync>((id<MTLCommandBuffer>)buffer);
      const auto expected =
          status == MTLCommandBufferStatusCompleted ? kClayFenceSyncSignaled
          : (status < MTLCommandBufferStatusCommitted || status == MTLCommandBufferStatusError)
              ? kClayFenceSyncFailed
              : kClayFenceSyncPending;
      FENCE_CHECK(testFencePoll(Ref(fence.get())) == expected && !buffer.waits);
      if (expected == kClayFenceSyncFailed)
        FENCE_CHECK(!fence->ClientWait() && !buffer.waits);
      else if (expected == kClayFenceSyncSignaled)
        FENCE_CHECK(fence->ClientWait() && !buffer.waits);
      else
        FENCE_CHECK(!fence->ClientWait() && buffer.waits == 1);
    }
  }
  clay::MTLCompleteFenceSync missing(nil);
  FENCE_CHECK(testFencePoll(Ref(&missing)) == kClayFenceSyncFailed);
  FenceTestBuffer* buffer = [FenceTestBuffer new];
  buffer.status = MTLCommandBufferStatusScheduled;
  FenceTestEvent* event = [FenceTestEvent new];
  auto* fence = new clay::MTLSharedEventFenceSync((id<MTLCommandBuffer>)buffer,
                                                  (id<MTLSharedEvent>)event, 42);
  ClayFenceSyncExportInfo packet{sizeof(packet)};
  FENCE_CHECK(ClayFenceSyncExport(Ref(fence), kClayFenceSyncExportMetalSharedEvent, &packet) ==
              kClayFenceSyncSuccess);
  __weak id weak_handle = (__bridge id)packet.metal_shared_event_handle;
  FENCE_CHECK(weak_handle && packet.metal_event_value == 42);
  ClayDestroyFenceSync(Ref(fence));
  FENCE_CHECK(weak_handle && !handle_destructions);
  packet.release_metal_shared_event_handle(packet.metal_shared_event_handle);
  FENCE_CHECK(!weak_handle && handle_destructions == 1);
  std::puts("PASS Metal invalid/uncommitted/error status and owned-handle release");
}
static void CrossQueue(bool exported) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  FENCE_CHECK(device);
  id<MTLCommandQueue> producer = [device newCommandQueue], consumer = [device newCommandQueue];
  id<MTLBuffer> source = [device newBufferWithLength:256 options:MTLResourceStorageModeShared];
  id<MTLBuffer> output = [device newBufferWithLength:256 options:MTLResourceStorageModeShared];
  id<MTLSharedEvent> gate = [device newSharedEvent], event = [device newSharedEvent];
  FENCE_CHECK(producer && consumer && source && output && gate && event);
  std::memset(output.contents, 0, 256);
  id<MTLCommandBuffer> writes = [producer commandBuffer];
  [writes encodeWaitForEvent:gate value:1];
  id<MTLBlitCommandEncoder> fill = [writes blitCommandEncoder];
  [fill fillBuffer:source range:NSMakeRange(0, 256) value:0x5a];
  [fill endEncoding];
  [writes encodeSignalEvent:event value:7];
  [writes commit];
  auto* fence = new clay::MTLSharedEventFenceSync(writes, event, 7);
  FENCE_CHECK(testFencePoll(Ref(fence)) == kClayFenceSyncPending);
  id<MTLCommandBuffer> reads = [consumer commandBuffer];
  FENCE_CHECK(!testFenceMetalWait(
      Ref(fence), (__bridge void*)[consumer commandBufferWithUnretainedReferences]));
  if (exported) {
    ClayFenceSyncExportInfo first{sizeof(first)}, second{sizeof(second)};
    FENCE_CHECK(ClayFenceSyncExport(Ref(fence), kClayFenceSyncExportMetalSharedEvent, &first) ==
                kClayFenceSyncSuccess);
    FENCE_CHECK(ClayFenceSyncExport(Ref(fence), kClayFenceSyncExportMetalSharedEvent, &second) ==
                kClayFenceSyncSuccess);
    first.release_metal_shared_event_handle(first.metal_shared_event_handle);
    ClayDestroyFenceSync(Ref(fence));
    fence = nullptr;
    id<MTLSharedEvent> imported = [device
        newSharedEventWithHandle:(__bridge MTLSharedEventHandle*)second.metal_shared_event_handle];
    FENCE_CHECK(imported && second.metal_event_value == 7);
    [reads encodeWaitForEvent:imported value:second.metal_event_value];
    second.release_metal_shared_event_handle(second.metal_shared_event_handle);
  } else {
    FENCE_CHECK(testFenceMetalWait(Ref(fence), (__bridge void*)reads));
  }
  ClayDestroyFenceSync(Ref(fence));
  event = nil;
  id<MTLBlitCommandEncoder> copy = [reads blitCommandEncoder];
  [copy copyFromBuffer:source sourceOffset:0 toBuffer:output destinationOffset:0 size:256];
  [copy endEncoding];
  [reads commit];
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  const bool premature = reads.status == MTLCommandBufferStatusCompleted;
  gate.signaledValue = 1;
  [reads waitUntilCompleted];  // Test readback only.
  FENCE_CHECK(!premature && reads.status == MTLCommandBufferStatusCompleted);
  for (int i = 0; i < 256; ++i)
    FENCE_CHECK(static_cast<unsigned char*>(output.contents)[i] == 0x5a);
  std::printf("PASS Metal cross-queue %s and 256-byte readback\n",
              exported ? "export/import" : "server wait");
}
static void ConcurrentClientWait(bool shared) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  id<MTLCommandQueue> producer = [device newCommandQueue];
  id<MTLCommandQueue> consumer = [device newCommandQueue];
  id<MTLSharedEvent> gate = [device newSharedEvent], event = [device newSharedEvent];
  FENCE_CHECK(device && producer && consumer && gate && event);
  id<MTLCommandBuffer> writes = [producer commandBuffer];
  [writes encodeWaitForEvent:gate value:1];
  [writes encodeSignalEvent:event value:7];
  [writes commit];
  std::unique_ptr<clay::FenceSync> fence;
  if (shared)
    fence = std::make_unique<clay::MTLSharedEventFenceSync>(writes, event, 7);
  else
    fence = std::make_unique<clay::MTLCompleteFenceSync>(writes);
  const auto ref = Ref(fence.get());
  std::atomic<bool> entered{false}, completed{false};
  std::thread waiter([&] {
    @autoreleasepool {
      entered = true;
      FENCE_CHECK(ClayFenceSyncClientWait(ref));
      completed = true;
    }
  });
  while (!entered) std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(15));
  FENCE_CHECK(!completed && testFencePoll(ref) == kClayFenceSyncPending);
  id<MTLCommandBuffer> reads = [consumer commandBuffer];
  FENCE_CHECK(testFenceMetalWait(ref, (__bridge void*)reads) == shared);
  ClayFenceSyncExportInfo packet{sizeof(packet)};
  FENCE_CHECK(ClayFenceSyncExport(ref, kClayFenceSyncExportMetalSharedEvent, &packet) ==
              (shared ? kClayFenceSyncSuccess : kClayFenceSyncUnsupported));
  if (shared) packet.release_metal_shared_event_handle(packet.metal_shared_event_handle);
  FENCE_CHECK(!completed);
  [reads commit];
  gate.signaledValue = 1;
  waiter.join();
  FENCE_CHECK(completed && testFencePoll(ref) == kClayFenceSyncSignaled);
  [reads waitUntilCompleted];
  FENCE_CHECK(reads.status == MTLCommandBufferStatusCompleted);
  std::printf("PASS Metal concurrent ClientWait (%s): poll/wait/export before GPU release\n",
              shared ? "shared event" : "completion buffer");
}

int main() {
  alarm(30);
  @autoreleasepool {
    FENCE_CHECK(testFencePublicContract());
    StatusAndOwnership();
    CrossQueue(false);
    CrossQueue(true);
    ConcurrentClientWait(false);
    ConcurrentClientWait(true);
  }
}
