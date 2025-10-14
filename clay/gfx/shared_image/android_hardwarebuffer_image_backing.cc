// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android_hardwarebuffer_image_backing.h"

#ifndef NDEBUG
#include <GLES2/gl2.h>
#endif

#include "clay/fml/logging.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/shared_image/android/android_hardwarebuffer_utils.h"
#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/shared_image/vulkan_image_hardwarebuffer_representation.h"
#include "clay/public/clay.h"
namespace clay {

AHardwareBufferImageBacking::AHardwareBufferImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  FML_DCHECK(AHardwareBufferUtils::GetInstance().IsSupportAvailable());
  AHardwareBuffer_Desc desc = {
      static_cast<uint32_t>(size.x),
      static_cast<uint32_t>(size.y),
      1,
      AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
      AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
          AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
      0,
      0,
      0};
  int error = AHardwareBufferUtils::GetInstance().Allocate(&desc, &buffer_);
  if (error != 0) {
    FML_LOG(ERROR) << "AHardwareBuffer allocate error: " << error;
    return;
  }
#ifndef NDEBUG
  GLint max_texture_size = 0;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
  if (size.x > max_texture_size || size.y > max_texture_size) {
    FML_LOG(ERROR)
        << "AHardwareBuffer size is too large, supported max size is: "
        << max_texture_size;
  }
#endif
}

EGLImageKHR AHardwareBufferImageBacking::CreateEGLImage(
    EGLDisplay egl_display) {
  EGLClientBuffer client_buffer =
      AHardwareBufferUtils::GetInstance().GetNativeClientBuffer(buffer_);
  if (!client_buffer) {
    return {};
  }
  EGLImageKHR egl_image;
  const EGLint egl_attrib_list[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
                                    EGL_NONE};
  egl_image =
      eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
                        client_buffer, egl_attrib_list);
  return egl_image;
}

AHardwareBufferImageBacking::BackingType AHardwareBufferImageBacking::GetType()
    const {
  return SharedImageBacking::BackingType::kAHardwareBuffer;
}

fml::RefPtr<SharedImageRepresentation>
AHardwareBufferImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  auto type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
          fml::Ref(this));
    }
    case kClaySharedImageRepresentationTypeVulkan: {
      return fml::MakeRefCounted<VulkanImageHardwareBufferRepresentation>(
          fml::Ref(this), config->vk_config.device,
          config->vk_config.physical_device, config->vk_config.queue);
    }
    default: {
      // NOT SUPPORTED.
      FML_DLOG(ERROR)
          << "Unable to call AHardwareBufferImageBacking::CreateRepresentation "
             "with type: "
          << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation>
AHardwareBufferImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
                          fml::Ref(this)));
    }
    default: {
      FML_DLOG(ERROR)
          << "Unable to call "
             "AHardwareBufferImageBacking::CreateSkiaRepresentation with "
             "backend: "
          << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}
#else
fml::RefPtr<SkityImageRepresentation>
AHardwareBufferImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context,
          fml::MakeRefCounted<NativeBufferEGLImageRepresentation>(
              fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "AHardwareBufferImageBacking::CreateSkityRepresentation with "
             "backend: "
          << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

AHardwareBufferImageBacking::~AHardwareBufferImageBacking() {
  if (buffer_) {
    AHardwareBufferUtils::GetInstance().Release(buffer_);
  }
}

}  // namespace clay
