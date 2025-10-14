// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/d3d_texture_image_backing.h"

#include <d3d11_3.h>

#include <memory>

#include "base/include/no_destructor.h"
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

#define __FORCE_DEDICATED_GPU 0

// For testing read memory from dedicated GPU
// https://stackoverflow.com/a/39047129
#if __FORCE_DEDICATED_GPU
extern "C" {
// NOLINTNEXTLINE(runtime/int) cspell:disable-next-line
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  // __FORCE_DEDICATED_GPU

namespace clay {

namespace {

class D3DTextureFactory {
 public:
  class ScopedDevice {
   public:
    ScopedDevice(std::mutex& m, ID3D11Device* device)
        : lock_(m), device_(device) {}

    ID3D11Device* GetDevice() const { return device_; }

   private:
    std::lock_guard<std::mutex> lock_;
    ID3D11Device* device_;
  };
  static D3DTextureFactory& Instance();

  ~D3DTextureFactory() = default;

  ScopedDevice GetDeviceScoped() {
    return ScopedDevice(mutex_, d3d11_device_.Get());
  }

  bool IsSupported() const { return d3d11_device_ != nullptr; }

 private:
  D3DTextureFactory();
  bool InitializeD3DDevice();

  fml::RefPtr<fml::NativeLibrary> d3d11_;
  fml::RefPtr<fml::NativeLibrary> dxgi_;
  std::mutex mutex_;
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;

  friend class fml::NoDestructor<D3DTextureFactory>;
};

D3DTextureFactory& D3DTextureFactory::Instance() {
  static fml::NoDestructor<D3DTextureFactory> instance;
  return *(instance.get());
}

D3DTextureFactory::D3DTextureFactory() {
  if (!InitializeD3DDevice()) {
    FML_LOG(ERROR) << "D3DTextureImageBacking failed to initialize D3D Device.";
  }
}

bool D3DTextureFactory::InitializeD3DDevice() {
  d3d11_ = fml::NativeLibrary::Create("d3d11.dll");
  dxgi_ = fml::NativeLibrary::Create("dxgi.dll");

  if (!d3d11_ || !dxgi_) {
    FML_LOG(ERROR) << "Could not load D3D11 or DXGI library.";
    return false;
  }

  std::optional<PFN_D3D11_CREATE_DEVICE> D3D11CreateDevice =
      d3d11_->ResolveFunction<PFN_D3D11_CREATE_DEVICE>("D3D11CreateDevice");

  if (!D3D11CreateDevice.has_value()) {
    FML_LOG(ERROR) << "Could not retrieve D3D11CreateDevice address.";
    return false;
  }
  HRESULT hr = D3D11CreateDevice.value()(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
      D3D11_SDK_VERSION, &d3d11_device_, nullptr, nullptr);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "D3DTextureImageBacking could not create D3D11 Device.";
    return false;
  }
  return true;
}

bool CreateD3DTextureSharedHandle(ID3D11Device* device,
                                  SharedImageBacking::PixelFormat pixel_format,
                                  skity::Vec2 size,
                                  ID3D11Texture2D** out_texture,
                                  HANDLE* out_handle, bool* is_nt_handle) {
  DXGI_FORMAT dxgi_format;
  switch (pixel_format) {
    case SharedImageBacking::PixelFormat::kNative8888:
      dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
      break;
    case SharedImageBacking::PixelFormat::kRGBA8888:
      dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
      break;
    default:
      FML_LOG(ERROR) << "Invalid pixel format";
      return false;
  }

  D3D11_TEXTURE2D_DESC desc = {
      static_cast<UINT>(size.x),
      static_cast<UINT>(size.y),
      1,
      1,
      dxgi_format,
      {1, 0},
      D3D11_USAGE_DEFAULT,
      D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
      0,
      D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
          D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX};

  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture;
  if (FAILED(device->CreateTexture2D(&desc, nullptr, &d3d11_texture))) {
    FML_LOG(ERROR) << "Could not create D3D texture2D.";
    return false;
  }

  d3d11_texture.CopyTo(out_texture);

  if (Microsoft::WRL::ComPtr<IDXGIResource1> dxgi_resource1;
      SUCCEEDED(d3d11_texture.As(&dxgi_resource1))) {
    if (SUCCEEDED(dxgi_resource1->CreateSharedHandle(
            nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
            nullptr, out_handle))) {
      *is_nt_handle = true;
      return true;
    } else {
      FML_LOG(ERROR) << "Could not create shared handle from IDXGIResource1.";
    }
  }

  if (Microsoft::WRL::ComPtr<IDXGIResource> dxgi_resource;
      SUCCEEDED(d3d11_texture.As(&dxgi_resource))) {
    if (SUCCEEDED(dxgi_resource->GetSharedHandle(out_handle))) {
      return true;
    } else {
      FML_LOG(ERROR) << "Could not create shared handle from IDXGIResource.";
    }
  }

  return false;
}

bool OpenD3DSharedHandle(ID3D11Device* device, HANDLE shared_handle,
                         ID3D11Texture2D** out_texture) {
  HRESULT hr =
      device->OpenSharedResource(shared_handle, __uuidof(ID3D11Texture2D),
                                 reinterpret_cast<void**>(out_texture));

  if (SUCCEEDED(hr)) {
    return true;
  }

  Microsoft::WRL::ComPtr<ID3D11Device1> d3d11_device1;
  device->QueryInterface(__uuidof(ID3D11Device1), &d3d11_device1);
  if (d3d11_device1) {
    hr = d3d11_device1->OpenSharedResource1(
        shared_handle, __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(out_texture));
    return SUCCEEDED(hr);
  }

  return false;
}
}  // namespace

D3DTextureImageBacking::D3DTextureImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBacking(pixel_format, size) {
  D3DTextureFactory::ScopedDevice scoped_device =
      D3DTextureFactory::Instance().GetDeviceScoped();
  ID3D11Device* d3d11_device = scoped_device.GetDevice();
  if (gfx_handle) {
    DuplicateHandle(GetCurrentProcess(), static_cast<HANDLE>(*gfx_handle),
                    GetCurrentProcess(), &shared_handle_, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    owned_nt_handle_ = true;
    bool result =
        OpenD3DSharedHandle(d3d11_device, shared_handle_, &d3d11_texture_);
    FML_CHECK(result);
  } else {
    bool result = CreateD3DTextureSharedHandle(d3d11_device, pixel_format, size,
                                               &d3d11_texture_, &shared_handle_,
                                               &owned_nt_handle_);
    FML_CHECK(result);
  }
  FML_CHECK(d3d11_texture_);
  d3d11_texture_->GetDesc(&d3d11_texture_desc_);
  if (gfx_handle && size_.x == 0 && size_.y == 0) {
    size_.x = d3d11_texture_desc_.Width;
    size_.y = d3d11_texture_desc_.Height;
  }
}

D3DTextureImageBacking::~D3DTextureImageBacking() {
  D3DTextureFactory::ScopedDevice scoped_device =
      D3DTextureFactory::Instance().GetDeviceScoped();
  if (owned_nt_handle_ && shared_handle_) {
    CloseHandle(shared_handle_);
    shared_handle_ = nullptr;
  }
  d3d11_texture_.Reset();
  staging_texture_.Reset();
  keyed_mutex_.Reset();
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  scoped_device.GetDevice()->GetImmediateContext(&device_context);
  if (device_context) {
    device_context->Flush();
  }
}

bool D3DTextureImageBacking::OpenForDevice(
    ID3D11Device* device, ID3D11Texture2D** out_texture,
    IDXGIKeyedMutex** out_keyed_mutex) const {
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture;
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex;

  bool result = OpenD3DSharedHandle(device, shared_handle_, &d3d11_texture);
  if (!result) {
    FML_LOG(ERROR) << "Failed to OpenD3DSharedHandle.";
    return false;
  }
  if (HasKeyedMutex()) {
    if (FAILED(d3d11_texture.As(&keyed_mutex))) {
      FML_LOG(ERROR) << "Failed to get keyed mutex.";
      return false;
    }
  }

  d3d11_texture.CopyTo(out_texture);
  keyed_mutex.CopyTo(out_keyed_mutex);
  return true;
}

SharedImageBacking::BackingType D3DTextureImageBacking::GetType() const {
  return BackingType::kD3DTexture;
}

GraphicsMemoryHandle D3DTextureImageBacking::GetGFXHandle() const {
  return shared_handle_;
}

fml::RefPtr<SharedImageRepresentation>
D3DTextureImageBacking::CreateRepresentation(
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
          static_cast<ID3D11Device*>(config->d3d_config.device),
          fml::Ref(this));
    default:
      break;
  }

  FML_LOG(ERROR) << "Unable to call "
                    "D3DTextureImageBacking::CreateRepresentation with type: "
                 << static_cast<uint32_t>(config->type);
  return nullptr;
}

