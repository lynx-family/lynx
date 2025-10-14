// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/d3d9_texture_image_backing.h"

#include <memory>

#include "clay/gfx/shared_image/utils/image_utils.h"
#if SKIA_ENABLE_GL
#include "clay/gfx/shared_image/angle_d3d_image_representation.h"
#include "clay/gfx/shared_image/skia_gl_image_representation.h"
#endif
#include "clay/fml/logging.h"
#include "clay/fml/native_library.h"
#include "clay/gfx/shared_image/d3d_image_representation.h"
#include "clay/gfx/shared_image/utils/angle_get_proc.h"

#if ENABLE_SKITY
#include "clay/gfx/shared_image/angle_d3d_image_representation.h"
#include "clay/gfx/shared_image/skity_gl_image_representation.h"
#else
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/src/gpu/ganesh/GrDirectContextPriv.h"  // nogncheck
#include "third_party/skia/src/gpu/ganesh/gl/GrGLGpu.h"           // nogncheck
#endif

namespace clay {

namespace {

class D3D9TextureFactory {
 public:
  static D3D9TextureFactory& Instance();

  ~D3D9TextureFactory() = default;

  IDirect3DDevice9* GetDevice() { return d3d9_device_.Get(); }

 private:
  D3D9TextureFactory();
  bool InitializeD3D9Device();

  fml::RefPtr<fml::NativeLibrary> d3d9_;
  Microsoft::WRL::ComPtr<IDirect3D9> d3d9_api_;
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9_device_;
};

D3D9TextureFactory& D3D9TextureFactory::Instance() {
  // It seems that one D3D device binding to multiple WGL context
  // can not access multiple texture simultaneously (even they are safely
  // lock/unlocked).
  // To support buffer queue, we must create device per thread.
  static thread_local std::unique_ptr<D3D9TextureFactory> instance;
  if (!instance.get()) {
    instance.reset(new D3D9TextureFactory());
  }
  return *(instance.get());
}

D3D9TextureFactory::D3D9TextureFactory() {
  if (!InitializeD3D9Device()) {
    FML_LOG(ERROR)
        << "D3D9TextureImageBacking failed to initialize D3D Device.";
  }
}

bool D3D9TextureFactory::InitializeD3D9Device() {
  d3d9_ = fml::NativeLibrary::Create("d3d9.dll");

  if (!d3d9_) {
    FML_LOG(ERROR) << "Could not load D3D9 library.";
    return false;
  }

  auto Direct3DCreate9ExFn =
      d3d9_->ResolveFunction<decltype(&Direct3DCreate9Ex)>("Direct3DCreate9Ex");

  if (!Direct3DCreate9ExFn.has_value()) {
    FML_LOG(ERROR) << "Could not retrieve Direct3DCreate9Ex address.";
    return false;
  }
  // Use Direct3D9Ex, in ANGLE Renderer9.cpp, it's said that
  // "this version is less inclined to report a lost context,"
  Microsoft::WRL::ComPtr<IDirect3D9Ex> d3d9ex_api;

  HRESULT hr = Direct3DCreate9ExFn.value()(D3D_SDK_VERSION, &d3d9ex_api);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "D3D9TextureImageBacking could not create D3D9Ex api.";
    return false;
  }
  if (FAILED(d3d9ex_api.As(&d3d9_api_))) {
    FML_LOG(ERROR) << "D3D9TextureImageBacking could not create D3D9 api.";
    return false;
  }
  D3DPRESENT_PARAMETERS present_parameters = {};

  // The default swap chain is never actually used. Surface will create a new
  // swap chain with the proper parameters.
  present_parameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
  present_parameters.BackBufferCount = 1;
  present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
  present_parameters.BackBufferWidth = 1;
  present_parameters.BackBufferHeight = 1;
  present_parameters.EnableAutoDepthStencil = FALSE;
  present_parameters.Flags = 0;
  present_parameters.hDeviceWindow = GetDesktopWindow();
  present_parameters.MultiSampleQuality = 0;
  present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
  present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
  present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
  present_parameters.Windowed = TRUE;

