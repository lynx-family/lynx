// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "base/trace/native/trace_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/event/event.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_target_exposure.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer_impl.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/renderer/utils/value_utils.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/lynx_shell.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx::tasm {
namespace {

int32_t ToInt(float value) { return static_cast<int32_t>(value); }

constexpr int64_t kEventTargetTreeUpdateIntervalMs = 50;
constexpr int32_t kUnknownEventTargetRootId = -1;

}  // namespace

NativePaintingCtxPlatformRef::NativePaintingCtxPlatformRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : view_factory_(std::move(view_factory)),
      event_target_task_runner_(base::UIThread::GetRunner()) {
  // TODO(hexionghui): The task runner should be obtained from the shell.
  event_target_exposure_ = std::make_shared<PlatformEventTargetExposure>(
      this, event_target_task_runner_);
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

  MarkEventTargetTreeDirty(id);
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
  MarkEventTargetTreeDirty(parent);
  if (event_target_helper_->IsActiveEventRoot(child)) {
    event_target_exposure_->RemoveExposureTargetsInEventRoot(child);
    event_target_helper_->RemoveActiveEventRoot(child);
    ClearEventTargetRootDirty(child);
  }
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
// Since iOS consumes logical pixels, device_pixel_ratio needs to be reset to 1.
#if defined(OS_IOS)
  device_pixel_ratio = 1.0f;
#endif
  event_target_helper_->SetDevicePixelRatio(device_pixel_ratio);
}

bool NativePaintingCtxPlatformRef::DispatchPlatformInputEvent(
    int int_event_data[], float float_event_data[],
    int32_t event_target_root_id) {
  auto event_target_tree = EnsureEventTargetTree(event_target_root_id);
  if (event_target_tree == nullptr) {
    return false;
  }
  return event_handler_->OnInputEvent(event_target_tree, int_event_data,
                                      float_event_data);
}

bool NativePaintingCtxPlatformRef::DispatchPlatformInputEvent(
    int int_event_data[], float float_event_data[]) {
  return DispatchPlatformInputEvent(int_event_data, float_event_data, kRootId);
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
  // TODO(hexionghui): When an Attribute does not trigger a rebuild, the
  // ApplyEventBundle needs to be executed actively for the PlatformEventTarget.
  if (bundle.Empty()) {
    platform_event_bundles_.erase(id);
    MarkEventTargetTreeDirty(id);
    return;
  }
  platform_event_bundles_.insert_or_assign(id, std::move(bundle));
  MarkEventTargetTreeDirty(id);
}

const PlatformEventBundle *NativePaintingCtxPlatformRef::GetPlatformEventBundle(
    int32_t id) const {
  auto it = platform_event_bundles_.find(id);
  if (it == platform_event_bundles_.end()) {
    return nullptr;
  }
  return &it->second;
}

int32_t NativePaintingCtxPlatformRef::GetEventTargetRootIdForRenderer(
    int32_t renderer_id) {
  if (renderer_id == kRootId ||
      event_target_helper_->IsActiveEventRoot(renderer_id)) {
    return renderer_id;
  }
  auto target = event_target_helper_->GetEventTarget(renderer_id);
  if (target != nullptr) {
    return target->RootId();
  }
  return kUnknownEventTargetRootId;
}

bool NativePaintingCtxPlatformRef::IsEventTargetRootDirty(
    int32_t root_id) const {
  return dirty_event_root_ids_.count(root_id) > 0;
}

void NativePaintingCtxPlatformRef::MarkEventTargetTreeDirty(
    int32_t renderer_id) {
  const int32_t root_id = GetEventTargetRootIdForRenderer(renderer_id);
  if (root_id != kUnknownEventTargetRootId) {
    MarkEventTargetRootDirty(root_id);
    return;
  }

  MarkEventTargetRootDirty(kRootId);
  for (const auto active_root_id :
       event_target_helper_->GetActiveEventRootIds()) {
    MarkEventTargetRootDirty(active_root_id);
  }
}

void NativePaintingCtxPlatformRef::MarkEventTargetRootDirty(int32_t root_id) {
  if (root_id != kRootId && !event_target_helper_->IsActiveEventRoot(root_id)) {
    root_id = kRootId;
  }
  dirty_event_root_ids_.insert(root_id);
}

void NativePaintingCtxPlatformRef::ClearEventTargetRootDirty(int32_t root_id) {
  dirty_event_root_ids_.erase(root_id);
}

