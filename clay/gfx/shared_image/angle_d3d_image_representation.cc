
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/angle_d3d_image_representation.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext_angle.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <Windows.h>
#include <d3d11_1.h>

#include <vector>

#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/d3d9_texture_image_backing.h"
#include "clay/gfx/shared_image/d3d_texture_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/utils/functions_angle.h"

namespace clay {

AngleD3DImageRepresentation::AngleD3DImageRepresentation(
    fml::RefPtr<D3DTextureImageBacking> backing,
    PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc)
    : GLImageRepresentation(backing) {
  EGLAttrib device_result = SetupEGL(egl_get_proc_address_proc, true);

  ID3D11Device* d3d11_device = reinterpret_cast<ID3D11Device*>(device_result);

  if (d3d11_device == nullptr) {
    FML_LOG(ERROR) << "Failed to get d3d11 device.";
    return;
  }

  if (!backing->OpenForDevice(d3d11_device, &d3d11_texture_, &keyed_mutex_)) {
    FML_LOG(ERROR) << "Failed to open for device.";
  }
}

AngleD3DImageRepresentation::AngleD3DImageRepresentation(
    fml::RefPtr<D3D9TextureImageBacking> backing,
    PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc)
    : GLImageRepresentation(backing) {
  EGLAttrib device_result = SetupEGL(egl_get_proc_address_proc, false);
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9_device(
      reinterpret_cast<IDirect3DDevice9*>(device_result));

  if (d3d9_device == nullptr) {
    FML_LOG(ERROR) << "Failed to get d3d9 device.";
    return;
  }

  if (!backing->OpenForDevice(d3d9_device.Get(), &d3d9_texture_)) {
    FML_LOG(ERROR) << "Failed to open for device.";
  }
}

EGLAttrib AngleD3DImageRepresentation::SetupEGL(
    PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc, bool is_d3d11) {
  functions_angle_ =
      std::make_unique<FunctionsAngle>(egl_get_proc_address_proc);
  gl_api_windows_ = std::make_unique<clay::GlApiWindows>();
  gl_api_windows_->gl_get_integerv_proc = functions_angle_->glGetIntegervProc;
  gl_api_windows_->gl_bind_texture_proc = functions_angle_->glBindTextureProc;
  gl_api_windows_->gl_bind_framebuffer_proc =
      functions_angle_->glBindFramebufferProc;

  egl_display_ = functions_angle_->eglGetCurrentDisplay();
  if (egl_display_ == EGL_NO_DISPLAY) {
    FML_LOG(ERROR) << "Failed to get current display.";
    return 0;
  }
  EGLDeviceEXT device = EGL_NO_DEVICE_EXT;
  EGLAttrib display_result = 0;

  PFNEGLQUERYDISPLAYATTRIBEXTPROC eglQueryDisplayAttribEXTProc =
      reinterpret_cast<PFNEGLQUERYDISPLAYATTRIBEXTPROC>(
          functions_angle_->eglGetProcAddressProc("eglQueryDisplayAttribEXT"));
  if (!eglQueryDisplayAttribEXTProc) {
    FML_LOG(ERROR) << "Failed to get proc address of eglQueryDisplayAttribEXT.";
    return 0;
  }

  eglQueryDisplayAttribEXTProc(egl_display_, EGL_DEVICE_EXT, &display_result);
  device = reinterpret_cast<EGLDeviceEXT>(display_result);

  if (device == EGL_NO_DEVICE_EXT) {
    FML_LOG(ERROR) << "Failed to get egl device.";
    return 0;
  }

  PFNEGLQUERYDEVICEATTRIBEXTPROC
  eglQueryDeviceAttribEXTProc =
      reinterpret_cast<PFNEGLQUERYDISPLAYATTRIBEXTPROC>(
          functions_angle_->eglGetProcAddressProc("eglQueryDeviceAttribEXT"));
  if (!eglQueryDeviceAttribEXTProc) {
    FML_LOG(ERROR) << "Failed to get proc address of eglQueryDeviceAttribEXT.";
    return 0;
  }

  EGLAttrib device_result = 0;

  if (!eglQueryDeviceAttribEXTProc(
          device, is_d3d11 ? EGL_D3D11_DEVICE_ANGLE : EGL_D3D9_DEVICE_ANGLE,
          &device_result)) {
    FML_LOG(ERROR) << "Failed to call eglQueryDeviceAttribEXTProc.";
  }

  return device_result;
}

AngleD3DImageRepresentation::~AngleD3DImageRepresentation() {
  if (fbo_id_ != 0) {
    UnbindFrameBuffer();
  }
  if (texture_id_ != 0) {
    ReleaseTexImage();
  }
  if (scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is still locked.";
    scoped_keyed_mutex_.reset();
  }
}

bool AngleD3DImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  if (!GLImageRepresentation::BeginRead(out)) {
    return false;
  }
  return LockKeyedMutex();
}

