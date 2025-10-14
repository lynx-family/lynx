// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_command_pool_helper.h"

#include <cstring>
#include <vector>

#include "clay/gfx/vulkan/vulkan_helper.h"

namespace clay {

namespace {
int findGraphicsQueueFamilyIndex(VkPhysicalDevice device) {
  uint32_t queue_family_count = 0;
  clay::VulkanHelper::GetInstance().GetPhysicalDeviceQueueFamilyProperties(
      device, &queue_family_count, nullptr);

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  clay::VulkanHelper::GetInstance().GetPhysicalDeviceQueueFamilyProperties(
      device, &queue_family_count, queue_families.data());

  int i = 0;
  for (const auto& queue_family : queue_families) {
    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      return i;  // Return the index of the queue family that supports graphics
    }
    i++;
  }

  FML_LOG(ERROR) << "Failed to find a graphics-supporting queue family!";
  return -1;
}
}  // namespace

VulkanCommandPoolHelper::VulkanCommandPoolHelper(
    VkDevice device, VkPhysicalDevice physical_device)
    : device_(device), physical_device_(physical_device) {
  CreateCommandPool();
}

void VulkanCommandPoolHelper::CreateCommandPool() {
  uint32_t graphics_queue_family_index =
      findGraphicsQueueFamilyIndex(physical_device_);

  VkCommandPoolCreateInfo pool_info;
  memset(&pool_info, 0, sizeof(VkCommandPoolCreateInfo));
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.pNext = nullptr;
  pool_info.queueFamilyIndex = graphics_queue_family_index;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (!LogVkIfNotSuccess(clay::VulkanHelper::GetInstance().CreateCommandPool(
                             device_, &pool_info, nullptr, &command_pool_),
                         "vkCreateCommandPool")) {
    return;
  }
}

void VulkanCommandPoolHelper::InsertUsedBuffer(VkCMBStatus cmb_status) {
  used_buffers_.push_back(cmb_status);
}

void VulkanCommandPoolHelper::GetCommandBuffer(
    VkCommandBuffer* command_buffer) {
  if (!command_buffer) {
    return;
  }

  if (!free_buffers_.empty()) {
    *command_buffer = free_buffers_.front();
    free_buffers_.pop_front();
    return;
  }

  VkCommandBufferAllocateInfo alloc_info;
  memset(&alloc_info, 0, sizeof(VkCommandBufferAllocateInfo));
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.pNext = nullptr;
  alloc_info.commandPool = command_pool_;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  if (!LogVkIfNotSuccess(
          clay::VulkanHelper::GetInstance().AllocateCommandBuffers(
              device_, &alloc_info, command_buffer),
          "vkAllocateCommandBuffers")) {
    return;
  }
}

void VulkanCommandPoolHelper::Destroy() {
  // CAUTION: We consume the command buffers are all not in use, and this needs
  // to be assured outside.

  while (!used_buffers_.empty()) {
    // Destroy vkCommandPool will destroy all command buffers created from it,
    // so we do not destroy any cmb here.
    VkCMBStatus front_cmb = used_buffers_.front();
    if (!front_cmb.has_signal_semaphore && front_cmb.fence != VK_NULL_HANDLE) {
      VulkanHelper::GetInstance().DestroyFence(device_, front_cmb.fence,
                                               nullptr);
    }
    for (auto semaphore : front_cmb.semaphores) {
      if (semaphore != VK_NULL_HANDLE) {
        VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore,
                                                     nullptr);
      }
    }
    used_buffers_.pop_front();
  }
  free_buffers_.clear();

  // fence_semaphores_ contains all fences that submitted with a signal
  // semaphore.
  for (auto fence_semaphore : fence_semaphores_) {
    if (fence_semaphore) {
      fence_semaphore->Destroy();
    }
  }

  if (command_pool_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroyCommandPool(device_, command_pool_,
                                                   nullptr);
    command_pool_ = VK_NULL_HANDLE;
  }
}

void VulkanCommandPoolHelper::FreeUsedBuffers() {
  while (!used_buffers_.empty()) {
    VkCMBStatus front_cmb = used_buffers_.front();
    VkResult result =
        VulkanHelper::GetInstance().GetFenceStatus(device_, front_cmb.fence);
    if (result == VK_NOT_READY) {
      break;
    }
    if (result == VK_SUCCESS) {
      // this command buffer has already been executed. It is safe to reset.
      VulkanHelper::GetInstance().ResetCommandBuffer(front_cmb.command_buffer,
                                                     0);
      free_buffers_.push_back(front_cmb.command_buffer);
      // If there has been signal semaphores, we need to keep fence alive until
      // the sempahore waiting is done.
      if (!front_cmb.has_signal_semaphore &&
          front_cmb.fence != VK_NULL_HANDLE) {
        VulkanHelper::GetInstance().DestroyFence(device_, front_cmb.fence,
                                                 nullptr);
      }
      for (auto semaphore : front_cmb.semaphores) {
        if (semaphore != VK_NULL_HANDLE) {
          VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore,
                                                       nullptr);
        }
      }
      used_buffers_.pop_front();
    } else {
      LogVkIfNotSuccess(result, "vkGetFenceStatus");
      return;
    }
  }
}

void VulkanCommandPoolHelper::InsertFenceSemaphore(
    std::shared_ptr<VulkanFenceHelper> fence_semaphore) {
  fence_semaphores_.push_back(fence_semaphore);
}

void VulkanCommandPoolHelper::FreeSignalSemaphoresAndFences() {
  while (!fence_semaphores_.empty()) {
    auto fence_semaphore = fence_semaphores_.front();
    if (fence_semaphore == nullptr) {
      fence_semaphores_.pop_front();
      continue;
    }
    std::list<VkFence> pending_fences;
    for (auto cmb_status : used_buffers_) {
      pending_fences.push_back(cmb_status.fence);
    }
    if (fence_semaphore->CanDestroy() &&
        !fence_semaphore->InPendingQueue(pending_fences)) {
      fence_semaphore->Destroy();
      fence_semaphores_.pop_front();
    } else {
      break;
    }
  }
}

VulkanCommandPoolHelper::~VulkanCommandPoolHelper() = default;

}  // namespace clay
