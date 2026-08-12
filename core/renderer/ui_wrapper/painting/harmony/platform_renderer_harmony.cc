// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"

#include <utility>

#include "core/renderer/dom/fragment/display_list_reader.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
PlatformRendererHarmony::PlatformRendererHarmony(
    std::shared_ptr<harmony::LynxRendererContext> context, int id,
    PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config)
    : PlatformRendererHarmony(std::move(context), id, type, base::String(),
                              init_data, init_config) {}

PlatformRendererHarmony::PlatformRendererHarmony(
    std::shared_ptr<harmony::LynxRendererContext> context, int id,
    const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config)
    : PlatformRendererHarmony(std::move(context), id,
                              PlatformRendererType::kUnknown, tag_name,
                              init_data, init_config) {}

PlatformRendererHarmony::PlatformRendererHarmony(
    std::shared_ptr<harmony::LynxRendererContext> context, int id,
    PlatformRendererType type, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config)
    : PlatformRendererImpl(id, type, tag_name), context_(std::move(context)) {
  SetDirectChildOfCompatibleComponent(
      init_config.is_direct_child_of_compatible_component);
  SetFragmentParentId(init_config.fragment_parent_id);
  if (ShouldCreatePlatformExtendedRenderer(init_config)) {
    is_platform_extended_renderer_ = true;
  }
  InitializePlatformRenderer(init_data);
}

PlatformRendererHarmony::~PlatformRendererHarmony() { CleanupRenderer(); }

void PlatformRendererHarmony::InitializePlatformRenderer(
    const fml::RefPtr<PropBundle>& init_data) {
  if (context_ == nullptr) {
    return;
  }

  if (IsPlatformExtendedRenderer() || type_ == PlatformRendererType::kPage) {
    if (InitializeUIOwnerRenderer(GetExtendedRendererTagName(), init_data)) {
      return;
    }
    if (type_ == PlatformRendererType::kPage) {
      return;
    }
  }
  InitializeFragmentLayerRenderer();
}

bool PlatformRendererHarmony::InitializeUIOwnerRenderer(
    const base::String& tag_name, const fml::RefPtr<PropBundle>& init_data) {
  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (ui_owner == nullptr || tag_name.empty()) {
    return false;
  }
  PropBundleHarmony empty_props;
  auto* props = init_data == nullptr
                    ? &empty_props
                    : reinterpret_cast<PropBundleHarmony*>(init_data.get());
  ui_owner->CreateUI(GetId(), tag_name.str(), props, GetId());

  auto* ui = ui_owner->FindUIBySign(GetId());
  if (ui == nullptr) {
    return false;
  }
  return AttachRendererToUI(ui);
}

bool PlatformRendererHarmony::InitializeFragmentLayerRenderer() {
  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (ui_owner == nullptr) {
    return false;
  }
  return AttachRendererToUI(ui_owner->CreateFragmentLayer(GetId()));
}

bool PlatformRendererHarmony::AttachRendererToUI(harmony::UIBase* ui) {
  if (ui == nullptr || ui->Node() == nullptr) {
    return false;
  }
  ui->AttachFragmentLayerRenderer(context_, GetId());
  host_ = ui->weak_from_this();
  return true;
}

void PlatformRendererHarmony::CleanupRenderer() {
  if (auto host = host_.lock()) {
    host->DetachFragmentLayerRenderer();
  }

  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (type_ != PlatformRendererType::kPage && ui_owner != nullptr &&
      !ui_owner->Destroyed() && ui_owner->FindUIBySign(GetId()) != nullptr) {
    ui_owner->DestroyUI(-1, GetId(), -1);
  }
  host_.reset();
}

void PlatformRendererHarmony::OnUpdateDisplayList(DisplayList display_list) {
  auto host = host_.lock();
  if (display_list.GetContentItemsSize() == 0 || host == nullptr) {
    return;
  }
  UpdateHostLayout(display_list);
  host->UpdateFragmentLayerDisplayList(std::move(display_list));
}

