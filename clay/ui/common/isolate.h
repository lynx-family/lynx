// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_ISOLATE_H_
#define CLAY_UI_COMMON_ISOLATE_H_

#include <map>
#include <memory>
#include <utility>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"
#include "clay/gfx/graphics_isolate.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/resource/gpu_resource_cache.h"

namespace txt {
class FontCollection;
}

namespace clay {

class FontCollection;
class ViewContext;

// Singleton to contain infras will be accessed by clay.
class Isolate : public GraphicsDelegate {
 public:
  static Isolate& Instance();

  Isolate();
  ~Isolate();

  std::shared_ptr<clay::FontCollection> GetFontCollection();
  std::shared_ptr<txt::FontCollection> GetTxtFontCollection();

  void SetPlatformTaskRunner(fml::RefPtr<fml::TaskRunner> runner) {
    platform_task_runner_ = std::move(runner);
  }

  fml::RefPtr<fml::TaskRunner> GetPlatformTaskRunner() {
    return platform_task_runner_;
  }

  void SetIOTaskRunner(fml::RefPtr<fml::TaskRunner> runner) {
    io_task_runner_ = runner;
  }

  fml::RefPtr<fml::TaskRunner> GetIOTaskRunner() { return io_task_runner_; }

  // The task runner whose tasks may be executed concurrently on a pool of
  // worker threads. All subsystems within a running shell instance use this
  // worker pool for their concurrent tasks. This also means that the concurrent
  // worker pool may service tasks from multiple shell instances.
  // The task runner for the concurrent worker thread pool.
  std::shared_ptr<fml::ConcurrentTaskRunner> GetConcurrentWorkerTaskRunner()
      const override;

  // The concurrent message loop hosts threads that are used by the engine to
  // perform tasks long running background tasks. Typically, to post tasks to
  // this message loop, the GetConcurrentWorkerTaskRunner` method may be used.
  std::shared_ptr<fml::ConcurrentMessageLoop> GetConcurrentMessageLoop();

  GpuResourceCache* GetResourceCache();

  void RegisterViewContext(uint32_t id, fml::WeakPtr<ViewContext> context) {
    view_contexts_.emplace(id, context);
  }

  void UnregisterViewContext(uint32_t id) { view_contexts_.erase(id); }

  fml::WeakPtr<ViewContext> GetViewContextById(uint32_t id) {
    auto iter = view_contexts_.find(id);
    if (iter != view_contexts_.end()) {
      return iter->second;
    }
    return fml::WeakPtr<ViewContext>();
  }

  void CacheStoreImage(fml::RefPtr<SkImageHolder> img) override {
    GetResourceCache()->StoreImage(img);
  }
  void CacheRemoveImage(fml::RefPtr<SkImageHolder> img) override {
    GetResourceCache()->RemoveImage(img);
  }

  void UpdateResourceCacheMaxMemoryLimit(int limit, int low_end_limit) {
    GetResourceCache()->UpdateResourceCacheMaxMemoryLimit(limit, low_end_limit);
  }

  void EnablePartialRepaint(bool enable_partial_repaint) {
    enable_partial_repaint_ = enable_partial_repaint;
  }
  bool IsPartialRepaintEnabled() const { return enable_partial_repaint_; }

 private:
  std::shared_ptr<fml::ConcurrentMessageLoop> concurrent_message_loop_;
  std::unique_ptr<GpuResourceCache> resource_cache_;
#ifndef ENABLE_SKITY
  clay::SkiaConcurrentExecutor skia_concurrent_executor_;
#endif  // ENABLE_SKITY
  fml::RefPtr<fml::TaskRunner> platform_task_runner_;
  fml::RefPtr<fml::TaskRunner> io_task_runner_;
  std::map<uint32_t, fml::WeakPtr<ViewContext>> view_contexts_;
  bool enable_partial_repaint_ = true;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_ISOLATE_H_
