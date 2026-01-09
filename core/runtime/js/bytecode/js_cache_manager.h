// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BYTECODE_JS_CACHE_MANAGER_H_
#define CORE_RUNTIME_JS_BYTECODE_JS_CACHE_MANAGER_H_

#include <stdint.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef QUICKJS_CACHE_UNITTEST
#include <thread>
#define UNITTEST_VIRTUAL virtual
#define UNITTEST_PUBLIC public
#else
#define UNITTEST_VIRTUAL
#define UNITTEST_PUBLIC private
#endif

#include "base/include/closure.h"
#include "core/public/page_options.h"
#include "core/runtime/js/bytecode/cache_generator.h"
#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/bytecode/meta_data.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace cache {
using BytecodeGenerateCallback = base::MoveOnlyClosure<
    void, std::string,
    std::unordered_map<std::string, std::shared_ptr<Buffer>>>;

class JsCacheManager {
 public:
  // Get js bytecode manager for quickjs.
  static JsCacheManager &GetQuickjsInstance() noexcept;

  // Get js bytecode manager for v8.
  static JsCacheManager &GetV8Instance() noexcept;

  static std::unique_ptr<BytecodeGenerateCallback> g_bytecode_generate_callback;

  /**
   * Struct of info needed to make a new js cache file.
   */
  struct TaskInfo {
    enum class TaskType {
      GENERATE_CACHE,
      GENERATE_CACHE_IF_NEEDED,
    } type;

    std::string template_key;  // template unique key
    std::vector<std::unique_ptr<CacheGenerator>>
        cache_generators;  // functions to generate cache
    std::unique_ptr<BytecodeGenerateCallback> callback;

    TaskInfo(TaskType type, std::string template_url,
             std::vector<std::unique_ptr<CacheGenerator>> generators,
             std::unique_ptr<BytecodeGenerateCallback> callback = nullptr)
        : type(type),
          template_key(std::move(template_url)),
          cache_generators(std::move(generators)),
          callback(std::move(callback)) {}
  };

  explicit JsCacheManager(JSRuntimeType type);
  UNITTEST_VIRTUAL ~JsCacheManager() = default;

  /**
   * Get cache if existed.
   * @param source_url url of js file.
   * @param template_url url of the template.
   * @param cache_generator function to generate cache.
   * @param bytecode_getter function provide bytecode from outside.
   * @return cache buffer; or nullptr if no cache existed.
   */
  std::shared_ptr<Buffer> TryGetCache(
      const std::string &source_url, const std::string &template_url,
      int64_t runtime_id, std::unique_ptr<CacheGenerator> cache_generator,
      BytecodeGetter *bytecode_getter = nullptr);

  /**
   * Request to generate a new cache file if it's not already existed. It's not
   * guaranteed that a valid cache file will be generated after the method call.
   *
   * Won't read cache file from storage, so it won't check if the cache file is
   * broken.
   * @param template_url url of the template.
   * @param generators functions to generate cache.
   * @param force If true, it will generate cache file even if it's already
   * existed.
   * @param callback this can be null. If it's not null, when exec finished,
   * pass result to it.
   */
  void RequestCacheGeneration(
      const std::string &template_url,
      std::vector<std::unique_ptr<CacheGenerator>> &&generators, bool force,
      std::unique_ptr<BytecodeGenerateCallback> callback = nullptr);

  /**
   * Remove bytecode by template_url.
   * This will remove all app-service.js and dynamic component's bytecode whose
   * url == template_url
   * If template_url_key is empty. this will remove all cache.
   */
  void ClearCache(const std::string &template_url_key);

  /**
   * Clear expired cache.
   */
  void ClearInvalidCache();

  JsCacheManager(const JsCacheManager &) = delete;
  void operator=(const JsCacheManager &) = delete;

  // clang-format off
UNITTEST_PUBLIC:

      // clang-format on
      class LockedMetaData {
   public:
    LockedMetaData(std::recursive_mutex &lock,
                   std::unique_ptr<MetaData> &meta_data)
        : guard_(lock), meta_data_(meta_data) {}
    MetaData *operator->() { return meta_data_.get(); }
    const MetaData *operator->() const { return meta_data_.get(); }
    MetaData &operator*() { return *meta_data_; }
    const MetaData &operator*() const { return *meta_data_; }

   private:
    std::lock_guard<std::recursive_mutex> guard_;
    std::unique_ptr<MetaData> &meta_data_;
  };

