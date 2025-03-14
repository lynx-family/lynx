// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynx_template_bundle.h"

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

bool LynxTemplateBundle::PrepareLepusContext(int32_t count) {
  if (!quick_context_pool_ || count <= 0) {
    return false;
  }

  // a maximum of 20 contexts can be created in a single task
  constexpr int32_t kOnePatchMaxSize = 20;
  quick_context_pool_->FillPool(std::min(kOnePatchMaxSize, count));

  use_context_pool_ = true;
  return true;
}

void LynxTemplateBundle::SetEnableVMAutoGenerate(bool enable) {
  if (quick_context_pool_) {
    quick_context_pool_->SetEnableAutoGenerate(enable);
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

}  // namespace tasm
}  // namespace lynx
