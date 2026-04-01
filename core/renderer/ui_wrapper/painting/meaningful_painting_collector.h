// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_MEANINGFUL_PAINTING_COLLECTOR_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_MEANINGFUL_PAINTING_COLLECTOR_H_

#include <cstddef>
#include <cstdint>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/vector.h"
#include "core/public/platform_renderer_type.h"

namespace lynx::tasm {

class PlatformRendererImpl;

constexpr size_t kMeaningfulPaintingAreaSerializedFieldCount = 6;

struct MeaningfulPaintingAreaRecord {
  int32_t sign = 0;
  PlatformRendererType type = PlatformRendererType::kUnknown;
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;
};

class MeaningfulPaintingCollector {
 public:
  explicit MeaningfulPaintingCollector(
      const fml::RefPtr<PlatformRendererImpl>& root_renderer);

  // Reconstruct meaningful text/image geometry from platform renderer display
  // lists. The returned records intentionally contain geometry only so each
  // platform can resolve presented/pending state from its own runtime objects.
  base::Vector<MeaningfulPaintingAreaRecord> Collect() const;

  static base::Vector<int32_t> Serialize(
      const base::Vector<MeaningfulPaintingAreaRecord>& records);

 private:
  void CollectFromRenderer(
      const fml::RefPtr<PlatformRendererImpl>& renderer, float parent_offset_x,
      float parent_offset_y,
      base::Vector<MeaningfulPaintingAreaRecord>* out_records) const;

  fml::RefPtr<PlatformRendererImpl> root_renderer_;
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_MEANINGFUL_PAINTING_COLLECTOR_H_