void NativePaintingCtxPlatformRef::ScheduleEnsureEventTargetTree(
    int32_t root_id) {
  if (scheduled_event_target_tree_update_.exchange(true)) {
    return;
  }

  auto weak_this = weak_from_this();
  if (weak_this.expired() || !event_target_task_runner_) {
    scheduled_event_target_tree_update_.store(false);
    EnsureEventTargetTree(root_id);
    return;
  }

  event_target_task_runner_->PostDelayedTask(
      [weak_this, root_id]() {
        auto self = weak_this.lock();
        if (!self) {
          return;
        }
        self->scheduled_event_target_tree_update_.store(false);
        self->EnsureEventTargetTree(root_id);
      },
      fml::TimeDelta::FromMilliseconds(kEventTargetTreeUpdateIntervalMs));
}

fml::RefPtr<PlatformEventTarget>
NativePaintingCtxPlatformRef::EnsureEventTargetTree(int32_t root_id) {
  if (root_id != kRootId && !event_target_helper_->IsActiveEventRoot(root_id)) {
    return nullptr;
  }

  auto page_renderer = renderers_.find(kRootId);
  if (page_renderer == renderers_.end() || page_renderer->second == nullptr) {
    return nullptr;
  }

  auto requested_tree = event_target_helper_->GetEventRootTree(root_id);
  if (requested_tree == nullptr || IsEventTargetRootDirty(root_id)) {
    return ReconstructEventTargetTreeForRoot(root_id);
  }
  event_target_helper_->RefreshScrollOffsets(requested_tree);
  return requested_tree;
}

fml::RefPtr<PlatformEventTarget>
NativePaintingCtxPlatformRef::ReconstructEventTargetTreeRecursively() {
  return EnsureEventTargetTree(kRootId);
}

fml::RefPtr<PlatformEventTarget>
NativePaintingCtxPlatformRef::ReconstructEventTargetTreeForRoot(
    int32_t root_id) {
  auto renderer_it = renderers_.find(root_id);
  if (renderer_it == renderers_.end() || renderer_it->second == nullptr) {
    ClearEventTargetRootDirty(root_id);
    return nullptr;
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              NATIVE_PAINTING_CONTEXT_RECONSTRUCT_EVENT_TARGET_TREE);
  event_target_exposure_->ClearExposureTargetsInEventRoot(root_id);
  auto tree = event_target_helper_->ReconstructEventTargetTreeRecursively(
      fml::static_ref_ptr_cast<PlatformRendererImpl>(renderer_it->second));
  ClearEventTargetRootDirty(root_id);
  event_target_helper_->RefreshScrollOffsets(tree);
  event_target_exposure_->InvalidateWindowRect();
  event_target_exposure_->DidRebuildExposureTargetMap();
  return tree;
}

bool NativePaintingCtxPlatformRef::EnsureEventTargetTreeForTarget(
    int32_t target_id) {
  if (EnsureEventTargetTree(kRootId) == nullptr) {
    return false;
  }
  auto target = event_target_helper_->GetEventTarget(target_id);
  if (target != nullptr) {
    const auto root_id = target->RootId();
    if (event_target_helper_->GetEventRootTree(root_id) == nullptr ||
        IsEventTargetRootDirty(root_id)) {
      EnsureEventTargetTree(root_id);
    }
    target = event_target_helper_->GetEventTarget(target_id);
    if (target != nullptr &&
        event_target_helper_->GetEventRootTree(target->RootId()) != nullptr) {
      return true;
    }
  }
  for (auto root_id : event_target_helper_->GetActiveEventRootIds()) {
    EnsureEventTargetTree(root_id);
    if (event_target_helper_->GetEventTarget(target_id) != nullptr) {
      return true;
    }
  }
  return event_target_helper_->GetEventTarget(target_id) != nullptr;
}

