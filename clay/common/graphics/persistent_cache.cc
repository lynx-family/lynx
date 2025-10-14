// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/persistent_cache.h"

#include <cstdlib>
#include <cstring>
#include <future>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/fml/make_copyable.h"
#include "base/trace/native/trace_event.h"
#include "clay/common/sha1.h"
#include "clay/fml/base32.h"
#include "clay/fml/file.h"
#include "clay/fml/logging.h"
#include "clay/fml/mapping.h"
#include "clay/fml/paths.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/version/version.h"
#include "third_party/rapidjson/document.h"

namespace clay {

std::string PersistentCache::cache_base_path_;  // NOLINT(runtime/string)

std::mutex PersistentCache::instance_mutex_;
std::unique_ptr<PersistentCache> PersistentCache::gPersistentCache;

bool PersistentCache::gIsReadOnly = false;

std::atomic<bool> PersistentCache::cache_sksl_ = false;
std::atomic<bool> PersistentCache::strategy_set_ = false;

void PersistentCache::SetCacheSkSL(bool value) {
  if (strategy_set_ && value != cache_sksl_) {
    FML_DLOG(ERROR) << "Cache SkSL can only be set before the "
                       "GrContextOptions::fShaderCacheStrategy is set.";
    return;
  }
  cache_sksl_ = value;
}

PersistentCache* PersistentCache::GetCacheForProcess() {
  std::scoped_lock lock(instance_mutex_);
  if (gPersistentCache == nullptr) {
    gPersistentCache.reset(new PersistentCache(gIsReadOnly));
  }
  return gPersistentCache.get();
}

void PersistentCache::ResetCacheForProcess() {
  std::scoped_lock lock(instance_mutex_);
  gPersistentCache.reset(new PersistentCache(gIsReadOnly));
  strategy_set_ = false;
}

void PersistentCache::SetCacheDirectoryPath(std::string path) {
  cache_base_path_ = std::move(path);
}

bool PersistentCache::Purge() {
  // Make sure that this is called after the worker task runner setup so all the
  // file system modifications would happen on that single thread to avoid
  // racing.
  FML_DCHECK(GetWorkerTaskRunner());

  std::promise<bool> removed;
  GetWorkerTaskRunner()->PostTask(
      [&removed, cache_directory = cache_directory_]() {
        if (cache_directory->is_valid()) {
          // Only remove files but not directories.
          FML_LOG(INFO) << "Purge persistent cache.";
          fml::FileVisitor delete_file = [](const fml::UniqueFD& directory,
                                            const std::string& filename) {
            // Do not delete directories. Return true to continue with other
            // files.
            if (fml::IsDirectory(directory, filename.c_str())) {
              return true;
            }
            return fml::UnlinkFile(directory, filename.c_str());
          };
          removed.set_value(
              fml::VisitFilesRecursively(*cache_directory, delete_file));
        } else {
          removed.set_value(false);
        }
      });
  return removed.get_future().get();
}

namespace {

constexpr char kEngineComponent[] = "clay_engine";

static void FreeOldCacheDirectory(const fml::UniqueFD& cache_base_dir) {
  fml::UniqueFD engine_dir =
      fml::OpenDirectoryReadOnly(cache_base_dir, kEngineComponent);
  if (!engine_dir.is_valid()) {
    return;
  }
  fml::VisitFiles(engine_dir, [](const fml::UniqueFD& directory,
                                 const std::string& filename) {
    if (filename != GetEngineVersion()) {
      auto dir = fml::OpenDirectory(directory, filename.c_str(), false,
                                    fml::FilePermission::kReadWrite);
      if (dir.is_valid()) {
        fml::RemoveDirectoryRecursively(directory, filename.c_str());
      }
    }
    return true;
  });
}

static std::shared_ptr<fml::UniqueFD> MakeCacheDirectory(
    const std::string& global_cache_base_path, bool read_only,
    bool cache_sksl) {
  fml::UniqueFD cache_base_dir;
  if (global_cache_base_path.length()) {
    cache_base_dir = fml::OpenDirectory(global_cache_base_path.c_str(), false,
                                        fml::FilePermission::kRead);
  } else {
    cache_base_dir = fml::paths::GetCachesDirectory();
  }

  if (cache_base_dir.is_valid()) {
    FreeOldCacheDirectory(cache_base_dir);
    std::vector<std::string> components = {kEngineComponent, GetEngineVersion(),
                                           "skia", GetSkiaVersion()};
    if (cache_sksl) {
      components.push_back(PersistentCache::kSkSLSubdirName);
    }
    return std::make_shared<fml::UniqueFD>(
        CreateDirectory(cache_base_dir, components,
                        read_only ? fml::FilePermission::kRead
                                  : fml::FilePermission::kReadWrite));
  } else {
    return std::make_shared<fml::UniqueFD>();
  }
}
}  // namespace

#ifndef ENABLE_SKITY
std::string PersistentCache::SkKeyToFilePath(const SkData& key) {
  return clay::SHA1HashBytes(key.data(), key.size());
}

sk_sp<SkData> ParseBase32(const std::string& input) {
  std::pair<bool, std::string> decode_result = fml::Base32Decode(input);
  if (!decode_result.first) {
    FML_LOG(ERROR) << "Base32 can't decode: " << input;
    return nullptr;
  }
  const std::string& data_string = decode_result.second;
  return SkData::MakeWithCopy(data_string.data(), data_string.length());
}

size_t PersistentCache::PrecompileKnownSkSLs(GrDirectContext* context) const {
  // clang-tidy has trouble reasoning about some of the complicated array and
  // pointer-arithmetic code in rapidjson.
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.PlacementNew)
  auto known_sksls = LoadSkSLs();
  // A trace must be present even if no pre-compilations have been completed.
  TRACE_EVENT("flutter", "PersistentCache::PrecompileKnownSkSLs", "count",
              known_sksls.size());

