// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore REFGUID EGLAPIENTRY

#include <wrl/implements.h>

#include <cstring>
#include <memory>
#include <utility>

#include "clay/gfx/shared_image/angle_d3d_image_representation.h"
#include "clay/gfx/shared_image/d3d_image_representation.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

class TestBacking final : public SharedImageBacking {
 public:
  TestBacking() : SharedImageBacking(PixelFormat::kNative8888, {1, 1}) {}
  BackingType GetType() const override { return BackingType::kMocking; }
  GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
#ifdef ENABLE_SKITY
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      GrContext*) override {
    return nullptr;
  }
#else
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrContext*) override {
    return nullptr;
  }
#endif
};

class TestKeyedMutex final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDXGIKeyedMutex> {
 public:
  HRESULT result = S_OK;
  int acquires = 0;
  int releases = 0;

  HRESULT STDMETHODCALLTYPE AcquireSync(UINT64 key, DWORD timeout) override {
    EXPECT_EQ(key, 0u);
    EXPECT_EQ(timeout, 2000u);
    ++acquires;
    return result;
  }
  HRESULT STDMETHODCALLTYPE ReleaseSync(UINT64 key) override {
    EXPECT_EQ(key, 0u);
    ++releases;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT,
                                           const void*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,
                                                    const IUnknown*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetParent(REFIID, void**) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE GetDevice(REFIID, void**) override {
    return E_NOTIMPL;
  }
};

// Only the COM lifetime is used: cached GL objects bypass texture creation.
class TestTexture final
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, ID3D11Texture2D> {
 public:
  void STDMETHODCALLTYPE GetDevice(ID3D11Device**) override {}
  HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID, UINT*, void*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID, UINT,
                                           const void*) override {
    return E_NOTIMPL;
  }
  HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID,
                                                    const IUnknown*) override {
    return E_NOTIMPL;
  }
  void STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION*) override {}
  void STDMETHODCALLTYPE SetEvictionPriority(UINT) override {}
  UINT STDMETHODCALLTYPE GetEvictionPriority() override { return 0; }
  void STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE2D_DESC*) override {}
};

class TestFence final : public FenceSync {
 public:
  explicit TestFence(bool success) : success_(success) {}
  bool ClientWait() override { return success_; }
  Type GetType() const override { return Type::kClientWaitOnly; }

 private:
  bool success_;
};

EGLDisplay EGLAPIENTRY CurrentDisplay() { return EGL_NO_DISPLAY; }
const char* EGLAPIENTRY Extensions(EGLDisplay, EGLint) { return ""; }
void GL_APIENTRY DeleteObjects(GLsizei, const GLuint*) {}
void GL_APIENTRY Flush() {}

__eglMustCastToProperFunctionPointerType EGLAPIENTRY GetProc(const char* name) {
  if (std::strcmp(name, "eglGetCurrentDisplay") == 0) {
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(
        CurrentDisplay);
  }
  if (std::strcmp(name, "eglQueryString") == 0) {
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(
        Extensions);
  }
  return nullptr;
}

}  // namespace

class WindowsImageRepresentationTest : public ::testing::TestWithParam<bool> {
 protected:
  void SetUp() override {
    backing_ = fml::MakeRefCounted<TestBacking>();
    mutex_ = Microsoft::WRL::Make<TestKeyedMutex>();
  }

  fml::RefPtr<SharedImageRepresentation> MakeRepresentation(
      bool with_mutex = true, bool with_texture = true) {
    auto texture = Microsoft::WRL::Make<TestTexture>();
    if (GetParam()) {
      auto repr = fml::AdoptRef(new AngleD3DImageRepresentation(backing_));
      repr->functions_angle_ = std::make_unique<FunctionsAngle>(GetProc);
      repr->functions_angle_->glDeleteTexturesProc = DeleteObjects;
      repr->functions_angle_->glDeleteFramebuffersProc = DeleteObjects;
      repr->functions_angle_->glFlushProc = Flush;
      if (with_texture) {
        repr->d3d11_texture_ = texture;
        repr->texture_id_ = 1;
        repr->fbo_id_ = 2;
      }
      if (with_mutex) repr->keyed_mutex_ = mutex_;
      return repr;
    }
    auto repr = fml::AdoptRef(new D3DImageRepresentation(backing_));
    if (with_texture) repr->d3d11_texture_ = texture;
    if (with_mutex) repr->keyed_mutex_ = mutex_;
    return repr;
  }

  bool Begin(SharedImageRepresentation* repr, bool write) {
    return write ? repr->BeginWrite(&write_) : repr->BeginRead(&read_);
  }
  bool End(SharedImageRepresentation* repr, bool write) {
    return write ? repr->EndWrite() : repr->EndRead();
  }
  void ReleaseOutput(bool write) {
    if (GetParam()) {
      if (write) {
        write_.opengl_framebuffer.destruction_callback(
            write_.opengl_framebuffer.user_data);
      } else {
        read_.opengl_texture.destruction_callback(
            read_.opengl_texture.user_data);
      }
    } else if (write) {
      write_.d3d_texture.destruction_callback(write_.d3d_texture.user_data);
    } else {
      read_.d3d_texture.destruction_callback(read_.d3d_texture.user_data);
    }
    read_ = {};
    write_ = {};
  }

