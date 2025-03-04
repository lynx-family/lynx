// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/js_cache_manager.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/log/logging.h"
#include "base/include/md5.h"
#include "base/include/no_destructor.h"
#include "base/include/path_utils.h"
#include "base/include/string/string_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/base/utils/file_utils.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jscache/js_cache_tracker.h"
#include "core/runtime/piper/js/runtime_constant.h"
#include "quickjs/include/quickjs.h"

#if defined(OS_ANDROID)
#include "core/base/android/android_jni.h"
#endif

#if defined(OS_WIN)
#include <io.h>

#include <cstdio>

#include "core/base/utils/paths_win.h"
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace lynx {
namespace piper {
namespace cache {

static constexpr size_t MAX_SIZE = 50 * 1024 * 1024;  // 50MB

constexpr char METADATA_FILE_NAME[] = "meta.json";
constexpr auto MIN_ACCESS_TIME_UPDATE_INTERVAL = std::chrono::hours(24);

JsCacheManager &JsCacheManager::GetQuickjsInstance() noexcept {
  static base::NoDestructor<JsCacheManager> instance(JSRuntimeType::quickjs);
  return *instance;
}

// Get js bytecode manager for v8.
JsCacheManager &JsCacheManager::GetV8Instance() noexcept {
  static base::NoDestructor<JsCacheManager> instance(JSRuntimeType::v8);
  return *instance;
}

JsCacheManager::JsCacheManager(JSRuntimeType type) : engine_type_(type) {}

bool JsCacheManager::ReadFile(const std::string &filename,
                              std::string &contents) {
  const std::string file_path = MakePath(filename);
  if (file_path.empty()) {
    LOGE("ReadFile failed (file_path is empty): " << filename);
    return false;
  }

  if (!base::FileUtils::ReadFileBinary(file_path, MAX_SIZE, contents)) {
    LOGE("ReadFile failed: " << file_path);
    return false;
  }
  return true;
}

bool JsCacheManager::WriteFile(const std::string &filename, uint8_t *out_buf,
                               size_t out_buf_len) {
  std::string file_path = MakePath(filename);
  if (file_path.empty()) {
    LOGE("WriteFile failed (file_path is empty): " << filename);
    return false;
  }

  // write to temp file
  std::string temp_file_path = MakePath("temp.tmp");
  if (!base::FileUtils::WriteFileBinary(temp_file_path, out_buf, out_buf_len)) {
    LOGE("WriteFile failed: " << file_path);
    return false;
  }

  // rename temp file to dest file
  remove(file_path.c_str());
  if (rename(temp_file_path.c_str(), file_path.c_str())) {
    remove(temp_file_path.c_str());
    LOGE("WriteFile failed (rename file failed): " << file_path);
    return false;
  }
  return true;
}

std::string JsCacheManager::MakeFilename(const std::string &file_md5) {
  return file_md5 + ".cache";
}

std::string JsCacheManager::GetCacheDir() {
  if (!cache_path_.empty() || !IsCacheEnabled()) {
    return cache_path_;
  }
#if defined(OS_WIN)
  auto [result, cache_dir] = lynx::base::GetExecutableDirectoryPath();
  if (!result) {
    can_create_cache_ = false;
    cache_path_ = "";
    return cache_path_;
  }

  auto cache_path = lynx::base::JoinPaths({cache_dir, CacheDirName()});
  if (lynx::base::DirectoryExists(cache_path)) {
    cache_path_ = cache_path;
    return cache_path_;
  }

  if (lynx::base::CreateDir(cache_path)) {
    LOGI("js_cache_dir created:" << cache_path);
  } else {
    LOGE("js_cache_dir create failed:");
    can_create_cache_ = false;
    cache_path_ = "";
    return cache_path_;
  }
  cache_path_ = cache_path;
  return cache_path_;

#else
  std::string cache_dir = GetPlatformCacheDir();
  std::string cache_path =
      base::PathUtils::JoinPaths({cache_dir, CacheDirName()});
  if (access(cache_path.c_str(), R_OK) == 0) {
    cache_path_ = cache_path;
    return cache_path_;
  }
  int is_created = mkdir(cache_path.c_str(),
                         S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
  if (!is_created) {
    LOGI("js_cache_dir created:" << cache_path);
  } else {
    LOGE("js_cache_dir create failed:" << is_created);
    can_create_cache_ = false;
    cache_path_ = "";
    return cache_path_;
  }
  cache_path_ = cache_path;
  return cache_path_;
#endif  // defined(OS_WIN)
}

std::string JsCacheManager::MakePath(const std::string &filename) {
  const std::string &cache_dir = GetCacheDir();
  if (cache_dir.empty()) {
    return cache_dir;
  }
  return base::PathUtils::JoinPaths({GetCacheDir(), filename});
}

bool JsCacheManager::IsCacheEnabled() {
  // TODO(zhenziqi) consider rename `IsQuickjsCacheEnabled` later
  // as this switch should also control the cache of v8 in the future
  return !tasm::LynxEnv::GetInstance().IsDevToolEnabled() &&
         tasm::LynxEnv::GetInstance().IsQuickjsCacheEnabled() &&
         can_create_cache_;
}

// JsCacheManager

//
// request thread
//
std::shared_ptr<Buffer> JsCacheManager::TryGetCache(
    const std::string &source_url, const std::string &template_url,
    int64_t runtime_id, const std::shared_ptr<const Buffer> &buffer,
    std::unique_ptr<CacheGenerator> cache_generator) {
  auto cost_start = base::CurrentTimeMilliseconds();
  if (!IsCacheEnabledForTemplate(template_url)) {
    JsCacheTracker::OnGetBytecodeDisable(runtime_id, engine_type_, source_url,
                                         true, false);
    return nullptr;
  }

  LOGI("bytecode enabled"
       << ", url: '" << source_url << "', template_url: '" << template_url
       << "', file_content size:" << buffer->size());

  TRACE_EVENT(LYNX_TRACE_CATEGORY, "JsCacheManager::TryGetCache",
              [&source_url](lynx::perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("source_url");
                debug->set_string_value(source_url);
              });

  std::optional<std::string> md5_optional;
  std::scoped_lock<std::mutex> lock(cache_lock_);

  // try to load cache from memory
  if (runtime::IsKernelJs(source_url) && !cache_.empty()) {
    auto cache_it = cache_.find(source_url);
    if (cache_it != cache_.end()) {
      LOGI("cache loaded from memory, size: " << cache_it->second->size()
                                              << " bytes");
      JsCacheTracker::OnGetBytecode(
          runtime_id, engine_type_, source_url, true, true, true,
          JsCacheType::MEMORY, JsCacheErrorCode::NO_ERROR,
          base::CurrentTimeMilliseconds() - cost_start,
          cache_it->second->size());
      return cache_it->second;
    }
  }

  auto identifier = BuildIdentifier(source_url, template_url);
  auto file_info = GetMetaData().GetFileInfo(identifier);
  JsCacheErrorCode error_code = JsCacheErrorCode::META_FILE_READ_ERROR;
  if (file_info) {
    auto cache =
        LoadCacheFromStorage(*file_info, EnsureMd5(buffer, md5_optional));
    if (cache) {
      LOGI("cache loaded from storage, size: " << cache->size() << " bytes");
      if (runtime::IsKernelJs(source_url)) {
        SaveCacheToMemory(source_url, cache);
        LOGV("loaded cache saved to memory");
      }
      JsCacheTracker::OnGetBytecode(
          runtime_id, engine_type_, source_url, true, true, true,
          JsCacheType::FILE, JsCacheErrorCode::NO_ERROR,
          base::CurrentTimeMilliseconds() - cost_start, cache->size());
      return cache;
    } else {
      error_code = JsCacheErrorCode::CACHE_FILE_READ_ERROR;
    }
  }

  JsCacheTracker::OnGetBytecode(runtime_id, engine_type_, source_url, true,
                                true, false, JsCacheType::NONE, error_code,
                                base::CurrentTimeMilliseconds() - cost_start,
                                0);
  LOGI("no cache matches this url.");

  PostTaskBackground(TaskInfo(TaskInfo::TaskType::GENERATE_CACHE,
                              std::move(identifier), std::move(md5_optional),
                              buffer, std::move(cache_generator)));
  return nullptr;
}

void JsCacheManager::RequestCacheGeneration(
    const std::string &source_url, const std::string &template_url,
    const std::shared_ptr<const Buffer> &buffer,
    std::unique_ptr<CacheGenerator> cache_generator, bool force) {
  LOGI("RequestCacheGeneration url: '"
       << source_url << "', template_url: '" << template_url
       << "', file_content size:" << buffer->size());
  if (!IsCacheEnabledForTemplate(template_url)) {
    LOGI("bytecode disabled");
    return;
  }

  auto identifier = BuildIdentifier(source_url, template_url);
  std::optional<std::string> md5_optional;
  PostTaskBackground(
      TaskInfo(force ? TaskInfo::TaskType::GENERATE_CACHE
                     : TaskInfo::TaskType::GENERATE_CACHE_IF_NEEDED,
               std::move(identifier), std::move(md5_optional), buffer,
               std::move(cache_generator)));
}

/*
 * 1. Check if the task is already inserted into the task list. If yes,
 * return.
 * 2. Insert the task into the list.
 * 3. If make_cache_thread is not running, start it.
 */
void JsCacheManager::PostTaskBackground(TaskInfo task) {
  std::scoped_lock<std::mutex> lock(task_lock_);

  AdjustTaskListWithNewTask(std::move(task));

  if (task_list_.empty()) {
    return;
  }

  // start background thread if not started
  if (!background_thread_working_) {
    LOGI("start background thread to make cache");
    background_thread_working_ = true;

#ifdef QUICKJS_CACHE_UNITTEST
    fml::AutoResetWaitableEvent latch;
#endif
    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [this
#ifdef QUICKJS_CACHE_UNITTEST
         ,
         &latch
#endif
    ] {
          RunTasks();
#ifdef QUICKJS_CACHE_UNITTEST
          latch.Signal();
#endif
        },
        base::ConcurrentTaskType::NORMAL_PRIORITY);

#ifdef QUICKJS_CACHE_UNITTEST
    task_lock_.unlock();
    cache_lock_.unlock();
    latch.Wait();
#endif
  }
}

void JsCacheManager::AdjustTaskListWithNewTask(TaskInfo task) {
  auto iter = std::find_if(task_list_.begin(), task_list_.end(),
                           [&task](const TaskInfo &existed_task) {
                             return existed_task.identifier == task.identifier;
                           });

  // no task with same identifier exists, insert it
  if (iter == task_list_.end()) {
    task_list_.push_back(std::move(task));
    return;
  }

  // task with same identifier exists, replace it if the new task is more
  // promising. Otherwise ignore it.
  if (iter->type == TaskInfo::TaskType::GENERATE_CACHE_IF_NEEDED &&
      task.type == TaskInfo::TaskType::GENERATE_CACHE) {
    *iter = std::move(task);
  } else {
    LOGI("task already exists, ignore");
  }
}

//
// background thread
//

void JsCacheManager::RunTasks() {
#if defined(OS_ANDROID)
  base::android::AttachCurrentThread();
#endif

  while (true) {
    // 1. get task
    std::optional<TaskInfo> task;
    {
      std::scoped_lock<std::mutex> lock(task_lock_);
      if (task_list_.empty()) {
        background_thread_working_ = false;
#if defined(OS_ANDROID)
        base::android::DetachFromVM();
#endif
        return;
      }
      task = std::move(task_list_.front());
      task_list_.pop_front();
    }

    // 2. run task
    RunTask(*task);
  }
}

bool JsCacheManager::RunTask(TaskInfo &task) {
  auto start = base::CurrentTimeMilliseconds();

  auto &[type, identifier, md5_optional, buffer, generator] = task;

  if (type == TaskInfo::TaskType::GENERATE_CACHE_IF_NEEDED) {
    if (auto info = GetMetaData().GetFileInfo(identifier)) {
      if (auto cache =
              LoadCacheFromStorage(*info, EnsureMd5(buffer, md5_optional))) {
        return true;
      }
    }
  }
  std::string file_md5 = EnsureMd5(buffer, md5_optional);

  LOGI("RunTask start"
       << ", url: '" << identifier.url << "', template_url: '"
       << identifier.template_url << "', file_md5: " << file_md5
       << ", buffer size: " << buffer->size() << " bytes");

  std::shared_ptr<Buffer> cache_buffer(generator->GenerateCache());
  if (!cache_buffer) {
    LOGE("GenerateCacheBuffer failed!");
    JsCacheTracker::OnGenerateBytecodeFailed(
        engine_type_, identifier.url, identifier.template_url,
        GetBytecodeGenerateEngineVersion(),
        JsCacheErrorCode::RUNTIME_GENERATE_FAILED);
    return false;
  }
  auto generate_cost = base::CurrentTimeMilliseconds() - start;

  std::scoped_lock<std::mutex> guard(cache_lock_);
  if (runtime::IsKernelJs(identifier.url)) {
    SaveCacheToMemory(identifier.url, cache_buffer);
  }

  JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
  auto persist_success =
      SaveCacheContentToStorage(identifier, cache_buffer, file_md5, error_code);
  JsCacheTracker::OnGenerateBytecode(
      engine_type_, identifier.url, identifier.template_url, true,
      task.js_buffer->size(), cache_buffer->size(), persist_success,
      GetBytecodeGenerateEngineVersion(), generate_cost, error_code);
  LOGI("MakeCache success:"
       << persist_success << ", cache size: " << cache_buffer->size()
       << " bytes, time spent: " << (base::CurrentTimeMilliseconds() - start)
       << " ms");
  return persist_success;
}

std::shared_ptr<Buffer> JsCacheManager::LoadCacheFromStorage(
    const CacheFileInfo &file_info, const std::string &file_md5) {
  std::string cache;
  if (file_info.md5 != file_md5 || !ReadFile(MakeFilename(file_md5), cache) ||
      cache.size() != file_info.cache_size) {
    if (file_info.md5 != file_md5) {
      LOGI("js file md5 mismatch.");
    } else {
      LOGI("cache file broken. cache size read from storage: "
           << cache.size()
           << ", size record in metadata: " << file_info.cache_size);
    }
    std::string path = MakePath(MakeFilename(file_info.md5));
    unlink(path.c_str());
    GetMetaData().RemoveFileInfo(file_info.identifier);
    // there's no need to save metadata to storage here. it will be saved
    // in the progress of generating cache later.
    return nullptr;
  }

  UpdateLastAccessTime(file_info);
  return std::make_shared<StringBuffer>(std::move(cache));
}

bool JsCacheManager::SaveCacheContentToStorage(
    const JsFileIdentifier &identifier, const std::shared_ptr<Buffer> &cache,
    const std::string &file_md5, JsCacheErrorCode &error_code) {
  LOGI("SaveCacheContentToStorage template_url=' "
       << identifier.template_url << "', url='" << identifier.url << "'");
  GetMetaData().UpdateFileInfo(identifier, file_md5, cache->size());
  std::string json = GetMetaData().ToJson();
  LOGV("metadata: " << json);
  if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                 json.size())) {
    LOGE("Write Metadata failed!");
    error_code = JsCacheErrorCode::META_FILE_WRITE_ERROR;
    return false;
  }
  if (!WriteFile(MakeFilename(file_md5), const_cast<uint8_t *>(cache->data()),
                 cache->size())) {
    LOGE("Write Cache File failed!");
    error_code = JsCacheErrorCode::CACHE_FILE_WRITE_ERROR;
    return false;
  }
  return true;
}

// update only when (now - last_accessed) >= MIN_ACCESS_TIME_UPDATE_INTERVAL.
bool JsCacheManager::UpdateLastAccessTime(const CacheFileInfo &info) {
  auto now = std::chrono::system_clock::now();
  auto last_accessed = std::chrono::time_point<std::chrono::system_clock>(
      std::chrono::seconds(info.last_accessed));
  GetMetaData().UpdateLastAccessTimeIfExists(info.identifier);
  if (now - last_accessed < MIN_ACCESS_TIME_UPDATE_INTERVAL) {
    return true;
  }

  LOGI("UpdateLastAccessTime: " << info.identifier.template_url << " "
                                << info.identifier.url);
  std::string json = GetMetaData().ToJson();
  LOGV("metadata: " << json);
  if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                 json.size())) {
    LOGE("Write Metadata failed!");
    return false;
  }
  return true;
}