  void InitMetaData();
  LockedMetaData GetLockedMetaData();

  bool ReadFile(const std::string &filename, std::string &contents);
  UNITTEST_VIRTUAL bool WriteFile(const std::string &filename, uint8_t *out_buf,
                                  size_t out_buf_len);
  std::string MakePath(const std::string &filename);
  UNITTEST_VIRTUAL std::string GetCacheDir();

  /**
   * Generate file name using md5 of file.
   * @param file_md5 Md5 of the file.
   */
  std::string MakeFilename(const std::string &file_md5);

  /**
   * Check if cache is enabled for the template.
   */
  bool IsCacheEnabled();

  /**
   * Post a task into background thread.
   *
   * @param task the task to run.
   */
  void PostTaskBackground(TaskInfo task);

  /**
   * Generate cache file and save to storage.
   * @param task info of the cache generation task.
   */
  void RunTask(TaskInfo task);

  /**
   * Try to load cache file from storage.
   * @param identifier The identifier of the original js file.
   * @param file_md5 The md5 of the js file.
   * @return Cache buffer; or nullptr if no matching cache exists.
   */
  std::shared_ptr<Buffer> LoadCacheFromStorage(JsFileIdentifier &identifier,
                                               const std::string &file_md5);

  /**
   * Save the cache content to storage.
   * @param identifier The identifier of the js file.
   * @param cache The cache to store.
   * @param file_md5 The md5 of the source js file.
   * @return If the save operation succeed.
   */
  bool SaveCacheContentToStorage(const JsFileIdentifier &identifier,
                                 const std::shared_ptr<Buffer> &cache,
                                 const std::string &file_md5,
                                 JsCacheErrorCode &error_code);

  /**
   * Get the maximum time to keep cache in storage.
   */
  int64_t ExpiredSeconds();

  /**
   * Get the maximum cache size, unit is byte.
   */
  UNITTEST_VIRTUAL size_t MaxCacheSize();

  /**
   * Update last access time. Metadata will be updated in memory in all cases,
   * but will be written to storage only when (now - last_accessed) >=
   * MIN_ACCESS_TIME_UPDATE_INTERVAL.
   * @param locked_meta_data the meta data
   * @param info Info containing the time when this file was
   * accessed.
   */
  void UpdateLastAccessTime(JsCacheManager::LockedMetaData &locked_meta_data,
                            const CacheFileInfo info);

  /**
   * enumerate files in cache directory.
   */
  void EnumerateFile(base::MoveOnlyClosure<void, const std::string &> func);

  /**
   * clear all cache files in cache directory.
   */
  void ClearCacheDir();

  /**
   * Calculate MD5.
   * If md5_str is empty, calculate MD5 and return.
   * If md5_str is not empty, return md5_str directly.
   * @param buffer buffer to calculate MD5.
   * @param md5_str either empty_str or calculated MD5 result.
   * @returns MD5 result.
   */
  const std::string &EnsureMd5(const std::shared_ptr<const Buffer> &buffer,
                               std::optional<std::string> &md5_str);

  /**
   * Save cache to memory.
   */
  void SaveCacheToMemory(const std::string &source_url,
                         std::shared_ptr<Buffer> cache) {
    std::scoped_lock<std::recursive_mutex> guard(cache_lock_);
    cache_[source_url] = std::move(cache);
  }

  bool IsCacheEnabledForTemplate(const std::string &source_url);

  JsFileIdentifier BuildIdentifier(const std::string &source_url,
                                   const std::string &template_url);

  std::string GetSourceCategory(const std::string &source_url);

  std::string CacheDirName();
  std::string GetPlatformCacheDir();
  UNITTEST_VIRTUAL std::string GetBytecodeGenerateEngineVersion();
  void DoPostTask(base::closure task);

  JSRuntimeType engine_type_;
  std::unordered_set<std::string> task_set_;
  std::mutex task_set_lock_;  // lock for task_list_
  std::unordered_map<std::string, std::shared_ptr<Buffer>> cache_;
  std::recursive_mutex cache_lock_;
  std::once_flag meta_data_init_flag_;
  std::unique_ptr<MetaData> meta_data_;
  std::string cache_path_;
  bool can_create_cache_ = true;
};

#undef UNITTEST_VIRTUAL
#undef UNITTEST_PUBLIC

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BYTECODE_JS_CACHE_MANAGER_H_
