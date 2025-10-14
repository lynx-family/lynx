// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_fence_helper.h"

#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/vulkan/vulkan_helper.h"
namespace clay {

VulkanFenceHelper::VulkanFenceHelper(VkDevice device, VkFence fence,
                                     VkSemaphore semaphore)
    : device_(device), fence_(fence), semaphore_(semaphore) {}

void VulkanFenceHelper::MarkCanDestroy() {
  std::unique_lock<std::mutex> lock(mutex_);
  can_destroy_ = true;
}

bool VulkanFenceHelper::CanDestroy() {
  std::unique_lock<std::mutex> lock(mutex_);
  return can_destroy_;
}

void VulkanFenceHelper::Destroy() {
  std::unique_lock<std::mutex> lock(valid_mutex_);
  if (!is_valid_) {
    return;
  }
  is_valid_ = false;
  if (fence_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroyFence(device_, fence_, nullptr);
    fence_ = VK_NULL_HANDLE;
  }
  if (semaphore_ != VK_NULL_HANDLE) {
    VulkanHelper::GetInstance().DestroySemaphore(device_, semaphore_, nullptr);
    semaphore_ = VK_NULL_HANDLE;
  }
}

bool VulkanFenceHelper::WaitForFence() {
  std::unique_lock<std::mutex> lock(valid_mutex_);
  if (!is_valid_) {
    return false;
  }
  if (fence_ == VK_NULL_HANDLE) {
    FML_LOG(ERROR) << "VkFenceSync::ClientWait: fence is VK_NULL_HANDLE";
    return false;
  }
  auto result = VulkanHelper::GetInstance().WaitForFences(device_, 1, &fence_,
                                                          VK_TRUE, UINT64_MAX);
  if (!LogVkIfNotSuccess(result, "vkWaitForFences")) {
    return false;
  }
  return true;
}

bool VulkanFenceHelper::WaitSemaphore() {
  std::unique_lock<std::mutex> lock(valid_mutex_);
  if (!is_valid_) {
    return false;
  }
  int32_t fd = GetSyncFd();
  if (fd == -1) {
    return false;
  }
  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fd, EGL_NONE};
  EGLDisplay egl_display = eglGetCurrentDisplay();
  EGLSyncKHR sync = AndroidEGLFenceSync::eglCreateSyncKHR(
      egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
  AndroidEGLFenceSync::eglWaitSyncKHR(egl_display, sync, 0);
  AndroidEGLFenceSync::eglDestroySyncKHR(egl_display, sync);
  FML_LOG(ERROR) << "service wait done";
  return true;
}

int32_t VulkanFenceHelper::GetSyncFd() {
  if (semaphore_ == VK_NULL_HANDLE) {
    return -1;
  }
  VkSemaphoreGetFdInfoKHR fd_info = {};
  fd_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
  fd_info.pNext = nullptr;
  fd_info.semaphore = semaphore_;
  fd_info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;

  int fd;
  VkResult result =
      VulkanHelper::GetInstance().GetSemaphoreFdKHR(device_, &fd_info, &fd);
  // Some devices or drivers may fail to get the FD for VkSemaphore.
  if (result != VK_SUCCESS) {
    FML_LOG(ERROR) << "GetSyncFd: failed to get FD for VkSemaphore";
    return -1;
  }
  return fd;
}

bool VulkanFenceHelper::InPendingQueue(
    const std::list<VkFence>& pending_fences) {
  std::unique_lock<std::mutex> lock(valid_mutex_);
  if (!is_valid_) {
    return false;
  }
  return std::find(pending_fences.begin(), pending_fences.end(), fence_) !=
         pending_fences.end();
}

}  // namespace clay
