// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_COMMAND_POOL_HELPER_H_
#define CLAY_GFX_VULKAN_VULKAN_COMMAND_POOL_HELPER_H_

#include <vulkan/vulkan.h>

#include <list>
#include <memory>

#include "clay/gfx/vulkan/vulkan_fence_helper.h"

namespace clay {

class VulkanCommandPoolHelper {
 public:
  struct VkCMBStatus {
    VkCommandBuffer command_buffer;
    VkFence fence;
    std::list<VkSemaphore> semaphores;
    bool has_signal_semaphore;
  };

  VulkanCommandPoolHelper(VkDevice device, VkPhysicalDevice physical_device);
  ~VulkanCommandPoolHelper();

  void GetCommandBuffer(VkCommandBuffer* command_buffer);

  void InsertUsedBuffer(VkCMBStatus cmb_status);

  void FreeUsedBuffers();

  void InsertFenceSemaphore(
      std::shared_ptr<clay::VulkanFenceHelper> fence_semaphore);
  void FreeSignalSemaphoresAndFences();

  void Destroy();

 private:
  void CreateCommandPool();

  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;

  VkCommandPool command_pool_ = VK_NULL_HANDLE;
  std::list<VkCMBStatus> used_buffers_;
  std::list<VkCommandBuffer> free_buffers_;

  // Signal semaphores and fences will not be destroyed when it is done in vk
  // queue, because clay raster thread may still want to check its status.
  std::list<std::shared_ptr<clay::VulkanFenceHelper>> fence_semaphores_;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_COMMAND_POOL_HELPER_H_
