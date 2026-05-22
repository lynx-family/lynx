// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/vulkan/vulkan_helper.h"

#include "clay/fml/native_library.h"

namespace clay {

std::string VulkanResultToString(VkResult result) {
  switch (result) {
    case VK_SUCCESS:
      return "VK_SUCCESS";
    case VK_NOT_READY:
      return "VK_NOT_READY";
    case VK_TIMEOUT:
      return "VK_TIMEOUT";
    case VK_EVENT_SET:
      return "VK_EVENT_SET";
    case VK_EVENT_RESET:
      return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
      return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
      return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
      return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
      return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
      return "VK_ERROR_INVALID_SHADER_NV";
#if VK_HEADER_VERSION < 140
    case VK_RESULT_RANGE_SIZE:
      return "VK_RESULT_RANGE_SIZE";
#endif
    case VK_RESULT_MAX_ENUM:
      return "VK_RESULT_MAX_ENUM";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      return "VK_ERROR_OUT_OF_POOL_MEMORY";
    default:
      return "Unknown Error";
  }
  return "";
}

bool LogVkIfNotSuccess(VkResult result, const char* pName) {
  if (result != VK_SUCCESS) {
    FML_LOG(ERROR) << "Vulkan error: " << VulkanResultToString(result) << " in "
                   << pName;
    return false;
  }
  return true;
}

uint32_t FindMemoryType(uint32_t type_filter, VkPhysicalDevice physical_device,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  VulkanHelper::GetInstance().GetPhysicalDeviceMemoryProperties(
      physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (mem_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  return 0;
}

VulkanHelper::VulkanHelper() = default;

// static
VulkanHelper& VulkanHelper::GetInstance() {
  static VulkanHelper instance;
  return instance;
}

void VulkanHelper::Init(VkInstance instance, VkDevice device) {
  if (instance_ == instance && device_ == device) {
    return;  // already init, do nothing.
  }

  auto vulkan_library = fml::NativeLibrary::Create("libvulkan.so");
  if (!vulkan_library) {
    FML_LOG(ERROR) << "Failed to load libvulkan.so";
    return;
  }
  auto instance_proc_addr =
      vulkan_library->ResolveFunction<PFN_vkGetInstanceProcAddr>(
          "vkGetInstanceProcAddr");
  if (!instance_proc_addr.has_value()) {
    FML_LOG(ERROR) << "Failed to load vkGetInstanceProcAddr";
    return;
  }
  vkGetInstanceProcAddr_ = instance_proc_addr.value();

  instance_ = instance;
  device_ = device;

  vkGetDeviceProcAddr_ = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
      vkGetInstanceProcAddr_(instance_, "vkGetDeviceProcAddr"));
  if (!vkGetDeviceProcAddr_) {
    FML_LOG(ERROR) << "Failed to load vkGetDeviceProcAddr";
  }

  InitializeVulkanFunctions();
}

PFN_vkVoidFunction VulkanHelper::GetInstanceProcAddr(const char* pName) {
  if (vkGetInstanceProcAddr_) {
    return vkGetInstanceProcAddr_(instance_, pName);
  }
  FML_LOG(ERROR) << "vkGetInstanceProcAddr is null";
  return nullptr;
}

PFN_vkVoidFunction VulkanHelper::GetDeviceProcAddr(const char* pName) {
  if (vkGetDeviceProcAddr_) {
    return vkGetDeviceProcAddr_(device_, pName);
  }
  FML_LOG(ERROR) << "vkGetDeviceProcAddr is null";
  return nullptr;
}

void VulkanHelper::InitializeVulkanFunctions() {
  FML_DCHECK(vkGetInstanceProcAddr_);
  FML_DCHECK(vkGetDeviceProcAddr_);
  FML_DCHECK(instance_ != VK_NULL_HANDLE);
  FML_DCHECK(device_ != VK_NULL_HANDLE);
  vkGetAndroidHardwareBufferPropertiesANDROID_ =
      reinterpret_cast<PFN_vkGetAndroidHardwareBufferPropertiesANDROID>(
          vkGetDeviceProcAddr_(device_,
                               "vkGetAndroidHardwareBufferPropertiesANDROID"));
  vkCreateImage_ = reinterpret_cast<PFN_vkCreateImage>(
      vkGetDeviceProcAddr_(device_, "vkCreateImage"));
  vkDestroyImage_ = reinterpret_cast<PFN_vkDestroyImage>(
      vkGetDeviceProcAddr_(device_, "vkDestroyImage"));
  vkAllocateMemory_ = reinterpret_cast<PFN_vkAllocateMemory>(
      vkGetDeviceProcAddr_(device_, "vkAllocateMemory"));
  vkBindImageMemory_ = reinterpret_cast<PFN_vkBindImageMemory>(
      vkGetDeviceProcAddr_(device_, "vkBindImageMemory"));
  vkCreateImageView_ = reinterpret_cast<PFN_vkCreateImageView>(
      vkGetDeviceProcAddr_(device_, "vkCreateImageView"));
  vkDestroyImageView_ = reinterpret_cast<PFN_vkDestroyImageView>(
      vkGetDeviceProcAddr_(device_, "vkDestroyImageView"));
  vkCreateFramebuffer_ = reinterpret_cast<PFN_vkCreateFramebuffer>(
      vkGetDeviceProcAddr_(device_, "vkCreateFramebuffer"));
  vkCreateDescriptorPool_ = reinterpret_cast<PFN_vkCreateDescriptorPool>(
      vkGetDeviceProcAddr_(device_, "vkCreateDescriptorPool"));
  vkCreateBuffer_ = reinterpret_cast<PFN_vkCreateBuffer>(
      vkGetDeviceProcAddr_(device_, "vkCreateBuffer"));
  vkGetBufferMemoryRequirements_ =
      reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(
          vkGetDeviceProcAddr_(device_, "vkGetBufferMemoryRequirements"));
  vkBindBufferMemory_ = reinterpret_cast<PFN_vkBindBufferMemory>(
      vkGetDeviceProcAddr_(device_, "vkBindBufferMemory"));
  vkMapMemory_ = reinterpret_cast<PFN_vkMapMemory>(
      vkGetDeviceProcAddr_(device_, "vkMapMemory"));
  vkUnmapMemory_ = reinterpret_cast<PFN_vkUnmapMemory>(
      vkGetDeviceProcAddr_(device_, "vkUnmapMemory"));
  vkCreatePipelineLayout_ = reinterpret_cast<PFN_vkCreatePipelineLayout>(
      vkGetDeviceProcAddr_(device_, "vkCreatePipelineLayout"));
  vkCreateGraphicsPipelines_ = reinterpret_cast<PFN_vkCreateGraphicsPipelines>(
      vkGetDeviceProcAddr_(device_, "vkCreateGraphicsPipelines"));
  vkCreateSampler_ = reinterpret_cast<PFN_vkCreateSampler>(
      vkGetDeviceProcAddr_(device_, "vkCreateSampler"));
  vkCreateDescriptorSetLayout_ =
      reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(
          vkGetDeviceProcAddr_(device_, "vkCreateDescriptorSetLayout"));
  vkAllocateDescriptorSets_ = reinterpret_cast<PFN_vkAllocateDescriptorSets>(
      vkGetDeviceProcAddr_(device_, "vkAllocateDescriptorSets"));
  vkUpdateDescriptorSets_ = reinterpret_cast<PFN_vkUpdateDescriptorSets>(
      vkGetDeviceProcAddr_(device_, "vkUpdateDescriptorSets"));
  vkCmdBindDescriptorSets_ = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(
      vkGetDeviceProcAddr_(device_, "vkCmdBindDescriptorSets"));
  vkCmdBindPipeline_ = reinterpret_cast<PFN_vkCmdBindPipeline>(
      vkGetDeviceProcAddr_(device_, "vkCmdBindPipeline"));
  vkBeginCommandBuffer_ = reinterpret_cast<PFN_vkBeginCommandBuffer>(
      vkGetDeviceProcAddr_(device_, "vkBeginCommandBuffer"));
  vkEndCommandBuffer_ = reinterpret_cast<PFN_vkEndCommandBuffer>(
      vkGetDeviceProcAddr_(device_, "vkEndCommandBuffer"));
  vkCmdBeginRenderPass_ = reinterpret_cast<PFN_vkCmdBeginRenderPass>(
      vkGetDeviceProcAddr_(device_, "vkCmdBeginRenderPass"));
  vkCmdEndRenderPass_ = reinterpret_cast<PFN_vkCmdEndRenderPass>(
      vkGetDeviceProcAddr_(device_, "vkCmdEndRenderPass"));
  vkCmdBindVertexBuffers_ = reinterpret_cast<PFN_vkCmdBindVertexBuffers>(
      vkGetDeviceProcAddr_(device_, "vkCmdBindVertexBuffers"));
  vkCmdBindIndexBuffer_ = reinterpret_cast<PFN_vkCmdBindIndexBuffer>(
      vkGetDeviceProcAddr_(device_, "vkCmdBindIndexBuffer"));
  vkCmdDrawIndexed_ = reinterpret_cast<PFN_vkCmdDrawIndexed>(
      vkGetDeviceProcAddr_(device_, "vkCmdDrawIndexed"));
  vkCmdDraw_ = reinterpret_cast<PFN_vkCmdDraw>(
      vkGetDeviceProcAddr_(device_, "vkCmdDraw"));
  vkQueueSubmit_ = reinterpret_cast<PFN_vkQueueSubmit>(
      vkGetDeviceProcAddr_(device_, "vkQueueSubmit"));
  vkQueueWaitIdle_ = reinterpret_cast<PFN_vkQueueWaitIdle>(
      vkGetDeviceProcAddr_(device_, "vkQueueWaitIdle"));
  vkGetPhysicalDeviceMemoryProperties_ =
      reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
          vkGetInstanceProcAddr_(instance_,
                                 "vkGetPhysicalDeviceMemoryProperties"));
  vkCreateShaderModule_ = reinterpret_cast<PFN_vkCreateShaderModule>(
      vkGetDeviceProcAddr_(device_, "vkCreateShaderModule"));
  vkDestroyShaderModule_ = reinterpret_cast<PFN_vkDestroyShaderModule>(
      vkGetDeviceProcAddr_(device_, "vkDestroyShaderModule"));
  vkDeviceWaitIdle_ = reinterpret_cast<PFN_vkDeviceWaitIdle>(
      vkGetDeviceProcAddr_(device_, "vkDeviceWaitIdle"));
  vkDestroyBuffer_ = reinterpret_cast<PFN_vkDestroyBuffer>(
      vkGetDeviceProcAddr_(device_, "vkDestroyBuffer"));
  vkFreeMemory_ = reinterpret_cast<PFN_vkFreeMemory>(
      vkGetDeviceProcAddr_(device_, "vkFreeMemory"));
  vkDestroySampler_ = reinterpret_cast<PFN_vkDestroySampler>(
      vkGetDeviceProcAddr_(device_, "vkDestroySampler"));
  vkDestroyDescriptorPool_ = reinterpret_cast<PFN_vkDestroyDescriptorPool>(
      vkGetDeviceProcAddr_(device_, "vkDestroyDescriptorPool"));
  vkDestroyDescriptorSetLayout_ =
      reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(
          vkGetDeviceProcAddr_(device_, "vkDestroyDescriptorSetLayout"));
  vkDestroyPipelineLayout_ = reinterpret_cast<PFN_vkDestroyPipelineLayout>(
      vkGetDeviceProcAddr_(device_, "vkDestroyPipelineLayout"));
  vkDestroyPipeline_ = reinterpret_cast<PFN_vkDestroyPipeline>(
      vkGetDeviceProcAddr_(device_, "vkDestroyPipeline"));
  vkDestroyFramebuffer_ = reinterpret_cast<PFN_vkDestroyFramebuffer>(
      vkGetDeviceProcAddr_(device_, "vkDestroyFramebuffer"));
  vkCmdSetViewport_ = reinterpret_cast<PFN_vkCmdSetViewport>(
      vkGetDeviceProcAddr_(device_, "vkCmdSetViewport"));
  vkCmdSetScissor_ = reinterpret_cast<PFN_vkCmdSetScissor>(
      vkGetDeviceProcAddr_(device_, "vkCmdSetScissor"));
  vkCmdCopyBuffer_ = reinterpret_cast<PFN_vkCmdCopyBuffer>(
      vkGetDeviceProcAddr_(device_, "vkCmdCopyBuffer"));
  vkResetDescriptorPool_ = reinterpret_cast<PFN_vkResetDescriptorPool>(
      vkGetDeviceProcAddr_(device_, "vkResetDescriptorPool"));
  vkCmdPipelineBarrier_ = reinterpret_cast<PFN_vkCmdPipelineBarrier>(
      vkGetDeviceProcAddr_(device_, "vkCmdPipelineBarrier"));
  vkCreateSemaphore_ = reinterpret_cast<PFN_vkCreateSemaphore>(
      vkGetDeviceProcAddr_(device_, "vkCreateSemaphore"));
  vkDestroySemaphore_ = reinterpret_cast<PFN_vkDestroySemaphore>(
      vkGetDeviceProcAddr_(device_, "vkDestroySemaphore"));
  vkImportSemaphoreFdKHR_ = reinterpret_cast<PFN_vkImportSemaphoreFdKHR>(
      vkGetDeviceProcAddr_(device_, "vkImportSemaphoreFdKHR"));
  vkGetSemaphoreFdKHR_ = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(
      vkGetDeviceProcAddr_(device_, "vkGetSemaphoreFdKHR"));
  vkCreateFence_ = reinterpret_cast<PFN_vkCreateFence>(
      vkGetDeviceProcAddr_(device_, "vkCreateFence"));
  vkDestroyFence_ = reinterpret_cast<PFN_vkDestroyFence>(
      vkGetDeviceProcAddr_(device_, "vkDestroyFence"));
  vkWaitForFences_ = reinterpret_cast<PFN_vkWaitForFences>(
      vkGetDeviceProcAddr_(device_, "vkWaitForFences"));
  vkGetFenceStatus_ = reinterpret_cast<PFN_vkGetFenceStatus>(
      vkGetDeviceProcAddr_(device_, "vkGetFenceStatus"));
  vkCmdPushConstants_ = reinterpret_cast<PFN_vkCmdPushConstants>(
      vkGetDeviceProcAddr_(device_, "vkCmdPushConstants"));
  vkCreateCommandPool_ = reinterpret_cast<PFN_vkCreateCommandPool>(
      vkGetDeviceProcAddr_(device_, "vkCreateCommandPool"));
  vkDestroyCommandPool_ = reinterpret_cast<PFN_vkDestroyCommandPool>(
      vkGetDeviceProcAddr_(device_, "vkDestroyCommandPool"));
  vkAllocateCommandBuffers_ = reinterpret_cast<PFN_vkAllocateCommandBuffers>(
      vkGetDeviceProcAddr_(device_, "vkAllocateCommandBuffers"));
  vkFreeCommandBuffers_ = reinterpret_cast<PFN_vkFreeCommandBuffers>(
      vkGetDeviceProcAddr_(device_, "vkFreeCommandBuffers"));
  vkResetCommandBuffer_ = reinterpret_cast<PFN_vkResetCommandBuffer>(
      vkGetDeviceProcAddr_(device_, "vkResetCommandBuffer"));
  vkGetPhysicalDeviceQueueFamilyProperties_ =
      reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
          vkGetInstanceProcAddr_(instance_,
                                 "vkGetPhysicalDeviceQueueFamilyProperties"));
}

