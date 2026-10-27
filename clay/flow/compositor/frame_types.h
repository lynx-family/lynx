// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_FRAME_TYPES_H_
#define CLAY_FLOW_COMPOSITOR_FRAME_TYPES_H_
#include <cmath>
#include <cstdint>
#include <tuple>
namespace clay::display {
// Independent read, latest pending and write storage at both handoffs. Slots
// rotate only after their CPU leases and all GPU uses have been released.
inline constexpr uint32_t kMaxRasterBuffersPerSegment = 3;
inline constexpr uint32_t kMaxDisplayOutputBuffers = 3;
inline constexpr uint32_t kMaxDisplayFramesInFlight = 2;
struct RasterBindingKey {
  uint64_t tree_id = 0, tree_generation = 0, group_generation = 0,
           binding_generation = 0;
  uint32_t worker_id = 0;
  bool operator==(const RasterBindingKey& b) const {
    return std::tie(tree_id, tree_generation, group_generation,
                    binding_generation, worker_id) ==
           std::tie(b.tree_id, b.tree_generation, b.group_generation,
                    b.binding_generation, b.worker_id);
  }
  bool operator!=(const RasterBindingKey& b) const { return !(*this == b); }
};
struct WindowKey {
  uint64_t id = 0, generation = 0;
  bool operator==(const WindowKey& b) const {
    return id == b.id && generation == b.generation;
  }
  bool operator!=(const WindowKey& b) const { return !(*this == b); }
};
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
  PixelRect valid_rect;
};
struct OutputConfig {
  uint32_t width = 0, height = 0;
  double device_pixel_ratio = 1;
  PixelFormat format = PixelFormat::kRGBA8888;
  AlphaMode alpha = AlphaMode::kPremultiplied;
  ColorSpace color = ColorSpace::kSRGB;
  uint64_t device_id = 0, device_generation = 0, version = 0;
  // Snapshotted on Platform; Raster must not read mutable host surface state.
  bool uses_window_attachments = true;
  bool IsValid() const {
    return width && height && width <= 16384 && height <= 16384 &&
           std::isfinite(device_pixel_ratio) && device_pixel_ratio > 0 &&
           device_id && device_generation && version;
  }
  bool SameContentConfig(const OutputConfig& b) const {
    return std::tie(width, height, device_pixel_ratio, format, alpha, color,
                    device_id, device_generation, uses_window_attachments) ==
           std::tie(b.width, b.height, b.device_pixel_ratio, b.format, b.alpha,
                    b.color, b.device_id, b.device_generation,
                    b.uses_window_attachments);
  }
  bool operator==(const OutputConfig& b) const {
    return version == b.version && SameContentConfig(b);
  }
  bool operator!=(const OutputConfig& b) const { return !(*this == b); }
};
struct CompositionProperties {
  // All ancestor effects are baked by Raster. Display restores pixel position.
  int32_t x = 0, y = 0;
};
struct MediaTiming {
  uint64_t source_generation = 0;
  int64_t presentation_time_ns = 0;
  bool clock_mapped = false;
};
struct TimingCorrelation {
  uint64_t tree_submission_id = 0, render_id = 0;
  int64_t producer_start_ns = 0, producer_submit_ns = 0;
};
}  // namespace clay::display
#endif  // CLAY_FLOW_COMPOSITOR_FRAME_TYPES_H_
