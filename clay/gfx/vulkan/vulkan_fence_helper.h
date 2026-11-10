// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_
#define CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_

#include <vulkan/vulkan.h>

#include <cstddef>
#include <list>
#include <mutex>
#include <utility>

#include "base/include/fml/unique_fd.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

// Used to help check when we can safely destroy a fence or semaphore in vulkan.
class VulkanFenceHelper {
 public:
  VulkanFenceHelper(VkDevice device, VkFence fence, VkSemaphore semaphore);
  ~VulkanFenceHelper() = default;

  void MarkCanDestroy();
  bool CanDestroy();
  // The representation retires the helper when replaced or destroyed. Keep
  // its native objects alive until every exported FenceSync has also gone.
  void RetainFenceSync();
  void ReleaseFenceSync();

  void Destroy();

  bool InPendingQueue(const std::list<VkFence>& pending_fences);

  bool WaitForFence();
  bool WaitSemaphore();
  FenceSync::PollResult Poll();
  FenceSync::NativeExportResult DupNativeFenceFD(int* out_fd);

 private:
  FenceSync::PollResult PollLocked();

  VkDevice device_ = VK_NULL_HANDLE;
  VkFence fence_ = VK_NULL_HANDLE;
  VkSemaphore semaphore_ = VK_NULL_HANDLE;

  std::mutex mutex_;
  bool can_destroy_ = false;
  size_t fence_sync_count_ = 0;

  std::mutex valid_mutex_;
  bool is_valid_ = true;
  bool failed_ = false;
  // SYNC_FD export transfers the semaphore payload. Export once, then duplicate
  // the retained descriptor for every caller (including EGL imports).
  bool exported_ = false;
  fml::UniqueFD sync_fd_;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_
