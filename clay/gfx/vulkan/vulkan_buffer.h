// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_BUFFER_H_
#define CLAY_GFX_VULKAN_VULKAN_BUFFER_H_

#include <vulkan/vulkan.h>

#include <vector>

namespace clay {

struct Vertex {
  float position[2];
  float tex_coord[2];
};

struct TransformBlock {
  float transform[16];
};

struct UniformBufferObject {
  float transform[16];
  float width;
  float height;
  float padding[2];  // padding align to mat4
};

class VulkanBuffer {
 public:
  VulkanBuffer() = default;
  ~VulkanBuffer() = default;

  void CreateIndexBuffer(VkDevice device, VkPhysicalDevice physical_device,
                         const std::vector<uint32_t>& indices);

  void CreateUniformBuffer(VkDevice device, VkPhysicalDevice physical_device,
                           UniformBufferObject ubo);

  void CreateVertexBuffer(VkDevice device, VkPhysicalDevice physical_device,
                          const std::vector<Vertex>& vertices,
                          VkCommandBuffer command_buffer);

  void Reset();

  VkBuffer GetBuffer() const { return buffer_; }
  VkDeviceMemory GetMemory() const { return memory_; }

 private:
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory memory_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_BUFFER_H_
