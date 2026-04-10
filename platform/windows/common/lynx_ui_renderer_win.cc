// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/windows/common/lynx_ui_renderer_win.h"

#include <Windows.h>

#include <cstdio>
#include <memory>
#include <string>
#include <utility>

#include "base/include/string/string_conversion_win.h"
#include "clay/common/service/service_manager.h"
#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_view_service_desktop.h"
#include "clay/lynx_adaptor/resource_loader_embedder.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/shell/common/services/instrumentation_service.h"
#include "clay/shell/platform/windows/dpi_utils.h"
#include "clay/ui/component/view_context.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"
#include "platform/embedder/public/capi/lynx_memory_capi.h"
#if ENABLE_TESTBENCH_REPLAY
#include <cmath>

#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
#endif

static constexpr UINT kRequestIME = WM_USER + 9;

namespace lynx {
namespace embedder {

std::unique_ptr<LynxUIRenderer> LynxUIRenderer::CreateWithBuilder(
    lynx_view_builder_t* builder) {
  return std::make_unique<LynxUIRendererWin>(builder);
}

LynxUIRendererWin::LynxUIRendererWin(lynx_view_builder_t* builder)
    : LynxUIRenderer(builder),
      frame_timing_listener_(std::make_shared<FrameTimingListenerImpl>()) {
  clay::FlutterDesktopEngineProperties properties = {};

  const char* icu_data_path = builder->GetICUDataPath();
  std::wstring icu_wstr;
  if (icu_data_path && icu_data_path[0] != '\0') {
    icu_wstr = base::Utf16FromUtf8(icu_data_path, strlen(icu_data_path));
  } else {
    icu_wstr = L"data\\icudtl.dat";
  }
  properties.icu_data_path = icu_wstr.c_str();

  HWND parent_hwnd = reinterpret_cast<HWND>(builder->parent);
  float pixel_ratio = builder->screen_size.pixel_ratio;
  if (pixel_ratio == 1.0) {
    pixel_ratio = clay::GetDpiForHWND(parent_hwnd) / 96.0;
  }

  std::unique_ptr<clay::WindowBindingHandler> window_wrapper =
      std::make_unique<clay::FlutterWindow>(
          parent_hwnd, builder->frame.x, builder->frame.y,
          builder->frame.width * pixel_ratio,
          builder->frame.height * pixel_ratio);
  flutter_view_ =
      std::make_unique<clay::FlutterWindowsView>(std::move(window_wrapper));

  clay::FlutterProjectBundle project_bundle(properties);
  // Take ownership of the engine, starting it if necessary.
  engine_ = std::make_unique<clay::FlutterWindowsEngine>(project_bundle);

  flutter_view_->SetEngine(engine_.get());
  flutter_view_->CreateRenderSurface();
  if (!flutter_view_->GetEngine()->running()) {
    if (!flutter_view_->GetEngine()->Run()) {
      NOTREACHED();
    }
  }

  // Must happen after engine is running.
  flutter_view_->SendInitialBounds();

  auto* view_context =
      static_cast<clay::ViewContext*>(engine_->GetViewContext());
  auto module_factory = std::unique_ptr<lynx::runtime::NativeModuleFactory>(
      lynx::LynxModuleFactory::CreateModuleFactory(view_context));
  ui_delegate_ = std::make_unique<lynx::tasm::UIDelegateClay>(
      view_context, std::move(module_factory));

  clay::NativeViewServiceDesktop::SetViewFactories(
      view_context, std::move(builder->native_view_creators));

  if (builder->generic_fetcher) {
    auto fetcher_holder =
        std::make_shared<LynxResourceFetcherHolder>(builder->generic_fetcher);
    auto intercept = engine_->GetResourceLoaderIntercept();
    if (intercept) {
      intercept->BindShouldInterceptUrlCallback(
          [fetcher_holder](const char* origin_url, bool should_decode,
                           char* intercept_url, int max_path_length) {
            char* redirected = lynx_generic_resource_fetcher_intercept(
                fetcher_holder->GenericFetcher(), origin_url, should_decode);
            if (!redirected || !intercept_url || max_path_length <= 0) {
              if (redirected) {
                lynx_free(redirected);
              }
              return;
            }
            std::snprintf(intercept_url, max_path_length, "%s", redirected);
            lynx_free(redirected);
          });
    }
    engine_->GetServiceManager()
        ->RegisterService<clay::ResourceLoaderCreatorService>(
            std::make_shared<clay::ResourceLoaderCreatorService>(
                [fetcher_holder](
                    fml::RefPtr<fml::TaskRunner> task_runner,
                    std::shared_ptr<clay::ResourceLoaderIntercept> intercept) {
                  return std::make_shared<clay::ResourceLoaderEmbedder>(
                      task_runner, intercept, fetcher_holder);
                }));
  }

  std::shared_ptr<clay::ServiceManager> service_manager =
      view_context->GetServiceManager();
  if (service_manager) {
    clay::Puppet<clay::Owner::kPlatform, clay::InstrumentationService>
        instrumentation_service =
            service_manager->GetService<clay::InstrumentationService>();
    instrumentation_service.Act(
        [frame_timing_listener = frame_timing_listener_](auto& impl) {
          impl.AddFrameTimingListener(frame_timing_listener);
        });
  }
}

LynxUIRendererWin::~LynxUIRendererWin() {
  frame_timing_listener_->RemoveAllClients();
}

void LynxUIRendererWin::SetParent(NativeWindow parent) {
  HWND native_window = std::get<HWND>(*flutter_view_->GetRenderTarget());
  HWND parent_hwnd = reinterpret_cast<HWND>(parent);
  if (!IsWindow(native_window) || !IsWindow(parent_hwnd)) {
    return;
  }
  ::SetParent(native_window, parent_hwnd);
}

NativeWindow LynxUIRendererWin::GetNativeWindow() {
  HWND native_window = std::get<HWND>(*flutter_view_->GetRenderTarget());
  return native_window;
}

void LynxUIRendererWin::SetPixelRatio(float pixel_ratio) {
  if (pixel_ratio_ != pixel_ratio) {
    pixel_ratio_ = pixel_ratio;
    AdjustWindowRect();
  }
}

void LynxUIRendererWin::SetFrame(float x, float y, float width, float height) {
  if (width_ != width || height_ != height) {
    width_ = width;
    height_ = height;
    AdjustWindowRect();
  }
}

void LynxUIRendererWin::Reset() {
  auto* view_context =
      static_cast<clay::ViewContext*>(engine_->GetViewContext());
  if (view_context) {
    view_context->ResetPageView();
  }
}

void LynxUIRendererWin::OnEnterForeground() { engine_->OnEnterForeground(); }

void LynxUIRendererWin::OnEnterBackground() { engine_->OnEnterBackground(); }

void LynxUIRendererWin::InjectBubbleEvent(const char* params) {
#if ENABLE_TESTBENCH_REPLAY
  rapidjson::Document dom;
  dom.Parse(params);
  if (dom.HasParseError()) {
    return;
  }
  std::string name;
  float pos_x = 0.0f;
  float pos_y = 0.0f;
  int delta_x = 0;
  int delta_y = 0;
  int delta = 0;
  if (dom.HasMember("name") && dom["name"].IsString()) {
    name = dom["name"].GetString();
  }
  if (dom.HasMember("params")) {
    if (dom["params"].HasMember("pageX") && dom["params"]["pageX"].IsNumber()) {
      pos_x = dom["params"]["pageX"].GetDouble();
    }
    if (dom["params"].HasMember("pageY") && dom["params"]["pageY"].IsNumber()) {
      pos_y = dom["params"]["pageY"].GetDouble();
    }
    if (dom["params"].HasMember("deltaX") && dom["params"]["deltaX"].IsInt()) {
      delta_x = dom["params"]["deltaX"].GetInt();
      if (delta_x != 0) {
        delta = delta_x;
      }
    }
    if (dom["params"].HasMember("deltaY") && dom["params"]["deltaY"].IsInt()) {
      delta_y = dom["params"]["deltaY"].GetInt();
      if (delta_y != 0) {
        delta = delta_y;
      }
    }
  }
  POINT point = {static_cast<LONG>(std::round(pos_x)),
                 static_cast<LONG>(std::round(pos_y))};
  HWND hwnd = reinterpret_cast<HWND>(GetNativeWindow());
  if (ClientToScreen(hwnd, &point)) {
    if (name.compare("wheel") == 0) {
      ::mouse_event(MOUSEEVENTF_WHEEL, 0, 0,
                    delta > 0 ? (-WHEEL_DELTA) : (WHEEL_DELTA), 0);
    } else if (name.compare("mouseenter") == 0 ||
               name.compare("mouseover") == 0 ||
               name.compare("mouseleave") == 0 ||
               name.compare("mousemove") == 0) {
      ::SetCursorPos(point.x, point.y);
    } else if (name.compare("mousedown") == 0) {
      ::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, point.x,
                    point.y, 0, 0);
    } else if (name.compare("mouseup") == 0) {
      ::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, point.x, point.y,
                    0, 0);
    } else if (name.compare("mouseclick") == 0) {
      ::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN, point.x,
                    point.y, 0, 0);
      ::mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, point.x, point.y,
                    0, 0);
    }
  }
