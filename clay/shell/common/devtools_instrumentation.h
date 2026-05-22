// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DEVTOOLS_INSTRUMENTATION_H_
#define CLAY_SHELL_COMMON_DEVTOOLS_INSTRUMENTATION_H_

#include <cstddef>
#include <string>
#include <vector>

#include "clay/gfx/rendering_backend.h"
#include "clay/shell/common/platform_view.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/rapidjson.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace clay {

class DevtoolsInstrumentation {
 public:
  explicit DevtoolsInstrumentation(clay::PlatformView* view_holder)
      : view_holder_(view_holder) {}

  virtual ~DevtoolsInstrumentation() = default;

  void ResetRasterDocument() { raster_document_.RemoveAllMembers(); }

  std::string CreateJsonStringResult();

  void UpdateRasterCacheInfo(const int64_t& single_cache_size,
                             const int64_t& single_cache_height,
                             const int64_t& single_cache_width,
                             const std::string& single_cache_color_type,
                             const int64_t& layer_address,
                             const int64_t& cache_address, GrImagePtr image);

  void UpdateRasterInfoToDevtool();

 protected:
#ifndef ENABLE_SKITY
  sk_sp<SkData> CompressImageData(const SkImage* image);
#endif  // ENABLE_SKITY

 private:
  rapidjson::Document raster_document_ =
      rapidjson::Document(rapidjson::kObjectType);
  clay::PlatformView* view_holder_;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_DEVTOOLS_INSTRUMENTATION_H_
