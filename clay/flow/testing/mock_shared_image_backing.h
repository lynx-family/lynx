// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_TESTING_MOCK_SHARED_IMAGE_BACKING_H_
#define CLAY_FLOW_TESTING_MOCK_SHARED_IMAGE_BACKING_H_

#include <vector>

#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {
namespace testing {

class MockSharedImageBacking final : public clay::SharedImageBacking {
 public:
  explicit MockSharedImageBacking(skity::Vec2 size);
  ~MockSharedImageBacking() override;

  BackingType GetType() const override;
  clay::GraphicsMemoryHandle GetGFXHandle() const override;
  fml::RefPtr<clay::SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override;
  fml::RefPtr<clay::SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
  const std::vector<uint8_t>& Bytes() const { return bytes_; }
  uint32_t GetSkImageId() const { return sk_image_id_; }

 private:
  std::vector<uint8_t> bytes_;
  uint32_t sk_image_id_ = 0;

  friend class MockSkiaImageRepresentation;
};

}  // namespace testing
}  // namespace clay

#endif  // CLAY_FLOW_TESTING_MOCK_SHARED_IMAGE_BACKING_H_
