// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h"

#include <cstring>
#include <utility>

#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/vulkan/vulkan_helper.h"

namespace clay {

VkFenceSync::VkFenceSync(std::shared_ptr<VulkanFenceHelper> fence_semaphore)
    : fence_semaphore_(fence_semaphore) {}

// VkFence and VkSemaphore would be destroyed by the VulkanCommandPoolHelper.
VkFenceSync::~VkFenceSync() {
  if (fence_semaphore_) {
    fence_semaphore_->MarkCanDestroy();
  }
}

bool VkFenceSync::ClientWait() {
  if (!fence_semaphore_) {
    FML_LOG(ERROR) << "VkFenceSync::ClientWait: fence_semaphore_ is null";
    return false;
  }
  return fence_semaphore_->WaitForFence();
}

void VkFenceSync::ServerWait() {
  if (!fence_semaphore_) {
    FML_LOG(ERROR) << "VkFenceSync::ServerWait: fence_semaphore_ is null";
    return;
  }
  if (!fence_semaphore_->WaitSemaphore()) {
    // Fallback to client wait.
    FML_LOG(ERROR) << "Vulkan fence sync fd is -1, fallback to client wait.";
    ClientWait();
    return;
  }
}

VulkanImageHardwareBufferRepresentation::
    VulkanImageHardwareBufferRepresentation(
        fml::RefPtr<SharedImageBacking> backing, void* device,
        void* physical_device, void* queue)
    : SharedImageRepresentation(std::move(backing)),
      device_(reinterpret_cast<VkDevice>(device)),
      physical_device_(reinterpret_cast<VkPhysicalDevice>(physical_device)),
      queue_(reinterpret_cast<VkQueue>(queue)) {
  AHardwareBufferImageBacking* buffer_backing =
      static_cast<AHardwareBufferImageBacking*>(GetBacking());
  CreateVulkanImage(buffer_backing->GetGFXHandle());
}

VulkanImageHardwareBufferRepresentation::
    ~VulkanImageHardwareBufferRepresentation() = default;

ImageRepresentationType VulkanImageHardwareBufferRepresentation::GetType()
    const {
  return ImageRepresentationType::kVulkanImage;
}

bool VulkanImageHardwareBufferRepresentation::BeginRead(
    ClaySharedImageReadResult* out) {
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeVulkan;
  out->vulkan_image.struct_size = sizeof(ClayVulkanImage);
  out->vulkan_image.vulkan_image = vulkan_image_.get();

  out->vulkan_image.user_data = this;
  this->AddRef();
  out->vulkan_image.destruction_callback = [](void* user_data) {
    static_cast<VulkanImageHardwareBufferRepresentation*>(user_data)->Release();
  };
  return true;
}

void VulkanImageHardwareBufferRepresentation::CreateVulkanImage(void* buffer) {
  if (vulkan_image_) {
    // Use the existing image.
    return;
  }
  vulkan_image_ = std::make_unique<VulkanImage>(device_, physical_device_);
  AHardwareBuffer* hw_buffer = reinterpret_cast<AHardwareBuffer*>(buffer);
  vulkan_image_->CreateFromAndroidBuffer(hw_buffer);
}

bool VulkanImageHardwareBufferRepresentation::EndRead() { return true; }

bool VulkanImageHardwareBufferRepresentation::BeginWrite(
    ClaySharedImageWriteResult* out) {
  FML_UNIMPLEMENTED();
  return false;
}

bool VulkanImageHardwareBufferRepresentation::EndWrite() {
  FML_UNIMPLEMENTED();
  return false;
}

void VulkanImageHardwareBufferRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (fence_sync->GetType() == FenceSync::Type::kEGL) {
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;
    auto result = VulkanHelper::GetInstance().CreateSemaphore(
        device_, &semaphore_info, nullptr, &semaphore);
    if (!LogVkIfNotSuccess(result, "vkCreateSemaphore")) {
      return;
    }

    auto sync_fd =
        static_cast<AndroidEGLFenceSync*>(fence_sync.get())->GetSyncFD();
    if (sync_fd == -1) {
      VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
      fence_sync->ClientWait();
      return;
    }

    VkImportSemaphoreFdInfoKHR import_semaphore_info = {};
    import_semaphore_info.sType =
        VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
    import_semaphore_info.semaphore = semaphore;
    import_semaphore_info.flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
    import_semaphore_info.handleType =
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR;
    import_semaphore_info.fd = sync_fd;
    result = VulkanHelper::GetInstance().ImportSemaphoreFdKHR(
        device_, &import_semaphore_info);
    if (!LogVkIfNotSuccess(result, "vkImportSemaphoreFdKHR")) {
      return;
    }

    VkFence fence;
    VkFenceCreateInfo fence_info;
    memset(&fence_info, 0, sizeof(VkFenceCreateInfo));
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.pNext = nullptr;
    fence_info.flags = 0;
    result = VulkanHelper::GetInstance().CreateFence(device_, &fence_info,
                                                     nullptr, &fence);
    if (!LogVkIfNotSuccess(result, "vkCreateFence")) {
      return;
    }

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

    ChangeImageLayout(old_layout_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 1, &semaphore,
                      wait_stages, 0, nullptr, fence);
    if (old_layout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
      old_layout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    return;
  }
  fence_sync->ClientWait();
}

void VulkanImageHardwareBufferRepresentation::ChangeImageLayout(
    VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
    VkPipelineStageFlagBits src_stage_mask,
    VkPipelineStageFlagBits dst_stage_mask, uint32_t wait_semaphore_count,
    const VkSemaphore* wait_semaphores, const VkPipelineStageFlags* wait_stages,
    uint32_t signal_semaphore_count, const VkSemaphore* signal_semaphores,
    VkFence fence) {
  VkCommandBuffer command_buffer;
  command_pool_helper_->GetCommandBuffer(&command_buffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (!LogVkIfNotSuccess(clay::VulkanHelper::GetInstance().BeginCommandBuffer(
                             command_buffer, &beginInfo),
                         "vkBeginCommandBuffer")) {
    return;
  }

  vulkan_image_->ApplyImagerBarrier(command_buffer, old_layout, new_layout,
                                    src_access_mask, dst_access_mask,
                                    src_stage_mask, dst_stage_mask);

  if (!LogVkIfNotSuccess(
          clay::VulkanHelper::GetInstance().EndCommandBuffer(command_buffer),
          "vkEndCommandBuffer")) {
    return;
  }

  VulkanCommandPoolHelper::VkCMBStatus cmb_status;
  cmb_status.command_buffer = command_buffer;
  cmb_status.fence = fence;
  for (auto i = 0u; i < wait_semaphore_count; i++) {
    cmb_status.semaphores.push_back(wait_semaphores[i]);
  }
  cmb_status.has_signal_semaphore = signal_semaphore_count > 0;

  command_pool_helper_->InsertUsedBuffer(cmb_status);

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = nullptr;
  submit_info.waitSemaphoreCount = wait_semaphore_count;
  submit_info.pWaitSemaphores = wait_semaphores;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.signalSemaphoreCount = signal_semaphore_count;
  submit_info.pSignalSemaphores = signal_semaphores;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  auto result =
      VulkanHelper::GetInstance().QueueSubmit(queue_, 1, &submit_info, fence);
  if (!LogVkIfNotSuccess(result, "vkQueueSubmit")) {
    return;
  }
}

std::unique_ptr<FenceSync>
VulkanImageHardwareBufferRepresentation::ProduceFence() {
  return std::make_unique<VkFenceSync>(last_fence_semaphore_);
}

void VulkanImageHardwareBufferRepresentation::SetCommandPoolHelper(
    std::shared_ptr<VulkanCommandPoolHelper> helper) {
  command_pool_helper_ = helper;
}

void VulkanImageHardwareBufferRepresentation::PostDrawVk() {
  // We need to change the VkImage's imageLayout from
  // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL to
  // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL. And signal a semaphore which will
  // be exported to OpenGL side once the barrier is applied.
  VkExportSemaphoreCreateInfo export_create_info = {};
  export_create_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
  export_create_info.pNext = nullptr;
  export_create_info.handleTypes =
      VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;  // use as export sync fd

  VkSemaphore semaphore;
  VkSemaphoreCreateInfo semaphore_create_info = {};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_create_info.pNext = &export_create_info;
  semaphore_create_info.flags = 0;
  auto result = VulkanHelper::GetInstance().CreateSemaphore(
      device_, &semaphore_create_info, nullptr, &semaphore);
  if (!LogVkIfNotSuccess(result, "vkCreateSemaphore")) {
    return;
  }

  VkFence fence;
  VkFenceCreateInfo fence_info;
  memset(&fence_info, 0, sizeof(VkFenceCreateInfo));
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.pNext = nullptr;
  fence_info.flags = 0;
  result = VulkanHelper::GetInstance().CreateFence(device_, &fence_info,
                                                   nullptr, &fence);
  if (!LogVkIfNotSuccess(result, "vkCreateFence")) {
    return;
  }

  ChangeImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_ACCESS_SHADER_READ_BIT,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, nullptr,
                    nullptr, 1, &semaphore, fence);

  // Signal semaphores and feneces will be waited in clay raster thread.
  // So we need to trace their lifetime to avoid destroy them before they are
  // waited.
  std::shared_ptr<clay::VulkanFenceHelper> fence_helper =
      std::make_shared<clay::VulkanFenceHelper>(device_, fence, semaphore);
  command_pool_helper_->InsertFenceSemaphore(fence_helper);

  last_fence_semaphore_ = fence_helper;
}

}  // namespace clay