  if (context == nullptr) {
    return 0;
  }

  size_t precompiled_count = 0;
  for (const auto& sksl : known_sksls) {
    TRACE_EVENT("flutter", "PrecompilingSkSL");
    if (context->precompileShader(*sksl.key, *sksl.value)) {
      precompiled_count++;
    }
  }

  TRACE_COUNTER("flutter", "PersistentCache::PrecompiledSkSLs.Successful",
                precompiled_count);
  return precompiled_count;
}

std::vector<PersistentCache::SkSLCache> PersistentCache::LoadSkSLs() const {
  TRACE_EVENT("flutter", "PersistentCache::LoadSkSLs");
  std::vector<PersistentCache::SkSLCache> result;
  fml::FileVisitor visitor = [&result](const fml::UniqueFD& directory,
                                       const std::string& filename) {
    SkSLCache cache = LoadFile(directory, filename, true);
    if (cache.key != nullptr && cache.value != nullptr) {
      result.push_back(cache);
    } else {
      FML_LOG(ERROR) << "Failed to load: " << filename;
    }
    return true;
  };

  // Only visit sksl_cache_directory_ if this persistent cache is valid.
  // However, we'd like to continue visit the asset dir even if this persistent
  // cache is invalid.
  if (IsValid()) {
    // In case `rewinddir` doesn't work reliably, load SkSLs from a freshly
    // opened directory (https://github.com/flutter/flutter/issues/65258).
    fml::UniqueFD fresh_dir =
        fml::OpenDirectoryReadOnly(*cache_directory_, kSkSLSubdirName);
    if (fresh_dir.is_valid()) {
      fml::VisitFiles(fresh_dir, visitor);
    }
  }
  return result;
}

PersistentCache::SkSLCache PersistentCache::LoadFile(
    const fml::UniqueFD& dir, const std::string& file_name, bool need_key) {
  SkSLCache result;
  auto file = fml::OpenFileReadOnly(dir, file_name.c_str());
  if (!file.is_valid()) {
    return result;
  }
  auto mapping = std::make_unique<fml::FileMapping>(file);
  if (mapping->GetSize() < sizeof(CacheObjectHeader)) {
    return result;
  }
  const CacheObjectHeader* header =
      reinterpret_cast<const CacheObjectHeader*>(mapping->GetMapping());
  if (header->signature != CacheObjectHeader::kSignature ||
      header->version != CacheObjectHeader::kVersion1) {
    FML_LOG(INFO) << "Persistent cache header is corrupt: " << file_name;
    return result;
  }
  if (mapping->GetSize() < sizeof(CacheObjectHeader) + header->key_size) {
    FML_LOG(INFO) << "Persistent cache size is corrupt: " << file_name;
    return result;
  }
  if (need_key) {
    result.key = SkData::MakeWithCopy(
        mapping->GetMapping() + sizeof(CacheObjectHeader), header->key_size);
  }
  size_t value_offset = sizeof(CacheObjectHeader) + header->key_size;
  result.value = SkData::MakeWithCopy(mapping->GetMapping() + value_offset,
                                      mapping->GetSize() - value_offset);
  return result;
}

// |GrContextOptions::PersistentCache|
sk_sp<SkData> PersistentCache::load(const SkData& key) {
  TRACE_EVENT("flutter", "PersistentCacheLoad");
  if (!IsValid()) {
    return nullptr;
  }
  auto file_name = SkKeyToFilePath(key);
  if (file_name.empty()) {
    return nullptr;
  }
  auto result =
      PersistentCache::LoadFile(*sksl_cache_directory_, file_name, false).value;
  if (result != nullptr) {
    TRACE_EVENT("flutter", "PersistentCacheLoadHit");
  }
  return result;
}

