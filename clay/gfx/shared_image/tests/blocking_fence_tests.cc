// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"

#define CHECK_WAIT(condition)       \
  do {                              \
    if (!(condition)) std::abort(); \
  } while (0)

namespace {
class Backing final : public clay::SharedImageBacking {
 public:
  Backing() : SharedImageBacking(PixelFormat::kRGBA8888, {4, 4}) {}
  BackingType GetType() const override { return BackingType::kMocking; }
  clay::GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<clay::SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
#ifdef ENABLE_SKITY
  fml::RefPtr<clay::SkityImageRepresentation> CreateSkityRepresentation(
      clay::GrContext*) override {
    return nullptr;
  }
#else
  fml::RefPtr<clay::SkiaImageRepresentation> CreateSkiaRepresentation(
      clay::GrContext*) override {
    return nullptr;
  }
#endif
};

class Dependency final : public clay::FenceSync {
 public:
  explicit Dependency(bool success) : success_(success) {}
  bool ClientWait() override { return success_; }
  Type GetType() const override { return Type::kClientWaitOnly; }

 private:
  bool success_;
};

class Representation final : public clay::GLImageRepresentation {
 public:
  explicit Representation(fml::RefPtr<Backing> backing)
      : GLImageRepresentation(std::move(backing)) {}
  clay::ImageRepresentationType GetType() const override {
    return clay::ImageRepresentationType::kGL;
  }
  void ConsumeFence(std::unique_ptr<clay::FenceSync> fence) override {
    WaitForFence(std::move(fence));
  }
  std::unique_ptr<clay::FenceSync> ProduceFence() override {
    return HasFailedFence() ? TakeFailedFence() : nullptr;
  }
  int accesses = 0;

 private:
  std::optional<TextureInfo> GetTexImage() override {
    ++accesses;
    return TextureInfo{0, 1, 0, std::nullopt};
  }
  bool ReleaseTexImage() override { return true; }
  std::optional<FramebufferInfo> BindFrameBuffer() override {
    ++accesses;
    return FramebufferInfo{0, 1};
  }
  bool UnbindFrameBuffer() override { return true; }
};
}  // namespace

int main() {
  auto backing = fml::MakeRefCounted<Backing>();
  auto representation = fml::MakeRefCounted<Representation>(backing);
  ClaySharedImageReadResult read{};
  ClaySharedImageWriteResult write{};
  representation->ConsumeFence(std::make_unique<Dependency>(true));
  CHECK_WAIT(representation->BeginRead(&read));
  read.opengl_texture.destruction_callback(read.opengl_texture.user_data);
  CHECK_WAIT(representation->BeginWrite(&write));
  write.opengl_framebuffer.destruction_callback(
      write.opengl_framebuffer.user_data);
  CHECK_WAIT(representation->EndRead() && representation->EndWrite());
  CHECK_WAIT(!representation->ProduceFence());
  auto failed = std::make_unique<Dependency>(false);
  auto* original = failed.get();
  representation->ConsumeFence(std::move(failed));
  const int accesses = representation->accesses;
  representation->ConsumeFence(nullptr);
  CHECK_WAIT(!representation->BeginRead(&read));
  CHECK_WAIT(!representation->BeginWrite(&write));
  CHECK_WAIT(!representation->EndRead() && !representation->EndWrite());
  CHECK_WAIT(representation->accesses == accesses);
  auto returned = representation->ProduceFence();
  CHECK_WAIT(returned.get() == original && !returned->ClientWait());
  CHECK_WAIT(representation->ProduceFence()->Poll() ==
             clay::FenceSync::PollResult::kFailed);
  representation = fml::MakeRefCounted<Representation>(backing);
  representation->ConsumeFence(std::move(returned));
  CHECK_WAIT(!representation->BeginRead(&read) &&
             representation->accesses == 0);
  std::puts(
      "PASS blocking fences: normal access, failed/null dependencies, "
      "no resource access after failure, returned proof and recreation");
}
