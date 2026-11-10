// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/mtl_fence_sync.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {
namespace {
FenceSync::PollResult PollMetalBuffer(id<MTLCommandBuffer> buffer) {
  if (!buffer || buffer.status < MTLCommandBufferStatusCommitted ||
      buffer.status == MTLCommandBufferStatusError) {
    return FenceSync::PollResult::kFailed;
  }
  return buffer.status == MTLCommandBufferStatusCompleted ? FenceSync::PollResult::kSignaled
                                                          : FenceSync::PollResult::kPending;
}

void ReleaseMetalSharedEventHandle(void* handle) {
  id object = (__bridge_transfer id)handle;
  (void)object;
}

bool WaitMetalBuffer(id<MTLCommandBuffer> buffer) {
  if (!buffer || buffer.status == MTLCommandBufferStatusError ||
      buffer.status < MTLCommandBufferStatusCommitted) {
    return false;  // An unsubmitted buffer cannot complete on its own.
  }
  if (buffer.status != MTLCommandBufferStatusCompleted) {
    [buffer waitUntilCompleted];
  }
  return buffer.status == MTLCommandBufferStatusCompleted;
}
}  // namespace

MTLCompleteFenceSync::MTLCompleteFenceSync(id<MTLCommandBuffer> buffer) : buffer_(buffer) {}

bool MTLCompleteFenceSync::ClientWait() { return WaitMetalBuffer(buffer_.get()); }

FenceSync::PollResult MTLCompleteFenceSync::Poll() { return PollMetalBuffer(buffer_.get()); }

FenceSync::Type MTLCompleteFenceSync::GetType() const { return Type::kClientWaitOnly; }

MTLCompleteFenceSync::~MTLCompleteFenceSync() = default;

MTLSharedEventFenceSync::MTLSharedEventFenceSync(id<MTLCommandBuffer> buffer,
                                                 id<MTLSharedEvent> event, uint64_t value)
    : buffer_(buffer), event_(event), value_(value) {}

MTLSharedEventFenceSync::~MTLSharedEventFenceSync() = default;

bool MTLSharedEventFenceSync::ClientWait() { return WaitMetalBuffer(buffer_.get()); }

FenceSync::PollResult MTLSharedEventFenceSync::Poll() { return PollMetalBuffer(buffer_.get()); }

bool MTLSharedEventFenceSync::EncodeMetalWait(void* command_buffer) {
  id<MTLCommandBuffer> commands = (__bridge id<MTLCommandBuffer>)command_buffer;
  if (!commands || !commands.retainedReferences ||
      commands.status >= MTLCommandBufferStatusCommitted) {
    return false;
  }
  const auto result = Poll();
  if (result == PollResult::kSignaled) {
    return true;
  }
  // Shared events can report a nil device when usable across devices.
  if (result == PollResult::kFailed || !event_ ||
      buffer_.get().status < MTLCommandBufferStatusCommitted ||
      (event_.get().device && commands.device != event_.get().device)) {
    return false;
  }
  [commands encodeWaitForEvent:event_.get() value:value_];
  return true;
}

FenceSync::NativeExportResult MTLSharedEventFenceSync::CopyMetalSharedEvent(
    void** handle, uint64_t* value, NativeHandleRelease* release) {
  if (handle) *handle = nullptr;
  if (value) *value = 0;
  if (release) *release = nullptr;
  if (!handle || !value || !release) return NativeExportResult::kFailed;
  const auto result = Poll();
  if (result == PollResult::kFailed || buffer_.get().status < MTLCommandBufferStatusCommitted) {
    return NativeExportResult::kFailed;
  }
  if (!event_) {
    return result == PollResult::kSignaled ? NativeExportResult::kSignaled
                                           : NativeExportResult::kUnsupported;
  }
  MTLSharedEventHandle* exported = [event_.get() newSharedEventHandle];
  if (!exported) return NativeExportResult::kFailed;
  *handle = (__bridge_retained void*)exported;
  *value = value_;
  *release = ReleaseMetalSharedEventHandle;
  return NativeExportResult::kExported;
}

FenceSync::Type MTLSharedEventFenceSync::GetType() const { return Type::kMetalEvent; }

}  // namespace clay
