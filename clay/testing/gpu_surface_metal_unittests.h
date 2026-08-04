// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_TESTING_GPU_SURFACE_METAL_UNITTESTS_H_
#define CLAY_TESTING_GPU_SURFACE_METAL_UNITTESTS_H_

#import <Metal/Metal.h>

#include <array>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

#include "clay/common/graphics/msaa_sample_count.h"
#include "clay/flow/surface_frame.h"
#include "clay/shell/gpu/gpu_surface_metal_delegate.h"
#include "clay/testing/testing.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {
namespace testing {
namespace {

constexpr skity::Vec2 kFrameSize = {16, 16};
constexpr std::array<size_t, 5> kSurfacePoolSizes = {0, 1, 2, 1, 0};
constexpr size_t kResizeCycleCount = 8;

using PendingSubmit = std::pair<SurfaceFrame::SubmitCallback, SurfaceFrame::SubmitInfo>;

struct TextureCounters {
  size_t acquired = 0;
  size_t presented = 0;
  size_t destroyed = 0;
};

class TestMetalDelegate final : public GPUSurfaceMetalDelegate {
 public:
  TestMetalDelegate(id<MTLDevice> device, std::shared_ptr<TextureCounters> counters)
      : GPUSurfaceMetalDelegate(MTLRenderTargetType::kMTLTexture),
        device_(device),
        counters_(std::move(counters)) {}

  GPUCAMetalLayerHandle GetCAMetalLayer(const skity::Vec2& frame_info) const override {
    return nullptr;
  }

  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& frame_info) const override {
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:static_cast<NSUInteger>(frame_info.x)
                                    height:static_cast<NSUInteger>(frame_info.y)
                                 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device_ newTextureWithDescriptor:descriptor];
    if (!texture) {
      return {};
    }

    auto* lifetime = new TextureLifetime{texture, counters_};
    const int64_t texture_id = static_cast<int64_t>(counters_->acquired++);
    return {
        .texture_id = texture_id,
        .texture = (__bridge GPUMTLTextureHandle)texture,
        .destruction_callback = DestroyTexture,
        .destruction_context = lifetime,
    };
  }

  bool PresentTexture(GPUMTLTextureInfo texture) const override {
    if (!texture.texture) {
      return false;
    }
    counters_->presented++;
    return true;
  }

 private:
  struct TextureLifetime {
    __strong id<MTLTexture> texture;
    std::shared_ptr<TextureCounters> counters;

    ~TextureLifetime() { counters->destroyed++; }
  };

  static void DestroyTexture(void* context) { delete static_cast<TextureLifetime*>(context); }

  __strong id<MTLDevice> device_;
  std::shared_ptr<TextureCounters> counters_;
};

class RealMetalContext {
 public:
  RealMetalContext()
      : device_(MTLCreateSystemDefaultDevice()), command_queue_([device_ newCommandQueue]) {
    if (!device_ || !command_queue_) {
      return;
    }
    context_ = CreateTestedMetalContext(device_, command_queue_);
  }

  bool IsValid() const { return context_ != nullptr; }

  id<MTLDevice> device() const { return device_; }

  const TestedMetalContext& context() const { return context_; }

 private:
  __strong id<MTLDevice> device_;
  __strong id<MTLCommandQueue> command_queue_;
  TestedMetalContext context_;
};

std::unique_ptr<TestedMetalSurface> CreateSurface(TestMetalDelegate* delegate,
                                                  const TestedMetalContext& context) {
  return std::make_unique<TestedMetalSurface>(delegate, context, MsaaSampleCount::kNone, true);
}

bool QueueFrame(TestedMetalSurface& surface, std::vector<PendingSubmit>& pending_submits) {
  auto frame = static_cast<Surface&>(surface).AcquireFrame(kFrameSize);
  if (!frame) {
    return false;
  }
  frame->Prepare(std::nullopt);
  PendingSubmit pending_submit = frame->PrepareSubmit();
  if (!pending_submit.first) {
    return false;
  }
  pending_submits.push_back(std::move(pending_submit));
  return true;
}

