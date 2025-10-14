// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GRAPHICS_ISOLATE_H_
#define CLAY_GFX_GRAPHICS_ISOLATE_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gpu_object.h"
#include "clay/gfx/image/image_decoder.h"
#include "clay/gfx/image/image_descriptor.h"
#include "clay/gfx/image/skimage_holder.h"

namespace clay {

class GraphicsDelegate {
 public:
  virtual void CacheStoreImage(fml::RefPtr<SkImageHolder> img) = 0;
  virtual void CacheRemoveImage(fml::RefPtr<SkImageHolder> img) = 0;

  virtual std::shared_ptr<fml::ConcurrentTaskRunner>
  GetConcurrentWorkerTaskRunner() const = 0;
};

class GraphicsIsolate final : public GraphicsDelegate {
 public:
  static GraphicsIsolate& Instance();

  void SetDelegate(GraphicsDelegate* delegate) { delegate_ = delegate; }

  void CacheStoreImage(fml::RefPtr<SkImageHolder> img) override {
    FML_DCHECK(delegate_);
    delegate_->CacheStoreImage(img);
  }
  void CacheRemoveImage(fml::RefPtr<SkImageHolder> img) override {
    FML_DCHECK(delegate_);
    delegate_->CacheRemoveImage(img);
  }
  std::shared_ptr<fml::ConcurrentTaskRunner> GetConcurrentWorkerTaskRunner()
      const override {
    FML_DCHECK(delegate_);
    return delegate_->GetConcurrentWorkerTaskRunner();
  }

  fml::RefPtr<GPUUnrefQueue> GetOrCreateUnrefQueue(
      fml::RefPtr<fml::TaskRunner> task_runner);

  void RemoveUnrefQueue(fml::RefPtr<fml::TaskRunner> task_runner);

  std::shared_ptr<ImageDecoder> GetImageDecoder();

 private:
  GraphicsDelegate* delegate_ = nullptr;
  std::shared_ptr<ImageDecoder> image_decoder_;
  std::unordered_map<int, fml::RefPtr<GPUUnrefQueue>> unref_queue_map_;
};

}  // namespace clay

#endif  // CLAY_GFX_GRAPHICS_ISOLATE_H_