#endif
}

void LynxUIRendererWin::RegisterNativeView(const char* name,
                                           lynx_native_view_creator creator,
                                           void* opaque) {
  auto* view_context =
      static_cast<clay::ViewContext*>(engine_->GetViewContext());
  clay::NativeViewServiceDesktop::AddViewFactory(view_context, name, creator,
                                                 opaque);
}

lynx::tasm::UIDelegate* LynxUIRendererWin::GetUIDelegate() {
  return ui_delegate_.get();
}

void LynxUIRendererWin::RegisterIMEHandler(void* handler, void* opaque) {
  HWND native_window = std::get<HWND>(*flutter_view_->GetRenderTarget());
  PostMessageW(native_window, kRequestIME, reinterpret_cast<WPARAM>(handler),
               reinterpret_cast<LPARAM>(opaque));
}

void LynxUIRendererWin::AdjustWindowRect() {
  HWND hwnd = reinterpret_cast<HWND>(GetNativeWindow());
  if (hwnd) {
    // Size and position the child window.
    MoveWindow(hwnd, 0, 0, width_ * pixel_ratio_, height_ * pixel_ratio_, TRUE);
  }
}

void LynxUIRendererWin::AddClient(LynxViewClients* client) {
  frame_timing_listener_->AddClient(client);
}

}  // namespace embedder
}  // namespace lynx
