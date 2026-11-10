// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_
#define CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_

#include <cstdint>

#include "base/include/fml/macros.h"
#include "build/build_config.h"

namespace clay {

class FenceSync {
 public:
  enum class Type {
    kCGL,
    kEAGL,
    kEGL,
    kANGLE,
    kMetalEvent,
    kClientWaitOnly,
    kVulkan,
    kLinuxEGL,
  };

  FenceSync() = default;

  virtual ~FenceSync() = default;

  virtual bool ClientWait() = 0;

  virtual Type GetType() const = 0;

  enum class PollResult { kSignaled, kPending, kUnsupported, kFailed };
  // Never falls back to a blocking ClientWait. Only orders the current context.
  bool EnqueueWait() { return EnqueueWaitResult() == WaitResult::kSuccess; }
  enum class WaitResult { kSuccess, kUnsupported, kFailed };
  virtual WaitResult EnqueueWaitResult() {
    return GetType() == Type::kEGL || GetType() == Type::kVulkan
               ? WaitResult::kFailed
               : WaitResult::kUnsupported;
  }
  // Unsupported implementations must not call ClientWait here.
  virtual PollResult Poll() { return PollResult::kUnsupported; }

#if OS_MACOSX || OS_IOS
  // Metal has no implicit current queue. Encode into the actual consumer
  // command buffer before its first encoder; never submit or CPU-wait here.
  virtual bool EncodeMetalWait(void*) { return false; }
#endif

  enum class NativeExportResult { kExported, kSignaled, kUnsupported, kFailed };
  // Duplicates a native fence without consuming this object or waiting.
  virtual NativeExportResult DupNativeFenceFD(int* out_fd) {
    if (out_fd) {
      *out_fd = -1;
    }
    return out_fd ? NativeExportResult::kUnsupported
                  : NativeExportResult::kFailed;
  }

  using NativeHandleRelease = void (*)(void*);
#if OS_MACOSX || OS_IOS
  virtual
#endif
      NativeExportResult
      CopyMetalSharedEvent(void** handle, uint64_t* value,
                           NativeHandleRelease* release) {
    if (handle) *handle = nullptr;
    if (value) *value = 0;
    if (release) *release = nullptr;
    return handle && value && release ? NativeExportResult::kUnsupported
                                      : NativeExportResult::kFailed;
  }

  BASE_DISALLOW_COPY_AND_ASSIGN(FenceSync);
};

// A legacy dependency whose completion could not be established.
class FailedFenceSync final : public FenceSync {
 public:
  bool ClientWait() override { return false; }
  PollResult Poll() override { return PollResult::kFailed; }
  Type GetType() const override { return Type::kClientWaitOnly; }
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_FENCE_SYNC_H_
