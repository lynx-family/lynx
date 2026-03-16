// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/windowless/lynx_ui_renderer_windowless.h"

#include <cassert>
#include <memory>
#include <utility>

#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_view_service_desktop.h"
#include "clay/lynx_adaptor/resource_loader_embedder.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/ui/component/view_context.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace embedder {

std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWindowlessUIRenderer(
    lynx_view_builder_t* builder) {
  return std::make_unique<LynxUIRendererWindowless>(builder);
}

LynxUIRendererWindowless::LynxUIRendererWindowless(lynx_view_builder_t* builder)
    : LynxUIRenderer(builder) {
#if !defined(OS_WIN)
  base::UIThread::Init();
#endif
  windowless_renderer_ = builder->windowless_renderer;
  windowless_renderer_->run_task = [this](lynx_task_t task) {
    ClayTask clay_task{};
    clay_task.task = task.task;
    clay_task.runner = reinterpret_cast<ClayTaskRunner>(task.runner);
    headless_engine_->RunTask(&clay_task);
  };
  windowless_renderer_->send_pointer_event =
      [this](lynx_pointer_event_t* event) {
        static_assert(sizeof(lynx_pointer_event_t) == sizeof(ClayPointerEvent),
                      "pointer event size not match");
        headless_engine_->SendPointerEvents(
            reinterpret_cast<ClayPointerEvent*>(event), 1);
      };
  windowless_renderer_->send_key_event = [this](lynx_key_event_t* event) {
    static_assert(sizeof(lynx_key_event_t) == sizeof(ClayKeyEvent),
                  "Key event size not match");
    headless_engine_->SendKeyEvent(reinterpret_cast<ClayKeyEvent*>(event),
                                   nullptr, nullptr);
  };
  main_thread_id_ = std::this_thread::get_id();
  static size_t sTaskRunnerIdentifiers = 0;
  description_.struct_size = sizeof(ClayTaskRunnerDescription);
  description_.user_data = this;
  description_.runs_task_on_current_thread_callback =
      [](void* user_data) -> bool {
    return reinterpret_cast<LynxUIRendererWindowless*>(user_data)
        ->RunsTaskOnCurrentThread();
  };
  description_.post_task_callback = [](ClayTask task, uint64_t target_time,
                                       void* user_data) {
    reinterpret_cast<LynxUIRendererWindowless*>(user_data)->PostTask(
        task, target_time);
  };
  description_.identifier = ++sTaskRunnerIdentifiers;

  const char* icu_data_path = builder->GetICUDataPath();
  if (icu_data_path == nullptr || icu_data_path[0] == '\0') {
#if OS_WIN
    icu_data_path = "data\\icudtl.dat";
#elif OS_MAC
    icu_data_path = "../Resources/data/icudtl.dat";
#else
    // other platforms not supported yet.
#endif
  }

  headless_engine_ = std::make_unique<clay::ClayHeadlessEngine>(
      icu_data_path, windowless_renderer_->GetRendererConfig(), &description_,
      windowless_renderer_.get());
  headless_engine_->SetHeadlessDelegate(this);

  auto* view_context =
      static_cast<clay::ViewContext*>(headless_engine_->GetViewContext());
  auto module_factory = std::unique_ptr<lynx::runtime::NativeModuleFactory>(
      lynx::LynxModuleFactory::CreateModuleFactory(view_context));
  ui_delegate_ = std::make_unique<lynx::tasm::UIDelegateClay>(
      view_context, std::move(module_factory));

  clay::NativeViewServiceDesktop::SetViewFactories(
      view_context, std::move(builder->native_view_creators));

  if (builder->generic_fetcher) {
    auto fetcher_holder =
        std::make_shared<LynxResourceFetcherHolder>(builder->generic_fetcher);
    headless_engine_->GetServiceManager()
        ->RegisterService<clay::ResourceLoaderCreatorService>(
            std::make_shared<clay::ResourceLoaderCreatorService>(
                [fetcher_holder](
                    fml::RefPtr<fml::TaskRunner> task_runner,
                    std::shared_ptr<clay::ResourceLoaderIntercept> intercept) {
                  return std::make_shared<clay::ResourceLoaderEmbedder>(
                      task_runner, intercept, fetcher_holder);
                }));
  }
  headless_engine_->SendViewportMetrics(
      builder->frame.width * builder->screen_size.pixel_ratio,
      builder->frame.height * builder->screen_size.pixel_ratio,
      builder->screen_size.pixel_ratio);
}