std::unique_ptr<fml::MallocMapping> PersistentCache::BuildCacheObject(
    const SkData& key, const SkData& data) {
  size_t total_size = sizeof(CacheObjectHeader) + key.size() + data.size();
  uint8_t* mapping_buf = reinterpret_cast<uint8_t*>(malloc(total_size));
  if (!mapping_buf) {
    return nullptr;
  }
  auto mapping = std::make_unique<fml::MallocMapping>(mapping_buf, total_size);

  CacheObjectHeader header(key.size());
  memcpy(mapping_buf, &header, sizeof(CacheObjectHeader));
  mapping_buf += sizeof(CacheObjectHeader);
  memcpy(mapping_buf, key.data(), key.size());
  mapping_buf += key.size();
  memcpy(mapping_buf, data.data(), data.size());

  return mapping;
}

static void PersistentCacheStore(
    const fml::RefPtr<fml::TaskRunner>& worker,
    const std::shared_ptr<fml::UniqueFD>& cache_directory, std::string key,
    std::unique_ptr<fml::Mapping> value) {
  // The static leak checker gets confused by the use of fml::MakeCopyable.
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  auto task = fml::MakeCopyable([cache_directory,             //
                                 file_name = std::move(key),  //
                                 mapping = std::move(value)   //
  ]() mutable {
    TRACE_EVENT("flutter", "PersistentCacheStore");
    if (!fml::WriteAtomically(*cache_directory,   //
                              file_name.c_str(),  //
                              *mapping)           //
    ) {
      FML_LOG(WARNING) << "Could not write cache contents to persistent store.";
    }
  });

  if (!worker) {
    FML_DLOG(WARNING)
        << "The persistent cache has no available workers. Performing the task "
           "on the current thread. This slow operation is going to occur on a "
           "frame workload.";
    task();
  } else {
    worker->PostTask(std::move(task));
  }
}

// |GrContextOptions::PersistentCache|
void PersistentCache::store(const SkData& key, const SkData& data) {
  stored_new_shaders_ = true;

  if (is_read_only_) {
    return;
  }

  if (!IsValid()) {
    return;
  }

  auto file_name = SkKeyToFilePath(key);

  if (file_name.empty()) {
    return;
  }

  std::unique_ptr<fml::MallocMapping> mapping = BuildCacheObject(key, data);
  if (!mapping) {
    return;
  }
  PersistentCacheStore(GetWorkerTaskRunner(), sksl_cache_directory_,
                       std::move(file_name), std::move(mapping));
}

void PersistentCache::DumpSkp(const SkData& data) {
  if (is_read_only_ || !IsValid()) {
    FML_DLOG(ERROR)
        << "Could not dump SKP from read-only or invalid persistent "
           "cache.";
    return;
  }

  std::stringstream name_stream;
  auto ticks = fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
  name_stream << "shader_dump_" << std::to_string(ticks) << ".skp";
  std::string file_name = name_stream.str();
  FML_DLOG(INFO) << "Dumping " << file_name;
  auto mapping = std::make_unique<fml::DataMapping>(
      std::vector<uint8_t>{data.bytes(), data.bytes() + data.size()});
  PersistentCacheStore(GetWorkerTaskRunner(), cache_directory_,
                       std::move(file_name), std::move(mapping));
}
#endif  // ENABLE_SKITY

PersistentCache::PersistentCache(bool read_only)
    : is_read_only_(read_only),
      cache_directory_(MakeCacheDirectory(cache_base_path_, read_only, false)),
      sksl_cache_directory_(
          MakeCacheDirectory(cache_base_path_, read_only, true)),
      resource_cache_(new ResourceCache) {
  if (!IsValid()) {
    FML_DLOG(WARNING) << "Could not acquire the persistent cache directory. "
                         "Caching of GPU resources on disk is disabled.";
  }
}

PersistentCache::~PersistentCache() = default;

bool PersistentCache::IsValid() const {
  return cache_directory_ && cache_directory_->is_valid();
}

void PersistentCache::AddWorkerTaskRunner(
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  std::scoped_lock lock(worker_task_runners_mutex_);
  worker_task_runners_.insert(task_runner);
}

void PersistentCache::RemoveWorkerTaskRunner(
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  std::scoped_lock lock(worker_task_runners_mutex_);
  auto found = worker_task_runners_.find(task_runner);
  if (found != worker_task_runners_.end()) {
    worker_task_runners_.erase(found);
  }
}

fml::RefPtr<fml::TaskRunner> PersistentCache::GetWorkerTaskRunner() const {
  fml::RefPtr<fml::TaskRunner> worker;

  std::scoped_lock lock(worker_task_runners_mutex_);
  if (!worker_task_runners_.empty()) {
    worker = *worker_task_runners_.begin();
  }

  return worker;
}

}  // namespace clay
