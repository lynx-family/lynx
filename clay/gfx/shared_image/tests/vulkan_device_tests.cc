// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <GLES2/gl2.h>
#include <poll.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

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

// The normal system loader is used by VulkanHelper. These optional private
// library paths must never be reached by this executable.
extern "C" void* faker_dlopen(const char*, int) { std::abort(); }
extern "C" void* faker_dlsym(void*, const char*) { std::abort(); }
extern "C" int faker_dlclose(void*) { std::abort(); }

int main() {
  alarm(30);
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  CHECK_FENCE(eglInitialize(display, nullptr, nullptr));
  const EGLint config_attributes[] = {EGL_SURFACE_TYPE,
                                      EGL_PBUFFER_BIT,
                                      EGL_RENDERABLE_TYPE,
                                      EGL_OPENGL_ES2_BIT,
                                      EGL_RED_SIZE,
                                      8,
                                      EGL_GREEN_SIZE,
                                      8,
                                      EGL_BLUE_SIZE,
                                      8,
                                      EGL_NONE};
  EGLConfig config;
  EGLint count;
  CHECK_FENCE(eglChooseConfig(display, config_attributes, &config, 1, &count));
  CHECK_FENCE(count > 0);
  const EGLint surface_attributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
  EGLSurface surface =
      eglCreatePbufferSurface(display, config, surface_attributes);
  const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext context =
      eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
  CHECK_FENCE(eglMakeCurrent(display, surface, surface, context));
  CHECK_FENCE(clay::AndroidEGLFenceSync::CheckAndroidNativeFenceSupported());
  auto create_sync = reinterpret_cast<PFNEGLCREATESYNCKHRPROC>(
      eglGetProcAddress("eglCreateSyncKHR"));
  auto client_wait = reinterpret_cast<PFNEGLCLIENTWAITSYNCKHRPROC>(
      eglGetProcAddress("eglClientWaitSyncKHR"));
  auto destroy_sync = reinterpret_cast<PFNEGLDESTROYSYNCKHRPROC>(
      eglGetProcAddress("eglDestroySyncKHR"));

  VkInstanceCreateInfo instance_info{};
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  VkInstance instance;
  CHECK_FENCE(vkCreateInstance(&instance_info, nullptr, &instance) ==
              VK_SUCCESS);
  uint32_t device_count = 0;
  CHECK_FENCE(vkEnumeratePhysicalDevices(instance, &device_count, nullptr) ==
              VK_SUCCESS);
  CHECK_FENCE(device_count > 0);
  std::vector<VkPhysicalDevice> physical_devices(device_count);
  CHECK_FENCE(vkEnumeratePhysicalDevices(instance, &device_count,
                                         physical_devices.data()) ==
              VK_SUCCESS);
  VkPhysicalDevice physical = physical_devices[0];
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(physical, &properties);
  std::printf("Vulkan device: %s, driver %u\n", properties.deviceName,
              properties.driverVersion);
  uint32_t family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count, nullptr);
  std::vector<VkQueueFamilyProperties> families(family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical, &family_count,
                                           families.data());
  uint32_t family = 0;
  while (family < family_count &&
         !(families[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
    ++family;
  }
  CHECK_FENCE(family < family_count);
  float priority = 1;
  VkDeviceQueueCreateInfo queue_info{};
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.queueFamilyIndex = family;
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = &priority;
  const char* extensions[] = {VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
                              VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME};
  VkDeviceCreateInfo device_info{};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount = 2;
  device_info.ppEnabledExtensionNames = extensions;
  VkDevice device;
  CHECK_FENCE(vkCreateDevice(physical, &device_info, nullptr, &device) ==
              VK_SUCCESS);
  VkQueue queue;
  vkGetDeviceQueue(device, family, 0, &queue);
  clay::VulkanHelper::GetInstance().Init(instance, device);

  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = 256;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  VkBuffer buffer;
  CHECK_FENCE(vkCreateBuffer(device, &buffer_info, nullptr, &buffer) ==
              VK_SUCCESS);
  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(device, buffer, &requirements);
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
  uint32_t memory_type = 0;
  const auto required_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  while (memory_type < memory_properties.memoryTypeCount &&
         (!(requirements.memoryTypeBits & (1u << memory_type)) ||
          (memory_properties.memoryTypes[memory_type].propertyFlags &
           required_flags) != required_flags)) {
    ++memory_type;
  }
  CHECK_FENCE(memory_type < memory_properties.memoryTypeCount);
  VkMemoryAllocateInfo memory_info{};
  memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memory_info.allocationSize = requirements.size;
  memory_info.memoryTypeIndex = memory_type;
  VkDeviceMemory memory;
  CHECK_FENCE(vkAllocateMemory(device, &memory_info, nullptr, &memory) ==
              VK_SUCCESS);
  CHECK_FENCE(vkBindBufferMemory(device, buffer, memory, 0) == VK_SUCCESS);

  VkCommandPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = family;
  VkCommandPool pool;
  CHECK_FENCE(vkCreateCommandPool(device, &pool_info, nullptr, &pool) ==
              VK_SUCCESS);
  VkCommandBufferAllocateInfo allocate_info{};
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.commandPool = pool;
  allocate_info.commandBufferCount = 1;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  VkCommandBuffer commands;
  CHECK_FENCE(vkAllocateCommandBuffers(device, &allocate_info, &commands) ==
              VK_SUCCESS);
  VkEventCreateInfo event_info{};
  event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
  VkEvent gate;
  CHECK_FENCE(vkCreateEvent(device, &event_info, nullptr, &gate) == VK_SUCCESS);
  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  CHECK_FENCE(vkBeginCommandBuffer(commands, &begin_info) == VK_SUCCESS);
  // Keep the producer pending until every nonblocking operation has returned.
  vkCmdWaitEvents(commands, 1, &gate, VK_PIPELINE_STAGE_HOST_BIT,
                  VK_PIPELINE_STAGE_TRANSFER_BIT, 0, nullptr, 0, nullptr, 0,
                  nullptr);
  vkCmdFillBuffer(commands, buffer, 0, 256, 0x12345678);
  VkMemoryBarrier host_barrier{};
  host_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  host_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  host_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host_barrier, 0,
                       nullptr, 0, nullptr);
  CHECK_FENCE(vkEndCommandBuffer(commands) == VK_SUCCESS);

  VkExportSemaphoreCreateInfo export_info{};
  export_info.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
  export_info.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semaphore_info.pNext = &export_info;
  VkSemaphore semaphore;
  CHECK_FENCE(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore) ==
              VK_SUCCESS);
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  VkFence native_fence;
  CHECK_FENCE(vkCreateFence(device, &fence_info, nullptr, &native_fence) ==
              VK_SUCCESS);
  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &commands;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = &semaphore;
  CHECK_FENCE(vkQueueSubmit(queue, 1, &submit, native_fence) == VK_SUCCESS);

  auto helper = std::make_shared<clay::VulkanFenceHelper>(device, native_fence,
                                                          semaphore);
  auto fence = std::make_unique<clay::VkFenceSync>(helper);
  auto ref = reinterpret_cast<ClayFenceSyncRef>(fence.get());
  CHECK_FENCE(testFencePoll(ref) == kClayFenceSyncPending);
  int fd = -1, second = -1;
  CHECK_FENCE(testFenceExportFD(ref, &fd) == kClayFenceSyncSuccess);
  CHECK_FENCE(testFenceExportFD(ref, &second) == kClayFenceSyncSuccess);
  CHECK_FENCE(fd >= 0 && second >= 0 && fd != second);
  pollfd exported{fd, POLLIN, 0};
  CHECK_FENCE(poll(&exported, 1, 0) == 0);
  close(second);
  CHECK_FENCE(testFenceServerWait(ref));
  CHECK_FENCE(testFenceServerWait(ref));
  EGLSyncKHR consumer_done = create_sync(display, EGL_SYNC_FENCE_KHR, nullptr);
  CHECK_FENCE(consumer_done != EGL_NO_SYNC_KHR);
  glFlush();
  CHECK_FENCE(client_wait(display, consumer_done, 0, 0) ==
              EGL_TIMEOUT_EXPIRED_KHR);
  CHECK_FENCE(testFencePoll(ref) == kClayFenceSyncPending);

  CHECK_FENCE(vkSetEvent(device, gate) == VK_SUCCESS);
  CHECK_FENCE(poll(&exported, 1, 5000) == 1);
  CHECK_FENCE((exported.revents & POLLIN) && !(exported.revents & POLLERR));
  CHECK_FENCE(vkWaitForFences(device, 1, &native_fence, VK_TRUE, 5000000000) ==
              VK_SUCCESS);
  CHECK_FENCE(testFencePoll(ref) == kClayFenceSyncSignaled);
  CHECK_FENCE(client_wait(display, consumer_done, 0, 5000000000) ==
              EGL_CONDITION_SATISFIED_KHR);
  void* mapped = nullptr;
  CHECK_FENCE(vkMapMemory(device, memory, 0, 256, 0, &mapped) == VK_SUCCESS);
  for (int i = 0; i < 64; ++i) {
    CHECK_FENCE(static_cast<uint32_t*>(mapped)[i] == 0x12345678);
  }
  vkUnmapMemory(device, memory);

  fence.reset();
  helper->Destroy();
  CHECK_FENCE(poll(&exported, 1, 0) == 1 && (exported.revents & POLLIN));
  close(fd);
  destroy_sync(display, consumer_done);
  vkDestroyEvent(device, gate, nullptr);
  vkDestroyCommandPool(device, pool, nullptr);
  vkDestroyBuffer(device, buffer, nullptr);
  vkFreeMemory(device, memory, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroyInstance(instance, nullptr);
  eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroyContext(display, context);
  eglDestroySurface(display, surface);
  eglTerminate(display);
  std::puts(
      "PASS real Vulkan/EGL: gated GPU producer, pending/signaled poll, "
      "repeated SYNC_FD export/import, GPU ordering, 256-byte readback "
      "and exported FD surviving fence destruction");
}