void JsCacheManager::ClearCache(std::string_view template_url_key) {
  std::lock_guard<std::mutex> lock(cache_lock_);
  // calc time spend.
  auto begin = base::CurrentTimeMilliseconds();
  auto removed_cfi = GetMetaData().GetAllCacheFileInfo(template_url_key);
  size_t cleaned_size = 0;
  for (auto &info : removed_cfi) {
    cleaned_size += info.cache_size;
    auto file_name = MakeFilename(info.md5);
    unlink(MakePath(file_name).c_str());
    GetMetaData().RemoveFileInfo(info.identifier);
  }
  std::string json = GetMetaData().ToJson();
  LOGV("metadata: " << json);
  JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
  if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                 json.size())) {
    LOGE("Write Metadata failed!");
    error_code = JsCacheErrorCode::META_FILE_WRITE_ERROR;
  }
  auto cost = base::CurrentTimeMilliseconds() - begin;
  JsCacheTracker::OnCleanUp(engine_type_, static_cast<int>(removed_cfi.size()),
                            -1, cleaned_size, cost, error_code);
  LOGI("ClearCache time spent: " << cost << " ms");
}

void JsCacheManager::ClearInvalidCache() {
  std::lock_guard<std::mutex> lock(cache_lock_);
  // calc time spend.
  auto begin = base::CurrentTimeMilliseconds();
  const auto expired_time_seconds = ExpiredSeconds();
  const auto max_cache_size = MaxCacheSize();
  // for calc expired time.
  int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  auto all_cache_file_info = GetMetaData().GetAllCacheFileInfo();
  // record total size
  size_t total_size = 0;
  size_t cleaned_size = 0;
  // record expired cache_file_info's index.
  std::vector<CacheFileInfo> removed_cfi;
  // keeped cache_file_info
  std::vector<CacheFileInfo> temp_keep_cfi;
  // find expired cache_file_info
  for (auto &cfi : all_cache_file_info) {
    if (cfi.last_accessed + expired_time_seconds < now) {
      cleaned_size += cfi.cache_size;
      removed_cfi.push_back(std::move(cfi));
    } else {
      total_size += cfi.cache_size;
      temp_keep_cfi.push_back(std::move(cfi));
    }
  }
  int file_count = static_cast<int>(temp_keep_cfi.size());
  // When the specified maximum value is exceeded, the files are sorted by
  // last_accessed, and the one with the oldest last_accessed is deleted.
  if (total_size > max_cache_size) {
    std::sort(temp_keep_cfi.begin(), temp_keep_cfi.end(),
              [](const CacheFileInfo &l, const CacheFileInfo &r) {
                return l.last_accessed > r.last_accessed;
              });
    for (auto it = temp_keep_cfi.rbegin(); it != temp_keep_cfi.rend(); it++) {
      total_size -= it->cache_size;
      cleaned_size += it->cache_size;
      removed_cfi.push_back(std::move(*it));
      file_count--;
      if (total_size < max_cache_size) {
        break;
      }
    }
  }

  for (auto &info : removed_cfi) {
    auto file_name = MakeFilename(info.md5);
    unlink(MakePath(file_name).c_str());
    GetMetaData().RemoveFileInfo(info.identifier);
  }
  std::string json = GetMetaData().ToJson();
  LOGV("metadata: " << json);
  JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
  if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                 json.size())) {
    LOGE("Write Metadata failed!");
    error_code = JsCacheErrorCode::META_FILE_WRITE_ERROR;
  }
  auto cost = base::CurrentTimeMilliseconds() - begin;
  JsCacheTracker::OnCleanUp(engine_type_, file_count, total_size, cleaned_size,
                            cost, error_code);
  LOGI("ClearExpiredCache time spent: " << cost << " ms");
}