#ifdef ENABLE_SKITY
fml::RefPtr<SkityImageRepresentation>
D3DTextureImageBacking::CreateSkityRepresentation(
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
D3DTextureImageBacking::CreateSkiaRepresentation(GrDirectContext* gr_context) {
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

  FML_LOG(ERROR)
      << "Unable to call D3DTextureImageBacking::CreateSkiaRepresentation with "
         "backend: "
      << static_cast<uint32_t>(gr_context->backend());
  return nullptr;
}

bool D3DTextureImageBacking::ReadbackToMemory(const SkPixmap* pixmaps,
                                              uint32_t planes) {
  D3DTextureFactory::ScopedDevice scoped_device =
      D3DTextureFactory::Instance().GetDeviceScoped();
  ID3D11Device* d3d11_device = scoped_device.GetDevice();
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  d3d11_device->GetImmediateContext(&device_context);

  FML_DCHECK(device_context);

  if (d3d11_texture_desc_.CPUAccessFlags & D3D11_CPU_ACCESS_READ) {
    Microsoft::WRL::ComPtr<ID3D11Device3> device3;
    HRESULT hr =
        d3d11_device->QueryInterface(__uuidof(ID3D11Device3), &device3);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to retrieve ID3D11Device3. hr=" << std::hex
                     << hr;
      return false;
    }
    hr = device_context->Map(d3d11_texture_.Get(), 0, D3D11_MAP_READ, 0,
                             nullptr);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to map texture for read. hr=" << std::hex << hr;
      return false;
    }

    uint8_t* dest_memory = static_cast<uint8_t*>(pixmaps[0].writable_addr());
    const size_t dest_stride = pixmaps[0].rowBytes();
    device3->ReadFromSubresource(dest_memory, dest_stride, 0,
                                 d3d11_texture_.Get(), 0, nullptr);
    device_context->Unmap(d3d11_texture_.Get(), 0);
  } else {
    ID3D11Texture2D* staging_texture = GetOrCreateStagingTexture(d3d11_device);
    if (!staging_texture) {
      return false;
    }

    IDXGIKeyedMutex* keyed_mutex = GetOrCreateKeyedMutex();

    {
      std::optional<ScopedDXGIKeyedMutex> scoped_keyed_mutex;
      if (keyed_mutex) {
        scoped_keyed_mutex.emplace(keyed_mutex);
      }
      device_context->CopyResource(staging_texture, d3d11_texture_.Get());
    }

    D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
    HRESULT hr = device_context->Map(staging_texture, 0, D3D11_MAP_READ, 0,
                                     &mapped_resource);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to map texture for read. hr=" << std::hex << hr;
      return false;
    }

    // The mapped staging texture pData points to the first plane's data so an
    // offset is needed for subsequent planes.
    size_t source_offset = 0;

    for (int plane = 0; plane < planes; ++plane) {
      auto& pixmap = pixmaps[plane];
      uint8_t* dest_memory = static_cast<uint8_t*>(pixmap.writable_addr());
      const size_t dest_stride = pixmap.rowBytes();
      const uint8_t* source_memory =
          static_cast<uint8_t*>(mapped_resource.pData) + source_offset;
      const size_t source_stride = mapped_resource.RowPitch;

      CopyPlane(source_memory, source_stride, dest_memory, dest_stride,
                pixmap.info().minRowBytes(), size_.y);

      source_offset += mapped_resource.RowPitch * size_.y;
    }

    device_context->Unmap(staging_texture, 0);
  }
  return true;
}
#endif  // ENABLE_SKITY

