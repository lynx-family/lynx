// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bytecode/js_cache_manager.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <utility>
#include <vector>

#include "base/include/file_utils.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/log/logging.h"
#include "base/include/md5.h"
#include "base/include/no_destructor.h"
#include "base/include/path_utils.h"
#include "base/include/string/string_utils.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/bytecode/js_cache_tracker.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

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

namespace {
static constexpr size_t MAX_SIZE = 50 * 1024 * 1024;  // 50MB

constexpr char METADATA_FILE_NAME[] = "meta.json";
constexpr auto MIN_ACCESS_TIME_UPDATE_INTERVAL = std::chrono::hours(24);

constexpr char TEMPLATE_KEY_SOURCE_URL_SEP[] = "##";

const std::string GetCacheUrlFromIdentifier(
    const JsFileIdentifier &identifier) {
  if (identifier.category == MetaData::PACKAGED) {
    return identifier.template_url + TEMPLATE_KEY_SOURCE_URL_SEP +
           identifier.url;
  }
  return identifier.url;
}

}  // namespace
std::unique_ptr<BytecodeGenerateCallback>
    JsCacheManager::g_bytecode_generate_callback = nullptr;

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
  auto [result, cache_dir] = lynx::base::GetModuleDirectoryPath();
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
  static std::string cache_dir = GetPlatformCacheDir();
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
  return !(tasm::LynxEnv::GetInstance().IsDevToolEnabled()) &&
         tasm::LynxEnv::GetInstance().IsQuickjsCacheEnabled() &&
         can_create_cache_;
}

// JsCacheManager

//
// request thread
//
std::shared_ptr<Buffer> JsCacheManager::TryGetCache(
    const std::string &source_url, const std::string &template_url,
    int64_t runtime_id, std::unique_ptr<CacheGenerator> cache_generator,
    BytecodeGetter *bytecode_getter) {
  if (!cache_generator) {
    return nullptr;
  }
  auto cost_start = base::CurrentTimeMilliseconds();
  if (!IsCacheEnabledForTemplate(template_url)) {
    JsCacheTracker::OnGetBytecodeDisable(runtime_id, engine_type_, source_url,
                                         true, false);
    return nullptr;
  }
  LOGI("bytecode enabled"
       << ", url: '" << source_url << "', template_url: '" << template_url
       << "', file_content size:" << cache_generator->SrcBuffer()->size());

  TRACE_EVENT(LYNX_TRACE_CATEGORY, JS_CACHE_MANAGER_TRY_GET_CACHE,
              [&source_url](lynx::perfetto::EventContext ctx) {
                auto *debug = ctx.event()->add_debug_annotations();
                debug->set_name("source_url");
                debug->set_string_value(source_url);
              });

  // try to load cache from memory
  if (runtime::IsKernelJs(source_url)) {
    std::scoped_lock<std::recursive_mutex> lock(cache_lock_);
    if (!cache_.empty()) {
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
  }
  auto identifier = BuildIdentifier(source_url, template_url);
  // try get bytecode from external
  if (bytecode_getter) {
    auto cache_url = GetCacheUrlFromIdentifier(identifier);
    auto maybe_bytecode = (*bytecode_getter)(cache_url);
    if (maybe_bytecode) {
      LOGI("cache loaded from external,cache_url:" << cache_url << " size: "
                                                   << maybe_bytecode->size());
      JsCacheTracker::OnGetBytecode(
          runtime_id, engine_type_, source_url, true, true, true,
          JsCacheType::EXTERNAL, JsCacheErrorCode::NO_ERROR,
          base::CurrentTimeMilliseconds() - cost_start, maybe_bytecode->size());
      return maybe_bytecode;
    }
  }

  JsCacheErrorCode error_code = JsCacheErrorCode::META_FILE_READ_ERROR;
  {
    auto cache = LoadCacheFromStorage(
        identifier,
        EnsureMd5(cache_generator->SrcBuffer(), cache_generator->Md5()));
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
    }
  }

  JsCacheTracker::OnGetBytecode(runtime_id, engine_type_, source_url, true,
                                true, false, JsCacheType::NONE, error_code,
                                base::CurrentTimeMilliseconds() - cost_start,
                                0);
  LOGI("no cache matches this url.");

  std::vector<std::unique_ptr<CacheGenerator>> generators;
  generators.push_back(std::move(cache_generator));
  PostTaskBackground(TaskInfo(TaskInfo::TaskType::GENERATE_CACHE, template_url,
                              std::move(generators)));
  return nullptr;
}

