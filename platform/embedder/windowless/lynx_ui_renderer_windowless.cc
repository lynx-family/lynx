// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/windowless/lynx_ui_renderer_windowless.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/string/string_utils.h"
#include "base/include/log/logging.h"
#include "clay/lynx_adaptor/native_module/lynx_module_factory.h"
#include "clay/lynx_adaptor/native_view_service_desktop.h"
#include "clay/lynx_adaptor/resource_loader_embedder.h"
#include "clay/net/loader/resource_loader_creator_service.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/rendering/text/render_text.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace embedder {

namespace {

using JsonBuffer = rapidjson::StringBuffer;
using JsonWriter = rapidjson::Writer<JsonBuffer>;

void WriteClayValue(JsonWriter* writer,
                    const clay::Value& value,
                    int depth = 0) {
  if (depth > 4) {
    writer->Null();
    return;
  }
  if (value.IsNone() || value.IsNull() || value.IsPointer() ||
      value.IsArrayBuffer()) {
    writer->Null();
  } else if (value.IsBool()) {
    writer->Bool(value.GetBool());
  } else if (value.IsInt()) {
    writer->Int(value.GetInt());
  } else if (value.IsUint()) {
    writer->Uint(value.GetUint());
  } else if (value.IsLong()) {
    writer->Int64(value.GetLong());
  } else if (value.IsFloat()) {
    writer->Double(value.GetFloat());
  } else if (value.IsDouble()) {
    writer->Double(value.GetDouble());
  } else if (value.IsString()) {
    writer->String(value.GetString().c_str());
  } else if (value.IsArray()) {
    writer->StartArray();
    for (const auto& item : value.GetArray()) {
      WriteClayValue(writer, item, depth + 1);
    }
    writer->EndArray();
  } else if (value.IsMap()) {
    writer->StartObject();
    std::vector<std::string> keys;
    keys.reserve(value.GetMap().size());
    for (const auto& item : value.GetMap()) {
      keys.push_back(item.first);
    }
    std::sort(keys.begin(), keys.end());
    for (const auto& key : keys) {
      writer->Key(key.c_str());
      WriteClayValue(writer, value.GetMap().at(key), depth + 1);
    }
    writer->EndObject();
  } else {
    writer->Null();
  }
}

void WriteFrame(JsonWriter* writer, clay::BaseView* view) {
  const auto rect = view->BoundsRelativeTo(nullptr);
  writer->StartObject();
  writer->Key("x");
  writer->Double(rect.x());
  writer->Key("y");
  writer->Double(rect.y());
  writer->Key("width");
  writer->Double(rect.width());
  writer->Key("height");
  writer->Double(rect.height());
  writer->EndObject();
}

void WriteViewNode(JsonWriter* writer, clay::BaseView* view, int depth) {
  writer->StartObject();
  writer->Key("nodeId");
  writer->Int(view->id());
  writer->Key("type");
  writer->String(view->GetName().c_str());
  if (!view->GetComponentName().empty()) {
    writer->Key("componentName");
    writer->String(view->GetComponentName().c_str());
  }
  writer->Key("visible");
  writer->Bool(view->Visible());
  writer->Key("anonymous");
  writer->Bool(view->IsAnonymousView());

  writer->Key("attributes");
  writer->StartObject();
  if (!view->GetIdSelector().empty()) {
    writer->Key("id");
    writer->String(view->GetIdSelector().c_str());
  }
  if (!view->GetRefIdSelector().empty()) {
    writer->Key("react-ref");
    writer->String(view->GetRefIdSelector().c_str());
  }
  const auto& dataset = view->GetDataSet();
  if (dataset.IsMap() && !dataset.GetMap().empty()) {
    writer->Key("dataset");
    WriteClayValue(writer, dataset);
  }
  writer->EndObject();

  writer->Key("box");
  WriteFrame(writer, view);

  if (view->Is<clay::TextView>()) {
    auto* text_view = static_cast<clay::TextView*>(view);
    const auto text =
        lynx::base::U16StringToU8(text_view->GetRenderText()->GetText());
    writer->Key("text");
    writer->String(text.c_str());
  }

  writer->Key("children");
  writer->StartArray();
  if (depth < 64) {
    for (auto* child : view->GetChildren()) {
      WriteViewNode(writer, child, depth + 1);
    }
  }
  writer->EndArray();
  writer->EndObject();
}

void CollectTexts(clay::BaseView* view, std::vector<std::string>* texts) {
  if (view->Is<clay::TextView>()) {
    auto* text_view = static_cast<clay::TextView*>(view);
    const auto text =
        lynx::base::U16StringToU8(text_view->GetRenderText()->GetText());
    if (!text.empty()) {
      texts->push_back(text);
    }
  }
  for (auto* child : view->GetChildren()) {
    CollectTexts(child, texts);
  }
}

int CountNodes(clay::BaseView* view) {
  int count = 1;
  for (auto* child : view->GetChildren()) {
    count += CountNodes(child);
  }
  return count;
}

}  // namespace

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
  windowless_renderer_->send_text_input = [this](const char* text) {
    return headless_engine_->CommitTextInput(text);
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

#if defined(OS_WIN) || defined(OS_MAC)
  clay::NativeViewServiceDesktop::SetViewFactories(
      view_context, std::move(builder->native_view_creators));
#endif

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
  windowless_renderer_->send_text_input = nullptr;
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

void LynxUIRendererWindowless::Reset() {
  if (auto* vc =
          static_cast<clay::ViewContext*>(headless_engine_->GetViewContext())) {
    vc->ResetPageView();
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

std::string LynxUIRendererWindowless::DumpUITreeForCDP() {
  auto* view_context =
      static_cast<clay::ViewContext*>(headless_engine_->GetViewContext());
  auto* page_view = view_context ? view_context->GetPageView() : nullptr;

  JsonBuffer buffer;
  JsonWriter writer(buffer);
  writer.StartObject();
  writer.Key("available");
  writer.Bool(page_view != nullptr);
  writer.Key("backend");
  writer.String("windowless-software");
  writer.Key("viewport");
  writer.StartObject();
  writer.Key("width");
  writer.Double(width_);
  writer.Key("height");
  writer.Double(height_);
  writer.Key("devicePixelRatio");
  writer.Double(pixel_ratio_);
  writer.EndObject();

  if (page_view) {
    std::vector<std::string> texts;
    CollectTexts(page_view, &texts);
    writer.Key("nodeCount");
    writer.Int(CountNodes(page_view));
    writer.Key("texts");
    writer.StartArray();
    for (const auto& text : texts) {
      writer.String(text.c_str());
    }
    writer.EndArray();
    writer.Key("root");
    WriteViewNode(&writer, page_view, 0);
  } else {
    writer.Key("nodeCount");
    writer.Int(0);
    writer.Key("texts");
    writer.StartArray();
    writer.EndArray();
  }

  writer.EndObject();
  return buffer.GetString();
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
