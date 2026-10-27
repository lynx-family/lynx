// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/resource/gpu_resource_cache.h"

#include "clay/common/sys_info.h"

namespace clay {

namespace {
// The maximum number of images that we can lock simultaneously in our working
// set. This is separate from the memory limit, as keeping very large numbers
// of small images simultaneously locked can lead to performance issues and
// memory spikes.
constexpr size_t kMaxItemsInCache = 256;

constexpr size_t kMaxMemoryLimitBytes = 32 * 1024 * 1024;        // 32MB
constexpr size_t kLowEndMaxMemoryLimitBytes = 16 * 1024 * 1024;  // 16MB

const fml::TimeDelta kTimeToLiveForActiveResource =
    fml::TimeDelta::FromMilliseconds(fml::kDefaultFrameBudget.count() * 3);

const fml::TimeDelta kDefaultTimeToLive =
    fml::TimeDelta::FromSeconds(3600 * 12);

constexpr size_t kMBytes = 1024 * 1024;  // 1MB
const float kRecycleRamThreshold = 0.8f;
}  // namespace

GpuResourceCache::GpuResourceCache()
    : max_memory_limit_(SysInfo::IsLowEndDevice() ? kLowEndMaxMemoryLimitBytes
                                                  : kMaxMemoryLimitBytes) {}

void GpuResourceCache::StoreImage(fml::RefPtr<SkImageHolder> img) {
  ReduceCacheUsage();

  fml::TimePoint timestamp = fml::TimePoint::Now();
  if (!is_low_memory_ && img->ShouldUseCache()) {
    timestamp = timestamp + kDefaultTimeToLive;
  } else {
    timestamp = timestamp + kTimeToLiveForActiveResource;
  }

  auto iter = images_.find(img);
  if (iter != images_.end()) {
    // Need to maintain ordering by removing then inserting.
    images_.erase(iter);
    img->set_timestamp(timestamp);
    images_.insert(img);
    return;
  }

  current_memory_bytes_ += img->GetAllocationSize();
  img->set_timestamp(timestamp);
  images_.insert(img);
}

void GpuResourceCache::RemoveImage(fml::RefPtr<SkImageHolder> img) {
  auto iter = images_.find(img);
  if (iter != images_.end()) {
    iter->get()->ReleaseResource();
    current_memory_bytes_ -= iter->get()->GetAllocationSize();
    images_.erase(iter);
    return;
  }
}

void GpuResourceCache::RecycleInternal() {
  // Release expired images
  fml::TimePoint now = fml::TimePoint::Now();
  while (!images_.empty()) {
    auto iter = images_.begin();
    if (iter->get()->timestamp() > now) {
      break;
    }
    iter->get()->ReleaseResource();
    current_memory_bytes_ -= iter->get()->GetAllocationSize();
    images_.erase(iter);
  }

  // Release more images to stay under the limits
  while (!images_.empty()) {
    if (current_memory_bytes_ < max_memory_limit_ &&
        images_.size() < kMaxItemsInCache) {
      break;
    }
    auto iter = images_.begin();
    iter->get()->ReleaseResource();
    current_memory_bytes_ -= iter->get()->GetAllocationSize();
    images_.erase(iter);
  }
}

void GpuResourceCache::ReduceCacheUsage(bool force) {
  if (force || NeedToRecycle()) {
    RecycleInternal();
  }
}

bool GpuResourceCache::NeedToRecycle() const {
  return static_cast<float>(current_memory_bytes_) / max_memory_limit_ >=
             kRecycleRamThreshold ||
         images_.size() >= kMaxItemsInCache;
}

void GpuResourceCache::ClearCache() {
  while (!images_.empty()) {
    auto iter = images_.begin();
    iter->get()->ReleaseResource();
    images_.erase(iter);
  }
  current_memory_bytes_ = 0u;
}

void GpuResourceCache::SetIsLowMemory(bool value) {
  is_low_memory_ = value;
  if (is_low_memory_) {
    ClearCache();
  }
}

void GpuResourceCache::UpdateResourceCacheMaxMemoryLimit(int limit,
                                                         int low_end_limit) {
  if (!SysInfo::IsLowEndDevice() && limit != -1) {
    max_memory_limit_ = limit * kMBytes;
  }
  if (SysInfo::IsLowEndDevice() && low_end_limit != -1) {
    max_memory_limit_ = low_end_limit * kMBytes;
  }
}

#ifndef NDEBUG
void GpuResourceCache::PrintCacheUsage() const {
  FML_DLOG(ERROR) << " Max count: " << kMaxItemsInCache
                  << " Ram limitation: " << kMaxMemoryLimitBytes
                  << " Count usage: "
                  << static_cast<float>(images_.size()) / kMaxItemsInCache
                  << " RAM usage:"
                  << static_cast<float>(current_memory_bytes_) /
                         max_memory_limit_;
}
#endif

}  // namespace clay
