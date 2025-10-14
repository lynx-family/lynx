// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_buffer.h"

#include <cstring>

#include "base/include/closure.h"
#include "clay/gfx/vulkan/vulkan_helper.h"

namespace clay {

namespace {
void createBuffer(VkDevice device, VkPhysicalDevice physical_device,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer *buffer,
                  VkDeviceMemory *buffer_memory) {
  VkBufferCreateInfo buffer_info;
  memset(&buffer_info, 0, sizeof(VkBufferCreateInfo));
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.flags = 0;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  buffer_info.queueFamilyIndexCount = 0;
  buffer_info.pQueueFamilyIndices = nullptr;

  auto result = VulkanHelper::GetInstance().CreateBuffer(device, &buffer_info,
                                                         nullptr, buffer);
  if (!LogVkIfNotSuccess(result, "vkCreateBuffer")) {
    FML_LOG(ERROR) << "vkCreateBuffer failed";
    return;
  }

  VkMemoryRequirements mem_requirements;
  VulkanHelper::GetInstance().GetBufferMemoryRequirements(device, *buffer,
                                                          &mem_requirements);

  VkMemoryAllocateInfo alloc_info;
  memset(&alloc_info, 0, sizeof(VkMemoryAllocateInfo));
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = nullptr;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = FindMemoryType(mem_requirements.memoryTypeBits,
                                              physical_device, properties);

  result = VulkanHelper::GetInstance().AllocateMemory(device, &alloc_info,
                                                      nullptr, buffer_memory);
  if (!LogVkIfNotSuccess(result, "vkAllocateMemory")) {
    return;
  }

  result = VulkanHelper::GetInstance().BindBufferMemory(device, *buffer,
                                                        *buffer_memory, 0);
  if (!LogVkIfNotSuccess(result, "vkBindBufferMemory")) {
    return;
  }
}
}  // namespace

void VulkanBuffer::Reset() {
  if (device_ == VK_NULL_HANDLE) {
    return;
  }
  if (buffer_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroyBuffer(device_, buffer_, nullptr);
    buffer_ = VK_NULL_HANDLE;
  }
  if (memory_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().FreeMemory(device_, memory_, nullptr);
    memory_ = VK_NULL_HANDLE;
  }
}

void VulkanBuffer::CreateIndexBuffer(VkDevice device,
                                     VkPhysicalDevice physical_device,
                                     const std::vector<uint32_t> &indices) {
  device_ = device;
  physical_device_ = physical_device;

  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = sizeof(indices[0]) * indices.size();
  buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  auto result = VulkanHelper::GetInstance().CreateBuffer(device, &buffer_info,
                                                         nullptr, &buffer_);
  if (!LogVkIfNotSuccess(result, "vkCreateBuffer")) {
    return;
  }

  VkMemoryRequirements mem_requirements;
  VulkanHelper::GetInstance().GetBufferMemoryRequirements(device, buffer_,
                                                          &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
      FindMemoryType(mem_requirements.memoryTypeBits, physical_device,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  VulkanHelper::GetInstance().AllocateMemory(device, &alloc_info, nullptr,
                                             &memory_);
  if (!LogVkIfNotSuccess(result, "vkAllocateMemory")) {
    return;
  }

  result =
      VulkanHelper::GetInstance().BindBufferMemory(device, buffer_, memory_, 0);
  if (!LogVkIfNotSuccess(result, "vkBindBufferMemory")) {
    return;
  }

  void *data;
  result = VulkanHelper::GetInstance().MapMemory(device, memory_, 0,
                                                 buffer_info.size, 0, &data);
  if (!LogVkIfNotSuccess(result, "vkMapMemory")) {
    return;
  }

  memcpy(data, indices.data(), buffer_info.size);
  VulkanHelper::GetInstance().UnmapMemory(device, memory_);
}

void VulkanBuffer::CreateUniformBuffer(VkDevice device,
                                       VkPhysicalDevice physical_device,
                                       UniformBufferObject ubo) {
  device_ = device;
  physical_device_ = physical_device;

  if (buffer_ == VK_NULL_HANDLE) {
    VkDeviceSize buffer_size = sizeof(UniformBufferObject);
    createBuffer(device, physical_device, buffer_size,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &buffer_, &memory_);
    if (buffer_ == VK_NULL_HANDLE) {
      return;
    }
  }

  void *data;
  auto result = VulkanHelper::GetInstance().MapMemory(device, memory_, 0,
                                                      sizeof(ubo), 0, &data);
  if (!LogVkIfNotSuccess(result, "vkMapMemory")) {
    return;
  }
  memcpy(data, &ubo, sizeof(ubo));
  VulkanHelper::GetInstance().UnmapMemory(device, memory_);
}

void VulkanBuffer::CreateVertexBuffer(VkDevice device,
                                      VkPhysicalDevice physical_device,
                                      const std::vector<Vertex> &vertices,
                                      VkCommandBuffer command_buffer) {
  device_ = device;
  physical_device_ = physical_device;

  VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

  if (buffer_ == VK_NULL_HANDLE) {
    // Create vertex buffer.
    createBuffer(device, physical_device, buffer_size,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &buffer_, &memory_);
  }

  void *data;
  auto result = VulkanHelper::GetInstance().MapMemory(device, memory_, 0,
                                                      buffer_size, 0, &data);
  if (!LogVkIfNotSuccess(result, "vkMapMemory")) {
    return;
  }
  memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
  VulkanHelper::GetInstance().UnmapMemory(device, memory_);
}

}  // namespace clay
