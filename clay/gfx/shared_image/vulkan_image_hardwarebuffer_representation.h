// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_VULKAN_IMAGE_HARDWAREBUFFER_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_VULKAN_IMAGE_HARDWAREBUFFER_REPRESENTATION_H_

#include <vulkan/vulkan.h>

#include <memory>

#include "clay/gfx/shared_image/android_hardwarebuffer_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/vulkan/vulkan_command_pool_helper.h"
#include "clay/gfx/vulkan/vulkan_image.h"

namespace clay {

class VkFenceSync final : public FenceSync {
 public:
  explicit VkFenceSync(std::shared_ptr<VulkanFenceHelper> fence_semaphore);

  bool ClientWait() override;

  void ServerWait();

  ~VkFenceSync() override;

  Type GetType() const override { return Type::kVulkan; }

 private:
  std::shared_ptr<VulkanFenceHelper> fence_semaphore_;
};

class VulkanImageHardwareBufferRepresentation
    : public SharedImageRepresentation {
 public:
  VulkanImageHardwareBufferRepresentation(
      fml::RefPtr<SharedImageBacking> backing, void* device,
      void* physical_device, void* queue);
  ~VulkanImageHardwareBufferRepresentation() override;

  ImageRepresentationType GetType() const override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndRead() override;
  bool EndWrite() override;

  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

  void SetCommandPoolHelper(std::shared_ptr<VulkanCommandPoolHelper> helper);

  void PostDrawVk();

  void ChangeImageLayout(VkImageLayout old_layout, VkImageLayout new_layout,
                         VkAccessFlags src_access_mask,
                         VkAccessFlags dst_access_mask,
                         VkPipelineStageFlagBits src_stage_mask,
                         VkPipelineStageFlagBits dst_stage_mask,
                         uint32_t wait_semaphore_count,
                         const VkSemaphore* wait_semaphores,
                         const VkPipelineStageFlags* wait_stages,
                         uint32_t signal_semaphore_count,
                         const VkSemaphore* signal_semaphores, VkFence fence);

 private:
  void CreateVulkanImage(void* buffer);

  VkDevice device_;
  VkPhysicalDevice physical_device_;
  VkQueue queue_;
  std::unique_ptr<VulkanImage> vulkan_image_;

  std::shared_ptr<VulkanFenceHelper> last_fence_semaphore_;

  std::shared_ptr<VulkanCommandPoolHelper> command_pool_helper_;
  VkImageLayout old_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
};
}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_VULKAN_IMAGE_HARDWAREBUFFER_REPRESENTATION_H_