void JsCacheManager::RequestCacheGeneration(
    const std::string &template_url,
    std::vector<std::unique_ptr<CacheGenerator>> &&generators, bool force,
    std::unique_ptr<BytecodeGenerateCallback> callback) {
  LOGI("RequestCacheGeneration template_url: '" << template_url);
  if (!IsCacheEnabledForTemplate(template_url)) {
    LOGI("bytecode disabled");
    if (callback) {
      (*callback)("disabled", {});
    }
    return;
  }

  PostTaskBackground(
      TaskInfo(force ? TaskInfo::TaskType::GENERATE_CACHE
                     : TaskInfo::TaskType::GENERATE_CACHE_IF_NEEDED,
               template_url, std::move(generators), std::move(callback)));
}

/*
 * Post task into threadpool.
 */
void JsCacheManager::PostTaskBackground(TaskInfo task) {
  DoPostTask(
      [this, task = std::move(task)]() mutable { RunTask(std::move(task)); });
}

//
// background thread
//

void JsCacheManager::RunTask(TaskInfo task) {
  auto start = base::CurrentTimeMilliseconds();

  auto &[type, template_key, generators, callback] = task;
  std::unordered_map<std::string, std::shared_ptr<Buffer>> generator_results;
  BytecodeGenerateCallback *callback_ptr =
      callback ? callback.get() : g_bytecode_generate_callback.get();

  for (const auto &generator : generators) {
    std::string file_md5 = EnsureMd5(generator->SrcBuffer(), generator->Md5());
    // check if task is already inserted into the task set.
    std::string task_id = template_key + generator->SourceUrl() + file_md5;
    {
      std::lock_guard<std::mutex> guard(task_set_lock_);
      if (task_set_.find(task_id) != task_set_.end()) {
        LOGI("task already exists, ignore:" << template_key << " source_url:"
                                            << generator->SourceUrl()
                                            << " md5:" << file_md5);
        continue;
      }
      task_set_.insert(task_id);
    }
    auto identifier = BuildIdentifier(generator->SourceUrl(), template_key);
    std::string cache_url;
    if (callback_ptr) {
      cache_url = GetCacheUrlFromIdentifier(identifier);
      LOGI("bytecode generation is controlled by external interface:"
           << cache_url);
    }
    if (type == TaskInfo::TaskType::GENERATE_CACHE_IF_NEEDED) {
      if (auto cache = LoadCacheFromStorage(identifier, file_md5)) {
        if (!cache_url.empty()) {
          generator_results[cache_url] = std::move(cache);
        }
        continue;
      }
    }

    LOGI("RunTask start"
         << ", url: '" << identifier.url << "', template_url: '"
         << identifier.template_url << "', file_md5: " << file_md5
         << ", buffer size: " << generator->SrcBuffer()->size() << " bytes");

    std::shared_ptr<Buffer> cache_buffer(generator->GenerateCache());
    if (!cache_buffer) {
      std::string error_msg = "generate failed:" + template_key +
                              " source_url:" + generator->SourceUrl();
      LOGE(error_msg);
      JsCacheTracker::OnGenerateBytecodeFailed(
          engine_type_, identifier.url, identifier.template_url,
          GetBytecodeGenerateEngineVersion(),
          JsCacheErrorCode::RUNTIME_GENERATE_FAILED);
      if (callback_ptr) {
        (*callback_ptr)(std::move(error_msg), {});
      }
      // when generate failed, remove it from task set.
      {
        std::lock_guard<std::mutex> guard(task_set_lock_);
        task_set_.erase(task_id);
      }
      continue;
    } else if (!cache_url.empty()) {
      generator_results[cache_url] = std::move(cache_buffer);
      continue;
    }
    auto generate_cost = base::CurrentTimeMilliseconds() - start;

    if (runtime::IsKernelJs(identifier.url)) {
      SaveCacheToMemory(identifier.url, cache_buffer);
    }

    JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
    auto persist_success = SaveCacheContentToStorage(identifier, cache_buffer,
                                                     file_md5, error_code);
    JsCacheTracker::OnGenerateBytecode(
        engine_type_, identifier.url, identifier.template_url, true,
        generator->SrcBuffer()->size(), cache_buffer->size(), persist_success,
        GetBytecodeGenerateEngineVersion(), generate_cost, error_code);
    LOGI("MakeCache success:"
         << persist_success << ", cache size: " << cache_buffer->size()
         << " bytes, time spent: " << (base::CurrentTimeMilliseconds() - start)
         << " ms");
  }
  if (callback_ptr) {
    if (generator_results.size() > 0) {
      (*callback_ptr)("", std::move(generator_results));
    } else {
      (*callback_ptr)("generate failed." + template_key, {});
    }
  }
}

