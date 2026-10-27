// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_
#define CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_

#include <vulkan/vulkan.h>

#include <list>
#include <mutex>
#include <utility>

namespace clay {

// Used to help check when we can safely destroy a fence or semaphore in vulkan.
class VulkanFenceHelper {
 public:
  VulkanFenceHelper(VkDevice device, VkFence fence, VkSemaphore semaphore);
  ~VulkanFenceHelper() = default;

  void MarkCanDestroy();
  bool CanDestroy();

  void Destroy();

  bool InPendingQueue(const std::list<VkFence> &pending_fences);

  bool WaitForFence();
  bool WaitSemaphore();

 private:
  int32_t GetSyncFd();

  VkDevice device_ = VK_NULL_HANDLE;
  VkFence fence_ = VK_NULL_HANDLE;
  VkSemaphore semaphore_ = VK_NULL_HANDLE;

  std::mutex mutex_;
  bool can_destroy_ = false;

  std::mutex valid_mutex_;
  bool is_valid_ = true;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_FENCE_HELPER_H_