bool AngleD3DImageRepresentation::EndRead() {
  if (!GLImageRepresentation::EndRead()) {
    return false;
  }
  return UnlockKeyedMutex();
}

bool AngleD3DImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  if (!GLImageRepresentation::BeginWrite(out)) {
    return false;
  }
  return LockKeyedMutex();
}

bool AngleD3DImageRepresentation::EndWrite() {
  if (!GLImageRepresentation::EndWrite()) {
    return false;
  }
  return UnlockKeyedMutex();
}

bool AngleD3DImageRepresentation::LockKeyedMutex() {
  if (!keyed_mutex_) {
    return true;
  }
  if (scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is already locked.";
  }
  scoped_keyed_mutex_.emplace(keyed_mutex_.Get());
  return true;
}

bool AngleD3DImageRepresentation::UnlockKeyedMutex() {
  if (!keyed_mutex_) {
    return true;
  }
  if (!scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is not locked.";
  }
  scoped_keyed_mutex_.reset();
  return true;
}

ImageRepresentationType AngleD3DImageRepresentation::GetType() const {
  return ImageRepresentationType::kANGLE;
}

std::optional<uint32_t> AngleD3DImageRepresentation::GetTexFromD3D9Texture() {
  FML_DCHECK(d3d9_texture_);

  EGLConfig config = EGL_NO_CONFIG_KHR;
  EGLint value;

  PFNEGLQUERYCONTEXTPROC
  eglQueryContextProc = reinterpret_cast<PFNEGLQUERYCONTEXTPROC>(
      functions_angle_->eglGetProcAddressProc("eglQueryContext"));
  if (!eglQueryContextProc) {
    FML_LOG(ERROR) << "Failed to get proc address of eglQueryContext.";
    return {};
  }

  if (!eglQueryContextProc(egl_display_,
                           functions_angle_->eglGetCurrentContext(),
                           EGL_CONFIG_ID, &value)) {
    FML_LOG(ERROR) << "Failed to query EGL_CONFIG_ID.";
    return {};
  }

  // Retrieve the EGLConfig that has the obtained EGL_CONFIG_ID.
  EGLint num_configs;

  if (!functions_angle_->eglGetConfigs(egl_display_, nullptr, 0,
                                       &num_configs)) {
    FML_LOG(ERROR) << "Failed to eglGetConfigs.";
    return {};
  }

  std::vector<EGLConfig> configs(num_configs);

  if (!functions_angle_->eglGetConfigs(egl_display_, configs.data(),
                                       num_configs, &num_configs)) {
    FML_LOG(ERROR) << "Failed to eglGetConfigs.";
    return {};
  }

  for (int i = 0; i < num_configs; ++i) {
    EGLint configID;
    functions_angle_->eglGetConfigAttrib(egl_display_, configs[i],
                                         EGL_CONFIG_ID, &configID);
    if (configID == value) {
      config = configs[i];
      break;
    }
  }

  if (config == EGL_NO_CONFIG_KHR) {
    FML_LOG(ERROR) << "Failed to find current EGLConfig.";
    return {};
  }

  GLuint texture;
  functions_angle_->glGenTextures(1, &texture);
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture,
                                                  gl_api_windows_.get());
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);
  GLenum error = functions_angle_->glGetError();
  if (error != GL_NO_ERROR) {
    functions_angle_->glDeleteTextures(1, &texture);
    FML_LOG(ERROR) << "GL generate and initialize texture failed: " << error;
    return {};
  }

  EGLint attributes[] = {EGL_WIDTH,
                         static_cast<EGLint>(GetBacking()->GetSize().x),
                         EGL_HEIGHT,
                         static_cast<EGLint>(GetBacking()->GetSize().y),
                         EGL_TEXTURE_TARGET,
                         EGL_TEXTURE_2D,
                         EGL_TEXTURE_FORMAT,
                         EGL_TEXTURE_RGBA,  // always EGL_TEXTURE_RGBA
                         EGL_NONE};

  PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC eglCreatePbufferFromClientBufferProc =
      reinterpret_cast<PFNEGLCREATEPBUFFERFROMCLIENTBUFFERPROC>(
          functions_angle_->eglGetProcAddressProc(
              "eglCreatePbufferFromClientBuffer"));
  if (eglCreatePbufferFromClientBufferProc == nullptr) {
    FML_LOG(ERROR)
        << "Failed to get 'eglCreatePbufferFromClientBufferProc' proc.";
    return {};
  }

  egl_surface_ = eglCreatePbufferFromClientBufferProc(
      egl_display_, EGL_D3D_TEXTURE_ANGLE, d3d9_texture_.Get(), config,
      attributes);

  if (egl_surface_ == EGL_NO_SURFACE ||
      functions_angle_->eglBindTexImage(egl_display_, egl_surface_,
                                        EGL_BACK_BUFFER) == EGL_FALSE) {
    functions_angle_->glDeleteTextures(1, &texture);
    FML_LOG(ERROR) << "Binding D3D surface failed.";
    return {};
  }

  return texture;
}

