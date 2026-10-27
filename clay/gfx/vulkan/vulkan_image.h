// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_IMAGE_H_
#define CLAY_GFX_VULKAN_VULKAN_IMAGE_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

namespace clay {

class VulkanImage {
 public:
  VulkanImage(VkDevice device, VkPhysicalDevice physical_device);
  ~VulkanImage();

  void CreateFromAndroidBuffer(AHardwareBuffer* buffer);

  void ApplyImagerBarrier(VkCommandBuffer command_buffer,
                          VkImageLayout old_layout, VkImageLayout new_layout,
                          VkAccessFlags src_access_mask,
                          VkAccessFlags dst_access_mask,
                          VkPipelineStageFlagBits src_stage_mask,
                          VkPipelineStageFlagBits dst_stage_mask);

  VkImage GetImage() { return image_; }
  VkImageView GetImageView() { return image_view_; }

  int Width() const { return width_; }
  int Height() const { return height_; }

 private:
  void CreateImage(AHardwareBuffer* buffer);
  void CreateImageView();

  VkDevice device_;
  VkPhysicalDevice physical_device_;

  VkImage image_ = VK_NULL_HANDLE;
  VkDeviceMemory image_device_memory_ = VK_NULL_HANDLE;
  VkImageView image_view_ = VK_NULL_HANDLE;

  int width_ = 0;
  int height_ = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_IMAGE_H_
