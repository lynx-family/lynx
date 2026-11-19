// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <fcntl.h>
#include <unistd.h>

#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

#include "clay/gfx/shared_image/android/android_hardwarebuffer_utils.h"
#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/shared_image/tests/api_contract.h"
#include "clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h"
#include "clay/gfx/vulkan/vulkan_helper.h"

#define CHECK_FENCE(x)                                     \
  do {                                                     \
    if (!(x)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); \
      std::abort();                                        \
    }                                                      \
  } while (0)

namespace {
VkResult status = VK_NOT_READY, export_result = VK_SUCCESS;
bool export_signaled = false, create_fails = false, wait_fails = false;
bool current_context = true, native_extension = true;
bool wait_extension = true, wait_proc = true;
int exports = 0, waits = 0, imports = 0, gpu_waits = 0, destroyed = 0;
int master_fd = -1, replacement_fd = -1;
std::mutex wait_mutex;
std::condition_variable wait_cv;
bool block_wait = false, entered_wait = false, release_wait = false;
VkResult import_result = VK_SUCCESS, submit_result = VK_SUCCESS;
VkResult create_semaphore_result = VK_SUCCESS, create_fence_result = VK_SUCCESS;
int barriers = 0, submissions = 0, imported_fd = -1;
uint32_t submitted_waits = 0;
uintptr_t next_handle = 10;
int image_failure = 0;
int live_images = 0, live_memory = 0, live_views = 0, view_creations = 0;

ClayFenceSyncRef Ref(clay::FenceSync* fence) {
  return reinterpret_cast<ClayFenceSyncRef>(fence);
}

auto MakeHelper() {
  return std::make_shared<clay::VulkanFenceHelper>(
      reinterpret_cast<VkDevice>(1), reinterpret_cast<VkFence>(2),
      reinterpret_cast<VkSemaphore>(3));
}

EGLSyncKHR Create(EGLDisplay, EGLenum type, const EGLint* attributes) {
  CHECK_FENCE(type == EGL_SYNC_NATIVE_FENCE_ANDROID);
  CHECK_FENCE(attributes[0] == EGL_SYNC_NATIVE_FENCE_FD_ANDROID);
  CHECK_FENCE(fcntl(attributes[1], F_GETFD) >= 0);
  ++imports;
  close(attributes[1]);
  if (create_fails) {
    // Reuse exactly the consumed descriptor to detect a caller closing it a
    // second time after eglCreateSyncKHR fails.
    replacement_fd = fcntl(master_fd, F_DUPFD_CLOEXEC, attributes[1]);
    CHECK_FENCE(replacement_fd == attributes[1]);
    return EGL_NO_SYNC_KHR;
  }
  return reinterpret_cast<EGLSyncKHR>(4);
}
EGLBoolean Destroy(EGLDisplay, EGLSyncKHR) { return EGL_TRUE; }
EGLBoolean Wait(EGLDisplay, EGLSyncKHR, EGLint) {
  ++gpu_waits;
  return wait_fails ? EGL_FALSE : EGL_TRUE;
}
EGLint ClientWait(EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR) {
  std::abort();  // New APIs must not CPU-wait through EGL either.
}
EGLint Dup(EGLDisplay, EGLSyncKHR) {
  std::abort();  // Vulkan imports do not export an EGL sync.
}
}  // namespace

extern "C" EGLDisplay eglGetCurrentDisplay() {
  return reinterpret_cast<EGLDisplay>(1);
}
extern "C" EGLContext eglGetCurrentContext() {
  return current_context ? reinterpret_cast<EGLContext>(2) : EGL_NO_CONTEXT;
}
extern "C" const char* eglQueryString(EGLDisplay, EGLint name) {
  if (name == EGL_VERSION) return "1.4";
  CHECK_FENCE(name == EGL_EXTENSIONS);
  if (!wait_extension)
    return "EGL_KHR_fence_sync EGL_ANDROID_native_fence_sync";
  return native_extension ? "EGL_KHR_fence_sync EGL_KHR_wait_sync "
                            "EGL_ANDROID_native_fence_sync"
                          : "EGL_KHR_fence_sync EGL_KHR_wait_sync";
}
extern "C" __eglMustCastToProperFunctionPointerType eglGetProcAddress(
    const char* name) {
#define PROC(n, f)               \
  if (std::strcmp(name, n) == 0) \
  return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(f)
  PROC("eglCreateSyncKHR", Create);
  PROC("eglDestroySyncKHR", Destroy);
  if (std::strcmp(name, "eglWaitSyncKHR") == 0 && !wait_proc) return nullptr;
  PROC("eglWaitSyncKHR", Wait);
  PROC("eglClientWaitSyncKHR", ClientWait);
  PROC("eglDupNativeFenceFDANDROID", Dup);
#undef PROC
  return nullptr;
}