std::optional<uint32_t> AngleD3DImageRepresentation::GetTexFromD3D11Texture() {
  FML_DCHECK(d3d11_texture_);

  SharedImageBacking::PixelFormat pixel_format = GetBacking()->GetPixelFormat();
  EGLint internal_format = GL_NONE;
  switch (pixel_format) {
    case SharedImageBacking::PixelFormat::kNative8888:
      internal_format = GL_BGRA_EXT;
      break;
    case SharedImageBacking::PixelFormat::kRGBA8888:
      internal_format = GL_RGBA;
      break;
    default:
      FML_LOG(ERROR) << "Invalid pixel format.";
      return {};
  }

  const EGLint attribs[] = {EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, internal_format,
                            EGL_NONE};
  egl_image_ = functions_angle_->eglCreateImageKHR(
      egl_display_, EGL_NO_CONTEXT, EGL_D3D11_TEXTURE_ANGLE,
      static_cast<EGLClientBuffer>(d3d11_texture_.Get()), attribs);
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    FML_LOG(ERROR) << "Failed to create egl image.";
    return {};
  }
  EGLint egl_error = functions_angle_->eglGetError();
  if (egl_error != EGL_SUCCESS) {
    FML_LOG(ERROR) << "eglCreateImageKHR failed: " << egl_error;
    return {};
  }
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESProc =
      reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
          functions_angle_->eglGetProcAddressProc(
              "glEGLImageTargetTexture2DOES"));
  if (glEGLImageTargetTexture2DOESProc == nullptr) {
    FML_LOG(ERROR) << "Failed to get 'glEGLImageTargetTexture2DOES' proc.";
    return {};
  }

  GLuint texture;
  functions_angle_->glGenTextures(1, &texture);
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_2D, texture,
                                                  gl_api_windows_.get());
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    GL_LINEAR);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                                    GL_CLAMP_TO_EDGE);
  functions_angle_->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                                    GL_CLAMP_TO_EDGE);
  GLenum error = functions_angle_->glGetError();
  if (error != GL_NO_ERROR) {
    functions_angle_->glDeleteTextures(1, &texture);
    FML_LOG(ERROR) << "GL generate and initialize texture failed: " << error;
    return {};
  }

  glEGLImageTargetTexture2DOESProc(GL_TEXTURE_2D, egl_image_);
  egl_error = functions_angle_->eglGetError();
  if (egl_error != EGL_SUCCESS) {
    functions_angle_->glDeleteTextures(1, &texture);
    FML_LOG(ERROR) << "glEGLImageTargetTexture2DOES failed: " << egl_error;
    return {};
  }

  return texture;
}

std::optional<GLImageRepresentation::TextureInfo>
AngleD3DImageRepresentation::GetTexImage() {
  if (!d3d11_texture_ && !d3d9_texture_) {
    FML_LOG(ERROR) << "Has invalid texture image backing.";
    return {};
  }
  if (texture_id_ == 0) {
    std::optional<uint32_t> texture =
        d3d11_texture_ ? GetTexFromD3D11Texture() : GetTexFromD3D9Texture();
    if (!texture) {
      return {};
    }
    texture_id_ = texture.value();
  }

  return TextureInfo{.target = GL_TEXTURE_2D,
                     .name = texture_id_,
                     .format = GL_RGBA8,
                     .size = GetSize()};
}

bool AngleD3DImageRepresentation::ReleaseTexImage() {
  if (texture_id_ == 0) {
    return false;
  }
  functions_angle_->glDeleteTextures(1, &texture_id_);
  if (egl_image_ != EGL_NO_IMAGE_KHR) {
    functions_angle_->eglDestroyImageKHR(egl_display_, egl_image_);
    egl_image_ = EGL_NO_IMAGE_KHR;
  }

  if (egl_surface_ != EGL_NO_SURFACE) {
    functions_angle_->eglReleaseTexImage(egl_display_, egl_surface_,
                                         EGL_BACK_BUFFER);
    functions_angle_->eglDestroySurface(egl_display_, egl_surface_);
    egl_surface_ = EGL_NO_SURFACE;
  }

  texture_id_ = 0;
  return true;
}

