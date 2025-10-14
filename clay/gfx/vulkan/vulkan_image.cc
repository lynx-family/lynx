// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_image.h"

#include <cstring>

#include "clay/gfx/shared_image/android/android_hardwarebuffer_utils.h"
#include "clay/gfx/vulkan/vulkan_helper.h"

namespace clay {

VulkanImage::VulkanImage(VkDevice device, VkPhysicalDevice physical_device)
    : device_(device), physical_device_(physical_device) {}

VulkanImage::~VulkanImage() {
  if (image_view_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroyImageView(device_, image_view_, nullptr);
    image_view_ = VK_NULL_HANDLE;
  }
  if (image_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroyImage(device_, image_, nullptr);
  }

  if (image_device_memory_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().FreeMemory(device_, image_device_memory_,
                                           nullptr);
  }
}

void VulkanImage::CreateFromAndroidBuffer(AHardwareBuffer* buffer) {
  CreateImage(buffer);
  CreateImageView();
}

void VulkanImage::CreateImage(AHardwareBuffer* hw_buffer) {
  VkAndroidHardwareBufferPropertiesANDROID buffer_properties = {};
  buffer_properties.sType =
      VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID;
  auto result =
      VulkanHelper::GetInstance().GetAndroidHardwareBufferPropertiesANDROID(
          device_, hw_buffer, &buffer_properties);
  if (!LogVkIfNotSuccess(result,
                         "vkGetAndroidHardwareBufferPropertiesANDROID")) {
    return;
  }

  AHardwareBuffer_Desc desc;
  AHardwareBufferUtils::GetInstance().Describe(hw_buffer, &desc);
  width_ = desc.width;
  height_ = desc.height;

  // Create VkImage that is related to AHardwareBuffer.
  VkExternalMemoryImageCreateInfo external_memory_image_create_info;
  memset(&external_memory_image_create_info, 0,
         sizeof(VkExternalMemoryImageCreateInfo));
  external_memory_image_create_info.sType =
      VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
  external_memory_image_create_info.pNext = nullptr;
  external_memory_image_create_info.handleTypes =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;

  VkImageCreateInfo image_create_info;
  memset(&image_create_info, 0, sizeof(VkImageCreateInfo));
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.flags = 0;
  image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  image_create_info.extent.width = width_;
  image_create_info.extent.height = height_;
  image_create_info.extent.depth = 1;
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.initialLayout =
      VK_IMAGE_LAYOUT_UNDEFINED;  // It has to be undefined if it is an external
                                  // image.
  image_create_info.queueFamilyIndexCount = 0,
  image_create_info.pQueueFamilyIndices = nullptr,
  image_create_info.pNext = &external_memory_image_create_info;

  VkImage image;
  result = VulkanHelper::GetInstance().CreateImage(device_, &image_create_info,
                                                   nullptr, &image);
  if (!LogVkIfNotSuccess(result, "vkCreateImage")) {
    return;
  }

  VkMemoryDedicatedAllocateInfo dedicated_info = {};
  dedicated_info.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
  dedicated_info.pNext = nullptr;
  dedicated_info.image = image;

  VkImportAndroidHardwareBufferInfoANDROID import_info = {};
  import_info.sType =
      VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID;
  import_info.pNext = &dedicated_info;
  import_info.buffer = hw_buffer;

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext = &import_info;
  alloc_info.allocationSize = buffer_properties.allocationSize;
  alloc_info.memoryTypeIndex =
      FindMemoryType(buffer_properties.memoryTypeBits, physical_device_,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkDeviceMemory memory;
  result = VulkanHelper::GetInstance().AllocateMemory(device_, &alloc_info,
                                                      nullptr, &memory);
  if (!LogVkIfNotSuccess(result, "vkAllocateMemory")) {
    return;
  }

  result =
      VulkanHelper::GetInstance().BindImageMemory(device_, image, memory, 0);
  if (!LogVkIfNotSuccess(result, "vkBindImageMemory")) {
    return;
  }

  image_ = image;
  image_device_memory_ = memory;
}

void VulkanImage::CreateImageView() {
  FML_DCHECK(image_ != VK_NULL_HANDLE);
  VkImageViewCreateInfo view_info;
  memset(&view_info, 0, sizeof(VkImageViewCreateInfo));
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.pNext = nullptr;
  view_info.flags = 0;
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  VkImageView image_view;
  auto result = VulkanHelper::GetInstance().CreateImageView(
      device_, &view_info, nullptr, &image_view);
  if (!LogVkIfNotSuccess(result, "vkCreateImageView")) {
    return;
  }

  image_view_ = image_view;
}

void VulkanImage::ApplyImagerBarrier(VkCommandBuffer command_buffer,
                                     VkImageLayout old_layout,
                                     VkImageLayout new_layout,
                                     VkAccessFlags src_access_mask,
                                     VkAccessFlags dst_access_mask,
                                     VkPipelineStageFlagBits src_stage_mask,
                                     VkPipelineStageFlagBits dst_stage_mask) {
  VkImageMemoryBarrier barrier;
  memset(&barrier, 0, sizeof(VkImageMemoryBarrier));
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.pNext = nullptr;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcAccessMask = src_access_mask;
  barrier.dstAccessMask = dst_access_mask;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image_;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VulkanHelper::GetInstance().CmdPipelineBarrier(command_buffer, src_stage_mask,
                                                 dst_stage_mask, 0, 0, nullptr,
                                                 0, nullptr, 1, &barrier);
}

}  // namespace clay
