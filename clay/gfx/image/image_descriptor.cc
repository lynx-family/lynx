// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/image/image_descriptor.h"

#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/trace/native/trace_event.h"
#include "build/build_config.h"
#include "clay/fml/logging.h"
#include "clay/fml/mapping.h"
#include "clay/gfx/graphics_isolate.h"

#ifdef ENABLE_TT_HEIF_DECODER
#include "clay/gfx/image/heif_decoder.h"
#endif

namespace clay {

ImageDescriptor::ImageDescriptor(GrDataPtr buffer)
    : buffer_(std::move(buffer)), mutex_(fml::SharedMutex::Create()) {}

ImageDescriptor::ImageDescriptor(GrDataPtr buffer,
                                 std::optional<size_t> row_bytes)
    : buffer_(std::move(buffer)),
      row_bytes_(row_bytes),
      mutex_(fml::SharedMutex::Create()) {}

}  // namespace clay
