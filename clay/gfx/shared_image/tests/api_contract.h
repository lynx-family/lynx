// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_TESTS_API_CONTRACT_H_
#define CLAY_GFX_SHARED_IMAGE_TESTS_API_CONTRACT_H_
#include <initializer_list>

#include "clay/public/clay.h"
static inline ClayFenceSyncResult testFencePoll(ClayFenceSyncRef fence) {
  return ClayFenceSyncPoll(fence);
}
static inline bool testFenceServerWait(ClayFenceSyncRef fence) {
  const ClayFenceSyncWaitContext context{sizeof(context),
                                         kClayFenceSyncWaitCurrentGL, nullptr};
  auto result = ClayFenceSyncServerWait(fence, &context);
  return result == kClayFenceSyncSuccess || result == kClayFenceSyncSignaled;
}
static inline bool testFenceMetalWait(ClayFenceSyncRef fence, void* commands) {
  const ClayFenceSyncWaitContext context{
      sizeof(context), kClayFenceSyncWaitMetalCommandBuffer, commands};
  auto result = ClayFenceSyncServerWait(fence, &context);
  return result == kClayFenceSyncSuccess || result == kClayFenceSyncSignaled;
}
static inline ClayFenceSyncResult testFenceExportFD(ClayFenceSyncRef fence,
                                                    int* fd) {
  ClayFenceSyncExportInfo output{sizeof(output)};
  auto result = ClayFenceSyncExport(fence, kClayFenceSyncExportNativeFD,
                                    fd ? &output : nullptr);
  if (fd) *fd = output.native_fence_fd;
  return result;
}
// Exercise the real C ABI in both EGL and Metal native probes.
static inline bool testFencePublicContract() {
#define FENCE_EXPECT(value)     \
  do {                          \
    if (!(value)) return false; \
  } while (0)
  FENCE_EXPECT(ClayFenceSyncPoll(nullptr) == kClayFenceSyncSignaled);
  ClayFenceSyncWaitContext context{sizeof(context), kClayFenceSyncWaitCurrentGL,
                                   nullptr};
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, nullptr) ==
               kClayFenceSyncFailed);
  --context.struct_size;
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, &context) ==
               kClayFenceSyncFailed);
  context.struct_size = sizeof(context);
  context.command_buffer = (void*)1;
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, &context) ==
               kClayFenceSyncFailed);
  context.type = kClayFenceSyncWaitMetalCommandBuffer;
  context.command_buffer = nullptr;
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, &context) ==
               kClayFenceSyncFailed);
  context.type = static_cast<ClayFenceSyncWaitType>(0x7fff);
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, &context) ==
               kClayFenceSyncUnsupported);
  struct FutureWait {
    ClayFenceSyncWaitContext base;
    uint64_t tail;
  } futureWait{{sizeof(FutureWait), kClayFenceSyncWaitCurrentGL, nullptr},
               0x12345678};
  FENCE_EXPECT(ClayFenceSyncServerWait(nullptr, &futureWait.base) ==
               kClayFenceSyncSignaled);
  FENCE_EXPECT(futureWait.tail == 0x12345678);
  FENCE_EXPECT(ClayFenceSyncExport(nullptr, kClayFenceSyncExportNativeFD,
                                   nullptr) == kClayFenceSyncFailed);
  struct FutureExport {
    ClayFenceSyncExportInfo base;
    uint64_t tail;
  } futureExport{};
  futureExport.base.struct_size = sizeof(futureExport);
  futureExport.tail = 0x12345678;
  for (auto type :
       {kClayFenceSyncExportNativeFD, kClayFenceSyncExportMetalSharedEvent}) {
    futureExport.base.native_fence_fd = 123;
    futureExport.base.metal_shared_event_handle = (void*)1;
    futureExport.base.metal_event_value = 99;
    futureExport.base.release_metal_shared_event_handle = [](void*) {};
    FENCE_EXPECT(ClayFenceSyncExport(nullptr, type, &futureExport.base) ==
                 kClayFenceSyncSignaled);
    FENCE_EXPECT(futureExport.base.native_fence_fd == -1 &&
                 !futureExport.base.metal_shared_event_handle &&
                 !futureExport.base.metal_event_value &&
                 !futureExport.base.release_metal_shared_event_handle);
    FENCE_EXPECT(futureExport.tail == 0x12345678);
  }
  FENCE_EXPECT(
      ClayFenceSyncExport(nullptr, static_cast<ClayFenceSyncExportType>(0x7fff),
                          &futureExport.base) == kClayFenceSyncUnsupported);
  ClayFenceSyncExportInfo shortOutput{sizeof(ClayFenceSyncExportInfo) - 1};
  shortOutput.native_fence_fd = 123;
  FENCE_EXPECT(ClayFenceSyncExport(nullptr, kClayFenceSyncExportNativeFD,
                                   &shortOutput) == kClayFenceSyncFailed);
  FENCE_EXPECT(shortOutput.native_fence_fd ==
               123);  // No write beyond an older caller's size.
  int callbacks = 0;
  auto external = ClayCreateExternalFenceSync(
      [](void* value) { ++*static_cast<int*>(value); }, nullptr, &callbacks);
  FENCE_EXPECT(ClayFenceSyncPoll(external) == kClayFenceSyncUnsupported);
  for (auto type :
       {kClayFenceSyncWaitCurrentGL, kClayFenceSyncWaitMetalCommandBuffer}) {
    context.type = type;
    context.command_buffer =
        type == kClayFenceSyncWaitCurrentGL ? nullptr : (void*)1;
    FENCE_EXPECT(ClayFenceSyncServerWait(external, &context) ==
                 kClayFenceSyncUnsupported);
  }
  for (auto type :
       {kClayFenceSyncExportNativeFD, kClayFenceSyncExportMetalSharedEvent})
    FENCE_EXPECT(ClayFenceSyncExport(external, type, &futureExport.base) ==
                 kClayFenceSyncUnsupported);
  ClayDestroyFenceSync(external);
  FENCE_EXPECT(callbacks == 0);
#undef FENCE_EXPECT
  return true;
}
#endif  // CLAY_GFX_SHARED_IMAGE_TESTS_API_CONTRACT_H_