//
// util
//
MetaData &JsCacheManager::GetMetaData() {
  LoadMetadataIfNotLoaded();
  return *meta_data_;
}

void JsCacheManager::LoadMetadataIfNotLoaded() {
  if (meta_data_) {
    return;
  }

  auto bytecode_generate_generate_version = GetBytecodeGenerateEngineVersion();
  LOGI("bytecode_generate_generate_version: "
       << bytecode_generate_generate_version);

  std::string json;
  if (ReadFile(METADATA_FILE_NAME, json)) {
    meta_data_ = MetaData::ParseJson(json);
    if (meta_data_ != nullptr &&
        base::Version(meta_data_->GetLynxVersion()) == LYNX_VERSION &&
        meta_data_->GetBytecodeGenerateEngineVersion() ==
            bytecode_generate_generate_version) {
      return;
    }
  }
  LOGI("Metadata load failed, clearing cache");
  ClearCacheDir();

  LOGI("Creating new Metadata");
  meta_data_ = std::make_unique<MetaData>(LYNX_VERSION.ToString(),
                                          bytecode_generate_generate_version);
}

#ifdef OS_WIN
void JsCacheManager::EnumerateFile(
    base::MoveOnlyClosure<void, const std::string &> func) {}
#else
void JsCacheManager::EnumerateFile(
    base::MoveOnlyClosure<void, const std::string &> func) {
  auto path = GetCacheDir();
  DIR *dir = opendir(path.c_str());
  if (dir == nullptr) {
    return;
  }

  for (dirent *file = readdir(dir); file != nullptr; file = readdir(dir)) {
    if (strcmp(file->d_name, ".") && strcmp(file->d_name, "..")) {
      auto file_path = base::PathUtils::JoinPaths({path, file->d_name});
      func(file_path);
    }
  }
  closedir(dir);
}
#endif

