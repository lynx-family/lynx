// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/meta_data.h"

#include <chrono>
#include <memory>

#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/pointer.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace piper {
namespace cache {

namespace {
rapidjson::Value *GetOrAddObject(rapidjson::Value *parent,
                                 const std::string &key,
                                 rapidjson::Document::AllocatorType &alloc) {
  auto iter = parent->FindMember(key);
  if (iter != parent->MemberEnd()) {
    return &iter->value;
  }
  rapidjson::Value object(rapidjson::kObjectType);
  parent->AddMember(rapidjson::Value(key, alloc), object, alloc);
  return &(*parent)[key];
}

std::string JoinPointerPaths(const std::string &parent,
                             const std::string &children) {
  return parent + "/" + children;
}
}  // namespace

MetaData::MetaData(const std::string &lynx_version,
                   const std::string &engine_sdk_version)
    : json_document_(rapidjson::kObjectType) {
  rapidjson::Pointer(POINTER_LYNX_VERSION).Set(json_document_, lynx_version);
  rapidjson::Pointer(POINTER_ENGINE_VERSION)
      .Set(json_document_, engine_sdk_version);
}

MetaData::MetaData(rapidjson::Document &&doc) {
  json_document_ = std::move(doc);
}

std::unique_ptr<MetaData> MetaData::ParseJson(const std::string &json) {
  rapidjson::Document document;
  document.Parse(json.c_str());
  if (document.HasParseError()) {
    return nullptr;
  }

  // make sure lynx_version & engine_sdk_version are valid
  rapidjson::Value *value =
      rapidjson::Pointer(POINTER_LYNX_VERSION).Get(document);
  if (!value || !value->IsString()) {
    return nullptr;
  }
  value = rapidjson::Pointer(POINTER_ENGINE_VERSION).Get(document);
  if (!value || !value->IsString()) {
    return nullptr;
  }

  return std::make_unique<MetaData>(std::move(document));
}

std::string MetaData::ToJson() const {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_document_.Accept(writer);
  return buffer.GetString();
}

std::optional<CacheFileInfo> MetaData::GetFileInfo(
    const JsFileIdentifier &identifier) const {
  const rapidjson::Value *value = GetValue(identifier);
  if (value == nullptr || !value->IsObject()) {
    return std::nullopt;
  }
  return GetFileInfo(identifier, *value);
}

CacheFileInfo MetaData::GetFileInfo(
    const JsFileIdentifier &identifier,
    const rapidjson::Value &file_metadata) const {
  CacheFileInfo info;
  info.identifier = identifier;
  const auto &meta_info = file_metadata.GetObject();
  if (meta_info.HasMember(KEY_MD5) && meta_info[KEY_MD5].IsString()) {
    info.md5 = meta_info[KEY_MD5].GetString();
  }
  if (meta_info.HasMember(KEY_CACHE_SIZE) &&
      meta_info[KEY_CACHE_SIZE].IsUint64()) {
    info.cache_size = meta_info[KEY_CACHE_SIZE].GetUint64();
  }
  if (meta_info.HasMember(KEY_LAST_ACCESSED) &&
      meta_info[KEY_LAST_ACCESSED].IsInt64()) {
    info.last_accessed = meta_info[KEY_LAST_ACCESSED].GetInt64();
  }
  return info;
}

void MetaData::UpdateFileInfo(const JsFileIdentifier &identifier,
                              const std::string &md5, uint64_t cache_size) {
  rapidjson::Value *value = GetValue(identifier);
  if (value == nullptr) {
    auto pointer = JoinPointerPaths(POINTER_CACHE_FILES, identifier.category);
    value = rapidjson::Pointer(pointer).Get(json_document_);
    if (value == nullptr) {
      rapidjson::Value object(rapidjson::kObjectType);
      rapidjson::Pointer(pointer).Set(json_document_, object,
                                      json_document_.GetAllocator());
      value = rapidjson::Pointer(pointer).Get(json_document_);
    }

    if (identifier.category == PACKAGED) {
      value = GetOrAddObject(value, identifier.template_url,
                             json_document_.GetAllocator());
    }
    if (identifier.category == PACKAGED || identifier.category == DYNAMIC) {
      value =
          GetOrAddObject(value, identifier.url, json_document_.GetAllocator());
    }
  }

  value->RemoveAllMembers();
  value->AddMember(KEY_MD5, md5, json_document_.GetAllocator());
  value->AddMember(KEY_CACHE_SIZE, cache_size, json_document_.GetAllocator());
  int64_t time = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  value->AddMember(KEY_LAST_ACCESSED, time, json_document_.GetAllocator());
}

bool MetaData::UpdateLastAccessTimeIfExists(
    const JsFileIdentifier &identifier) {
  rapidjson::Value *value = GetValue(identifier);
  if (value == nullptr || !value->IsObject() ||
      !value->HasMember(KEY_LAST_ACCESSED)) {
    return false;
  }

  int64_t time = std::chrono::duration_cast<std::chrono::seconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  value->GetObject()[KEY_LAST_ACCESSED] = time;
  return true;
}

void MetaData::RemoveFileInfo(const JsFileIdentifier &identifier) {
  auto pointer = JoinPointerPaths(POINTER_CACHE_FILES, identifier.category);
  if (identifier.category == CORE_JS) {
    rapidjson::Pointer(pointer).Erase(json_document_);
    return;
  }

  rapidjson::Value *value = rapidjson::Pointer(pointer).Get(json_document_);
  if (value == nullptr) {
    return;
  }

  if (identifier.category == PACKAGED) {
    auto iter = value->FindMember(identifier.template_url);
    if (iter == value->MemberEnd() || !iter->value.IsObject()) {
      return;
    }
    iter->value.RemoveMember(identifier.url);
    if (iter->value.ObjectEmpty()) {
      value->RemoveMember(iter);
    }
  }

  if (identifier.category == DYNAMIC) {
    value->RemoveMember(identifier.url);
  }
}

std::vector<CacheFileInfo> MetaData::GetAllCacheFileInfo(
    std::string_view template_url_key) const {
  std::vector<CacheFileInfo> cfl;
  const rapidjson::Value *packaged_files =
      rapidjson::Pointer(
          JoinPointerPaths(POINTER_CACHE_FILES, PACKAGED).c_str())
          .Get(json_document_);
  if (packaged_files != nullptr && packaged_files->IsObject()) {
    for (auto &template_url : packaged_files->GetObject()) {
      if (template_url.value.IsObject() &&
          (template_url_key.empty() ||
           template_url_key == template_url.name.GetString())) {
        for (auto &url : template_url.value.GetObject()) {
          if (url.value.IsObject()) {
            JsFileIdentifier identifier;
            identifier.category = PACKAGED;
            identifier.url = url.name.GetString();
            identifier.template_url = template_url.name.GetString();
            auto file_info = GetFileInfo(identifier, url.value);
            cfl.emplace_back(std::move(file_info));
          }
        }
      }
    }
  }
  const rapidjson::Value *dynamic_files =
      rapidjson::Pointer(JoinPointerPaths(POINTER_CACHE_FILES, DYNAMIC).c_str())
          .Get(json_document_);
  if (dynamic_files != nullptr && dynamic_files->IsObject()) {
    for (auto &url : dynamic_files->GetObject()) {
      if (url.value.IsObject() && (template_url_key.empty() ||
                                   template_url_key == url.name.GetString())) {
        JsFileIdentifier identifier;
        identifier.category = DYNAMIC;
        identifier.url = url.name.GetString();
        identifier.template_url = "";
        auto file_info = GetFileInfo(identifier, url.value);
        cfl.emplace_back(std::move(file_info));
      }
    }
  }
  return cfl;
}

const rapidjson::Value *MetaData::GetValue(
    const JsFileIdentifier &identifier) const {
  auto pointer = JoinPointerPaths(POINTER_CACHE_FILES, identifier.category);
  const rapidjson::Value *value =
      rapidjson::Pointer(pointer).Get(json_document_);
  if (value == nullptr) {
    return nullptr;
  }

  if (identifier.category == PACKAGED) {
    auto iter = value->FindMember(identifier.template_url);
    if (iter == value->MemberEnd() || !iter->value.IsObject()) {
      return nullptr;
    }
    value = &iter->value;
  }
  if (identifier.category == PACKAGED || identifier.category == DYNAMIC) {
    auto iter = value->FindMember(identifier.url);
    if (iter == value->MemberEnd() || !iter->value.IsObject()) {
      return nullptr;
    }
    value = &iter->value;
  }
  return value;
}

rapidjson::Value *MetaData::GetValue(const JsFileIdentifier &identifier) {
  return const_cast<rapidjson::Value *>(
      const_cast<const MetaData *>(this)->GetValue(identifier));
}

std::string MetaData::GetLynxVersion() const {
  return rapidjson::Pointer(POINTER_LYNX_VERSION)
      .Get(json_document_)
      ->GetString();
}

std::string MetaData::GetBytecodeGenerateEngineVersion() const {
  return rapidjson::Pointer(POINTER_ENGINE_VERSION)
      .Get(json_document_)
      ->GetString();
}
}  // namespace cache
}  // namespace piper
}  // namespace lynx
