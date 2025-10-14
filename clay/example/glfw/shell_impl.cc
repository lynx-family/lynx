// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/example/glfw/shell_impl.h"

#include <string>
#include <utility>

#include "clay/common/settings.h"
#include "clay/example/glfw/clay_logo.h"
#include "clay/example/glfw/platform_view_impl.h"
#include "clay/gfx/style/color.h"
#include "clay/shell/common/pointer_data_dispatcher.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/image_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/internal_text_view.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/window/pointer_data_helper.h"

namespace clay {
namespace example {

namespace {

static std::unique_ptr<clay::Rasterizer> CreateRasterizer(
    std::shared_ptr<clay::ServiceManager> service_manager) {
  return std::make_unique<clay::Rasterizer>(service_manager);
};

clay::ImageView* CreateLogo(int left, int top, int width, int height,
                            clay::PageView* page_view) {
  clay::ImageView* image_view = new clay::ImageView(-1, page_view);
  image_view->SetX(left);
  image_view->SetY(top);
  image_view->SetWidth(width);
  image_view->SetHeight(height);
  image_view->SetSource(kClayLogo);
  return image_view;
}

clay::InternalTextView* CreateText(const std::string& text, int left, int top,
                                   int width, int height,
                                   clay::PageView* page_view) {
  auto* text_view = new clay::InternalTextView(-1, page_view);
  text_view->SetX(left);
  text_view->SetY(top);
  text_view->SetText(text);
  text_view->SetFontSize(page_view->FromLogical(40.0f));
  MeasureResult result;
  text_view->Measure(
      {page_view->FromLogical(300), clay::TextMeasureMode::kAtMost,
       page_view->FromLogical(40), clay::TextMeasureMode::kAtMost},
      result);
  text_view->SetPaddings(9.5f, 20.0f, 9.5f, 20.0f);
  text_view->SetContentWidth(result.width);
  text_view->SetContentHeight(result.height);

  return text_view;
}

void DrawTextAndImage(clay::PageView* page_view) {
  clay::Color color = clay::Color::kWhite();
  page_view->SetBackgroundColor(color);
  page_view->AddChild(CreateLogo(400, 120, 202, 202, page_view));
  page_view->AddChild(CreateText("Hello Clay", 400, 300, 400, 100, page_view));
  page_view->RequestPaintBase();
}

}  // namespace

ShellImpl::ShellImpl(const char* icu_data_path, SurfaceDelegate* delegate,
                     ClayTaskRunnerDescription* platform_task_runner_desc)
    : thread_host_(
          ThreadHostHolder::CreateThreadHostHolder(platform_task_runner_desc)),
      task_runners_(thread_host_->GetTaskRunners()) {
  service_manager_ = clay::ServiceManager::Create(
      {task_runners_.GetPlatformTaskRunner(), task_runners_.GetUITaskRunner(),
       task_runners_.GetRasterTaskRunner(), task_runners_.GetIOTaskRunner()});
  clay::Shell::CreateCallback<clay::PlatformView> on_create_platform_view =
      [=](std::shared_ptr<clay::ServiceManager> service_manager,
          clay::Shell& shell) {
        return std::make_unique<PlatformViewImpl>(
            service_manager,
            shell,                   // delegate
            shell.GetTaskRunners(),  // task runners
            delegate);
      };
  clay::Shell::CreateCallbackFnPtr<clay::Rasterizer> on_create_rasterizer =
      &CreateRasterizer;
  clay::Settings settings;
  settings.icu_data_path = icu_data_path;
  shell_ = Shell::Create(service_manager_, task_runners_, settings,
                         on_create_platform_view, on_create_rasterizer);
  auto platform_view = shell_->GetPlatformView();
  if (platform_view) {
    platform_view->NotifyCreated();
  }

  auto view_context = shell_->GetEngine()->GetViewContext();
  if (view_context) {
    view_context->OnFirstMeaningfulLayout();
  }
}

ShellImpl::~ShellImpl() {
  auto platform_view = shell_->GetPlatformView();
  if (platform_view) {
    platform_view->NotifyDestroyed();
  }
  shell_.reset();
}

void ShellImpl::DrawUI() {
  auto view_context = shell_->GetEngine()->GetViewContext();
  if (view_context) {
    DrawTextAndImage(view_context->GetPageView());
  }
}

bool ShellImpl::RunTask(const ClayTask* task) {
  if (task == nullptr) {
    return false;
  }
  return thread_host_->PostTask(task->task);
}

void ShellImpl::SendViewportMetrics(int32_t width, int32_t height,
                                    double pixel_ratio) {
  if (shell_) {
    clay::ViewportMetrics metrics;
    metrics.physical_width = width;
    metrics.physical_height = height;
    metrics.device_pixel_ratio = pixel_ratio;
    // Default logical pixel is 96
    metrics.device_density_dpi = metrics.device_pixel_ratio * 96.f;
    metrics.physical_view_inset_top = 0.0;
    metrics.physical_view_inset_right = 0.0;
    metrics.physical_view_inset_bottom = 0.0;
    metrics.physical_view_inset_left = 0.0;
    shell_->GetPlatformView()->SetViewportMetrics(metrics);
  }
}

void ShellImpl::SendPointerEvents(const ClayPointerEvent* events,
                                  size_t events_count) {
  if (events == nullptr || events_count == 0 || !shell_) {
    return;
  }
  auto packet = std::make_unique<clay::PointerDataPacket>(events_count);
  const ClayPointerEvent* current = events;
  for (size_t i = 0; i < events_count; ++i) {
    clay::PointerData pointer_data;
    pointer_data.Clear();
    pointer_data.embedder_id = 0;
    pointer_data.time_stamp = current->timestamp;
    pointer_data.change =
        PointerDataHelper::ToPointerDataChange(current->phase);
    pointer_data.physical_x = current->x;
    pointer_data.physical_y = current->y;
    pointer_data.physical_delta_x = 0.0;
    pointer_data.physical_delta_y = 0.0;
    pointer_data.device = current->device;
    pointer_data.pointer_identifier = 0;
    pointer_data.signal_kind =
        PointerDataHelper::ToPointerDataSignalKind(current->signal_kind);
    pointer_data.scroll_delta_x = current->scroll_delta_x;
    pointer_data.scroll_delta_y = current->scroll_delta_y;
    ClayPointerDeviceKind device_kind = current->device_kind;
    if (device_kind == 0) {
      pointer_data.kind = clay::PointerData::DeviceKind::kMouse;
      pointer_data.buttons =
          PointerDataHelper::PointerDataButtonsForLegacyEvent(
              pointer_data.change);

    } else {
      pointer_data.kind = PointerDataHelper::ToPointerDataKind(device_kind);
      if (pointer_data.kind == clay::PointerData::DeviceKind::kTouch) {
        if (pointer_data.change == clay::PointerData::Change::kDown ||
            pointer_data.change == clay::PointerData::Change::kMove) {
          pointer_data.buttons = clay::kPointerButtonTouchContact;
        }
      } else {
        pointer_data.buttons = current->buttons;
      }
    }
    pointer_data.pan_x = 0.0;
    pointer_data.pan_y = 0.0;
    pointer_data.pan_delta_x = 0.0;
    pointer_data.pan_delta_y = 0.0;
    pointer_data.scale = 0.0;
    pointer_data.rotation = 0.0;
    pointer_data.is_precise_scroll = true;
    packet->SetPointerData(i, pointer_data);
    current = reinterpret_cast<const ClayPointerEvent*>(
        reinterpret_cast<const uint8_t*>(current) + current->struct_size);
  }
  shell_->GetPlatformView()->DispatchPointerDataPacket(std::move(packet));
}

}  // namespace example
}  // namespace clay