std::vector<int32_t>
NativePaintingCtxPlatformRef::CollectMeaningfulPaintingAreaRecords() {
  if (EnsureEventTargetTree(kRootId) == nullptr) {
    return {};
  }
  std::vector<int32_t> active_root_ids;
  for (const auto root_id : event_target_helper_->GetActiveEventRootIds()) {
    active_root_ids.push_back(root_id);
  }
  for (const auto root_id : active_root_ids) {
    EnsureEventTargetTree(root_id);
  }
  for (const auto &it : event_target_helper_->GetEventRootTrees()) {
    event_target_helper_->RefreshScrollOffsets(it.second);
  }

  std::vector<int32_t> flattened;
  const auto &event_targets = event_target_helper_->GetEventTargets();
  flattened.reserve(event_targets.size() * 6);
  for (const auto &it : event_targets) {
    const auto &target = it.second;
    if (target == nullptr) {
      continue;
    }

    auto type = target->GetPlatformRendererType();
    if (type == PlatformRendererType::kUnknown) {
      auto renderer_it = renderers_.find(target->Sign());
      if (renderer_it != renderers_.end() && renderer_it->second != nullptr) {
        auto renderer =
            fml::static_ref_ptr_cast<PlatformRendererImpl>(renderer_it->second);
        type = renderer->GetPlatformRendererType();
      }
    }
    if (type == PlatformRendererType::kUnknown) {
      continue;
    }

    float rect[4] = {0.f, 0.f, target->Width(), target->Height()};
    if (event_target_helper_->GetEventRootTree(target->RootId()) == nullptr) {
      continue;
    }
    event_target_helper_->ConvertRectFromTargetToPageRootTarget(rect, target,
                                                                rect);
    int32_t x = ToInt(rect[0]);
    int32_t y = ToInt(rect[1]);
    int32_t width = ToInt(rect[2] - rect[0]);
    int32_t height = ToInt(rect[3] - rect[1]);
    if (width <= 0 || height <= 0) {
      continue;
    }

    flattened.push_back(target->Sign());
    flattened.push_back(static_cast<int32_t>(type));
    flattened.push_back(x);
    flattened.push_back(y);
    flattened.push_back(width);
    flattened.push_back(height);
  }
  return flattened;
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

void NativePaintingCtxPlatformRef::SetPlatformEventRootActive(int32_t root_id,
                                                              bool active) {
  if (root_id == kRootId) {
    return;
  }
  if (active) {
    event_target_helper_->AddActiveEventRoot(root_id);
    MarkEventTargetRootDirty(root_id);
    EnsureEventTargetTree(root_id);
    return;
  }

  event_target_exposure_->RemoveExposureTargetsInEventRoot(root_id);
  event_target_helper_->RemoveActiveEventRoot(root_id);
  ClearEventTargetRootDirty(root_id);
}

void NativePaintingCtxPlatformRef::SetPlatformEventRootOffset(int32_t root_id,
                                                              float offset_x,
                                                              float offset_y) {
  if (root_id == kRootId) {
    return;
  }
  event_target_helper_->SetEventRootOffsetToPageRoot(root_id, offset_x,
                                                     offset_y);
}

void NativePaintingCtxPlatformRef::StopExposure(const lepus::Value &options) {
  event_target_exposure_->StopExposureCheck(options);
}

void NativePaintingCtxPlatformRef::ResumeExposure() {
  event_target_exposure_->StartExposureCheck();
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
  // Invoke ui method on the event target.
  if (method == "boundingClientRect") {
    base::MoveOnlyClosure<void, int32_t, const lepus::Value &> cb =
        [engine_actor = engine_actor_, callback = std::move(callback)](
            int32_t code, const lepus::Value &data) mutable {
          if (engine_actor == nullptr) {
            return;
          }
          lepus::Value data_copy = data;
          engine_actor->Act(
              [callback = std::move(callback), code,
               data = std::move(data_copy)](auto &engine) mutable {
                callback(code, PubLepusValue(data));
              });
        };
    if (!EnsureEventTargetTreeForTarget(id)) {
      cb(LynxGetUIResult::UNKNOWN,
         lepus::Value("failed to ensure event target tree"));
      return;
    }
    event_target_helper_->InvokeMethod(id, method, params, std::move(cb));
  }
  // Invoke ui method on the platform renderer.
  else {
    base::MoveOnlyClosure<void, int32_t, const pub::Value &> cb =
        [engine_actor = engine_actor_, callback = std::move(callback)](
            int32_t code, const pub::Value &data) mutable {
          if (engine_actor == nullptr) {
            return;
          }
          lepus::Value data_copy =
              pub::ValueUtils::ConvertValueToLepusValue(data);
          engine_actor->Act(
              [callback = std::move(callback), code,
               data = std::move(data_copy)](auto &engine) mutable {
                callback(code, PubLepusValue(data));
              });
        };
    InvokePlatformRendererUIMethod(id, method, params, std::move(cb));
  }
}

void NativePaintingCtxPlatformRef::InvokePlatformRendererUIMethod(
    int32_t id, const std::string &method, const lepus::Value &params,
    base::MoveOnlyClosure<void, int32_t, const pub::Value &> callback) {
  if (callback) {
    callback(LynxGetUIResult::UNKNOWN,
             PubLepusValue(lepus::Value("method not supported: " + method)));
  }
}

void NativePaintingCtxPlatformRef::Destroy() {
  renderers_.clear();
  platform_event_bundles_.clear();
  scheduled_event_target_tree_update_.store(false);
  dirty_event_root_ids_.clear();
  event_target_helper_->ClearActiveEventRoots();
  event_target_helper_->ClearEventTargets();
  if (event_target_exposure_) {
    event_target_exposure_->ClearExposureTargetMap();
  }
  engine_actor_.reset();
}

}  // namespace lynx::tasm