void VulkanHelper::DestroyBuffer(VkDevice device, VkBuffer buffer,
                                 const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyBuffer";
    return;
  }
  vkDestroyBuffer_(device, buffer, pAllocator);
}

void VulkanHelper::DestroyFramebuffer(VkDevice device,
                                      VkFramebuffer framebuffer,
                                      const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyFramebuffer_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyFramebuffer";
    return;
  }
  vkDestroyFramebuffer_(device, framebuffer, pAllocator);
}

void VulkanHelper::FreeMemory(VkDevice device, VkDeviceMemory memory,
                              const VkAllocationCallbacks* pAllocator) {
  if (!vkFreeMemory_) {
    FML_LOG(ERROR) << "Failed to load vkFreeMemory";
    return;
  }
  vkFreeMemory_(device, memory, pAllocator);
}

void VulkanHelper::DestroySampler(VkDevice device, VkSampler sampler,
                                  const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroySampler_) {
    FML_LOG(ERROR) << "Failed to load vkDestroySampler";
    return;
  }
  vkDestroySampler_(device, sampler, pAllocator);
}

void VulkanHelper::DestroyDescriptorPool(
    VkDevice device, VkDescriptorPool descriptorPool,
    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyDescriptorPool_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyDescriptorPool";
    return;
  }
  vkDestroyDescriptorPool_(device, descriptorPool, pAllocator);
}

