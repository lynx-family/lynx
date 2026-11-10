// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_fence_helper.h"

#include <fcntl.h>

#include <algorithm>

#include "base/include/fml/unique_fd.h"
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
  return can_destroy_ && fence_sync_count_ == 0;
}

void VulkanFenceHelper::RetainFenceSync() {
  std::lock_guard<std::mutex> lock(mutex_);
  ++fence_sync_count_;
}

void VulkanFenceHelper::ReleaseFenceSync() {
  std::lock_guard<std::mutex> lock(mutex_);
  --fence_sync_count_;
}

void VulkanFenceHelper::Destroy() {
  std::unique_lock<std::mutex> lock(valid_mutex_);
  if (!is_valid_) {
    return;
  }
  is_valid_ = false;
  sync_fd_.reset();
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
  if (!is_valid_ || failed_) {
    return false;
  }
  if (fence_ == VK_NULL_HANDLE) {
    FML_LOG(ERROR) << "VkFenceSync::ClientWait: fence is VK_NULL_HANDLE";
    return false;
  }
  auto result = VulkanHelper::GetInstance().WaitForFences(device_, 1, &fence_,
                                                          VK_TRUE, UINT64_MAX);
  if (!LogVkIfNotSuccess(result, "vkWaitForFences")) {
    failed_ = true;
    return false;
  }
  return true;
}

bool VulkanFenceHelper::WaitSemaphore() {
  if (eglGetCurrentContext() == EGL_NO_CONTEXT ||
      !AndroidEGLFenceSync::CheckAndroidNativeFenceSupported() ||
      !AndroidEGLFenceSync::CheckEGLWaitSupported()) {
    return false;
  }
  int exported_fd = -1;
  auto result = DupNativeFenceFD(&exported_fd);
  if (result == FenceSync::NativeExportResult::kSignaled) {
    return true;
  }
  if (result != FenceSync::NativeExportResult::kExported) {
    return false;
  }
  // EGL takes ownership on the call, even when sync creation fails.
  EGLint attribs[] = {EGL_SYNC_NATIVE_FENCE_FD_ANDROID, exported_fd, EGL_NONE};
  EGLDisplay egl_display = eglGetCurrentDisplay();
  EGLSyncKHR sync = AndroidEGLFenceSync::eglCreateSyncKHR(
      egl_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs);
  if (sync == EGL_NO_SYNC_KHR) {
    FML_LOG(ERROR) << "WaitSemaphore: failed to create EGL native fence sync";
    return false;
  }
  const bool waited = AndroidEGLFenceSync::WaitEGLSync(egl_display, sync);
  AndroidEGLFenceSync::eglDestroySyncKHR(egl_display, sync);
  return waited;
}

FenceSync::PollResult VulkanFenceHelper::PollLocked() {
  if (!is_valid_ || failed_ || fence_ == VK_NULL_HANDLE) {
    return FenceSync::PollResult::kFailed;
  }
  auto result = VulkanHelper::GetInstance().GetFenceStatus(device_, fence_);
  if (result == VK_SUCCESS) return FenceSync::PollResult::kSignaled;
  if (result == VK_NOT_READY) return FenceSync::PollResult::kPending;
  failed_ = true;
  return FenceSync::PollResult::kFailed;
}

FenceSync::PollResult VulkanFenceHelper::Poll() {
  // A concurrent legacy ClientWait may hold this mutex until GPU completion.
  std::unique_lock<std::mutex> lock(valid_mutex_, std::try_to_lock);
  if (!lock.owns_lock()) return FenceSync::PollResult::kPending;
  return PollLocked();
}

FenceSync::NativeExportResult VulkanFenceHelper::DupNativeFenceFD(int* out_fd) {
  using Result = FenceSync::NativeExportResult;
  if (!out_fd) return Result::kFailed;
  *out_fd = -1;
  std::unique_lock<std::mutex> lock(valid_mutex_, std::try_to_lock);
  if (!lock.owns_lock()) return Result::kFailed;
  auto status = PollLocked();
  if (status == FenceSync::PollResult::kFailed) return Result::kFailed;
  if (status == FenceSync::PollResult::kSignaled) return Result::kSignaled;
  if (!exported_) {
    if (semaphore_ == VK_NULL_HANDLE) return Result::kFailed;
    VkSemaphoreGetFdInfoKHR fd_info = {};
    fd_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    fd_info.semaphore = semaphore_;
    fd_info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
    int fd = -1;
    auto result =
        VulkanHelper::GetInstance().GetSemaphoreFdKHR(device_, &fd_info, &fd);
    if (result != VK_SUCCESS) {
      // Export failures do not consume the payload; callers can retry.
      if (result == VK_ERROR_DEVICE_LOST) failed_ = true;
      return Result::kFailed;
    }
    exported_ = true;
    sync_fd_.reset(fd);
  }
  // Vulkan permits -1 on successful SYNC_FD export for a signaled payload.
  if (!sync_fd_.is_valid()) return Result::kSignaled;
  *out_fd = fcntl(sync_fd_.get(), F_DUPFD_CLOEXEC, 0);
  return *out_fd >= 0 ? Result::kExported : Result::kFailed;
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
