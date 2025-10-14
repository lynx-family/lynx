// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_EXAMPLE_GLFW_PLATFORM_VIEW_IMPL_H_
#define CLAY_EXAMPLE_GLFW_PLATFORM_VIEW_IMPL_H_

#include <memory>

#include "build/build_config.h"
#include "clay/common/service/service_manager.h"
#include "clay/common/thread_host.h"
#include "clay/public/clay.h"
#include "clay/shell/common/output_surface.h"
#include "clay/shell/common/platform_view.h"
#ifdef SHELL_ENABLE_GL
#include "clay/shell/platform/glfw/surface_gl_impl.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"
#include "third_party/skia/src/gpu/gl/GrGLDefines.h"  // nogncheck
#endif

namespace clay {
namespace example {

class PlatformViewImpl final : public clay::PlatformView {
 public:
  PlatformViewImpl(std::shared_ptr<clay::ServiceManager> service_manager,
                   clay::PlatformView::Delegate& delegate,
                   const clay::TaskRunners& task_runners,
                   SurfaceDelegate* surface_delegate)
      : clay::PlatformView(service_manager, delegate, task_runners),
        surface_(fml::MakeRefCounted<SurfaceGLImpl>(surface_delegate)) {}

  ~PlatformViewImpl() override {}

 private:
  // |PlatformView|
  fml::RefPtr<clay::OutputSurface> GetOutputSurface() const override {
    return surface_;
  }

  // |PlatformView|
  void NotifyDestroyed() override {
    clay::PlatformView::NotifyDestroyed();
    fml::AutoResetWaitableEvent latch;
    fml::TaskRunner::RunNowOrPostTask(
        task_runners_.GetRasterTaskRunner(),
        [&latch, surface = surface_.get()]() {
          auto gr_context =
              static_cast<clay::OutputSurface*>(surface)->GetMainGrContext();
          if (gr_context) {
            bool has_released = false;
#ifdef SHELL_ENABLE_GL
            if (gr_context->backend() == GrBackendApi::kOpenGL) {
              auto surface_gl = static_cast<SurfaceGLImpl*>(surface);
              auto status = static_cast<clay::GPUSurfaceGLDelegate*>(surface_gl)
                                ->GLContextMakeCurrent();
              if (status->GetResult()) {
                gr_context->releaseResourcesAndAbandonContext();
                static_cast<clay::GPUSurfaceGLDelegate*>(surface_gl)
                    ->GLContextClearCurrent();
                has_released = true;
              }
            }
#endif
            if (!has_released) {
              gr_context->releaseResourcesAndAbandonContext();
            }
          }
          latch.Signal();
        });
    latch.Wait();
  }

  fml::RefPtr<SurfaceGLImpl> surface_;
};

}  // namespace example
}  // namespace clay

#endif  // CLAY_EXAMPLE_GLFW_PLATFORM_VIEW_IMPL_H_
