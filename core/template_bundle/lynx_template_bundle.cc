// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynx_template_bundle.h"

#include <algorithm>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_style_sheet_manager.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/css/unit_handler.h"
#include "core/renderer/simple_styling/style_object.h"
#include "core/runtime/lepus/binary_input_stream.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_decoder/element_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_reader.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "third_party/rapidjson/document.h"

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

static bool ParseCSSFromJSON(const std::string &css_json,
                             CSSParserTokenMap &css,
                             CSSKeyframesTokenMap &keyframes,
                             CSSFontFaceRuleMap &fontfaces) {
  if (css_json.empty()) {
    return true;
  }

  rapidjson::Document document;
  document.Parse(css_json.c_str());
  if (document.HasParseError() || !document.IsArray()) {
    return false;
  }

  CSSParserConfigs configs;
  for (const auto &rule : document.GetArray()) {
    if (!rule.IsObject()) {
      continue;
    }

    if (!rule.HasMember("type") || !rule["type"].IsString() ||
        std::string("StyleRule") != rule["type"].GetString()) {
      continue;
    }

    if (!rule.HasMember("selectorText") || !rule["selectorText"].IsObject() ||
        !rule["selectorText"].HasMember("value") ||
        !rule["selectorText"]["value"].IsString()) {
      continue;
    }
    std::string selector = rule["selectorText"]["value"].GetString();

    if (!rule.HasMember("style") || !rule["style"].IsArray()) {
      continue;
    }

    auto token = fml::MakeRefCounted<CSSParseToken>(configs);
    const auto &style_array = rule["style"].GetArray();
    for (const auto &attr : style_array) {
      if (!attr.IsObject() || !attr.HasMember("name") ||
          !attr["name"].IsString() || !attr.HasMember("value") ||
          !attr["value"].IsString()) {
        continue;
      }

      CSSPropertyID id = CSSProperty::GetPropertyID(attr["name"].GetString());
      if (!CSSProperty::IsPropertyValid(id)) {
        continue;
      }

      lepus::Value css_value(attr["value"].GetString());
      StyleMap output;
      UnitHandler::Process(id, css_value, output, configs);
      if (!output.empty()) {
        for (auto &pair : output) {
          token->attributes()[pair.first] = std::move(pair.second);
        }
      }
    }

    auto sheet = std::make_shared<CSSSheet>(selector);
    token->sheets().push_back(sheet);
    css.emplace(std::move(selector), std::move(token));
  }

  return true;
}

std::string LynxTemplateBundle::FromJSON(const std::string &json) {
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (document.HasParseError() || !document.IsObject()) {
    return "Failed to parse JSON";
  }

  std::string css_json;
  std::string lepus_source;
  std::string js_source;

  if (document.HasMember("css") && document["css"].IsString()) {
    css_json = document["css"].GetString();
  }
  if (document.HasMember("lepus.js") && document["lepus.js"].IsString()) {
    lepus_source = document["lepus.js"].GetString();
  }
  if (document.HasMember("app-service.js") &&
      document["app-service.js"].IsString()) {
    js_source = document["app-service.js"].GetString();
  }

  compile_options_.enable_fiber_arch_ = true;
  compile_options_.arch_option_ = ArchOption::FIBER_ARCH;
  compile_options_.enable_flexible_template_ = true;
  compile_options_.target_sdk_version_ = "3.0";
  is_lepusng_binary_ = true;
  context_type_ = runtime::ContextType::LepusNGContextType;
  app_type_ = APP_TYPE_CARD;
  page_configs_ = std::make_shared<PageConfig>();

  context_bundle_ =
      runtime::ContextBundle::Create(runtime::ContextType::LepusNGContextType);
  auto *quick_bundle =
      static_cast<lepus::QuickContextBundle *>(context_bundle_.get());
  quick_bundle->SetSourceCode(lepus_source);

  js_bundle_.AddJsContent(
      "/app-service.js",
      runtime::js::JsContent(std::move(js_source),
                             runtime::js::JsContent::Type::SOURCE));

  CSSParserTokenMap css;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  if (!ParseCSSFromJSON(css_json, css, keyframes, fontfaces)) {
    return "Failed to parse CSS JSON";
  }

  if (!css.empty()) {
    auto fragment = std::make_unique<SharedCSSFragment>(
        0, std::vector<int32_t>(), std::move(css), std::move(keyframes),
        std::move(fontfaces), css_style_manager_.get());
    css_style_manager_->AddSharedCSSFragment(std::move(fragment));
    css_style_manager_->FlattenAllCSSFragment();
  }

  return "";
}

}  // namespace tasm
}  // namespace lynx
