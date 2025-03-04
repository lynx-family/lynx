// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_H_
#define CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

#ifdef QUICKJS_CACHE_UNITTEST
#include <thread>
#define UNITTEST_VIRTUAL virtual
#define UNITTEST_PUBLIC public
#else
#define UNITTEST_VIRTUAL
#define UNITTEST_PUBLIC private
#endif

#include "base/include/closure.h"
#include "core/runtime/jscache/cache_generator.h"
#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/jscache/meta_data.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
namespace cache {

class JsCacheManager {
 public:
  // Get js bytecode manager for quickjs.
  static JsCacheManager &GetQuickjsInstance() noexcept;

  // Get js bytecode manager for v8.
  static JsCacheManager &GetV8Instance() noexcept;

  /**
   * Struct of info needed to make a new js cache file.
   */
  struct TaskInfo {
    enum class TaskType {
      GENERATE_CACHE,
      GENERATE_CACHE_IF_NEEDED,
    } type;

    JsFileIdentifier identifier;              // information of source js file
    std::optional<std::string> md5_optional;  // md5 of source js file
    std::shared_ptr<const Buffer> js_buffer;  // source code of js file
    std::unique_ptr<CacheGenerator>
        cache_generator;  // function to generate cache

    TaskInfo(TaskType type, JsFileIdentifier identifier,
             std::optional<std::string> md5_optional,
             std::shared_ptr<const Buffer> js_buffer,
             std::unique_ptr<CacheGenerator> generator)
        : type(type),
          identifier(std::move(identifier)),
          md5_optional(std::move(md5_optional)),
          js_buffer(std::move(js_buffer)),
          cache_generator(std::move(generator)) {}
  };

  explicit JsCacheManager(JSRuntimeType type);
  UNITTEST_VIRTUAL ~JsCacheManager() = default;

  /**
   * Get cache if existed.
   * @param source_url url of js file.
   * @param template_url url of the template.
   * @param buffer source code of js file.
   * @param cache_generator function to generate cache.
   * @return cache buffer; or nullptr if no cache existed.
   */
  std::shared_ptr<Buffer> TryGetCache(
      const std::string &source_url, const std::string &template_url,
      int64_t runtime_id, const std::shared_ptr<const Buffer> &buffer,
      std::unique_ptr<CacheGenerator> cache_generator);

  /**
   * Request to generate a new cache file if it's not already existed. It's not
   * guaranteed that a valid cache file will be generated after the method call.
   *
   * Won't read cache file from storage, so it won't check if the cache file is
   * broken.
   * @param source_url url of js file.
   * @param template_url url of the template.
   * @param buffer source code of js file.
   * @param cache_generator function to generate cache.
   * @param force If true, it will generate cache file even if it's already
   * existed.
   */
  void RequestCacheGeneration(const std::string &source_url,
                              const std::string &template_url,
                              const std::shared_ptr<const Buffer> &buffer,
                              std::unique_ptr<CacheGenerator> cache_generator,
                              bool force);

  /**
   * Remove bytecode by template_url.
   * This will remove all app-service.js and dynamic component's bytecode whose
   * url == template_url
   * If template_url_key is empty. this will remove all cache.
   */
  void ClearCache(std::string_view template_url_key);

  /**
   * Clear expired cache.
   */
  void ClearInvalidCache();

  JsCacheManager(const JsCacheManager &) = delete;
  void operator=(const JsCacheManager &) = delete;

  // clang-format off
UNITTEST_PUBLIC:
      // clang-format on
      bool
      ReadFile(const std::string &filename, std::string &contents);
  UNITTEST_VIRTUAL bool WriteFile(const std::string &filename, uint8_t *out_buf,
                                  size_t out_buf_len);
  std::string MakePath(const std::string &filename);
  UNITTEST_VIRTUAL std::string GetCacheDir();

  /**
   * Generate file name using md5 of file.
   * @param file_md5 Md5 of the file.
   */
  std::string MakeFilename(const std::string &file_md5);

  bool IsCacheEnabled();
  /**
   * Post a task into background thread. If the background thread is not
   * running, start a new background task.
   *
   * @param task the task to run.
   */
  void PostTaskBackground(TaskInfo task);

  /**
   * Adjust the task list with a new task.
   *
   * @param task the new task to run.
   */
  void AdjustTaskListWithNewTask(TaskInfo task);

  /**
   * Run the tasks in task queue.
   */
  void RunTasks();

  /**
   * Generate cache file and save to storage.
   * @param task info of the cache generation task.
   * @return If the task succeed.
   */
  bool RunTask(TaskInfo &task);

  /**
   * Try to load cache file from storage.
   * @param info The info of the original js file.
   * @param file_md5 The md5 of the js file.
   * @return Cache buffer; or nullptr if no matching cache exists.
   */
  std::shared_ptr<Buffer> LoadCacheFromStorage(const CacheFileInfo &info,
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
   * @param info Info containing the time when this file was
   * accessed.
   * @return If this operation succeed.
   */
  bool UpdateLastAccessTime(const CacheFileInfo &info);

  /**
   * Get metadata from memory.
   * Load it from file or create a new one if it does not exist.
   * @return MetaData
   */
  MetaData &GetMetaData();

  /**
   * Load metadata from storage if not loaded.
   * If metadata doesn't exist on storage, clear cache dir first, and then
   * create a new metadata in memory.
   */
  void LoadMetadataIfNotLoaded();

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
    cache_[source_url] = std::move(cache);
  }

  bool IsCacheEnabledForTemplate(const std::string &source_url);

  JsFileIdentifier BuildIdentifier(const std::string &source_url,
                                   const std::string &template_url);

  std::string GetSourceCategory(const std::string &source_url);

  std::string CacheDirName();
  std::string GetPlatformCacheDir();
  UNITTEST_VIRTUAL std::string GetBytecodeGenerateEngineVersion();

  JSRuntimeType engine_type_;
  std::list<TaskInfo> task_list_;
  std::mutex task_lock_;  // lock for task_list_
  bool background_thread_working_ = false;
  std::unordered_map<std::string, std::shared_ptr<Buffer>> cache_;
  std::mutex cache_lock_;
  std::unique_ptr<MetaData> meta_data_;
  std::string cache_path_;
  bool can_create_cache_ = true;
};

#undef UNITTEST_VIRTUAL
#undef UNITTEST_PUBLIC

}  // namespace cache
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSCACHE_JS_CACHE_MANAGER_H_
