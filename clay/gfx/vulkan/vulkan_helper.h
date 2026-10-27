// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_VULKAN_VULKAN_HELPER_H_
#define CLAY_GFX_VULKAN_VULKAN_HELPER_H_

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include <string>

#include "clay/fml/logging.h"

namespace clay {

std::string VulkanResultToString(VkResult result);
bool LogVkIfNotSuccess(VkResult result, const char* pName);
uint32_t FindMemoryType(uint32_t type_filter, VkPhysicalDevice physical_device,
                        VkMemoryPropertyFlags properties);

struct InitVkParams {
  int version;
  VkInstance instance;
  VkPhysicalDevice physical_device;
  VkDevice device;
  VkQueue queue;
  uint32_t graphics_queue_index;
  uint32_t api_version;
  const char* const* enabled_instance_extension_names;
  uint32_t enabled_instance_extension_names_length;
  const char* const* enabled_device_extension_names;
  uint32_t enabled_device_extension_names_length;

  // Only one of device_features and device_features_2 should be non-null.
  // If both are null then no features are enabled.
  VkPhysicalDeviceFeatures* device_features;
  VkPhysicalDeviceFeatures2* device_features_2;
};
struct DrawVkParams {
  // Input: current width/height of destination surface.
  int width;
  int height;

  // Input: current transform matrix
  float transform[16];

  // Input WebView should do its main compositing draws into this. It cannot
  // do anything that would require stopping the render pass.
  VkCommandBuffer secondary_command_buffer;

  // Input: A render pass which will be compatible to the one which the
  // secondary_command_buffer will be submitted into.
  VkRenderPass compatible_render_pass;

  // Input: Format of the destination surface.
  VkFormat format;

  // Input: The main color attachment index where secondary_command_buffer
  // will eventually be submitted.
  uint32_t color_attachment_index;

  // Input: current clip rect
  int clip_left;
  int clip_top;
  int clip_right;
  int clip_bottom;
};

class VulkanHelper {
 public:
  static VulkanHelper& GetInstance();

  void Init(VkInstance instance, VkDevice device);

  PFN_vkVoidFunction GetInstanceProcAddr(const char* pName);

  PFN_vkVoidFunction GetDeviceProcAddr(const char* pName);

  VkResult GetAndroidHardwareBufferPropertiesANDROID(
      VkDevice device, const struct AHardwareBuffer* buffer,
      VkAndroidHardwareBufferPropertiesANDROID* pProperties);

  VkResult CreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkImage* pImage);

  void DestroyImage(VkDevice device, VkImage image,
                    const VkAllocationCallbacks* pAllocator);

  VkResult AllocateMemory(VkDevice device,
                          const VkMemoryAllocateInfo* pAllocateInfo,
                          const VkAllocationCallbacks* pAllocator,
                          VkDeviceMemory* pMemory);

  VkResult BindImageMemory(VkDevice device, VkImage image,
                           VkDeviceMemory memory, VkDeviceSize memoryOffset);

  VkResult CreateImageView(VkDevice device,
                           const VkImageViewCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkImageView* pView);

  void DestroyImageView(VkDevice device, VkImageView imageView,
                        const VkAllocationCallbacks* pAllocator);

  VkResult CreateFramebuffer(VkDevice device,
                             const VkFramebufferCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkFramebuffer* pFramebuffer);

  VkResult CreateDescriptorPool(VkDevice device,
                                const VkDescriptorPoolCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkDescriptorPool* pDescriptorPool);