void VulkanHelper::DestroyDescriptorSetLayout(
    VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyDescriptorSetLayout_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyDescriptorSetLayout";
    return;
  }
  vkDestroyDescriptorSetLayout_(device, descriptorSetLayout, pAllocator);
}

void VulkanHelper::DestroyPipelineLayout(
    VkDevice device, VkPipelineLayout pipelineLayout,
    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyPipelineLayout_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyPipelineLayout";
    return;
  }
  vkDestroyPipelineLayout_(device, pipelineLayout, pAllocator);
}

void VulkanHelper::DestroyPipeline(VkDevice device, VkPipeline pipeline,
                                   const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyPipeline_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyPipeline";
    return;
  }
  vkDestroyPipeline_(device, pipeline, pAllocator);
}

VkResult VulkanHelper::GetAndroidHardwareBufferPropertiesANDROID(
    VkDevice device, const struct AHardwareBuffer* buffer,
    VkAndroidHardwareBufferPropertiesANDROID* pProperties) {
  if (!vkGetAndroidHardwareBufferPropertiesANDROID_) {
    FML_LOG(ERROR)
        << "Failed to load vkGetAndroidHardwareBufferPropertiesANDROID";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkGetAndroidHardwareBufferPropertiesANDROID_(device, buffer,
                                                      pProperties);
}

VkResult VulkanHelper::CreateImage(VkDevice device,
                                   const VkImageCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkImage* pImage) {
  if (!vkCreateImage_) {
    FML_LOG(ERROR) << "Failed to load vkCreateImage";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateImage_(device, pCreateInfo, pAllocator, pImage);
}

void VulkanHelper::DestroyImage(VkDevice device, VkImage image,
                                const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyImage_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyImage";
    return;
  }
  vkDestroyImage_(device, image, pAllocator);
}

VkResult VulkanHelper::AllocateMemory(VkDevice device,
                                      const VkMemoryAllocateInfo* pAllocateInfo,
                                      const VkAllocationCallbacks* pAllocator,
                                      VkDeviceMemory* pMemory) {
  if (!vkAllocateMemory_) {
    FML_LOG(ERROR) << "Failed to load vkAllocateMemory";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkAllocateMemory_(device, pAllocateInfo, pAllocator, pMemory);
}

VkResult VulkanHelper::BindImageMemory(VkDevice device, VkImage image,
                                       VkDeviceMemory memory,
                                       VkDeviceSize memoryOffset) {
  if (!vkBindImageMemory_) {
    FML_LOG(ERROR) << "Failed to load vkBindImageMemory";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkBindImageMemory_(device, image, memory, memoryOffset);
}

VkResult VulkanHelper::CreateImageView(VkDevice device,
                                       const VkImageViewCreateInfo* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkImageView* pView) {
  if (!vkCreateImageView_) {
    FML_LOG(ERROR) << "Failed to load vkCreateImageView";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateImageView_(device, pCreateInfo, pAllocator, pView);
}

void VulkanHelper::DestroyImageView(VkDevice device, VkImageView imageView,
                                    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyImageView_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyImageView";
    return;
  }
  vkDestroyImageView_(device, imageView, pAllocator);
}

VkResult VulkanHelper::CreateFramebuffer(
    VkDevice device, const VkFramebufferCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) {
  if (!vkCreateFramebuffer_) {
    FML_LOG(ERROR) << "Failed to load vkCreateFramebuffer";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateFramebuffer_(device, pCreateInfo, pAllocator, pFramebuffer);
}

VkResult VulkanHelper::CreateDescriptorPool(
    VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorPool* pDescriptorPool) {
  if (!vkCreateDescriptorPool_) {
    FML_LOG(ERROR) << "Failed to load vkCreateDescriptorPool";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateDescriptorPool_(device, pCreateInfo, pAllocator,
                                 pDescriptorPool);
}

VkResult VulkanHelper::CreateBuffer(VkDevice device,
                                    const VkBufferCreateInfo* pCreateInfo,
                                    const VkAllocationCallbacks* pAllocator,
                                    VkBuffer* pBuffer) {
  if (!vkCreateBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkCreateBuffer";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateBuffer_(device, pCreateInfo, pAllocator, pBuffer);
}

void VulkanHelper::GetBufferMemoryRequirements(
    VkDevice device, VkBuffer buffer,
    VkMemoryRequirements* pMemoryRequirements) {
  if (!vkGetBufferMemoryRequirements_) {
    FML_LOG(ERROR) << "Failed to load vkGetBufferMemoryRequirements";
    return;
  }
  vkGetBufferMemoryRequirements_(device, buffer, pMemoryRequirements);
}

VkResult VulkanHelper::BindBufferMemory(VkDevice device, VkBuffer buffer,
                                        VkDeviceMemory memory,
                                        VkDeviceSize memoryOffset) {
  if (!vkBindBufferMemory_) {
    FML_LOG(ERROR) << "Failed to load vkBindBufferMemory";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkBindBufferMemory_(device, buffer, memory, memoryOffset);
}

VkResult VulkanHelper::MapMemory(VkDevice device, VkDeviceMemory memory,
                                 VkDeviceSize offset, VkDeviceSize size,
                                 VkMemoryMapFlags flags, void** ppData) {
  if (!vkMapMemory_) {
    FML_LOG(ERROR) << "Failed to load vkMapMemory";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkMapMemory_(device, memory, offset, size, flags, ppData);
}

void VulkanHelper::UnmapMemory(VkDevice device, VkDeviceMemory memory) {
  if (!vkUnmapMemory_) {
    FML_LOG(ERROR) << "Failed to load vkUnmapMemory";
    return;
  }
  vkUnmapMemory_(device, memory);
}

VkResult VulkanHelper::CreatePipelineLayout(
    VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkPipelineLayout* pPipelineLayout) {
  if (!vkCreatePipelineLayout_) {
    FML_LOG(ERROR) << "Failed to load vkCreatePipelineLayout";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreatePipelineLayout_(device, pCreateInfo, pAllocator,
                                 pPipelineLayout);
}

VkResult VulkanHelper::CreateGraphicsPipelines(
    VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkGraphicsPipelineCreateInfo* pCreateInfos,
    const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {
  if (!vkCreateGraphicsPipelines_) {
    FML_LOG(ERROR) << "Failed to load vkCreateGraphicsPipelines";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateGraphicsPipelines_(device, pipelineCache, createInfoCount,
                                    pCreateInfos, pAllocator, pPipelines);
}

VkResult VulkanHelper::CreateSampler(VkDevice device,
                                     const VkSamplerCreateInfo* pCreateInfo,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkSampler* pSampler) {
  if (!vkCreateSampler_) {
    FML_LOG(ERROR) << "Failed to load vkCreateSampler";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateSampler_(device, pCreateInfo, pAllocator, pSampler);
}

VkResult VulkanHelper::CreateDescriptorSetLayout(
    VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDescriptorSetLayout* pSetLayout) {
  if (!vkCreateDescriptorSetLayout_) {
    FML_LOG(ERROR) << "Failed to load vkCreateDescriptorSetLayout";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateDescriptorSetLayout_(device, pCreateInfo, pAllocator,
                                      pSetLayout);
}

VkResult VulkanHelper::AllocateDescriptorSets(
    VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo,
    VkDescriptorSet* pDescriptorSets) {
  if (!vkAllocateDescriptorSets_) {
    FML_LOG(ERROR) << "Failed to load vkAllocateDescriptorSets";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkAllocateDescriptorSets_(device, pAllocateInfo, pDescriptorSets);
}

void VulkanHelper::UpdateDescriptorSets(
    VkDevice device, uint32_t descriptorWriteCount,
    const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount,
    const VkCopyDescriptorSet* pDescriptorCopies) {
  if (!vkUpdateDescriptorSets_) {
    FML_LOG(ERROR) << "Failed to load vkUpdateDescriptorSets";
    return;
  }
  vkUpdateDescriptorSets_(device, descriptorWriteCount, pDescriptorWrites,
                          descriptorCopyCount, pDescriptorCopies);
}

void VulkanHelper::CmdBindDescriptorSets(
    VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
    VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount,
    const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
    const uint32_t* pDynamicOffsets) {
  if (!vkCmdBindDescriptorSets_) {
    FML_LOG(ERROR) << "Failed to load vkCmdBindDescriptorSets";
    return;
  }
  vkCmdBindDescriptorSets_(commandBuffer, pipelineBindPoint, layout, firstSet,
                           descriptorSetCount, pDescriptorSets,
                           dynamicOffsetCount, pDynamicOffsets);
}

void VulkanHelper::CmdBindPipeline(VkCommandBuffer commandBuffer,
                                   VkPipelineBindPoint pipelineBindPoint,
                                   VkPipeline pipeline) {
  if (!vkCmdBindPipeline_) {
    FML_LOG(ERROR) << "Failed to load vkCmdBindPipeline";
    return;
  }
  vkCmdBindPipeline_(commandBuffer, pipelineBindPoint, pipeline);
}

VkResult VulkanHelper::BeginCommandBuffer(
    VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) {
  if (!vkBeginCommandBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkBeginCommandBuffer";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkBeginCommandBuffer_(commandBuffer, pBeginInfo);
}

VkResult VulkanHelper::EndCommandBuffer(VkCommandBuffer commandBuffer) {
  if (!vkEndCommandBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkEndCommandBuffer";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkEndCommandBuffer_(commandBuffer);
}

void VulkanHelper::CmdBeginRenderPass(
    VkCommandBuffer commandBuffer,
    const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) {
  if (!vkCmdBeginRenderPass_) {
    FML_LOG(ERROR) << "Failed to load vkCmdBeginRenderPass";
    return;
  }
  vkCmdBeginRenderPass_(commandBuffer, pRenderPassBegin, contents);
}

void VulkanHelper::CmdEndRenderPass(VkCommandBuffer commandBuffer) {
  if (!vkCmdEndRenderPass_) {
    FML_LOG(ERROR) << "Failed to load vkCmdEndRenderPass";
    return;
  }
  vkCmdEndRenderPass_(commandBuffer);
}

void VulkanHelper::CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                                        uint32_t firstBinding,
                                        uint32_t bindingCount,
                                        const VkBuffer* pBuffers,
                                        const VkDeviceSize* pOffsets) {
  if (!vkCmdBindVertexBuffers_) {
    FML_LOG(ERROR) << "Failed to load vkCmdBindVertexBuffers";
    return;
  }
  vkCmdBindVertexBuffers_(commandBuffer, firstBinding, bindingCount, pBuffers,
                          pOffsets);
}

void VulkanHelper::CmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                                      VkBuffer buffer, VkDeviceSize offset,
                                      VkIndexType indexType) {
  if (!vkCmdBindIndexBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkCmdBindIndexBuffer";
    return;
  }
  vkCmdBindIndexBuffer_(commandBuffer, buffer, offset, indexType);
}

void VulkanHelper::CmdDrawIndexed(VkCommandBuffer commandBuffer,
                                  uint32_t indexCount, uint32_t instanceCount,
                                  uint32_t firstIndex, int32_t vertexOffset,
                                  uint32_t firstInstance) {
  if (!vkCmdDrawIndexed_) {
    FML_LOG(ERROR) << "Failed to load vkCmdDrawIndexed";
    return;
  }
  vkCmdDrawIndexed_(commandBuffer, indexCount, instanceCount, firstIndex,
                    vertexOffset, firstInstance);
}

void VulkanHelper::CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount,
                           uint32_t instanceCount, uint32_t firstVertex,
                           uint32_t firstInstance) {
  if (!vkCmdDraw_) {
    FML_LOG(ERROR) << "Failed to load vkCmdDraw";
    return;
  }
  vkCmdDraw_(commandBuffer, vertexCount, instanceCount, firstVertex,
             firstInstance);
}

VkResult VulkanHelper::QueueSubmit(VkQueue queue, uint32_t submitCount,
                                   const VkSubmitInfo* pSubmits,
                                   VkFence fence) {
  if (!vkQueueSubmit_) {
    FML_LOG(ERROR) << "Failed to load vkQueueSubmit";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkQueueSubmit_(queue, submitCount, pSubmits, fence);
}

VkResult VulkanHelper::QueueWaitIdle(VkQueue queue) {
  if (!vkQueueWaitIdle_) {
    FML_LOG(ERROR) << "Failed to load vkQueueWaitIdle";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkQueueWaitIdle_(queue);
}

void VulkanHelper::GetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice physicalDevice,
    VkPhysicalDeviceMemoryProperties* pProperties) {
  if (!vkGetPhysicalDeviceMemoryProperties_) {
    FML_LOG(ERROR) << "Failed to load vkGetPhysicalDeviceMemoryProperties";
    return;
  }
  vkGetPhysicalDeviceMemoryProperties_(physicalDevice, pProperties);
}

VkResult VulkanHelper::CreateShaderModule(
    VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) {
  if (!vkCreateShaderModule_) {
    FML_LOG(ERROR) << "Failed to load vkCreateShaderModule";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateShaderModule_(device, pCreateInfo, pAllocator, pShaderModule);
}

void VulkanHelper::DestroyShaderModule(
    VkDevice device, VkShaderModule shaderModule,
    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyShaderModule_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyShaderModule";
    return;
  }
  vkDestroyShaderModule_(device, shaderModule, pAllocator);
}

VkResult VulkanHelper::DeviceWaitIdle(VkDevice device) {
  if (!vkDeviceWaitIdle_) {
    FML_LOG(ERROR) << "Failed to load vkDeviceWaitIdle";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkDeviceWaitIdle_(device);
}

void VulkanHelper::CmdSetViewport(VkCommandBuffer commandBuffer,
                                  uint32_t firstViewport,
                                  uint32_t viewportCount,
                                  const VkViewport* pViewports) {
  if (!vkCmdSetViewport_) {
    FML_LOG(ERROR) << "Failed to load vkCmdSetViewport";
    return;
  }
  vkCmdSetViewport_(commandBuffer, firstViewport, viewportCount, pViewports);
}

void VulkanHelper::CmdSetScissor(VkCommandBuffer commandBuffer,
                                 uint32_t firstScissor, uint32_t scissorCount,
                                 const VkRect2D* pScissors) {
  if (!vkCmdSetScissor_) {
    FML_LOG(ERROR) << "Failed to load vkCmdSetScissor";
    return;
  }
  vkCmdSetScissor_(commandBuffer, firstScissor, scissorCount, pScissors);
}

void VulkanHelper::CmdCopyBuffer(VkCommandBuffer commandBuffer,
                                 VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 uint32_t regionCount,
                                 const VkBufferCopy* pRegions) {
  if (!vkCmdCopyBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkCmdCopyBuffer";
    return;
  }
  vkCmdCopyBuffer_(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

VkResult VulkanHelper::ResetDescriptorPool(VkDevice device,
                                           VkDescriptorPool descriptorPool,
                                           uint32_t flags) {
  if (!vkResetDescriptorPool_) {
    FML_LOG(ERROR) << "Failed to load vkResetDescriptorPool";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkResetDescriptorPool_(device, descriptorPool, flags);
}

void VulkanHelper::CmdPipelineBarrier(
    VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
    uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
    uint32_t bufferMemoryBarrierCount,
    const VkBufferMemoryBarrier* pBufferMemoryBarriers,
    uint32_t imageMemoryBarrierCount,
    const VkImageMemoryBarrier* pImageMemoryBarriers) {
  if (!vkCmdPipelineBarrier_) {
    FML_LOG(ERROR) << "Failed to load vkCmdPipelineBarrier";
    return;
  }
  vkCmdPipelineBarrier_(commandBuffer, srcStageMask, dstStageMask,
                        dependencyFlags, memoryBarrierCount, pMemoryBarriers,
                        bufferMemoryBarrierCount, pBufferMemoryBarriers,
                        imageMemoryBarrierCount, pImageMemoryBarriers);
}

VkResult VulkanHelper::CreateSemaphore(VkDevice device,
                                       const VkSemaphoreCreateInfo* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkSemaphore* pSemaphore) {
  if (!vkCreateSemaphore_) {
    FML_LOG(ERROR) << "Failed to load vkCreateSemaphore";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateSemaphore_(device, pCreateInfo, pAllocator, pSemaphore);
}

void VulkanHelper::DestroySemaphore(VkDevice device, VkSemaphore semaphore,
                                    const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroySemaphore_) {
    FML_LOG(ERROR) << "Failed to load vkDestroySemaphore";
    return;
  }
  vkDestroySemaphore_(device, semaphore, pAllocator);
}

VkResult VulkanHelper::ImportSemaphoreFdKHR(
    VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) {
  if (!vkImportSemaphoreFdKHR_) {
    FML_LOG(ERROR) << "Failed to load vkImportSemaphoreFdKHR";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkImportSemaphoreFdKHR_(device, pImportSemaphoreFdInfo);
}

VkResult VulkanHelper::GetSemaphoreFdKHR(
    VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) {
  if (!vkGetSemaphoreFdKHR_) {
    FML_LOG(ERROR) << "Failed to load vkGetSemaphoreFdKHR";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkGetSemaphoreFdKHR_(device, pGetFdInfo, pFd);
}

VkResult VulkanHelper::CreateFence(VkDevice device,
                                   const VkFenceCreateInfo* pCreateInfo,
                                   const VkAllocationCallbacks* pAllocator,
                                   VkFence* pFence) {
  if (!vkCreateFence_) {
    FML_LOG(ERROR) << "Failed to load vkCreateFence";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateFence_(device, pCreateInfo, pAllocator, pFence);
}

void VulkanHelper::DestroyFence(VkDevice device, VkFence fence,
                                const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyFence_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyFence";
    return;
  }
  vkDestroyFence_(device, fence, pAllocator);
}

VkResult VulkanHelper::WaitForFences(VkDevice device, uint32_t fenceCount,
                                     const VkFence* pFences, VkBool32 waitAll,
                                     uint64_t timeout) {
  if (!vkWaitForFences_) {
    FML_LOG(ERROR) << "Failed to load vkWaitForFences";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkWaitForFences_(device, fenceCount, pFences, waitAll, timeout);
}

VkResult VulkanHelper::GetFenceStatus(VkDevice device, VkFence fence) {
  if (!vkGetFenceStatus_) {
    FML_LOG(ERROR) << "Failed to load vkGetFenceStatus";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkGetFenceStatus_(device, fence);
}

void VulkanHelper::CmdPushConstants(VkCommandBuffer commandBuffer,
                                    VkPipelineLayout layout,
                                    VkShaderStageFlags stageFlags,
                                    uint32_t offset, uint32_t size,
                                    const void* pValues) {
  if (!vkCmdPushConstants_) {
    FML_LOG(ERROR) << "Failed to load vkCmdPushConstants";
    return;
  }
  vkCmdPushConstants_(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VkResult VulkanHelper::CreateCommandPool(
    VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo,
    const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) {
  if (!vkCreateCommandPool_) {
    FML_LOG(ERROR) << "Failed to load vkCreateCommandPool";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkCreateCommandPool_(device, pCreateInfo, pAllocator, pCommandPool);
}

void VulkanHelper::DestroyCommandPool(VkDevice device,
                                      VkCommandPool commandPool,
                                      const VkAllocationCallbacks* pAllocator) {
  if (!vkDestroyCommandPool_) {
    FML_LOG(ERROR) << "Failed to load vkDestroyCommandPool";
    return;
  }
  vkDestroyCommandPool_(device, commandPool, pAllocator);
}

VkResult VulkanHelper::AllocateCommandBuffers(
    VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
    VkCommandBuffer* pCommandBuffers) {
  if (!vkAllocateCommandBuffers_) {
    FML_LOG(ERROR) << "Failed to load vkAllocateCommandBuffers";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkAllocateCommandBuffers_(device, pAllocateInfo, pCommandBuffers);
}

void VulkanHelper::FreeCommandBuffers(VkDevice device,
                                      VkCommandPool commandPool,
                                      uint32_t bufferCount,
                                      const VkCommandBuffer* pBuffers) {
  if (!vkFreeCommandBuffers_) {
    FML_LOG(ERROR) << "Failed to load vkFreeCommandBuffers";
    return;
  }
  vkFreeCommandBuffers_(device, commandPool, bufferCount, pBuffers);
}

VkResult VulkanHelper::ResetCommandBuffer(VkCommandBuffer commandBuffer,
                                          VkCommandBufferResetFlags flags) {
  if (!vkResetCommandBuffer_) {
    FML_LOG(ERROR) << "Failed to load vkResetCommandBuffer";
    return VK_ERROR_INITIALIZATION_FAILED;
  }
  return vkResetCommandBuffer_(commandBuffer, flags);
}

void VulkanHelper::GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
    VkQueueFamilyProperties* pQueueFamilies) {
  if (!vkGetPhysicalDeviceQueueFamilyProperties_) {
    FML_LOG(ERROR) << "Failed to load vkGetPhysicalDeviceQueueFamilyProperties";
    return;
  }
  vkGetPhysicalDeviceQueueFamilyProperties_(
      physicalDevice, pQueueFamilyPropertyCount, pQueueFamilies);
}

}  // namespace clay