#ifdef OS_WIN
void JsCacheManager::ClearCacheDir() {}
#else
void JsCacheManager::ClearCacheDir() {
  LOGI("Clearing cache dir");
  EnumerateFile([](const std::string &file_path) {
    if (unlink(file_path.c_str())) {
      LOGE("remove file failed, file: " << file_path << " errno: " << errno);
    }
  });
}
#endif

int64_t JsCacheManager::ExpiredSeconds() {
  // TODO(zhenziqi) let user decide the expired time
  // 15 days for now
  return 15 * 24 * 3600;
}

size_t JsCacheManager::MaxCacheSize() {
  return tasm::LynxEnv::GetInstance().GetLongEnv(
      tasm::LynxEnv::Key::BYTECODE_MAX_SIZE, 100 * 1024 * 1024);
}

const std::string &JsCacheManager::EnsureMd5(
    const std::shared_ptr<const Buffer> &buffer,
    std::optional<std::string> &md5_str) {
  if (!md5_str.has_value()) {
    md5_str = base::md5(reinterpret_cast<const char *>(buffer->data()),
                        buffer->size());
  }
  return *md5_str;
}

std::string JsCacheManager::GetSourceCategory(const std::string &source_url) {
  if (runtime::IsCoreJS(source_url)) {
    // lynx_core.js
    return MetaData::CORE_JS;
  } else if (runtime::IsLynxTemplateAssets(source_url)) {
    // js filed bundled in template.js
    return MetaData::PACKAGED;
  } else {
    // dynamic js files
    return MetaData::DYNAMIC;
  }
}