LynxUIRendererWindowless::~LynxUIRendererWindowless() {
  headless_engine_->SetHeadlessDelegate(nullptr);
  windowless_renderer_->run_task = nullptr;
  windowless_renderer_->send_pointer_event = nullptr;
  windowless_renderer_->send_key_event = nullptr;
}

bool LynxUIRendererWindowless::RunsTaskOnCurrentThread() const {
  return std::this_thread::get_id() == main_thread_id_;
}

void LynxUIRendererWindowless::PostTask(ClayTask task, uint64_t target_time) {
  lynx_task_t lynx_task{};
  lynx_task.task = task.task;
  lynx_task.runner = reinterpret_cast<lynx_task_runner>(task.runner);

  // In nanoseconds.
  uint64_t interval = 0;
  uint64_t engine_time = clay::ClayHeadlessEngine::GetEngineCurrentTime();
  if (target_time > engine_time) {
    interval = target_time - engine_time;
  }
  if (windowless_renderer_) {
    windowless_renderer_->PostTask(lynx_task, interval);
  }
}

void LynxUIRendererWindowless::SetPixelRatio(float pixel_ratio) {
  if (pixel_ratio_ != pixel_ratio) {
    pixel_ratio_ = pixel_ratio;
    headless_engine_->SendViewportMetrics(width_ * pixel_ratio_,
                                          height_ * pixel_ratio_, pixel_ratio_);
  }
}
void LynxUIRendererWindowless::SetFrame(float x, float y, float width,
                                        float height) {
  if (width_ != width || height_ != height) {
    width_ = width;
    height_ = height;
    headless_engine_->SendViewportMetrics(width_ * pixel_ratio_,
                                          height_ * pixel_ratio_, pixel_ratio_);
  }
}

void LynxUIRendererWindowless::OnEnterForeground() {
  headless_engine_->OnEnterForeground();
}
void LynxUIRendererWindowless::OnEnterBackground() {
  headless_engine_->OnEnterBackground();
}

lynx::tasm::UIDelegate* LynxUIRendererWindowless::GetUIDelegate() {
  return ui_delegate_.get();
}

const char* LynxUIRendererWindowless::GetClipboardData() const {
  if (windowless_renderer_ && windowless_renderer_->get_clipboard_data) {
    return windowless_renderer_->get_clipboard_data(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()));
  }
  return "";
}
void LynxUIRendererWindowless::SetClipboardData(const char* data) {
  if (windowless_renderer_ && windowless_renderer_->set_clipboard_data) {
    windowless_renderer_->set_clipboard_data(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        data);
  }
}
void LynxUIRendererWindowless::ActivateSystemCursor(int type,
                                                    const char* path) {
  if (windowless_renderer_ && windowless_renderer_->activate_system_cursor) {
    windowless_renderer_->activate_system_cursor(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        static_cast<lynx_cursor_type_e>(type), path);
  }
}
void LynxUIRendererWindowless::ShowTextInput() {
  if (windowless_renderer_ && windowless_renderer_->show_text_input) {
    windowless_renderer_->show_text_input(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        true);
  }
}
void LynxUIRendererWindowless::HideTextInput() {
  if (windowless_renderer_ && windowless_renderer_->show_text_input) {
    windowless_renderer_->show_text_input(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        false);
  }
}
void LynxUIRendererWindowless::SetMarkedTextRect(float x, float y, float width,
                                                 float height) {
  if (windowless_renderer_ && windowless_renderer_->set_marked_text_rect) {
    windowless_renderer_->set_marked_text_rect(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        x, y, width, height);
  }
}
void LynxUIRendererWindowless::SetEditableTransform(
    const float transform_matrix[16]) {
  if (windowless_renderer_ && windowless_renderer_->set_editable_transform) {
    windowless_renderer_->set_editable_transform(
        reinterpret_cast<lynx_windowless_renderer_t*>(
            windowless_renderer_.get()),
        transform_matrix);
  }
}

}  // namespace embedder
}  // namespace lynx