ID3D11Texture2D* D3DTextureImageBacking::GetOrCreateStagingTexture(
    ID3D11Device* device) {
  if (!staging_texture_) {
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = d3d11_texture_desc_.Width;
    staging_desc.Height = d3d11_texture_desc_.Height;
    staging_desc.Format = d3d11_texture_desc_.Format;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.CPUAccessFlags =
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    HRESULT hr =
        device->CreateTexture2D(&staging_desc, nullptr, &staging_texture_);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to create staging texture. hr=" << std::hex
                     << hr;
      return nullptr;
    }

    constexpr char kStagingTextureLabel[] = "SharedImageD3D_StagingTexture";
    // Add debug label to the long lived texture.
    /* cspell:disable-next-line */
    staging_texture_->SetPrivateData(WKPDID_D3DDebugObjectName,
                                     strlen(kStagingTextureLabel),
                                     kStagingTextureLabel);
  }
  return staging_texture_.Get();
}

IDXGIKeyedMutex* D3DTextureImageBacking::GetOrCreateKeyedMutex() {
  if (!HasKeyedMutex()) {
    return nullptr;
  }
  if (!keyed_mutex_) {
    if (FAILED(d3d11_texture_.As(&keyed_mutex_))) {
      FML_LOG(ERROR) << "Failed to get keyed mutex.";
    }
  }
  return keyed_mutex_.Get();
}

bool D3DTextureImageBacking::IsSupported() {
#ifdef CLAY_FORCE_D3D9
  return false;
#else
  return D3DTextureFactory::Instance().IsSupported();
#endif
}

}  // namespace clay