std::shared_ptr<Buffer> JsCacheManager::LoadCacheFromStorage(
    JsFileIdentifier &identifier, const std::string &file_md5) {
  auto locked_meta_data = GetLockedMetaData();
  auto opt_info = locked_meta_data->GetFileInfo(identifier);
  if (!opt_info) {
    return nullptr;
  }
  auto file_info = *opt_info;
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
    // there's no need to save metadata to storage here. it will be saved
    // in the progress of generating cache later.
    locked_meta_data->RemoveFileInfo(identifier);
    return nullptr;
  }

  UpdateLastAccessTime(locked_meta_data, std::move(file_info));
  return std::make_shared<StringBuffer>(std::move(cache));
}

bool JsCacheManager::SaveCacheContentToStorage(
    const JsFileIdentifier &identifier, const std::shared_ptr<Buffer> &cache,
    const std::string &file_md5, JsCacheErrorCode &error_code) {
  LOGI("SaveCacheContentToStorage template_url=' "
       << identifier.template_url << "', url='" << identifier.url << "'");

  auto locked_meta_data = GetLockedMetaData();
  locked_meta_data->UpdateFileInfo(identifier, file_md5, cache->size());
  std::string json = locked_meta_data->ToJson();
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
void JsCacheManager::UpdateLastAccessTime(
    JsCacheManager::LockedMetaData &locked_meta_data,
    const CacheFileInfo info) {
  auto now = std::chrono::system_clock::now();
  auto last_accessed = std::chrono::time_point<std::chrono::system_clock>(
      std::chrono::seconds(info.last_accessed));
  locked_meta_data->UpdateLastAccessTimeIfExists(info.identifier);
  if (now - last_accessed < MIN_ACCESS_TIME_UPDATE_INTERVAL) {
    return;
  }

  LOGI("UpdateLastAccessTime: " << info.identifier.template_url << " "
                                << info.identifier.url);
  auto task = [this](std::string json) {
    LOGV("metadata: " << json);
    if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                   json.size())) {
      LOGE("Write Metadata failed!");
    }
  };
  if (base::TaskRunnerManufactor::IsOnConcurrentLoopWorker(
          base::ConcurrentTaskType::NORMAL_PRIORITY)) {
    task(locked_meta_data->ToJson());
  } else {
    DoPostTask([this, task = std::move(task)]() {
      auto locked_meta_data = GetLockedMetaData();
      task(locked_meta_data->ToJson());
    });
  }
}

