// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_OUTPUT_TEXTURE_H_
#define CLAY_GFX_SHARED_IMAGE_OUTPUT_TEXTURE_H_
#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>

#include "clay/flow/compositor/frame_types.h"
namespace clay::display {
uint64_t AllocateResourceId();
enum class CompletionStatus { kPending, kComplete, kFailed };
class GPUCompletion {
 public:
  virtual ~GPUCompletion() = default;
  virtual CompletionStatus Poll() const = 0;
  // A query error is not evidence that the GPU stopped accessing storage.
  // Backends may also release on a terminal command-buffer error.
  virtual bool IsSafeToRelease() const {
    return Poll() == CompletionStatus::kComplete;
  }
};
// This record, including native storage/context, outlives the page and binding.
// Only the owning Raster sequence obtains write access; consumers register each
// submitted GPU use separately. A failed dependency never permits rewriting.
class ResourceRecord {
 public:
  ResourceRecord(ResourceKey key, ImageSpec image, uint64_t bytes,
                 std::shared_ptr<void> storage)
      : key_(key), image_(image), bytes_(bytes), storage_(std::move(storage)) {}
  const ResourceKey& key() const { return key_; }
  const ImageSpec& image() const { return image_; }
  uint64_t bytes() const { return bytes_; }
  const std::shared_ptr<void>& storage() const { return storage_; }
  void RetainCPU() { cpu_leases_.fetch_add(1); }
  void ReleaseCPU() { cpu_leases_.fetch_sub(1); }
  bool BeginWrite();
  void EndWrite(std::shared_ptr<const GPUCompletion> ready);
  uint64_t BeginRead();
  void EndRead(uint64_t use_id,
               std::shared_ptr<const GPUCompletion> completion);
  void CancelRead(uint64_t use_id);
  void Retire() { retired_.store(true); }
  bool CanReuse();
  bool CanDestroy();
  std::shared_ptr<const GPUCompletion> ready() const;

 private:
  bool PollLocked();
  const ResourceKey key_;
  const ImageSpec image_;
  const uint64_t bytes_;
  const std::shared_ptr<void> storage_;
  std::atomic<uint32_t> cpu_leases_{0};
  std::atomic<bool> retired_{false};
  mutable std::mutex mutex_;
  bool writing_ = false, unresolved_write_ = false;
  uint64_t next_use_id_ = 0;
  std::shared_ptr<const GPUCompletion> ready_;
  std::unordered_map<uint64_t, std::shared_ptr<const GPUCompletion>> reads_;
};
class TextureLease {
 public:
  explicit TextureLease(std::shared_ptr<ResourceRecord> record)
      : record_(std::move(record)) {
    record_->RetainCPU();
  }
  ~TextureLease() { record_->ReleaseCPU(); }
  TextureLease(const TextureLease&) = delete;
  TextureLease& operator=(const TextureLease&) = delete;

 private:
  std::shared_ptr<ResourceRecord> record_;
};
class OutputTexture final {
 public:
  OutputTexture(std::shared_ptr<ResourceRecord> record,
                uint64_t content_version)
      : record_(std::move(record)),
        version_(content_version),
        ready_(record_->ready()),
        lease_(record_) {}
  const std::shared_ptr<ResourceRecord>& resource() const { return record_; }
  const ImageSpec& image() const { return record_->image(); }
  uint64_t content_version() const { return version_; }
  const std::shared_ptr<const GPUCompletion>& ready() const { return ready_; }
  void Retire() const { record_->Retire(); }

 private:
  const std::shared_ptr<ResourceRecord> record_;
  const uint64_t version_;
  const std::shared_ptr<const GPUCompletion> ready_;
  const TextureLease lease_;
};
}  // namespace clay::display
#endif  // CLAY_GFX_SHARED_IMAGE_OUTPUT_TEXTURE_H_