JsFileIdentifier JsCacheManager::BuildIdentifier(
    const std::string &source_url, const std::string &template_url) {
  JsFileIdentifier identifier;
  identifier.url = source_url;
  identifier.template_url = template_url;
  identifier.category = GetSourceCategory(source_url);
  return identifier;
}

bool JsCacheManager::IsCacheEnabledForTemplate(
    const std::string &template_url) {
  if (!IsCacheEnabled()) {
    LOGI("bytecode disabled by switch");
    return false;
  }
  return true;
}

std::string JsCacheManager::CacheDirName() {
  if (engine_type_ == JSRuntimeType::quickjs) {
    return "quickjs_cache";
  } else if (engine_type_ == JSRuntimeType::v8) {
    return "v8_cache";
  } else {
    LOGF("unsupport bytecode runtime type.");
    return "";
  }
}

std::string JsCacheManager::GetBytecodeGenerateEngineVersion() {
  return std::to_string(LEPUS_GetPrimjsVersion());
}

// Use for v8
__attribute__((visibility("default"))) std::shared_ptr<Buffer> TryGetCacheV8(
    const std::string &source_url, const std::string &template_url,
    int64_t runtime_id, const std::shared_ptr<const Buffer> &buffer,
    std::unique_ptr<CacheGenerator> cache_generator) {
  return JsCacheManager::GetV8Instance().TryGetCache(
      source_url, template_url, runtime_id, buffer, std::move(cache_generator));
}

// Use for v8
__attribute__((visibility("default"))) void RequestCacheGenerationV8(
    const std::string &source_url, const std::string &template_url,
    const std::shared_ptr<const Buffer> &buffer,
    std::unique_ptr<CacheGenerator> cache_generator, bool force) {
  JsCacheManager::GetV8Instance().RequestCacheGeneration(
      source_url, template_url, buffer, std::move(cache_generator), force);
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx
