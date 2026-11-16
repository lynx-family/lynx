// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_RESOURCE_TYPES_H_
#define CLAY_GFX_SHARED_IMAGE_RESOURCE_TYPES_H_

#include <cstdint>
#include <tuple>

namespace clay::display {
// Independent read, latest pending and write storage at both handoffs. Slots
// rotate only after their CPU leases and all GPU uses have been released.
inline constexpr uint32_t kMaxRasterBuffersPerSegment = 3;
inline constexpr uint32_t kMaxDisplayOutputBuffers = 3;
inline constexpr uint32_t kMaxDisplayFramesInFlight = 2;
struct ResourceKey {
  uint64_t device_id = 0, device_generation = 0, resource_id = 0,
           allocation_generation = 0;
  bool operator==(const ResourceKey& b) const {
    return std::tie(device_id, device_generation, resource_id,
                    allocation_generation) ==
           std::tie(b.device_id, b.device_generation, b.resource_id,
                    b.allocation_generation);
  }
};
enum class PixelFormat { kRGBA8888, kBGRA8888 };
enum class AlphaMode { kPremultiplied };
enum class ColorSpace { kSRGB };
enum class ImageOrigin { kTopLeft, kBottomLeft };
struct PixelRect {
  int32_t x = 0, y = 0;
  uint32_t width = 0, height = 0;
  bool operator==(const PixelRect& b) const {
    return std::tie(x, y, width, height) ==
           std::tie(b.x, b.y, b.width, b.height);
  }
};
struct ImageSpec {
  uint32_t width = 0, height = 0;
  PixelFormat format = PixelFormat::kRGBA8888;
  AlphaMode alpha = AlphaMode::kPremultiplied;
  ColorSpace color = ColorSpace::kSRGB;
  ImageOrigin origin = ImageOrigin::kTopLeft;
  // Source rectangle in physical texture coordinates, before origin correction.
  PixelRect valid_rect;
};
}  // namespace clay::display
#endif  // CLAY_GFX_SHARED_IMAGE_RESOURCE_TYPES_H_
