// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h"

#include <cstring>
#include <utility>

#include "base/include/fml/unique_fd.h"
#include "clay/gfx/vulkan/vulkan_helper.h"

namespace clay {

VkFenceSync::VkFenceSync(std::shared_ptr<VulkanFenceHelper> fence_semaphore)
    : fence_semaphore_(std::move(fence_semaphore)) {
  if (fence_semaphore_) fence_semaphore_->RetainFenceSync();
}

// VkFence and VkSemaphore would be destroyed by the VulkanCommandPoolHelper.
VkFenceSync::~VkFenceSync() {
  if (fence_semaphore_) {
    fence_semaphore_->ReleaseFenceSync();
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
  if (!EnqueueWait()) {
    ClientWait();
  }
}

FenceSync::WaitResult VkFenceSync::EnqueueWaitResult() {
  return fence_semaphore_ && fence_semaphore_->WaitSemaphore()
             ? WaitResult::kSuccess
             : WaitResult::kFailed;
}

FenceSync::PollResult VkFenceSync::Poll() {
  return fence_semaphore_ ? fence_semaphore_->Poll() : PollResult::kFailed;
}

FenceSync::NativeExportResult VkFenceSync::DupNativeFenceFD(int* out_fd) {
  if (out_fd) *out_fd = -1;
  return fence_semaphore_ ? fence_semaphore_->DupNativeFenceFD(out_fd)
                          : NativeExportResult::kFailed;
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
    ~VulkanImageHardwareBufferRepresentation() {
  if (last_fence_semaphore_) last_fence_semaphore_->MarkCanDestroy();
}

ImageRepresentationType VulkanImageHardwareBufferRepresentation::GetType()
    const {
  return ImageRepresentationType::kVulkanImage;
}

bool VulkanImageHardwareBufferRepresentation::BeginRead(
    ClaySharedImageReadResult* out) {
  if (!access_ready_ || !vulkan_image_ || !vulkan_image_->IsValid())
    return false;
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
  access_ready_ = vulkan_image_->CreateFromAndroidBuffer(hw_buffer);
  if (!access_ready_) vulkan_image_.reset();
}

bool VulkanImageHardwareBufferRepresentation::EndRead() {
  return access_ready_;
}

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
  access_ready_ = vulkan_image_ && vulkan_image_->IsValid() &&
                  ConsumeFenceInternal(fence_sync.get());
  if (!access_ready_) GetBacking()->SetFenceSync(std::move(fence_sync));
}

bool VulkanImageHardwareBufferRepresentation::ConsumeFenceInternal(
    FenceSync* fence_sync) {
  int fd = -1;
  auto exported = fence_sync->DupNativeFenceFD(&fd);
  fml::UniqueFD sync_fd(fd);
  VkSemaphore semaphore = VK_NULL_HANDLE;
  if (exported == FenceSync::NativeExportResult::kExported) {
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = nullptr;
    semaphore_info.flags = 0;
    auto result = VulkanHelper::GetInstance().CreateSemaphore(
        device_, &semaphore_info, nullptr, &semaphore);
    if (!LogVkIfNotSuccess(result, "vkCreateSemaphore")) {
      return false;
    }

    VkImportSemaphoreFdInfoKHR import_semaphore_info = {};
    import_semaphore_info.sType =
        VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
    import_semaphore_info.semaphore = semaphore;
    import_semaphore_info.flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
    import_semaphore_info.handleType =
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR;
    import_semaphore_info.fd = sync_fd.get();
    result = VulkanHelper::GetInstance().ImportSemaphoreFdKHR(
        device_, &import_semaphore_info);
    if (!LogVkIfNotSuccess(result, "vkImportSemaphoreFdKHR")) {
      VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
      return false;
    }
    [[maybe_unused]] const int transferred_fd = sync_fd.release();
  } else if (exported != FenceSync::NativeExportResult::kSignaled &&
             !fence_sync->ClientWait()) {
    return false;
  }

  // Even a completed fence or a CPU fallback still needs the image barrier.
  VkFence fence = VK_NULL_HANDLE;
  VkFenceCreateInfo fence_info = {};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  auto result = VulkanHelper::GetInstance().CreateFence(device_, &fence_info,
                                                        nullptr, &fence);
  if (!LogVkIfNotSuccess(result, "vkCreateFence")) {
    if (semaphore != VK_NULL_HANDLE) {
      VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
    }
    return false;
  }

  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT};

  if (!ChangeImageLayout(old_layout_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         semaphore != VK_NULL_HANDLE ? 1 : 0, &semaphore,
                         wait_stages, 0, nullptr, fence)) {
    VulkanHelper::GetInstance().DestroyFence(device_, fence, nullptr);
    if (semaphore != VK_NULL_HANDLE) {
      VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
    }
    return false;
  }
  if (old_layout_ == VK_IMAGE_LAYOUT_UNDEFINED) {
    old_layout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
  return true;
}

bool VulkanImageHardwareBufferRepresentation::ChangeImageLayout(
    VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask,
    VkPipelineStageFlagBits src_stage_mask,
    VkPipelineStageFlagBits dst_stage_mask, uint32_t wait_semaphore_count,
    const VkSemaphore* wait_semaphores, const VkPipelineStageFlags* wait_stages,
    uint32_t signal_semaphore_count, const VkSemaphore* signal_semaphores,
    VkFence fence) {
  if (!command_pool_helper_ || !vulkan_image_ || !vulkan_image_->IsValid())
    return false;
  VkCommandBuffer command_buffer = VK_NULL_HANDLE;
  command_pool_helper_->GetCommandBuffer(&command_buffer);
  if (command_buffer == VK_NULL_HANDLE) {
    FML_LOG(ERROR) << "Failed to allocate command buffer";
    return false;
  }

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (!LogVkIfNotSuccess(clay::VulkanHelper::GetInstance().BeginCommandBuffer(
                             command_buffer, &beginInfo),
                         "vkBeginCommandBuffer")) {
    return false;
  }

  vulkan_image_->ApplyImagerBarrier(command_buffer, old_layout, new_layout,
                                    src_access_mask, dst_access_mask,
                                    src_stage_mask, dst_stage_mask);

  if (!LogVkIfNotSuccess(
          clay::VulkanHelper::GetInstance().EndCommandBuffer(command_buffer),
          "vkEndCommandBuffer")) {
    return false;
  }

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
    return false;
  }

  VulkanCommandPoolHelper::VkCMBStatus cmb_status;
  cmb_status.command_buffer = command_buffer;
  cmb_status.fence = fence;
  for (auto i = 0u; i < wait_semaphore_count; i++) {
    cmb_status.semaphores.push_back(wait_semaphores[i]);
  }
  cmb_status.has_signal_semaphore = signal_semaphore_count > 0;
  command_pool_helper_->InsertUsedBuffer(cmb_status);
  return true;
}

std::unique_ptr<FenceSync>
VulkanImageHardwareBufferRepresentation::ProduceFence() {
  if (!vulkan_image_ || !vulkan_image_->IsValid()) {
    return std::make_unique<VkFenceSync>(nullptr);
  }
  if (!access_ready_) {
    auto failed = GetBacking()->GetFenceSync();
    if (failed) return failed;
    return std::make_unique<VkFenceSync>(nullptr);
  }
  return std::make_unique<VkFenceSync>(last_fence_semaphore_);
}

void VulkanImageHardwareBufferRepresentation::SetCommandPoolHelper(
    std::shared_ptr<VulkanCommandPoolHelper> helper) {
  command_pool_helper_ = helper;
}

void VulkanImageHardwareBufferRepresentation::PostDrawVk() {
  if (!access_ready_) return;
  if (last_fence_semaphore_) last_fence_semaphore_->MarkCanDestroy();
  last_fence_semaphore_.reset();
  // A failed submission must not publish the preceding frame's proof.
  access_ready_ = false;

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
    VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
    return;
  }

  if (!ChangeImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_ACCESS_SHADER_READ_BIT,
                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
                         nullptr, nullptr, 1, &semaphore, fence)) {
    VulkanHelper::GetInstance().DestroyFence(device_, fence, nullptr);
    VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore, nullptr);
    return;
  }

  // Signal semaphores and feneces will be waited in clay raster thread.
  // So we need to trace their lifetime to avoid destroy them before they are
  // waited.
  std::shared_ptr<clay::VulkanFenceHelper> fence_helper =
      std::make_shared<clay::VulkanFenceHelper>(device_, fence, semaphore);
  command_pool_helper_->InsertFenceSemaphore(fence_helper);

  last_fence_semaphore_ = fence_helper;
  access_ready_ = true;
}

}  // namespace clay
