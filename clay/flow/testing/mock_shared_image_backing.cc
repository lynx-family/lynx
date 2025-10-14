// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/testing/mock_shared_image_backing.h"

#include <memory>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkData.h"

namespace clay {
namespace testing {

class MockFenceSync : public clay::FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override {
    return clay::FenceSync::Type::kClientWaitOnly;
  }
};

class MockSkiaImageRepresentation final : public clay::SkiaImageRepresentation {
 public:
  explicit MockSkiaImageRepresentation(
      fml::RefPtr<MockSharedImageBacking> backing)
      : SkiaImageRepresentation(backing) {}

  clay::ImageRepresentationType GetType() const override {
    return clay::ImageRepresentationType::kSkia;
  }

  sk_sp<SkImage> GetSkImage() override {
    if (sk_image_) {
      return sk_image_;
    }
    MockSharedImageBacking* backing =
        static_cast<MockSharedImageBacking*>(GetBacking());
    SkColorInfo color_info(kRGBA_8888_SkColorType, kPremul_SkAlphaType,
                           nullptr);
    SkImageInfo image_info = SkImageInfo::Make(
        SkISize::Make(backing->GetSize().x, backing->GetSize().y), color_info);
    sk_sp<SkData> data = SkData::MakeWithoutCopy(backing->Bytes().data(),
                                                 backing->Bytes().size());
    sk_image_ =
        SkImages::RasterFromData(image_info, data, backing->GetSize().x * 4);
    backing->sk_image_id_ = sk_image_->uniqueID();
    return sk_image_;
  }

  bool EndRead() override { return true; }

  void ConsumeFence(std::unique_ptr<clay::FenceSync> fence_sync) override {
    if (!fence_sync) {
      return;
    }
    fence_sync->ClientWait();
  }

  std::unique_ptr<clay::FenceSync> ProduceFence() override {
    return std::make_unique<MockFenceSync>();
  }

 private:
  sk_sp<SkImage> sk_image_;
};

MockSharedImageBacking::MockSharedImageBacking(skity::Vec2 size)
    : SharedImageBacking(PixelFormat::kNative8888, size) {
  bytes_.resize(size.x * size.y * 4);
}

MockSharedImageBacking::~MockSharedImageBacking() = default;

clay::SharedImageBacking::BackingType MockSharedImageBacking::GetType() const {
  return BackingType::kMocking;
}

clay::GraphicsMemoryHandle MockSharedImageBacking::GetGFXHandle() const {
  return const_cast<uint8_t*>(bytes_.data());
}

fml::RefPtr<clay::SharedImageRepresentation>
MockSharedImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig*) {
  return fml::MakeRefCounted<MockSkiaImageRepresentation>(fml::Ref(this));
}

fml::RefPtr<clay::SkiaImageRepresentation>
MockSharedImageBacking::CreateSkiaRepresentation(GrDirectContext* gr_context) {
  return fml::MakeRefCounted<MockSkiaImageRepresentation>(fml::Ref(this));
}

}  // namespace testing
}  // namespace clay