  fml::RefPtr<TestBacking> backing_;
  Microsoft::WRL::ComPtr<TestKeyedMutex> mutex_;
  ClaySharedImageReadResult read_{};
  ClaySharedImageWriteResult write_{};
};

TEST_P(WindowsImageRepresentationTest, FailedAcquireDoesNotPublishOrRetain) {
  for (HRESULT result :
       {HRESULT(WAIT_TIMEOUT), HRESULT(WAIT_ABANDONED), E_FAIL}) {
    for (bool write : {false, true}) {
      mutex_->result = result;
      auto repr = MakeRepresentation();
      EXPECT_FALSE(Begin(repr.get(), write));
      EXPECT_EQ(read_.struct_size, 0u);
      EXPECT_EQ(write_.struct_size, 0u);
      EXPECT_TRUE(repr->HasOneRef());
      EXPECT_FALSE(End(repr.get(), write));
      EXPECT_EQ(mutex_->releases, 0);
      // Begin can fail before a caller ever invokes End or ProduceFence.
      auto failure = backing_->GetFenceSync();
      ASSERT_NE(failure, nullptr);
      EXPECT_FALSE(failure->ClientWait());
      EXPECT_EQ(failure->Poll(), FenceSync::PollResult::kFailed);
      auto next = MakeRepresentation();
      next->ConsumeFence(std::move(failure));
      EXPECT_FALSE(Begin(next.get(), write));
      EXPECT_FALSE(next->ProduceFence()->ClientWait());
    }
  }
}

TEST_P(WindowsImageRepresentationTest, FailedDependencySurvivesSlotReturn) {
  auto dependency = std::make_unique<TestFence>(false);
  auto* original = dependency.get();
  backing_->SetFenceSync(std::move(dependency));
  for (int i = 0; i != 3; ++i) {
    auto repr = MakeRepresentation();
    repr->ConsumeFence(backing_->GetFenceSync());
    repr->ConsumeFence(nullptr);
    EXPECT_FALSE(Begin(repr.get(), false));
    EXPECT_FALSE(Begin(repr.get(), true));
    EXPECT_FALSE(End(repr.get(), false));
    EXPECT_FALSE(End(repr.get(), true));
    EXPECT_TRUE(repr->HasOneRef());
    auto returned = repr->ProduceFence();
    EXPECT_EQ(returned.get(), original);
    EXPECT_FALSE(returned->ClientWait());
    EXPECT_FALSE(repr->ProduceFence()->ClientWait());
    backing_->SetFenceSync(std::move(returned));
  }
  EXPECT_EQ(mutex_->acquires, 0);
}

TEST_P(WindowsImageRepresentationTest, LaterDependencyCannotClearFailure) {
  auto repr = MakeRepresentation();
  repr->ConsumeFence(std::make_unique<TestFence>(false));
  repr->ConsumeFence(std::make_unique<TestFence>(true));
  EXPECT_FALSE(Begin(repr.get(), false));
  EXPECT_FALSE(repr->ProduceFence()->ClientWait());
}

TEST_P(WindowsImageRepresentationTest, SuccessfulAndMutexFreeAccess) {
  for (bool with_mutex : {false, true}) {
    for (bool write : {false, true}) {
      auto repr = MakeRepresentation(with_mutex);
      repr->ConsumeFence(std::make_unique<TestFence>(true));
      ASSERT_TRUE(Begin(repr.get(), write));
      EXPECT_FALSE(repr->HasOneRef());
      EXPECT_TRUE(End(repr.get(), write));
      ReleaseOutput(write);
      EXPECT_TRUE(repr->HasOneRef());
      auto fence = repr->ProduceFence();
      EXPECT_TRUE(fence->ClientWait());
      EXPECT_EQ(fence->Poll(), FenceSync::PollResult::kUnsupported);
    }
  }
  EXPECT_EQ(mutex_->acquires, 2);
  EXPECT_EQ(mutex_->releases, 2);
}

TEST_P(WindowsImageRepresentationTest, RepeatedBeginKeepsOriginalLock) {
  auto repr = MakeRepresentation();
  ASSERT_TRUE(Begin(repr.get(), false));
  EXPECT_FALSE(Begin(repr.get(), true));
  EXPECT_EQ(mutex_->acquires, 1);
  EXPECT_EQ(mutex_->releases, 0);
  EXPECT_TRUE(End(repr.get(), false));
  ReleaseOutput(false);
  EXPECT_EQ(mutex_->releases, 1);
}

TEST_P(WindowsImageRepresentationTest, FailedGLSetupReleasesAcquiredMutex) {
  if (!GetParam()) return;
  auto repr = MakeRepresentation(true, false);
  for (bool write : {false, true}) {
    EXPECT_FALSE(Begin(repr.get(), write));
    EXPECT_TRUE(repr->HasOneRef());
  }
  EXPECT_EQ(mutex_->acquires, 2);
  EXPECT_EQ(mutex_->releases, 2);
}

TEST_P(WindowsImageRepresentationTest, DestructorReleasesOutstandingMutex) {
  auto repr = MakeRepresentation();
  ASSERT_TRUE(Begin(repr.get(), false));
  ReleaseOutput(false);
  repr = nullptr;
  EXPECT_EQ(mutex_->releases, 1);
}

INSTANTIATE_TEST_SUITE_P(D3DAndANGLE, WindowsImageRepresentationTest,
                         ::testing::Bool());

}  // namespace clay
