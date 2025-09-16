// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_CORE_PERFORMANCE_GENERIC_INFO_STORAGE_H_
#define PLATFORM_EMBEDDER_CORE_PERFORMANCE_GENERIC_INFO_STORAGE_H_

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/value/base_value.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace embedder {

class GenericInfo {
 public:
  GenericInfo() {}
  GenericInfo(const GenericInfo&) = default;
  GenericInfo& operator=(const GenericInfo&) = delete;
  GenericInfo(GenericInfo&&) = default;
  GenericInfo& operator=(GenericInfo&&) = default;

  void UpdateGenericInfo(
      const std::unordered_map<std::string, std::string>& infos);
  void UpdateGenericInfo(const std::unordered_map<std::string, float>& infos);
  void UpdateGenericInfo(const std::string& key, const std::string& value);
  void UpdateGenericInfo(const std::string& key, int64_t value);
  void UpdateGenericInfo(const std::string& key, float value);
  void RemoveGenericInfo(const std::string& key);
  std::unique_ptr<const pub::Value> GetGenericInfo();

 private:
  void MarkDirty(bool dirty) { is_dirty_ = dirty; };

 private:
  std::unordered_map<std::string, std::string> str_infos_;
  std::unordered_map<std::string, int64_t> i64_infos_;
  std::unordered_map<std::string, float> float_infos_;

  lepus::Value lepus_value_;
  bool is_dirty_ = false;
};

class GenericInfoStorage {
 public:
  static GenericInfoStorage& Instance();

  void UpdateGenericInfo(const std::string& key, const std::string& value,
                         int32_t instance_id);

  void UpdateGenericInfo(
      const std::unordered_map<std::string, std::string>& infos,
      int32_t instance_id);

  void UpdateGenericInfo(const std::string& key, float value,
                         int32_t instance_id);

  void UpdateGenericInfo(const std::unordered_map<std::string, float>& infos,
                         int32_t instance_id);

  void UpdateGenericInfo(const std::string& key, int64_t value,
                         int32_t instance_id);

  void RemoveGenericInfo(const std::string& key, int32_t instance_id);

  std::unique_ptr<const pub::Value> GetAllGenericInfosInReportThread(
      int32_t instance_id);

  void ClearCache(int32_t instance_id);

  void ClearAll() { generic_infos_.clear(); }

 private:
  // {instance_id -> generic_infos}
  std::unordered_map<int32_t, GenericInfo> generic_infos_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_PERFORMANCE_GENERIC_INFO_STORAGE_H_
