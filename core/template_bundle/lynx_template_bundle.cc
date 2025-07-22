// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynx_template_bundle.h"

#include "core/renderer/simple_styling/style_object.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"

namespace lynx {
namespace tasm {

std::optional<std::shared_ptr<lepus::ContextBundle>>
LepusChunkManager::GetLepusChunk(const std::string &chunk_key) {
  std::lock_guard<std::mutex> g_lock(lepus_chunk_mutex_);
  decoded_lepus_chunks_.emplace(chunk_key);
  auto iter = lepus_chunk_map_.find(chunk_key);
  if (iter != lepus_chunk_map_.end()) {
    return iter->second;
  } else {
    return std::nullopt;
  }
}

bool LepusChunkManager::IsLepusChunkDecoded(const std::string &chunk_path) {
  std::lock_guard<std::mutex> g_lock(lepus_chunk_mutex_);
  if (decoded_lepus_chunks_.find(chunk_path) != decoded_lepus_chunks_.end()) {
    return true;
  }
  return false;
}

void LepusChunkManager::AddLepusChunk(
    const std::string &chunk_key,
    std::shared_ptr<lepus::ContextBundle> bundle) {
  std::lock_guard<std::mutex> g_lock(lepus_chunk_mutex_);
  lepus_chunk_map_.emplace(chunk_key, std::move(bundle));
}

lepus::Value LynxTemplateBundle::GetExtraInfo() {
  if (page_configs_) {
    return page_configs_->GetExtraInfo();
  }
  return lepus::Value();
}

void LynxTemplateBundle::PrepareVMByConfigs() {
  // Contexts cannot be pre-created in following case:
  // will reuse context (dynamic component && no-diff)
  if (ShouldReuseLepusContext()) {
    return;
  }

  const bool disable_tracing_gc =
      page_configs_ && page_configs_->GetDisableQuickTracingGC();

  context_pool_ = lepus::LynxContextPool::Create(
      is_lepusng_binary(), disable_tracing_gc, context_bundle_,
      compile_options_, page_configs_.get());

  // if FE disables it in card, do not pre-create contexts. However, we reserve
  // the ability for the client to force pre-creation
  if (page_configs_ && page_configs_->GetEnableUseContextPool()) {
    constexpr int32_t kLocalQuickContextPoolSize = 1;
    context_pool_->FillPool(kLocalQuickContextPoolSize);
  }
}

bool LynxTemplateBundle::PrepareLepusContext(int32_t count) {
  if (!context_pool_ || count <= 0) {
    return false;
  }

  // a maximum of 20 contexts can be created in a single task
  constexpr int32_t kOnePatchMaxSize = 20;
  context_pool_->FillPool(std::min(kOnePatchMaxSize, count));

  force_use_context_pool_ = true;
  return true;
}

void LynxTemplateBundle::SetEnableVMAutoGenerate(bool enable) {
  if (context_pool_) {
    context_pool_->SetEnableAutoGenerate(enable);
  }
}

void LynxTemplateBundle::AddCustomSection(const std::string &key,
                                          const lepus::Value &value) {
  if (!custom_sections_.IsTable()) {
    custom_sections_ = lepus::Value{lepus::Dictionary::Create()};
  }
  custom_sections_.SetProperty(key, value);
}

lepus::Value LynxTemplateBundle::GetCustomSection(const std::string &key) {
  return custom_sections_.GetProperty(key);
}

bool LynxTemplateBundle::ShouldReuseLepusContext() const {
  // the lepus context of dynamic component in FiberArch should reuse the
  // context in card
  return !IsCard() && compile_options_.enable_fiber_arch_;
}

void LynxTemplateBundle::EnsureParseTaskScheduler() {
  if (task_schedular_ == nullptr) {
    task_schedular_ = std::make_shared<ParallelParseTaskScheduler>();
  }
}

void LynxTemplateBundle::GreedyConstructElements() {
  EnsureParseTaskScheduler();
  for (const auto &pair : element_template_infos_) {
    task_schedular_->ConstructElement(pair.first, pair.second, true);
  }
}

std::optional<Elements> LynxTemplateBundle::TryGetElements(
    const std::string &key) {
  if (task_schedular_ == nullptr) {
    return std::nullopt;
  }
  return task_schedular_->TryGetElements(key, element_template_infos_[key]);
}

static void StyleObjectArrayDeleter(style::StyleObject **obj) {
  for (auto **p = obj; *p != nullptr; p++) {
    (*p)->Release();
  }
  delete[] obj;
}

std::shared_ptr<style::StyleObject *> &LynxTemplateBundle::InitStyleObjectList(
    size_t size) {
  if (style_object_list_) {
    return style_object_list_;
  }
  style::StyleObject **style_object_array = new style::StyleObject *[size + 1];
  style_object_list_ = std::shared_ptr<style::StyleObject *>(
      style_object_array, StyleObjectArrayDeleter);
  style_object_array[size] = nullptr;
  return style_object_list_;
}

base::LinearFlatMap<base::String, fml::RefPtr<CSSKeyframesToken>> &
LynxTemplateBundle::InitKeyframesMap(size_t size) {
  if (!keyframes_.empty()) {
    return keyframes_;
  }
  keyframes_.reserve(size);
  return keyframes_;
}

}  // namespace tasm
}  // namespace lynx
