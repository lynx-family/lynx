// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_PRODUCE_CONTEXT_H_
#define CLAY_GFX_IMAGE_IMAGE_PRODUCE_CONTEXT_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "clay/gfx/gpu_object.h"

namespace clay {

// Produce context for construct a SkImage and potential texture uploading.
struct ImageProduceContext {
  bool use_promise = false;
  bool is_deferred = false;
  bool decode_with_priority = false;
  std::function<void(bool)> decode_callback;
  std::function<void(bool)> upload_callback;
  std::function<void(const std::function<void()>&)> register_upload_callback;
  fml::RefPtr<GPUUnrefQueue> unref_queue;
  fml::RefPtr<fml::TaskRunner> ui_task_runner;
  fml::RefPtr<fml::TaskRunner> raster_task_runner;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_PRODUCE_CONTEXT_H_