TEST(GPUSurfaceMetalTest, SubmitCallbackDoesNotDependOnSurfaceLifetime) {
  @autoreleasepool {
    RealMetalContext metal;
    if (!metal.IsValid()) {
      GTEST_SKIP() << "Metal is unavailable.";
    }

    auto first_counters = std::make_shared<TextureCounters>();
    auto replacement_counters = std::make_shared<TextureCounters>();
    auto first_delegate = std::make_shared<TestMetalDelegate>(metal.device(), first_counters);
    auto replacement_delegate =
        std::make_shared<TestMetalDelegate>(metal.device(), replacement_counters);

    alignas(TestedMetalSurface) std::byte surface_storage[sizeof(TestedMetalSurface)];
    auto* first_surface = new (surface_storage)
        TestedMetalSurface(first_delegate.get(), metal.context(), MsaaSampleCount::kNone, true);

    std::vector<PendingSubmit> pending_submits;
    ASSERT_TRUE(QueueFrame(*first_surface, pending_submits));
    first_surface->~TestedMetalSurface();

    auto* replacement_surface = new (surface_storage) TestedMetalSurface(
        replacement_delegate.get(), metal.context(), MsaaSampleCount::kNone, true);

    ASSERT_TRUE(pending_submits.front().first(pending_submits.front().second));
    EXPECT_EQ(first_counters->presented, 1u);
    EXPECT_EQ(replacement_counters->presented, 0u);
    EXPECT_EQ(first_counters->destroyed, 0u);

    replacement_surface->~TestedMetalSurface();
    pending_submits.clear();
    EXPECT_EQ(first_counters->destroyed, 1u);
  }
}

TEST(GPUSurfaceMetalTest, TwoSurfacePoolsResizeRepeatedlyWithDelayedSubmitCallbacks) {
  @autoreleasepool {
    RealMetalContext metal;
    if (!metal.IsValid()) {
      GTEST_SKIP() << "Metal is unavailable.";
    }

    std::array<std::shared_ptr<TextureCounters>, 2> counters = {
        std::make_shared<TextureCounters>(),
        std::make_shared<TextureCounters>(),
    };
    std::array<std::shared_ptr<TestMetalDelegate>, 2> delegates = {
        std::make_shared<TestMetalDelegate>(metal.device(), counters[0]),
        std::make_shared<TestMetalDelegate>(metal.device(), counters[1]),
    };
    std::array<std::vector<std::unique_ptr<TestedMetalSurface>>, 2> pools;
    std::vector<PendingSubmit> pending_submits;

    for (size_t cycle = 0; cycle < kResizeCycleCount; cycle++) {
      for (size_t target_size : kSurfacePoolSizes) {
        for (size_t pool_index = 0; pool_index < pools.size(); pool_index++) {
          auto& pool = pools[pool_index];
          while (pool.size() < target_size) {
            pool.push_back(CreateSurface(delegates[pool_index].get(), metal.context()));
          }
          pool.resize(target_size);
          for (const auto& surface : pool) {
            ASSERT_TRUE(QueueFrame(*surface, pending_submits));
          }
        }
      }
    }

    ASSERT_TRUE(pools[0].empty());
    ASSERT_TRUE(pools[1].empty());
    const size_t expected_textures_per_pool = kResizeCycleCount * 4;
    EXPECT_EQ(counters[0]->acquired, expected_textures_per_pool);
    EXPECT_EQ(counters[1]->acquired, expected_textures_per_pool);
    EXPECT_EQ(counters[0]->presented, 0u);
    EXPECT_EQ(counters[1]->presented, 0u);
    EXPECT_EQ(counters[0]->destroyed, 0u);
    EXPECT_EQ(counters[1]->destroyed, 0u);

    for (const auto& [callback, submit_info] : pending_submits) {
      ASSERT_TRUE(callback(submit_info));
    }
    EXPECT_EQ(counters[0]->presented, expected_textures_per_pool);
    EXPECT_EQ(counters[1]->presented, expected_textures_per_pool);
    EXPECT_EQ(counters[0]->destroyed, 0u);
    EXPECT_EQ(counters[1]->destroyed, 0u);

    pending_submits.clear();
    EXPECT_EQ(counters[0]->destroyed, expected_textures_per_pool);
    EXPECT_EQ(counters[1]->destroyed, expected_textures_per_pool);
  }
}

}  // namespace
}  // namespace testing
}  // namespace clay

#endif  // CLAY_TESTING_GPU_SURFACE_METAL_UNITTESTS_H_