  hr = d3d9_api_->CreateDevice(
      D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
      D3DCREATE_FPU_PRESERVE | D3DCREATE_NOWINDOWCHANGES |
          D3DCREATE_MULTITHREADED | D3DCREATE_HARDWARE_VERTEXPROCESSING |
          D3DCREATE_PUREDEVICE,
      &present_parameters, &d3d9_device_);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "D3D9TextureImageBacking could not create D3D9Ex Device.";
    return false;
  }
  return true;
}

std::optional<D3DFORMAT> ToD3DFormat(SharedImageBacking::PixelFormat format) {
  switch (format) {
    case SharedImageBacking::PixelFormat::kNative8888:
      // TODO(youfeng): D3D9 only support A8R8G8B8, but other platforms use BGRA
      // as default.
      return D3DFMT_A8R8G8B8;
      break;
    default:
      FML_LOG(ERROR) << "Invalid pixel format" << static_cast<uint32_t>(format);
      return {};
  }
}

bool CreateD3D9TextureSharedHandle(IDirect3DDevice9* device, D3DFORMAT format,
                                   skity::Vec2 size,
                                   IDirect3DTexture9** out_texture,
                                   HANDLE* out_handle) {
  // Use the shared handle with another Direct3D device (otherDevice)
  HRESULT hr =
      device->CreateTexture(size.x, size.y, 1, D3DUSAGE_RENDERTARGET, format,
                            D3DPOOL_DEFAULT, out_texture, out_handle);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "Failed to create D3D9 texture.";
    return false;
  }
  return true;
}

bool OpenD3D9SharedHandle(IDirect3DDevice9* device, D3DFORMAT format,
                          skity::Vec2 size, HANDLE shared_handle,
                          IDirect3DTexture9** out_texture) {
  HRESULT hr =
      device->CreateTexture(size.x, size.y, 1, D3DUSAGE_RENDERTARGET, format,
                            D3DPOOL_DEFAULT, out_texture, &shared_handle);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "Failed to open D3D9 texture.";
    return false;
  }
  return true;
}
}  // namespace

D3D9TextureImageBacking::D3D9TextureImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  d3d9_device_ = D3D9TextureFactory::Instance().GetDevice();
  auto opt_format = ToD3DFormat(pixel_format);
  if (!opt_format) {
    return;
  }
  d3d_format_ = opt_format.value();
  if (gfx_handle) {
    shared_handle_ = static_cast<HANDLE>(*gfx_handle);
    bool result = OpenD3D9SharedHandle(d3d9_device_.Get(), d3d_format_, size,
                                       shared_handle_, &d3d9_texture_);
    FML_CHECK(result);
  } else {
    bool result = CreateD3D9TextureSharedHandle(
        d3d9_device_.Get(), d3d_format_, size, &d3d9_texture_, &shared_handle_);
    FML_CHECK(result);
  }
  FML_CHECK(d3d9_texture_);
}

D3D9TextureImageBacking::~D3D9TextureImageBacking() = default;

bool D3D9TextureImageBacking::OpenForDevice(
    IDirect3DDevice9* device, IDirect3DTexture9** out_texture) const {
  return OpenD3D9SharedHandle(device, d3d_format_, size_, shared_handle_,
                              out_texture);
}

SharedImageBacking::BackingType D3D9TextureImageBacking::GetType() const {
  return BackingType::kD3DTexture;
}

GraphicsMemoryHandle D3D9TextureImageBacking::GetGFXHandle() const {
  return shared_handle_;
}

fml::RefPtr<SharedImageRepresentation>
D3D9TextureImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  FML_CHECK(config->struct_size == sizeof(ClaySharedImageRepresentationConfig));

  switch (config->type) {
    case kClaySharedImageRepresentationTypeGL: {
      PFNEGLGETPROCADDRESSPROC eglGetProcAddressProc =
          reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(
              config->gl_config.get_proc_address);
      if (!eglGetProcAddressProc) {
        eglGetProcAddressProc = GetAngleEglGetProcAddressProc();
      }
      return fml::MakeRefCounted<AngleD3DImageRepresentation>(
          fml::Ref(this), eglGetProcAddressProc);
    }
    case kClaySharedImageRepresentationTypeD3D:
      FML_CHECK(config->d3d_config.struct_size ==
                sizeof(ClaySharedImageD3DRepresentationConfig));
      return fml::MakeRefCounted<D3DImageRepresentation>(
          static_cast<IDirect3DDevice9*>(config->d3d_config.device),
          fml::Ref(this));
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call "
                    "D3D9TextureImageBacking::CreateRepresentation with type: "
                 << static_cast<uint32_t>(config->type);
  return nullptr;
}

