// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynx_template_bundle.h"

#include <algorithm>

#include "core/renderer/simple_styling/style_object.h"
#include "core/runtime/lepus/binary_input_stream.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"

namespace lynx {
namespace tasm {

std::string LynxTemplateBundle::FromBinary(std::vector<uint8_t> binary,
                                           bool is_card,
                                           const std::string &template_url) {
  auto input_stream =
      std::make_unique<lepus::ByteArrayInputStream>(std::move(binary));
  auto reader =
      std::make_unique<TemplateBinaryReader>(std::move(input_stream), this);
  reader->SetIsCardType(is_card);
  reader->SetTemplateUrl(template_url);

  if (!reader->Decode()) {
    return reader->error_message_;
  }

  lazy_reader_ =
      std::shared_ptr<LynxBinaryLazyReaderDelegate>(std::move(reader));
  return "";
}

std::string LynxTemplateBundle::FromBinaryGreedy(
    std::vector<uint8_t> binary, const std::string &template_url,
    bool skip_css_decode, std::optional<bool> is_card) {
  auto reader = LynxBinaryReader::CreateLynxBinaryReader(std::move(binary));
  if (is_card.has_value()) {
    reader.SetIsCardType(*is_card);
  }
  reader.SetTemplateUrl(template_url);
  reader.SetSkipCSSDecode(skip_css_decode);
  if (!reader.Decode()) {
    return reader.error_message_;
  }
  *this = reader.GetTemplateBundle();
  return "";
}

std::optional<std::shared_ptr<runtime::ContextBundle>>
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
    std::shared_ptr<runtime::ContextBundle> bundle) {
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

  // RTSNative bypasses bundle-level context pool preparation.
  if (context_type_ == runtime::ContextType::RTSNativeContextType) {
    return;
  }

  const bool disable_tracing_gc =
      page_configs_ && page_configs_->GetDisableQuickTracingGC();

  mts_runtime_pool_ = shell::MTSRuntimePool::Create(
      context_type_, disable_tracing_gc, context_bundle_, compile_options_,
      page_configs_.get());
  mts_runtime_pool_->SetDevToolPool(devtool_pool_);

  // if FE disables it in card, do not pre-create contexts. However, we reserve
  // the ability for the client to force pre-creation
  if (page_configs_ && page_configs_->GetEnableUseContextPool()) {
    constexpr int32_t kLocalQuickContextPoolSize = 1;
    mts_runtime_pool_->FillPool(kLocalQuickContextPoolSize);
  }
}

bool LynxTemplateBundle::PrepareLepusContext(int32_t count) {
  if (context_type_ == runtime::ContextType::RTSNativeContextType ||
      !mts_runtime_pool_ || count <= 0) {
    return false;
  }

  // a maximum of 20 contexts can be created in a single task
  constexpr int32_t kOnePatchMaxSize = 20;
  mts_runtime_pool_->FillPool(std::min(kOnePatchMaxSize, count));

  force_use_context_pool_ = true;
  return true;
}

void LynxTemplateBundle::SetEnableVMAutoGenerate(bool enable) {
  if (mts_runtime_pool_) {
    mts_runtime_pool_->SetEnableAutoGenerate(enable);
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

std::shared_ptr<CSSKeyframesTokenMap> &LynxTemplateBundle::InitKeyframesMap(
    size_t size) {
  if (keyframes_) {
    return keyframes_;
  }
  keyframes_ = std::make_shared<CSSKeyframesTokenMap>();
  keyframes_->reserve(size);
  return keyframes_;
}

CSSFontFaceRuleMap &LynxTemplateBundle::InitFontFacesMap(size_t size) {
  if (!font_face_rules_.empty()) {
    return font_face_rules_;
  }
  font_face_rules_.reserve(size);
  return font_face_rules_;
}

std::optional<std::shared_ptr<runtime::ContextBundle>>
LynxTemplateBundle::GetLepusChunk(const std::string &chunk_key) {
  auto chunk = lepus_chunk_manager_->GetLepusChunk(chunk_key);
  if (!chunk && lazy_reader_) {
    lazy_reader_->DecodeContextBundleInRender(chunk_key);
    chunk = lepus_chunk_manager_->GetLepusChunk(chunk_key);
  }
  return chunk;
}

bool LynxTemplateBundle::DecodeCSSFragmentById(int32_t fragment_id) {
  return lazy_reader_ &&
         lazy_reader_->DecodeCSSFragmentByIdInRender(fragment_id);
}

}  // namespace tasm
}  // namespace lynx