std::optional<GLImageRepresentation::FramebufferInfo>
AngleD3DImageRepresentation::BindFrameBuffer() {
  if (fbo_id_ == 0) {
    auto texture_info = GetTexImage();
    if (!texture_info) {
      return {};
    }

    GLuint fbo_id;
    functions_angle_->glGenFramebuffers(1, &fbo_id);
    clay::ScopedFramebufferBinder scoped_framebuffer_binder(
        GL_FRAMEBUFFER, fbo_id, gl_api_windows_.get());
    clay::ScopedTextureBinder scoped_texture_binder(
        GL_TEXTURE_2D, texture_info->name, gl_api_windows_.get());

    functions_angle_->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_info->name,
        0);
    GLenum framebuffer_status =
        functions_angle_->glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
      FML_LOG(ERROR) << "Failed to create framebuffer.";
      functions_angle_->glDeleteFramebuffers(1, &fbo_id);
      return {};
    }

    fbo_id_ = fbo_id;
  }

  return FramebufferInfo{.target = GL_TEXTURE_2D, .name = fbo_id_};
}

bool AngleD3DImageRepresentation::UnbindFrameBuffer() {
  if (fbo_id_ == 0) {
    return false;
  }
  functions_angle_->glDeleteFramebuffers(1, &fbo_id_);
  fbo_id_ = 0;

  return true;
}

class AngleFenceSync final : public FenceSync {
 public:
  explicit AngleFenceSync(FunctionsAngle* functions_angle)
      : functions_angle_(functions_angle) {
    FML_DCHECK(functions_angle_->angle_fence_supported_);
    display_ = functions_angle_->eglGetCurrentDisplay();
    FML_DCHECK(display_ != EGL_NO_DISPLAY);
    fence_ = functions_angle_->eglCreateSyncKHR(display_, EGL_SYNC_FENCE_KHR,
                                                nullptr);
    if (fence_ == EGL_NO_SYNC_KHR) {
      FML_LOG(ERROR) << "Failed to create fence sync in ANGLE env, error: "
                     << functions_angle_->eglGetError();
    }
    functions_angle_->glFlush();
  }

  ~AngleFenceSync() override {
    FML_DCHECK(fence_ != EGL_NO_SYNC_KHR);
    EGLBoolean success = functions_angle_->eglDestroySyncKHR(display_, fence_);
    if (success == EGL_FALSE) {
      FML_LOG(ERROR) << "Failed to delete fence sync in ANGLE env, error: "
                     << functions_angle_->eglGetError();
    }
  }

  bool ClientWait() override {
    FML_DCHECK(fence_ != EGL_NO_SYNC_KHR);
    EGLint result = functions_angle_->eglClientWaitSyncKHR(display_, fence_, 0,
                                                           EGL_FOREVER_KHR);
    if (result != EGL_CONDITION_SATISFIED_KHR) {
      FML_LOG(ERROR) << "Failed to eglClientWaitSync in ANGLE env, error: "
                     << functions_angle_->eglGetError();
    }
    return result == EGL_CONDITION_SATISFIED_KHR;
  }

  Type GetType() const override { return Type::kANGLE; }

  void ServerWait() {
    FML_DCHECK(fence_ != EGL_NO_SYNC_KHR);
    EGLint result = functions_angle_->eglWaitSyncKHR(display_, fence_, 0);
    if (result == EGL_FALSE) {
      FML_LOG(ERROR) << "Failed to eglWaitSyncKHR in ANGLE env, error: "
                     << functions_angle_->eglGetError();
    }
  }

 private:
  EGLSyncKHR fence_;
  EGLDisplay display_ = EGL_NO_DISPLAY;
  FunctionsAngle* functions_angle_ = nullptr;
};

class DummyFenceSync final : public FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override { return Type::kClientWaitOnly; }
};

void AngleD3DImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  if (fence_sync->GetType() == FenceSync::Type::kANGLE) {
    AngleFenceSync* angle_fence_sync =
        static_cast<AngleFenceSync*>(fence_sync.get());
    angle_fence_sync->ServerWait();
    return;
  }
  fence_sync->ClientWait();
}

std::unique_ptr<FenceSync> AngleD3DImageRepresentation::ProduceFence() {
  // Sadly it seems that Angle sync doesn't support usage without context
  // if (functions_angle_->angle_fence_supported_) {
  //   return std::make_unique<AngleFenceSync>();
  // }
  // We use keyed mutex when BeginRW/EndRW instead of fence
  functions_angle_->glFlush();
  return std::make_unique<DummyFenceSync>();
}

}  // namespace clay
