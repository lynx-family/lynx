// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/graphics_isolate.h"

#include "base/include/no_destructor.h"

namespace clay {

GraphicsIsolate& GraphicsIsolate::Instance() {
  static fml::NoDestructor<GraphicsIsolate> instance;
  return *(instance.get());
}

fml::RefPtr<GPUUnrefQueue> GraphicsIsolate::GetOrCreateUnrefQueue(
    fml::RefPtr<fml::TaskRunner> task_runner) {
  auto gpu_queue_id = task_runner->GetTaskQueueId();
  auto iter = unref_queue_map_.find(gpu_queue_id);
  if (iter != unref_queue_map_.end()) {
    return iter->second;
  }
  // On Windows or Mac, each page is represented by a separate window, with
  // each window running its own raster thread. Each raster thread, in turn
  // is bound to a dedicated GPU queue, so the GPUUnRefQueue is not shared.
#if OS_WIN || OS_MAC
  auto unref_queue = fml::MakeRefCounted<GPUUnrefQueue>(task_runner, false);
#else
  auto unref_queue = fml::MakeRefCounted<GPUUnrefQueue>(task_runner);
#endif

  unref_queue_map_.emplace(gpu_queue_id, unref_queue);
  return unref_queue;
}

void GraphicsIsolate::RemoveUnrefQueue(
    fml::RefPtr<fml::TaskRunner> task_runner) {
  auto gpu_queue_id = task_runner->GetTaskQueueId();
  auto iter = unref_queue_map_.find(gpu_queue_id);
  if (iter != unref_queue_map_.end()) {
    unref_queue_map_.erase(iter);
  }
}

std::shared_ptr<ImageDecoder> GraphicsIsolate::GetImageDecoder() {
  if (!image_decoder_) {
    image_decoder_ =
        std::make_shared<ImageDecoder>(GetConcurrentWorkerTaskRunner());
  }
  return image_decoder_;
}

}  // namespace clay