#ifdef ENABLE_SKITY
fml::RefPtr<SkityImageRepresentation>
D3D9TextureImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context, fml::MakeRefCounted<AngleD3DImageRepresentation>(
                             fml::Ref(this), GetAngleEglGetProcAddressProc()));
    }
    default:
      break;
  }
  FML_LOG(ERROR) << "Unable to call "
                    "D3D9TextureImageBacking::CreateSkityRepresentation with "
                    "backend: "
                 << static_cast<uint32_t>(skity_context->GetBackendType());
  return nullptr;
}
#else
fml::RefPtr<SkiaImageRepresentation>
D3D9TextureImageBacking::CreateSkiaRepresentation(GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
#if SKIA_ENABLE_GL
    case GrBackendApi::kOpenGL: {
      GrGLGpu* gpu = static_cast<GrGLGpu*>(gr_context->priv().getGpu());
      // Skia repr is always internal, so we always use internal Angle symbols
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<AngleD3DImageRepresentation>(
                          fml::Ref(this), GetAngleEglGetProcAddressProc()));
    }
#endif
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call "
                    "D3D9TextureImageBacking::CreateSkiaRepresentation with "
                    "backend: "
                 << static_cast<uint32_t>(gr_context->backend());
  return nullptr;
}

bool D3D9TextureImageBacking::ReadbackToMemory(const SkPixmap* pixmaps,
                                               uint32_t planes) {
  IDirect3DTexture9* staging_texture = GetOrCreateStagingTexture();
  if (!staging_texture) {
    return false;
  }

  for (int plane = 0; plane < planes; ++plane) {
    Microsoft::WRL::ComPtr<IDirect3DSurface9> src_surface, dst_surface;
    if (FAILED(d3d9_texture_->GetSurfaceLevel(plane, &src_surface)) ||
        FAILED(staging_texture->GetSurfaceLevel(plane, &dst_surface))) {
      FML_LOG(ERROR) << "Failed to get surface.";
      return false;
    }
    HRESULT hr =
        d3d9_device_->GetRenderTargetData(src_surface.Get(), dst_surface.Get());
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to update texture.";
      return false;
    }

    D3DLOCKED_RECT locked_rect;
    hr = dst_surface->LockRect(&locked_rect, nullptr, D3DLOCK_READONLY);

    if (FAILED(hr)) {
      staging_texture->UnlockRect(plane);
      FML_LOG(ERROR) << "Failed to lock texture.";
      return false;
    }

    auto& pixmap = pixmaps[plane];
    uint8_t* dest_memory = static_cast<uint8_t*>(pixmap.writable_addr());
    const size_t dest_stride = pixmap.rowBytes();
    const uint8_t* source_memory = static_cast<uint8_t*>(locked_rect.pBits);
    const size_t source_stride = locked_rect.Pitch;

    CopyPlane(source_memory, source_stride, dest_memory, dest_stride,
              pixmap.info().minRowBytes(), size_.y);

    hr = staging_texture->UnlockRect(plane);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to unlock texture.";
      return false;
    }
  }

  return true;
}
#endif  // ENABLE_SKITY

IDirect3DTexture9* D3D9TextureImageBacking::GetOrCreateStagingTexture() {
  if (!staging_texture_) {
    HRESULT hr = d3d9_device_->CreateTexture(size_.x, size_.y, 1, 0,
                                             d3d_format_, D3DPOOL_SYSTEMMEM,
                                             &staging_texture_, nullptr);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to create staging texture.";
      return nullptr;
    }
  }

  return staging_texture_.Get();
}

}  // namespace clay
