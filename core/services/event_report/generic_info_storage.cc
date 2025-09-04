// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "lynx/core/services/event_report/generic_info_storage.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "lynx/core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace report {

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

std::unique_ptr<pub::Value> GenericInfo::GetGenericInfo() {
  if (is_dirty) {
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
    lepus_value = lepus::Value(std::move(dic));
    MarkDirty(false);
  }

  return std::make_unique<pub::ValueImplLepus>(lepus_value);
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

  RunTaskInReportThread([key, value, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it == generic_infos_.end()) {
      auto new_info = GenericInfo();
      new_info.UpdateGenericInfo(key, value);
      generic_infos_.emplace(instance_id, std::move(new_info));
    } else {
      it->second.UpdateGenericInfo(key, value);
    }
  });
}

void GenericInfoStorage::UpdateGenericInfo(
    const std::unordered_map<std::string, std::string>& infos,
    int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }
  RunTaskInReportThread([infos, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it == generic_infos_.end()) {
      auto new_info = GenericInfo();
      new_info.UpdateGenericInfo(infos);
      generic_infos_.emplace(instance_id, std::move(new_info));
    } else {
      it->second.UpdateGenericInfo(infos);
    }
  });
}

void GenericInfoStorage::UpdateGenericInfo(const std::string& key, float value,
                                           int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }
  RunTaskInReportThread([key, value, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it == generic_infos_.end()) {
      auto new_info = GenericInfo();
      new_info.UpdateGenericInfo(key, value);
      generic_infos_.emplace(instance_id, std::move(new_info));
    } else {
      it->second.UpdateGenericInfo(key, value);
    }
  });
}

void GenericInfoStorage::UpdateGenericInfo(const std::string& key,
                                           int64_t value, int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }
  RunTaskInReportThread([key, value, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it == generic_infos_.end()) {
      auto new_info = GenericInfo();
      new_info.UpdateGenericInfo(key, value);
      generic_infos_.emplace(instance_id, std::move(new_info));
    } else {
      it->second.UpdateGenericInfo(key, value);
    }
  });
}

void GenericInfoStorage::UpdateGenericInfo(
    const std::unordered_map<std::string, float>& infos, int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }
  RunTaskInReportThread([infos, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it == generic_infos_.end()) {
      auto new_info = GenericInfo();
      new_info.UpdateGenericInfo(infos);
      generic_infos_.emplace(instance_id, std::move(new_info));
    } else {
      it->second.UpdateGenericInfo(infos);
    }
  });
}

void GenericInfoStorage::RemoveGenericInfo(const std::string& key,
                                           int32_t instance_id) {
  if (key.empty() || instance_id < 0) {
    return;
  }
  RunTaskInReportThread([key, instance_id, this]() {
    auto it = generic_infos_.find(instance_id);
    if (it != generic_infos_.end()) {
      it->second.RemoveGenericInfo(key);
    }
  });
}

void GenericInfoStorage::RunTaskInReportThread(base::closure task) {
  auto task_runner = EventTrackerPlatformImpl::GetReportTaskRunner();
  if (task_runner) {
    if (task_runner->RunsTasksOnCurrentThread()) {
      task();
    } else {
      task_runner->PostTask([task = std::move(task)]() { task(); });
    }
  }
}

void GenericInfoStorage::GetAllGenericInfosInReportThread(
    int32_t instance_id,
    const std::function<void(std::unique_ptr<pub::Value> entry)>&
        on_get_generic_infos_cb) {
  if (!on_get_generic_infos_cb) {
    return;
  }

  RunTaskInReportThread([instance_id, on_get_generic_infos_cb, this]() {
    auto it = generic_infos_.find(instance_id);
    std::unique_ptr<pub::Value> info = nullptr;
    if (it != generic_infos_.end()) {
      info = it->second.GetGenericInfo();
    }
    on_get_generic_infos_cb(std::move(info));
  });
}

void GenericInfoStorage::ClearCache(int32_t instance_id) {
  if (instance_id < 0) {
    return;
  }
  RunTaskInReportThread(
      [instance_id, this]() { generic_infos_.erase(instance_id); });
}

}  // namespace report
}  // namespace tasm
}  // namespace lynx
