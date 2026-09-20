// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"

#include <string>
#include <utility>

#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_context.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/shell/lynx_engine.h"

namespace lynx {
namespace tasm {

NativePaintingCtxAndroidRef::NativePaintingCtxAndroidRef(
    std::unique_ptr<PlatformRendererFactory> view_factory,
    std::unique_ptr<PlatformRendererContext> view_manager)
    : NativePaintingCtxPlatformRef(std::move(view_factory)),
      view_manager_(std::move(view_manager)),
      enable_external_memory_report_(
          LynxEnv::GetInstance().EnableFiberElementMemoryReport()) {}

NativePaintingCtxAndroidRef::~NativePaintingCtxAndroidRef() { Destroy(); }

void NativePaintingCtxAndroidRef::RequestExternalMemoryReport(
    int64_t delay_ms) {
  if (!enable_external_memory_report_ || external_memory_report_pending_ ||
      destroyed_.load() || !event_target_task_runner_) {
    return;
  }
  external_memory_report_pending_ = true;
  auto weak_self = std::weak_ptr<NativePaintingCtxAndroidRef>(
      std::static_pointer_cast<NativePaintingCtxAndroidRef>(
          shared_from_this()));
  event_target_task_runner_->PostDelayedTask(
      [weak_self]() {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        self->external_memory_report_pending_ = false;
        if (self->destroyed_.load() || !self->engine_actor_) {
          return;
        }
        // A pending request may survive an engine handoff. Sample the current
        // owner, then reject the result if it moves again before delivery.
        const auto generation = self->engine_generation_->load();
        std::vector<std::pair<int32_t, int64_t>> nodes;
        nodes.reserve(self->renderers_.size());
        for (const auto& entry : self->renderers_) {
          if (entry.second) {
            nodes.emplace_back(entry.first,
                               entry.second->GetMemoryUsageBytes());
          }
        }
        self->engine_actor_->ActAsync([lifecycle = self->engine_generation_,
                                       generation,
                                       nodes = std::move(nodes)](auto& engine) {
          // Do not retain UI-owned objects on the engine thread.
          if (generation != lifecycle->load()) {
            return;
          }
          auto* tasm = engine->GetTasm();
          if (!tasm) {
            return;
          }
          auto* manager = tasm->page_proxy()->element_manager().get();
          if (!manager || !manager->EnableFiberElementMemoryReport()) {
            return;
          }
          ExternalMemorySnapshot ui_snapshot;
          for (const auto& node : nodes) {
            auto* element = manager->node_manager()->Get(node.first);
            // An Element can have been destroyed since the UI sample was taken.
            // Its queued renderer destruction must not become GC pressure
            // again.
            if (!element || node.second <= 0) {
              continue;
            }
            ui_snapshot.total_size += node.second;
            // A renderer removal can be part of a move. Classify using current
            // Element state instead of treating the removal as garbage.
            if (element->IsDetached()) {
              ui_snapshot.garbage_size += node.second;
            }
          }
          tasm->ReportExternalMemory(ui_snapshot);
        });
      },
      fml::TimeDelta::FromMilliseconds(delay_ms));
}

std::vector<float> NativePaintingCtxAndroidRef::GetTransformValue(
    int32_t sign, const std::vector<float>& offsets) {
  return GetTransformValueForEventTarget(sign, offsets);
}

void NativePaintingCtxAndroidRef::GetRootViewLocationOnScreen(
    float location[2]) {
  if (location == nullptr) {
    return;
  }
  location[0] = 0.f;
  location[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRootViewLocationOnScreen();
  if (res.size() >= 2) {
    location[0] = res[0];
    location[1] = res[1];
  }
}

void NativePaintingCtxAndroidRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetScreenSize();
  if (res.size() >= 2) {
    size[0] = res[0];
    size[1] = res[1];
  }
}

void NativePaintingCtxAndroidRef::GetPlatformRendererScrollOffset(
    int32_t sign, float offset[2]) {
  if (offset == nullptr) {
    return;
  }
  offset[0] = 0.f;
  offset[1] = 0.f;

  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRendererHostScrollOffset(sign);
  if (res.size() >= 2) {
    offset[0] = res[0];
    offset[1] = res[1];
  }
}

bool NativePaintingCtxAndroidRef::IsPlatformRendererScrollable(int32_t sign) {
  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return false;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return false;
  }
  return context->IsRendererHostScrollable(sign);
}

PlatformTextEventTargetRegions
NativePaintingCtxAndroidRef::GetTextEventTargetRegions(int32_t text_id) {
  const auto* ranges = GetTextEventTargetRanges(text_id);
  return view_manager_ != nullptr && ranges != nullptr
             ? view_manager_->GetTextEventTargetRegions(text_id, *ranges)
             : PlatformTextEventTargetRegions();
}

void NativePaintingCtxAndroidRef::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (view_manager_) {
    view_manager_->SetNeedMarkPaintEndTiming(pipeline_id);
  }
}

void NativePaintingCtxAndroidRef::InvokePlatformViewUIMethod(
    int32_t id, const std::string& method, const lepus::Value& params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value&> callback) {
  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(
        id, method, params, std::move(callback));
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    NativePaintingCtxPlatformRef::InvokePlatformViewUIMethod(
        id, method, params, std::move(callback));
    return;
  }
  context->InvokeUIMethod(id, method, params, std::move(callback));
}

void NativePaintingCtxAndroidRef::NotifyNodeReady(
    const std::vector<int32_t>& signs) {
  auto* factory =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  context->OnNodeReady(signs);
}

void NativePaintingCtxAndroidRef::DestroyImageOnPlatformThread(
    int32_t image_key) {
  if (!view_manager_) {
    return;
  }
  view_manager_->DestroyImage(image_key);
}

}  // namespace tasm
}  // namespace lynx