void PlatformRendererHarmony::OnUpdateAttributes(
    const fml::RefPtr<PropBundle>& attributes, bool tends_to_flatten) {
  if (!IsPlatformExtendedRenderer() || attributes == nullptr) {
    return;
  }
  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (ui_owner != nullptr) {
    ui_owner->UpdateUI(GetId(),
                       reinterpret_cast<PropBundleHarmony*>(attributes.get()));
  }
}

void PlatformRendererHarmony::OnAddChild(PlatformRenderer* child, int index,
                                         bool should_update_ui_owner) {
  auto host = host_.lock();
  if (host == nullptr || child == nullptr) {
    return;
  }
  auto* harmony_child = static_cast<PlatformRendererHarmony*>(child);
  auto child_host = harmony_child->host_.lock();
  if (child_host == nullptr) {
    return;
  }

  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (!should_update_ui_owner && harmony_child->IsOverlay()) {
    return;
  }
  if (ui_owner != nullptr && child_host->Parent() != host.get()) {
    child_host->RemoveFromParent();
    ui_owner->InsertUI(GetId(), harmony_child->GetId(), index);
    child_host->OnAttachedToFragmentLayerTree();
    host->OnFragmentLayerChildrenChanged();
  }
}

void PlatformRendererHarmony::OnRemoveFromParent(bool should_update_ui_owner) {
  auto* parent = static_cast<PlatformRendererHarmony*>(GetParent());
  auto host = host_.lock();
  auto parent_host = parent != nullptr ? parent->host_.lock() : nullptr;
  if (host == nullptr || parent_host == nullptr) {
    return;
  }

  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (!should_update_ui_owner && IsOverlay()) {
    return;
  }
  if (ui_owner != nullptr && host->Parent() == parent_host.get()) {
    ui_owner->RemoveUI(parent->GetId(), GetId(), -1, false);
    parent_host->OnFragmentLayerChildrenChanged();
  }
}

void PlatformRendererHarmony::OnUpdateSubtreeProperties(
    const DisplayList& subtree_properties) {}

bool PlatformRendererHarmony::ShouldCreatePlatformExtendedRenderer(
    const PlatformRendererInitConfig& init_config) const {
  if (init_config.is_direct_child_of_compatible_component) {
    return true;
  }
  if (type_ == PlatformRendererType::kText ||
      type_ == PlatformRendererType::kImage ||
      type_ == PlatformRendererType::kView ||
      type_ == PlatformRendererType::kPage) {
    return false;
  }
  if (type_ != PlatformRendererType::kUnknown) {
    return true;
  }
  return !tag_name_.empty();
}

void PlatformRendererHarmony::UpdateHostLayout(
    const DisplayList& display_list) {
  if (host_.expired()) {
    return;
  }
  DisplayListReader reader(display_list);
  if (!reader.HasNext()) {
    return;
  }
  const auto& item = reader.Next();
  if (item.type != DisplayListOpType::kBegin) {
    return;
  }

  const auto& frame = item.payload.begin;
  const float left = frame.x + display_list.GetRenderOffset()[0];
  const float top = frame.y + display_list.GetRenderOffset()[1];
  harmony::UIOwner* ui_owner =
      context_ != nullptr ? context_->GetUIOwner() : nullptr;
  if (ui_owner != nullptr) {
    constexpr float kZeroMetrics[4] = {0.f, 0.f, 0.f, 0.f};
    ui_owner->UpdateLayout(GetId(), left, top, frame.w, frame.h, kZeroMetrics,
                           kZeroMetrics, nullptr, 0.f, 0);
  }
}

PlatformRendererHarmonyFactory::PlatformRendererHarmonyFactory(
    std::shared_ptr<harmony::LynxRendererContext> context)
    : context_(std::move(context)) {}

fml::RefPtr<PlatformRenderer> PlatformRendererHarmonyFactory::CreateRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  return fml::MakeRefCounted<PlatformRendererHarmony>(context_, id, type,
                                                      init_data, init_config);
}

fml::RefPtr<PlatformRenderer>
PlatformRendererHarmonyFactory::CreateExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {
  return fml::MakeRefCounted<PlatformRendererHarmony>(context_, id, tag_name,
                                                      init_data, init_config);
}

}  // namespace tasm
}  // namespace lynx