  VkResult CreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo,
                        const VkAllocationCallbacks* pAllocator,
                        VkBuffer* pBuffer);

  void GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                   VkMemoryRequirements* pMemoryRequirements);

  VkResult BindBufferMemory(VkDevice device, VkBuffer buffer,
                            VkDeviceMemory memory, VkDeviceSize memoryOffset);

  VkResult MapMemory(VkDevice device, VkDeviceMemory memory,
                     VkDeviceSize offset, VkDeviceSize size,
                     VkMemoryMapFlags flags, void** ppData);

  void UnmapMemory(VkDevice device, VkDeviceMemory memory);

  VkResult CreatePipelineLayout(VkDevice device,
                                const VkPipelineLayoutCreateInfo* pCreateInfo,
                                const VkAllocationCallbacks* pAllocator,
                                VkPipelineLayout* pPipelineLayout);

  VkResult CreateGraphicsPipelines(
      VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
      const VkGraphicsPipelineCreateInfo* pCreateInfos,
      const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);

  VkResult CreateSampler(VkDevice device,
                         const VkSamplerCreateInfo* pCreateInfo,
                         const VkAllocationCallbacks* pAllocator,
                         VkSampler* pSampler);

  VkResult CreateDescriptorSetLayout(
      VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
      const VkAllocationCallbacks* pAllocator,
      VkDescriptorSetLayout* pSetLayout);

  VkResult AllocateDescriptorSets(
      VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo,
      VkDescriptorSet* pDescriptorSets);

  void UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                            const VkWriteDescriptorSet* pDescriptorWrites,
                            uint32_t descriptorCopyCount,
                            const VkCopyDescriptorSet* pDescriptorCopies);

  void CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                             VkPipelineBindPoint pipelineBindPoint,
                             VkPipelineLayout layout, uint32_t firstSet,
                             uint32_t descriptorSetCount,
                             const VkDescriptorSet* pDescriptorSets,
                             uint32_t dynamicOffsetCount,
                             const uint32_t* pDynamicOffsets);

  void CmdBindPipeline(VkCommandBuffer commandBuffer,
                       VkPipelineBindPoint pipelineBindPoint,
                       VkPipeline pipeline);

  VkResult BeginCommandBuffer(VkCommandBuffer commandBuffer,
                              const VkCommandBufferBeginInfo* pBeginInfo);

  VkResult EndCommandBuffer(VkCommandBuffer commandBuffer);

  void CmdBeginRenderPass(VkCommandBuffer commandBuffer,
                          const VkRenderPassBeginInfo* pRenderPassBegin,
                          VkSubpassContents contents);

  void CmdEndRenderPass(VkCommandBuffer commandBuffer);

  void CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                            uint32_t firstBinding, uint32_t bindingCount,
                            const VkBuffer* pBuffers,
                            const VkDeviceSize* pOffsets);

  void CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer,
                          VkDeviceSize offset, VkIndexType indexType);

  void CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount,
                      uint32_t instanceCount, uint32_t firstIndex,
                      int32_t vertexOffset, uint32_t firstInstance);

  void CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount,
               uint32_t instanceCount, uint32_t firstVertex,
               uint32_t firstInstance);

  VkResult QueueSubmit(VkQueue queue, uint32_t submitCount,
                       const VkSubmitInfo* pSubmits, VkFence fence);

  VkResult QueueWaitIdle(VkQueue queue);

  void GetPhysicalDeviceMemoryProperties(
      VkPhysicalDevice physicalDevice,
      VkPhysicalDeviceMemoryProperties* pProperties);

  VkResult CreateShaderModule(VkDevice device,
                              const VkShaderModuleCreateInfo* pCreateInfo,
                              const VkAllocationCallbacks* pAllocator,
                              VkShaderModule* pShaderModule);

  void DestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                           const VkAllocationCallbacks* pAllocator);

  VkResult DeviceWaitIdle(VkDevice device);

  void DestroyBuffer(VkDevice device, VkBuffer buffer,
                     const VkAllocationCallbacks* pAllocator);

  void DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer,
                          const VkAllocationCallbacks* pAllocator);

  void FreeMemory(VkDevice device, VkDeviceMemory memory,
                  const VkAllocationCallbacks* pAllocator);

  void DestroySampler(VkDevice device, VkSampler sampler,
                      const VkAllocationCallbacks* pAllocator);

  void DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                             const VkAllocationCallbacks* pAllocator);

  void DestroyDescriptorSetLayout(VkDevice device,
                                  VkDescriptorSetLayout descriptorSetLayout,
                                  const VkAllocationCallbacks* pAllocator);

  void DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                             const VkAllocationCallbacks* pAllocator);

  void DestroyPipeline(VkDevice device, VkPipeline pipeline,
                       const VkAllocationCallbacks* pAllocator);

  void CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport,
                      uint32_t viewportCount, const VkViewport* pViewports);

  void CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor,
                     uint32_t scissorCount, const VkRect2D* pScissors);

  void CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer,
                     VkBuffer dstBuffer, uint32_t regionCount,
                     const VkBufferCopy* pRegions);

  VkResult ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                               uint32_t flags);

  void CmdPipelineBarrier(VkCommandBuffer commandBuffer,
                          VkPipelineStageFlags srcStageMask,
                          VkPipelineStageFlags dstStageMask,
                          VkDependencyFlags dependencyFlags,
                          uint32_t memoryBarrierCount,
                          const VkMemoryBarrier* pMemoryBarriers,
                          uint32_t bufferMemoryBarrierCount,
                          const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                          uint32_t imageMemoryBarrierCount,
                          const VkImageMemoryBarrier* pImageMemoryBarriers);

  VkResult CreateSemaphore(VkDevice device,
                           const VkSemaphoreCreateInfo* pCreateInfo,
                           const VkAllocationCallbacks* pAllocator,
                           VkSemaphore* pSemaphore);

  void DestroySemaphore(VkDevice device, VkSemaphore semaphore,
                        const VkAllocationCallbacks* pAllocator);

  VkResult ImportSemaphoreFdKHR(
      VkDevice device,
      const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo);

  VkResult GetSemaphoreFdKHR(VkDevice device,
                             const VkSemaphoreGetFdInfoKHR* pGetFdInfo,
                             int* pFd);

  VkResult CreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo,
                       const VkAllocationCallbacks* pAllocator,
                       VkFence* pFence);

  void DestroyFence(VkDevice device, VkFence fence,
                    const VkAllocationCallbacks* pAllocator);

  VkResult WaitForFences(VkDevice device, uint32_t fenceCount,
                         const VkFence* pFences, VkBool32 waitAll,
                         uint64_t timeout);

  VkResult GetFenceStatus(VkDevice device, VkFence fence);

  void CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
                        VkShaderStageFlags stageFlags, uint32_t offset,
                        uint32_t size, const void* pValues);

  VkResult CreateCommandPool(VkDevice device,
                             const VkCommandPoolCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkCommandPool* pCommandPool);
  void DestroyCommandPool(VkDevice device, VkCommandPool commandPool,
                          const VkAllocationCallbacks* pAllocator);

  VkResult AllocateCommandBuffers(
      VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo,
      VkCommandBuffer* pCommandBuffers);

  void FreeCommandBuffers(VkDevice device, VkCommandPool commandPool,
                          uint32_t commandBufferCount,
                          const VkCommandBuffer* pCommandBuffers);

  VkResult ResetCommandBuffer(VkCommandBuffer commandBuffer,
                              VkCommandBufferResetFlags flags);

  void GetPhysicalDeviceQueueFamilyProperties(
      VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount,
      VkQueueFamilyProperties* pQueueFamilyProperties);

 private:
  VulkanHelper();
  void InitializeVulkanFunctions();

  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_ = nullptr;
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr_ = nullptr;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;

  PFN_vkGetAndroidHardwareBufferPropertiesANDROID
      vkGetAndroidHardwareBufferPropertiesANDROID_;
  PFN_vkCreateImage vkCreateImage_;
  PFN_vkDestroyImage vkDestroyImage_;
  PFN_vkAllocateMemory vkAllocateMemory_;
  PFN_vkBindImageMemory vkBindImageMemory_;
  PFN_vkCreateImageView vkCreateImageView_;
  PFN_vkDestroyImageView vkDestroyImageView_;
  PFN_vkCreateFramebuffer vkCreateFramebuffer_;
  PFN_vkCreateDescriptorPool vkCreateDescriptorPool_;
  PFN_vkCreateBuffer vkCreateBuffer_;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements_;
  PFN_vkBindBufferMemory vkBindBufferMemory_;
  PFN_vkMapMemory vkMapMemory_;
  PFN_vkUnmapMemory vkUnmapMemory_;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout_;
  PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines_;
  PFN_vkCreateSampler vkCreateSampler_;
  PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout_;
  PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets_;
  PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets_;
  PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets_;
  PFN_vkCmdBindPipeline vkCmdBindPipeline_;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer_;
  PFN_vkEndCommandBuffer vkEndCommandBuffer_;
  PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass_;
  PFN_vkCmdEndRenderPass vkCmdEndRenderPass_;
  PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers_;
  PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer_;
  PFN_vkCmdDrawIndexed vkCmdDrawIndexed_;
  PFN_vkCmdDraw vkCmdDraw_;
  PFN_vkQueueSubmit vkQueueSubmit_;
  PFN_vkQueueWaitIdle vkQueueWaitIdle_;
  PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties_;
  PFN_vkCreateShaderModule vkCreateShaderModule_;
  PFN_vkDestroyShaderModule vkDestroyShaderModule_;
  PFN_vkDeviceWaitIdle vkDeviceWaitIdle_;
  PFN_vkDestroyBuffer vkDestroyBuffer_;
  PFN_vkFreeMemory vkFreeMemory_;
  PFN_vkDestroySampler vkDestroySampler_;
  PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool_;
  PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout_;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout_;
  PFN_vkDestroyPipeline vkDestroyPipeline_;
  PFN_vkDestroyFramebuffer vkDestroyFramebuffer_;
  PFN_vkCmdSetViewport vkCmdSetViewport_;
  PFN_vkCmdSetScissor vkCmdSetScissor_;
  PFN_vkCmdCopyBuffer vkCmdCopyBuffer_;
  PFN_vkResetDescriptorPool vkResetDescriptorPool_;
  PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier_;
  PFN_vkCreateSemaphore vkCreateSemaphore_;
  PFN_vkDestroySemaphore vkDestroySemaphore_;
  PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR_;
  PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR_;
  PFN_vkCreateFence vkCreateFence_;
  PFN_vkDestroyFence vkDestroyFence_;
  PFN_vkWaitForFences vkWaitForFences_;
  PFN_vkGetFenceStatus vkGetFenceStatus_;
  PFN_vkCmdPushConstants vkCmdPushConstants_;
  PFN_vkCreateCommandPool vkCreateCommandPool_;
  PFN_vkDestroyCommandPool vkDestroyCommandPool_;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers_;
  PFN_vkFreeCommandBuffers vkFreeCommandBuffers_;
  PFN_vkResetCommandBuffer vkResetCommandBuffer_;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties
      vkGetPhysicalDeviceQueueFamilyProperties_;
};

}  // namespace clay

#endif  // CLAY_GFX_VULKAN_VULKAN_HELPER_H_
