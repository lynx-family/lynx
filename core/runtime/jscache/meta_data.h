// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_META_DATA_H_
#define CORE_RUNTIME_JSCACHE_META_DATA_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace piper {
namespace cache {
/**
 * Used to identify a js file.
 */
struct JsFileIdentifier {
  std::string category;      // source of js file: core_js, packaged, dynamic
  std::string url;           // url, e.g. '/app-service.js'
  std::string template_url;  // template url, e.g. 'https://xxx/template.js'

  bool operator==(const JsFileIdentifier &r) const {
    return category == r.category && url == r.url &&
           template_url == r.template_url;
  }
};

/**
 * Information of a js file and its cache.
 */
struct CacheFileInfo {
  CacheFileInfo() = default;
  CacheFileInfo(JsFileIdentifier identifier, uint64_t size_in_bytes,
                int64_t last_accessed, std::string md5)
      : identifier(std::move(identifier)),
        cache_size(size_in_bytes),
        last_accessed(last_accessed),
        md5(std::move(md5)) {}

  JsFileIdentifier identifier;  // identifier of js file
  uint64_t cache_size;          // in bytes
  int64_t last_accessed;        // seconds since epoch
  std::string md5;              // md5 of js file
};

class MetaData {
 public:
  // category
  static inline constexpr char CORE_JS[] = "core_js";
  static inline constexpr char PACKAGED[] = "packaged";
  static inline constexpr char DYNAMIC[] = "dynamic";

  // json keys & pointers
  static inline constexpr char POINTER_LYNX_VERSION[] = "/lynx/version";
  static inline constexpr char POINTER_ENGINE_VERSION[] =
      "/lynx/engine_sdk_version";
  static inline constexpr char POINTER_CACHE_FILES[] = "/cache_files";
  static inline constexpr char KEY_MD5[] = "md5";
  static inline constexpr char KEY_LAST_ACCESSED[] = "last_accessed";
  static inline constexpr char KEY_CACHE_SIZE[] = "cache_size";

  explicit MetaData(const std::string &lynx_version,
                    const std::string &engine_sdk_version);
  explicit MetaData(rapidjson::Document &&doc);

  MetaData(const MetaData &) = delete;
  MetaData &operator=(const MetaData &) = delete;

  MetaData(MetaData &&) = default;
  MetaData &operator=(MetaData &&) = default;

  static std::unique_ptr<MetaData> ParseJson(const std::string &json);
  std::string ToJson() const;

  std::optional<CacheFileInfo> GetFileInfo(
      const JsFileIdentifier &identifier) const;
  void UpdateFileInfo(const JsFileIdentifier &identifier,
                      const std::string &md5, uint64_t cache_size);
  bool UpdateLastAccessTimeIfExists(const JsFileIdentifier &identifier);

  void RemoveFileInfo(const JsFileIdentifier &identifier);

  std::vector<CacheFileInfo> GetAllCacheFileInfo(
      std::string_view template_url = {}) const;

  std::string GetLynxVersion() const;
  std::string GetBytecodeGenerateEngineVersion() const;

 private:
  const rapidjson::Value *GetValue(const JsFileIdentifier &identifier) const;
  rapidjson::Value *GetValue(const JsFileIdentifier &identifier);

  CacheFileInfo GetFileInfo(const JsFileIdentifier &identifier,
                            const rapidjson::Value &file_metadata) const;

  rapidjson::Document json_document_;
};

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_META_DATA_H_
