// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/core/performance/generic_info_storage.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace embedder {

void GenericInfo::UpdateGenericInfo(
    const std::unordered_map<std::string, std::string>& infos) {
  MarkDirty(true);
  str_infos_.insert(infos.begin(), infos.end());
}

void GenericInfo::UpdateGenericInfo(
    const std::unordered_map<std::string, float>& infos) {
  MarkDirty(true);
  float_infos_.insert(infos.begin(), infos.end());
}

void GenericInfo::UpdateGenericInfo(const std::string& key,
                                    const std::string& value) {
  MarkDirty(true);
  str_infos_[key] = value;
}

void GenericInfo::UpdateGenericInfo(const std::string& key, int64_t value) {
  MarkDirty(true);
  i64_infos_[key] = value;
}

void GenericInfo::UpdateGenericInfo(const std::string& key, float value) {
  MarkDirty(true);
  float_infos_[key] = value;
}

void GenericInfo::RemoveGenericInfo(const std::string& key) {
  MarkDirty(true);
  str_infos_.erase(key);
  i64_infos_.erase(key);
  float_infos_.erase(key);
}

std::unique_ptr<const pub::Value> GenericInfo::GetGenericInfo() {
  if (is_dirty_) {
    fml::RefPtr<lepus::Dictionary> dic = lynx::lepus::Dictionary::Create();
    {
      for (const auto& [key, value_str] : str_infos_) {
        dic->SetValue(key, value_str);
      }
    }

    {
      for (const auto& [key, value_i64] : i64_infos_) {
        dic->SetValue(key, value_i64);
      }
    }

    {
      for (const auto& [key, value_float] : float_infos_) {
        dic->SetValue(key, value_float);
      }
    }
    lepus_value_ = lepus::Value(std::move(dic));
    MarkDirty(false);
  }

  return std::make_unique<const pub::ValueImplLepus>(lepus_value_);
}

GenericInfoStorage& GenericInfoStorage::Instance() {
  static lynx::base::NoDestructor<GenericInfoStorage> instance;
  return *instance;
}

void GenericInfoStorage::UpdateGenericInfo(const std::string& key,
                                           const std::string& value,
                                           int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }

  generic_infos_.try_emplace(instance_id)
      .first->second.UpdateGenericInfo(key, value);
}

void GenericInfoStorage::UpdateGenericInfo(
    const std::unordered_map<std::string, std::string>& infos,
    int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }

  generic_infos_.try_emplace(instance_id)
      .first->second.UpdateGenericInfo(infos);
}

void GenericInfoStorage::UpdateGenericInfo(const std::string& key, float value,
                                           int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }

  generic_infos_.try_emplace(instance_id)
      .first->second.UpdateGenericInfo(key, value);
}

void GenericInfoStorage::UpdateGenericInfo(const std::string& key,
                                           int64_t value, int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }

  generic_infos_.try_emplace(instance_id)
      .first->second.UpdateGenericInfo(key, value);
}

void GenericInfoStorage::UpdateGenericInfo(
    const std::unordered_map<std::string, float>& infos, int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }

  generic_infos_.try_emplace(instance_id)
      .first->second.UpdateGenericInfo(infos);
}

void GenericInfoStorage::RemoveGenericInfo(const std::string& key,
                                           int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }

  auto it = generic_infos_.find(instance_id);
  if (it != generic_infos_.end()) {
    it->second.RemoveGenericInfo(key);
  }
}

std::unique_ptr<const pub::Value>
GenericInfoStorage::GetAllGenericInfosInReportThread(int32_t instance_id) {
  auto it = generic_infos_.find(instance_id);
  std::unique_ptr<const pub::Value> info = nullptr;
  if (it != generic_infos_.end()) {
    info = it->second.GetGenericInfo();
  }
  return info;
}

void GenericInfoStorage::ClearCache(int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }

  generic_infos_.erase(instance_id);
}

}  // namespace embedder
}  // namespace lynx
