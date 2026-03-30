// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/event/event.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_target_exposure.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/lynx_shell.h"

namespace lynx::tasm {

NativePaintingCtxPlatformRef::NativePaintingCtxPlatformRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : view_factory_(std::move(view_factory)) {
  // TODO(hexionghui): The task runner should be obtained from the shell.
  event_target_exposure_ = std::make_shared<PlatformEventTargetExposure>(
      this, base::UIThread::GetRunner());
}

void NativePaintingCtxPlatformRef::CreatePlatformRenderer(
    int id, PlatformRendererType type,
    const fml::RefPtr<PropBundle> &init_data) {
  renderers_.insert_or_assign(
      id, view_factory_->CreateRenderer(id, type, init_data));
}

void NativePaintingCtxPlatformRef::CreatePlatformExtendedRenderer(
    int id, const base::String &tag_name,
    const fml::RefPtr<PropBundle> &init_data) {
  renderers_.insert_or_assign(
      id, view_factory_->CreateExtendedRenderer(id, tag_name, init_data));
}

void NativePaintingCtxPlatformRef::UpdateDisplayList(
    int id, DisplayList &&display_list) {
  auto it = renderers_.find(id);
  if (it == renderers_.end()) {
    return;
  }

  need_reconstruct_event_target_tree_ = true;
  const auto &layer = it->second;
  // Rebuild the sublayers according to the new SubLayers in the display list
  // with MyersDiff. And generate actual addChild and removeChild actions for
  // PlatformRenderer here.
  if (display_list.HasContent()) {
    // When it has no content op, it's a shortcut just update subtree properties
    // without a entire draw pass.
    RebuildSubLayers(layer, display_list.SubLayers());
  }

  layer->UpdateDisplayList(std::move(display_list));
}

void NativePaintingCtxPlatformRef::RemovePaintingNode(int parent, int child,
                                                      int index, bool is_move) {
  if (auto it_child = renderers_.find(child); it_child != renderers_.end()) {
    it_child->second->RemoveFromParent();
  }
}

void NativePaintingCtxPlatformRef::DestroyPaintingNode(int parent, int child,
                                                       int index) {
  if (auto it_child = renderers_.find(child); it_child != renderers_.end()) {
    it_child->second->RemoveFromParent();
    renderers_.erase(child);
  }
  platform_event_bundles_.erase(child);
}

void NativePaintingCtxPlatformRef::RebuildSubLayers(
    const fml::RefPtr<PlatformRenderer> &renderer,
    const base::InlineVector<int, 16> &new_children) {
  const auto &existing_children = renderer->Children();

  if (existing_children.empty()) {
    // If there are no existing children, simply add all new sublayers.
    for (int child_id : new_children) {
      auto child_it = renderers_.find(child_id);
      if (child_it != renderers_.end()) {
        renderer->AddChild(child_it->second);
      }
    }
    return;
  }

  // Use MyersDiff to compare existing children with new display_list
  // SubLayers Custom comparator: compare existing child's ID with new child
  // ID
  auto id_compare = [](const fml::RefPtr<PlatformRenderer> &existing_child,
                       int new_child_id) {
    return existing_child->GetId() == new_child_id;
  };

  // Perform diff
  auto diff_result = myers_diff::MyersDiffWithoutUpdate(
      existing_children.begin(), existing_children.end(), new_children.begin(),
      new_children.end(), id_compare);

  // Apply removals: process in reverse order to avoid index shifting
  std::sort(diff_result.removals_.begin(), diff_result.removals_.end(),
            std::greater<>());
  for (int idx : diff_result.removals_) {
    if (idx >= 0 && static_cast<size_t>(idx) < existing_children.size()) {
      existing_children[idx]->RemoveFromParent();
    }
  }

  // Apply insertions
  for (int insert_pos : diff_result.insertions_) {
    if (insert_pos < 0 ||
        static_cast<size_t>(insert_pos) >= new_children.size()) {
      continue;
    }
    int child_id = new_children[insert_pos];
    auto child_it = renderers_.find(child_id);
    if (child_it != renderers_.end()) {
      renderer->AddChild(child_it->second);
    }
  }
}

void NativePaintingCtxPlatformRef::SetLynxEngineActorForPlatformContextRef(
    std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor) {
  engine_actor_ = engine_actor;
  float device_pixel_ratio =
      engine_actor_ != nullptr
          ? engine_actor_->Impl()->GetTasm()->GetDevicePixelRatio()
          : 1.0f;
  event_target_helper_->SetDevicePixelRatio(device_pixel_ratio);
}

bool NativePaintingCtxPlatformRef::DispatchPlatformInputEvent(
    int int_event_data[], float float_event_data[]) {
  auto event_target_tree = ReconstructEventTargetTreeRecursively();
  if (event_target_tree == nullptr) {
    return false;
  }
  return event_handler_->OnInputEvent(event_target_tree, int_event_data,
                                      float_event_data);
}

int NativePaintingCtxPlatformRef::GetPlatformEventHandlerState() {
  return event_handler_->EventHandlerState();
}

void NativePaintingCtxPlatformRef::SendEvent(int32_t target_id,
                                             fml::RefPtr<event::Event> event) {
  if (engine_actor_ == nullptr) {
    return;
  }
  engine_actor_->Act([target_id, event = std::move(event)](auto &engine) {
    engine->SendEvent(target_id, event);
  });
}

void NativePaintingCtxPlatformRef::UpdatePseudoStatusStatus(
    int32_t target_id, uint32_t pre_status, uint32_t current_status) {
  if (engine_actor_ == nullptr) {
    return;
  }
  engine_actor_->Act([target_id, pre_status, current_status](auto &engine) {
    engine->OnPseudoStatusChanged(target_id, static_cast<int>(pre_status),
                                  static_cast<int>(current_status));
  });
}

PlatformEventEmitter *NativePaintingCtxPlatformRef::GetEventEmitter() {
  return event_emitter_.get();
}

PlatformEventTargetHelper *
NativePaintingCtxPlatformRef::GetEventTargetHelper() {
  return event_target_helper_.get();
}

void NativePaintingCtxPlatformRef::UpdatePlatformEventBundle(
    int32_t id, PlatformEventBundle bundle) {
  if (bundle.Empty()) {
    platform_event_bundles_.erase(id);
    return;
  }
  platform_event_bundles_.insert_or_assign(id, std::move(bundle));
}

const PlatformEventBundle *NativePaintingCtxPlatformRef::GetPlatformEventBundle(
    int32_t id) const {
  auto it = platform_event_bundles_.find(id);
  if (it == platform_event_bundles_.end()) {
    return nullptr;
  }
  return &it->second;
}

fml::RefPtr<PlatformEventTarget>
NativePaintingCtxPlatformRef::ReconstructEventTargetTreeRecursively() {
  return ReconstructEventTargetTreeRecursively(nullptr);
}

fml::RefPtr<PlatformEventTarget>
NativePaintingCtxPlatformRef::ReconstructEventTargetTreeRecursively(
    bool *did_reconstruct) {
  auto page_renderer = renderers_.find(kRootId);
  if (page_renderer == renderers_.end() || page_renderer->second == nullptr) {
    if (did_reconstruct != nullptr) {
      *did_reconstruct = false;
    }
    return nullptr;
  }
  auto event_target_tree = event_target_helper_->GetRootEventTarget();
  if (need_reconstruct_event_target_tree_ == false &&
      event_target_tree != nullptr) {
    if (did_reconstruct != nullptr) {
      *did_reconstruct = false;
    }
    return event_target_tree;
  }
  need_reconstruct_event_target_tree_ = false;
  if (did_reconstruct != nullptr) {
    *did_reconstruct = true;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              NATIVE_PAINTING_CONTEXT_RECONSTRUCT_EVENT_TARGET_TREE);
  return event_target_helper_->ReconstructEventTargetTreeRecursively(
      fml::RefPtr<PlatformRendererImpl>(
          static_cast<PlatformRendererImpl *>(page_renderer->second.get())));
}

void NativePaintingCtxPlatformRef::AddPlatformEventTargetToExposure(
    const fml::RefPtr<PlatformEventTarget> &target,
    const lepus::Value &detail) {
  if (target == nullptr) {
    return;
  }
  event_target_exposure_->AddExposureTarget(target, detail);
}

void NativePaintingCtxPlatformRef::RemovePlatformEventTargetFromExposure(
    const fml::RefPtr<PlatformEventTarget> &target,
    const lepus::Value &detail) {
  if (target == nullptr) {
    return;
  }
  event_target_exposure_->RemoveExposureTarget(target, detail);
}

void NativePaintingCtxPlatformRef::ClearExposureTargetMap() {
  event_target_exposure_->ClearExposureTargetMap();
}

void NativePaintingCtxPlatformRef::UpdateAttributes(
    int id, const fml::RefPtr<PropBundle> &attributes, bool tend_to_flatten) {
  auto it = renderers_.find(id);
  if (it == renderers_.end()) {
    return;
  }
  it->second->UpdateAttributes(attributes, tend_to_flatten);
}

void NativePaintingCtxPlatformRef::InvokeUIMethod(
    int32_t id, const std::string &method, const lepus::Value &params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value &> callback) {
  base::MoveOnlyClosure<void, int32_t, const lepus::Value &> cb =
      [engine_actor = engine_actor_, callback = std::move(callback)](
          int32_t code, const lepus::Value &data) mutable {
        if (engine_actor == nullptr) {
          return;
        }
        lepus::Value data_copy = data;
        engine_actor->Act([callback = std::move(callback), code,
                           data = std::move(data_copy)](auto &engine) mutable {
          callback(code, PubLepusValue(data));
        });
      };

  // Invoke ui method on the event target.
  if (method == "boundingClientRect") {
    if (ReconstructEventTargetTreeRecursively() == nullptr) {
      cb(LynxGetUIResult::UNKNOWN,
         lepus::Value("failed to reconstruct event target tree"));
      return;
    }
    event_target_helper_->InvokeMethod(id, method, params, std::move(cb));
  }
  // Invoke ui method on the platform renderer.
  else {
    cb(LynxGetUIResult::UNKNOWN,
       lepus::Value("method not supported: " + method));
  }
}

void NativePaintingCtxPlatformRef::Destroy() {
  renderers_.clear();
  platform_event_bundles_.clear();
  if (event_target_exposure_) {
    event_target_exposure_->ClearExposureTargetMap();
  }
  engine_actor_.reset();
}

}  // namespace lynx::tasm