// Simulate only the driver boundary. The helper, VkFenceSync and C API are
// compiled from production sources.
namespace clay {
VulkanHelper::VulkanHelper() = default;
VulkanHelper& VulkanHelper::GetInstance() {
  static VulkanHelper instance;
  return instance;
}
bool LogVkIfNotSuccess(VkResult result, const char*) {
  return result == VK_SUCCESS;
}
VkResult VulkanHelper::GetFenceStatus(VkDevice, VkFence) { return status; }
VkResult VulkanHelper::GetSemaphoreFdKHR(VkDevice,
                                         const VkSemaphoreGetFdInfoKHR* info,
                                         int* fd) {
  CHECK_FENCE(info->handleType ==
              VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT);
  ++exports;
  if (export_result != VK_SUCCESS) return export_result;
  if (export_signaled) {
    *fd = -1;
  } else {
    int descriptors[2];
    CHECK_FENCE(pipe(descriptors) == 0);
    close(descriptors[1]);
    *fd = master_fd = descriptors[0];
  }
  return VK_SUCCESS;
}
VkResult VulkanHelper::WaitForFences(VkDevice, uint32_t, const VkFence*,
                                     VkBool32, uint64_t) {
  ++waits;
  std::unique_lock<std::mutex> lock(wait_mutex);
  if (block_wait) {
    entered_wait = true;
    wait_cv.notify_all();
    wait_cv.wait(lock, [] { return release_wait; });
  }
  return status == VK_ERROR_DEVICE_LOST ? status : VK_SUCCESS;
}
void VulkanHelper::DestroyFence(VkDevice, VkFence,
                                const VkAllocationCallbacks*) {
  ++destroyed;
}
void VulkanHelper::DestroySemaphore(VkDevice, VkSemaphore,
                                    const VkAllocationCallbacks*) {
  ++destroyed;
}
VkResult VulkanHelper::CreateSemaphore(VkDevice, const VkSemaphoreCreateInfo*,
                                       const VkAllocationCallbacks*,
                                       VkSemaphore* semaphore) {
  *semaphore = reinterpret_cast<VkSemaphore>(next_handle++);
  return create_semaphore_result;
}
VkResult VulkanHelper::CreateFence(VkDevice, const VkFenceCreateInfo*,
                                   const VkAllocationCallbacks*,
                                   VkFence* fence) {
  *fence = reinterpret_cast<VkFence>(next_handle++);
  return create_fence_result;
}
VkResult VulkanHelper::ImportSemaphoreFdKHR(
    VkDevice, const VkImportSemaphoreFdInfoKHR* info) {
  imported_fd = info->fd;
  if (import_result == VK_SUCCESS) close(imported_fd);
  return import_result;
}
VkResult VulkanHelper::BeginCommandBuffer(VkCommandBuffer,
                                          const VkCommandBufferBeginInfo*) {
  return VK_SUCCESS;
}
VkResult VulkanHelper::EndCommandBuffer(VkCommandBuffer) { return VK_SUCCESS; }
VkResult VulkanHelper::QueueSubmit(VkQueue, uint32_t, const VkSubmitInfo* info,
                                   VkFence) {
  ++submissions;
  submitted_waits = info->waitSemaphoreCount;
  return submit_result;
}
void VulkanHelper::GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice, uint32_t* count, VkQueueFamilyProperties* families) {
  *count = 1;
  if (families) families[0].queueFlags = VK_QUEUE_GRAPHICS_BIT;
}
VkResult VulkanHelper::CreateCommandPool(VkDevice,
                                         const VkCommandPoolCreateInfo*,
                                         const VkAllocationCallbacks*,
                                         VkCommandPool* pool) {
  *pool = reinterpret_cast<VkCommandPool>(next_handle++);
  return VK_SUCCESS;
}
void VulkanHelper::DestroyCommandPool(VkDevice, VkCommandPool,
                                      const VkAllocationCallbacks*) {}