void JsCacheManager::ClearCache(const std::string &template_url_key) {
  DoPostTask([this, template_url_key]() {
    auto begin = base::CurrentTimeMilliseconds();
    auto locked_meta_data = GetLockedMetaData();
    auto removed_cfi = locked_meta_data->GetAllCacheFileInfo(template_url_key);
    size_t cleaned_size = 0;
    for (auto &info : removed_cfi) {
      cleaned_size += info.cache_size;
      auto file_name = MakeFilename(info.md5);
      unlink(MakePath(file_name).c_str());
      locked_meta_data->RemoveFileInfo(info.identifier);
    }
    std::string json = locked_meta_data->ToJson();
    LOGV("metadata: " << json);
    JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
    if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                   json.size())) {
      LOGE("Write Metadata failed!");
      error_code = JsCacheErrorCode::META_FILE_WRITE_ERROR;
    }
    auto cost = base::CurrentTimeMilliseconds() - begin;
    JsCacheTracker::OnCleanUp(engine_type_,
                              static_cast<int>(removed_cfi.size()), -1,
                              cleaned_size, cost, error_code);
    LOGI("ClearCache time spent: " << cost << " ms");
  });
}

void JsCacheManager::ClearInvalidCache() {
  DoPostTask([this]() {
    auto begin = base::CurrentTimeMilliseconds();
    auto locked_meta_data = GetLockedMetaData();
    const auto expired_time_seconds = ExpiredSeconds();
    const auto max_cache_size = MaxCacheSize();
    // for calc expired time.
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    auto all_cache_file_info = locked_meta_data->GetAllCacheFileInfo();
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
      locked_meta_data->RemoveFileInfo(info.identifier);
    }
    std::string json = locked_meta_data->ToJson();
    LOGV("metadata: " << json);
    JsCacheErrorCode error_code = JsCacheErrorCode::NO_ERROR;
    if (!WriteFile(METADATA_FILE_NAME, reinterpret_cast<uint8_t *>(json.data()),
                   json.size())) {
      LOGE("Write Metadata failed!");
      error_code = JsCacheErrorCode::META_FILE_WRITE_ERROR;
    }
    auto cost = base::CurrentTimeMilliseconds() - begin;
    JsCacheTracker::OnCleanUp(engine_type_, file_count, total_size,
                              cleaned_size, cost, error_code);
    LOGI("ClearExpiredCache time spent: " << cost << " ms");
  });
}

//
// util
//
void JsCacheManager::InitMetaData() {
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

JsCacheManager::LockedMetaData JsCacheManager::GetLockedMetaData() {
  std::call_once(meta_data_init_flag_, &JsCacheManager::InitMetaData, this);
  return LockedMetaData(cache_lock_, meta_data_);
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

void JsCacheManager::DoPostTask(base::closure task) {
  if (base::TaskRunnerManufactor::IsOnConcurrentLoopWorker(
          base::ConcurrentTaskType::NORMAL_PRIORITY)) {
    task();
  }
  base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
      [task = std::move(task)] { task(); },
      base::ConcurrentTaskType::NORMAL_PRIORITY);
}

// Use for v8
__attribute__((visibility("default"))) std::shared_ptr<Buffer> TryGetCacheV8(
    const std::string &source_url, const std::string &template_url,
    int64_t runtime_id, std::unique_ptr<CacheGenerator> cache_generator) {
  return JsCacheManager::GetV8Instance().TryGetCache(
      source_url, template_url, runtime_id, std::move(cache_generator),
      nullptr);
}

// Use for v8
__attribute__((visibility("default"))) void RequestCacheGenerationV8(
    const std::string &source_url, const std::string &template_url,
    const std::shared_ptr<const Buffer> &buffer,
    std::unique_ptr<CacheGenerator> cache_generator, bool force) {
  std::vector<std::unique_ptr<cache::CacheGenerator>> generators;
  generators.push_back(std::move(cache_generator));
  JsCacheManager::GetV8Instance().RequestCacheGeneration(
      template_url, std::move(generators), force);
}

}  // namespace cache
}  // namespace piper
}  // namespace lynx