VkResult VulkanHelper::AllocateCommandBuffers(
    VkDevice, const VkCommandBufferAllocateInfo*, VkCommandBuffer* commands) {
  *commands = reinterpret_cast<VkCommandBuffer>(next_handle++);
  return VK_SUCCESS;
}
VkResult VulkanHelper::ResetCommandBuffer(VkCommandBuffer,
                                          VkCommandBufferResetFlags) {
  return VK_SUCCESS;
}

// Substitute driver entry points, keeping the real image import and cleanup.
AHardwareBufferImageBacking::AHardwareBufferImageBacking(
    PixelFormat format, skity::Vec2 size, std::optional<GraphicsMemoryHandle>)
    : SharedImageBacking(format, size) {
  buffer_ = reinterpret_cast<AHardwareBuffer*>(1);
}
AHardwareBufferImageBacking::~AHardwareBufferImageBacking() = default;
SharedImageBacking::BackingType AHardwareBufferImageBacking::GetType() const {
  return BackingType::kAHardwareBuffer;
}
fml::RefPtr<SharedImageRepresentation>
AHardwareBufferImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig*) {
  return nullptr;
}
#ifdef ENABLE_SKITY
fml::RefPtr<SkityImageRepresentation>
AHardwareBufferImageBacking::CreateSkityRepresentation(skity::GPUContext*) {
  return nullptr;
}
#else
fml::RefPtr<SkiaImageRepresentation>
AHardwareBufferImageBacking::CreateSkiaRepresentation(GrDirectContext*) {
  return nullptr;
}
#endif
AHardwareBufferUtils::AHardwareBufferUtils() = default;
AHardwareBufferUtils& AHardwareBufferUtils::GetInstance() {
  static AHardwareBufferUtils instance;
  return instance;
}
void AHardwareBufferUtils::Describe(const AHardwareBuffer*,
                                    AHardwareBuffer_Desc* desc) {
  *desc = {};
  desc->width = desc->height = 8;
}
uint32_t FindMemoryType(uint32_t, VkPhysicalDevice, VkMemoryPropertyFlags) {
  return 0;
}
VkResult VulkanHelper::GetAndroidHardwareBufferPropertiesANDROID(
    VkDevice, const AHardwareBuffer*,
    VkAndroidHardwareBufferPropertiesANDROID* props) {
  props->allocationSize = 256;
  props->memoryTypeBits = 1;
  return image_failure == 1 ? VK_ERROR_INVALID_EXTERNAL_HANDLE : VK_SUCCESS;
}
VkResult VulkanHelper::CreateImage(VkDevice, const VkImageCreateInfo*,
                                   const VkAllocationCallbacks*,
                                   VkImage* image) {
  if (image_failure == 2) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  *image = reinterpret_cast<VkImage>(next_handle++);
  ++live_images;
  return VK_SUCCESS;
}
VkResult VulkanHelper::AllocateMemory(VkDevice, const VkMemoryAllocateInfo*,
                                      const VkAllocationCallbacks*,
                                      VkDeviceMemory* memory) {
  if (image_failure == 3) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  *memory = reinterpret_cast<VkDeviceMemory>(next_handle++);
  ++live_memory;
  return VK_SUCCESS;
}
VkResult VulkanHelper::BindImageMemory(VkDevice, VkImage, VkDeviceMemory,
                                       VkDeviceSize) {
  return image_failure == 4 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
}
VkResult VulkanHelper::CreateImageView(VkDevice,
                                       const VkImageViewCreateInfo* info,
                                       const VkAllocationCallbacks*,
                                       VkImageView* view) {
  CHECK_FENCE(info->image != VK_NULL_HANDLE);
  ++view_creations;
  if (image_failure == 5) return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  *view = reinterpret_cast<VkImageView>(next_handle++);
  ++live_views;
  return VK_SUCCESS;
}
void VulkanHelper::DestroyImageView(VkDevice, VkImageView,
                                    const VkAllocationCallbacks*) {
  CHECK_FENCE(live_views > 0);
  --live_views;
}
void VulkanHelper::DestroyImage(VkDevice, VkImage,
                                const VkAllocationCallbacks*) {
  CHECK_FENCE(live_images > 0);
  --live_images;
}
void VulkanHelper::FreeMemory(VkDevice, VkDeviceMemory,
                              const VkAllocationCallbacks*) {
  CHECK_FENCE(live_memory > 0);
  --live_memory;
}
void VulkanHelper::CmdPipelineBarrier(VkCommandBuffer, VkPipelineStageFlags,
                                      VkPipelineStageFlags, VkDependencyFlags,
                                      uint32_t, const VkMemoryBarrier*,
                                      uint32_t, const VkBufferMemoryBarrier*,
                                      uint32_t count,
                                      const VkImageMemoryBarrier* barrier) {
  CHECK_FENCE(count == 1 && barrier->image != VK_NULL_HANDLE);
  ++barriers;
}
}  // namespace clay

void TestRepresentation() {
  auto device = reinterpret_cast<VkDevice>(1);
  auto physical = reinterpret_cast<VkPhysicalDevice>(2);
  auto pool = std::make_shared<clay::VulkanCommandPoolHelper>(device, physical);
  auto backing = fml::MakeRefCounted<clay::AHardwareBufferImageBacking>(
      clay::SharedImageBacking::PixelFormat::kRGBA8888, skity::Vec2(8, 8));
  auto make_representation = [&] {
    auto representation =
        fml::MakeRefCounted<clay::VulkanImageHardwareBufferRepresentation>(
            backing, device, physical, reinterpret_cast<void*>(3));
    representation->SetCommandPoolHelper(pool);
    return representation;
  };
  for (image_failure = 1; image_failure <= 5; ++image_failure) {
    const int old_views = view_creations;
    auto invalid = make_representation();
    ClaySharedImageReadResult output{};
    CHECK_FENCE(!invalid->BeginRead(&output) && !invalid->EndRead());
    CHECK_FENCE(!output.vulkan_image.vulkan_image);
    const int old_barriers = barriers, old_submissions = submissions;
    invalid->ConsumeFence(nullptr);
    invalid->PostDrawVk();
    CHECK_FENCE(invalid->ProduceFence()->Poll() ==
                clay::FenceSync::PollResult::kFailed);
    CHECK_FENCE(barriers == old_barriers && submissions == old_submissions);
    CHECK_FENCE(live_images == 0 && live_memory == 0 && live_views == 0);
    CHECK_FENCE(view_creations == old_views + (image_failure == 5 ? 1 : 0));
  }
  image_failure = 0;
  auto representation = make_representation();
  auto check_read = [&](bool ready) {
    ClaySharedImageReadResult output{};
    CHECK_FENCE(representation->BeginRead(&output) == ready);
    if (ready) {
      output.vulkan_image.destruction_callback(output.vulkan_image.user_data);
    }
    CHECK_FENCE(representation->EndRead() == ready);
  };
  // Each failure must preserve the exact dependency through recreation.
  for (int failure = 0; failure < 4; ++failure) {
    status = VK_NOT_READY;
    auto dependency = MakeHelper();
    auto incoming = std::make_unique<clay::VkFenceSync>(dependency);
    auto* original = incoming.get();
    create_semaphore_result =
        failure == 0 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
    import_result =
        failure == 1 ? VK_ERROR_INVALID_EXTERNAL_HANDLE : VK_SUCCESS;
    create_fence_result =
        failure == 2 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
    submit_result = failure == 3 ? VK_ERROR_DEVICE_LOST : VK_SUCCESS;
    representation->ConsumeFence(std::move(incoming));
    check_read(false);
    auto retained = representation->ProduceFence();
    CHECK_FENCE(retained.get() == original);
    CHECK_FENCE(fcntl(master_fd, F_GETFD) >= 0);
    if (failure > 0) CHECK_FENCE(fcntl(imported_fd, F_GETFD) == -1);
    representation = make_representation();
    create_semaphore_result = create_fence_result = VK_SUCCESS;
    import_result = submit_result = VK_SUCCESS;
    representation->ConsumeFence(std::move(retained));
    check_read(true);
    CHECK_FENCE(submitted_waits == 1);
    dependency->Destroy();
  }
  // Signaled export and ClientWait fallback still submit the layout barrier.
  for (bool fallback : {false, true}) {
    status = fallback ? VK_NOT_READY : VK_SUCCESS;
    export_result = fallback ? VK_ERROR_TOO_MANY_OBJECTS : VK_SUCCESS;
    auto dependency = MakeHelper();
    int old_submissions = submissions, old_barriers = barriers;
    representation->ConsumeFence(
        std::make_unique<clay::VkFenceSync>(dependency));
    CHECK_FENCE(submissions == old_submissions + 1 &&
                barriers == old_barriers + 1);
    CHECK_FENCE(submitted_waits == 0);
    check_read(true);
    dependency->Destroy();
  }
  export_result = VK_SUCCESS;
  status = VK_SUCCESS;
  representation->PostDrawVk();
  auto first = representation->ProduceFence();
  auto second = representation->ProduceFence();
  first.reset();
  pool->FreeUsedBuffers();
  pool->FreeSignalSemaphoresAndFences();
  CHECK_FENCE(second->Poll() == clay::FenceSync::PollResult::kSignaled);
  // Every failed PostDraw must stop returning the prior successful fence.
  for (int failure = 0; failure < 3; ++failure) {
    create_semaphore_result =
        failure == 0 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
    create_fence_result =
        failure == 1 ? VK_ERROR_OUT_OF_DEVICE_MEMORY : VK_SUCCESS;
    submit_result = failure == 2 ? VK_ERROR_DEVICE_LOST : VK_SUCCESS;
    representation->PostDrawVk();
    check_read(false);
    CHECK_FENCE(representation->ProduceFence()->Poll() ==
                clay::FenceSync::PollResult::kFailed);
    CHECK_FENCE(second->Poll() == clay::FenceSync::PollResult::kSignaled);
    representation = make_representation();
    create_semaphore_result = create_fence_result = submit_result = VK_SUCCESS;
    representation->PostDrawVk();
  }
  representation = nullptr;
  CHECK_FENCE(live_images == 0 && live_memory == 0 && live_views == 0);
  second.reset();
  pool->Destroy();
  std::puts(
      "PASS Vulkan representation: import/submit failures preserve fences, "
      "completed dependencies still transition layout, failed PostDraw "
      "cannot reuse stale success, exported fences survive cleanup");
}

int main(int argc, char**) {
  wait_proc = argc == 1;
  alarm(20);
  CHECK_FENCE(testFencePublicContract());
  clay::VkFenceSync missing(nullptr);
  int fd = 123;
  CHECK_FENCE(testFencePoll(Ref(&missing)) == kClayFenceSyncFailed);
  CHECK_FENCE(testFenceExportFD(Ref(&missing), &fd) == kClayFenceSyncFailed);
  CHECK_FENCE(fd == -1);
  const ClayFenceSyncWaitContext context{sizeof(context),
                                         kClayFenceSyncWaitCurrentGL, nullptr};
  CHECK_FENCE(ClayFenceSyncServerWait(Ref(&missing), &context) ==
              kClayFenceSyncFailed);

  auto helper = MakeHelper();
  auto fence = std::make_unique<clay::VkFenceSync>(helper);
  CHECK_FENCE(testFencePoll(Ref(fence.get())) == kClayFenceSyncPending);
  current_context = false;
  CHECK_FENCE(!testFenceServerWait(Ref(fence.get())) && exports == 0);
  current_context = true;
  native_extension = false;
  CHECK_FENCE(!testFenceServerWait(Ref(fence.get())) && exports == 0);
  native_extension = true;
  wait_extension = false;
  CHECK_FENCE(!testFenceServerWait(Ref(fence.get())) && exports == 0);
  wait_extension = true;
  if (!wait_proc) {
    CHECK_FENCE(!testFenceServerWait(Ref(fence.get())) && exports == 0);
    CHECK_FENCE(waits == 0 && imports == 0 && gpu_waits == 0);
    std::puts("PASS Vulkan missing EGL wait entry point without CPU fallback");
    return 0;
  }
  export_result = VK_ERROR_TOO_MANY_OBJECTS;
  CHECK_FENCE(testFenceExportFD(Ref(fence.get()), &fd) == kClayFenceSyncFailed);
  CHECK_FENCE(fd == -1);
  export_result = VK_SUCCESS;
  CHECK_FENCE(testFenceExportFD(Ref(fence.get()), &fd) ==
              kClayFenceSyncSuccess);
  int second = -1;
  CHECK_FENCE(testFenceExportFD(Ref(fence.get()), &second) ==
              kClayFenceSyncSuccess);
  CHECK_FENCE(exports == 2 && fd != second && second != master_fd);
  CHECK_FENCE((fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0);
  close(fd);
  CHECK_FENCE(testFencePoll(Ref(fence.get())) == kClayFenceSyncPending);
  create_fails = true;
  CHECK_FENCE(!testFenceServerWait(Ref(fence.get())));
  CHECK_FENCE(fcntl(replacement_fd, F_GETFD) >= 0);
  close(replacement_fd);
  create_fails = false;
  wait_fails = true;
  CHECK_FENCE(!testFenceServerWait(Ref(fence.get())));
  wait_fails = false;
  CHECK_FENCE(testFenceServerWait(Ref(fence.get())));
  CHECK_FENCE(exports == 2 && imports == 3 && gpu_waits == 2 && waits == 0);
  status = VK_SUCCESS;
  CHECK_FENCE(testFencePoll(Ref(fence.get())) == kClayFenceSyncSignaled);
  CHECK_FENCE(testFenceExportFD(Ref(fence.get()), &fd) ==
              kClayFenceSyncSignaled);
  CHECK_FENCE(fd == -1);

  auto other = std::make_unique<clay::VkFenceSync>(helper);
  helper->MarkCanDestroy();
  fence.reset();
  CHECK_FENCE(!helper->CanDestroy());
  CHECK_FENCE(testFencePoll(Ref(other.get())) == kClayFenceSyncSignaled);
  other.reset();
  CHECK_FENCE(helper->CanDestroy());
  helper->Destroy();
  CHECK_FENCE(destroyed == 2 && fcntl(master_fd, F_GETFD) == -1);
  CHECK_FENCE(fcntl(second, F_GETFD) >= 0);
  close(second);
  CHECK_FENCE(helper->Poll() == clay::FenceSync::PollResult::kFailed);
  CHECK_FENCE(!helper->WaitSemaphore());
  helper->Destroy();
  CHECK_FENCE(destroyed == 2);

  status = VK_NOT_READY;
  export_signaled = true;
  helper = MakeHelper();
  {
    clay::VkFenceSync signaled(helper);
    CHECK_FENCE(testFenceExportFD(Ref(&signaled), &fd) ==
                kClayFenceSyncSignaled);
    CHECK_FENCE(fd == -1 && testFenceServerWait(Ref(&signaled)));
    CHECK_FENCE(exports == 3 && imports == 3);
  }
  helper->Destroy();
  export_signaled = false;
  helper = MakeHelper();
  {
    clay::VkFenceSync failed(helper);
    status = VK_ERROR_DEVICE_LOST;
    CHECK_FENCE(testFencePoll(Ref(&failed)) == kClayFenceSyncFailed);
    status = VK_SUCCESS;  // Once lost, a later success cannot erase the error.
    CHECK_FENCE(testFencePoll(Ref(&failed)) == kClayFenceSyncFailed);
    CHECK_FENCE(testFenceExportFD(Ref(&failed), &fd) == kClayFenceSyncFailed);
    CHECK_FENCE(!testFenceServerWait(Ref(&failed)) && !failed.ClientWait());
    CHECK_FENCE(waits == 0);
  }
  helper->Destroy();

  status = VK_NOT_READY;
  helper = MakeHelper();
  {
    clay::VkFenceSync concurrent(helper);
    block_wait = true;
    std::thread blocked([&] { CHECK_FENCE(concurrent.ClientWait()); });
    {
      std::unique_lock<std::mutex> lock(wait_mutex);
      wait_cv.wait(lock, [] { return entered_wait; });
    }
    CHECK_FENCE(testFencePoll(Ref(&concurrent)) == kClayFenceSyncPending);
    CHECK_FENCE(testFenceExportFD(Ref(&concurrent), &fd) ==
                kClayFenceSyncFailed);
    CHECK_FENCE(fd == -1 && !testFenceServerWait(Ref(&concurrent)));
    {
      std::lock_guard<std::mutex> lock(wait_mutex);
      release_wait = true;
    }
    wait_cv.notify_all();
    blocked.join();
    CHECK_FENCE(waits == 1);
    block_wait = false;
    current_context = false;
    concurrent.ServerWait();  // Only the legacy API permits a CPU fallback.
    CHECK_FENCE(waits == 2);
  }
  helper->Destroy();
  std::puts(
      "PASS Vulkan: poll, repeated FD export, EGL wait/errors, FD ownership, "
      "shared lifetime, device loss and nonblocking lock contention");
  TestRepresentation();
}
