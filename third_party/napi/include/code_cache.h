/**
 * Copyright (c) 2017 Node.js API collaborators. All Rights Reserved.
 *
 * Use of this source code is governed by a MIT license that can be
 * found in the LICENSE file in the root of the source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef SRC_NAPI_COMMON_CODE_CACHE_H_
#define SRC_NAPI_COMMON_CODE_CACHE_H_

#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// In-memory image of one code cache file: bytecode keyed by script filename
// and bound to the hash of the source it was compiled from. Runtimes opening
// the same file share one blob (see Open); all public methods are thread-safe.
class CacheBlob {
 public:
  // Persisted in the cache file, so the function must stay stable.
  static uint64_t HashSource(const char* script, size_t length);

  // The capacity of an already opened blob is kept.
  static std::shared_ptr<CacheBlob> Open(const std::string& path,
                                         int max_capacity);

  explicit CacheBlob(const std::string& path, int max_capacity = 1 << 20);

  // Replaces any previous entry for `filename`; evicts least used entries
  // when full and fails when the data cannot fit at all.
  bool insert(const std::string& filename, uint64_t source_hash,
              const uint8_t* data, int length);

  // Misses when the entry was compiled from a different source.
  bool find(const std::string& filename, uint64_t source_hash,
            std::vector<uint8_t>* out);

  void remove(const std::string& filename);

  // Only the first call reads the disk. An invalid file is ignored and
  // replaced by the next output().
  bool input();

  // Rewrites the file atomically when entries changed since the last
  // input() or output(). Only the snapshot is taken under the lock, so a
  // worker publishing on exit never stalls another worker's find().
  bool output();

  int size() const;

#ifdef PROFILE_CODECACHE
  void dump_status(std::vector<std::pair<std::string, int>>* status);
#endif  // PROFILE_CODECACHE

 private:
  struct Entry {
    uint64_t source_hash;
    std::vector<uint8_t> data;
    int used_times;
  };
  using EntryMap = std::unordered_map<std::string, Entry>;

  // The following require mutex_ to be held.
  bool make_room(int length);
  bool read_entries(FILE* file, EntryMap* entries, int* total_size);
  std::string serialize() const;

  const std::string path_;
  const int max_capacity_;
  mutable std::mutex mutex_;
  // Serializes output() calls; taken before mutex_, never after it.
  std::mutex publish_mutex_;
  EntryMap entries_;
  int current_size_ = 0;
  bool loaded_ = false;
  bool load_result_ = false;
  bool dirty_ = false;

#ifdef PROFILE_CODECACHE
  int total_query_ = 0;
  int missed_query_ = 0;
  int expired_query_ = 0;
#endif  // PROFILE_CODECACHE
};

#endif  // SRC_NAPI_COMMON_CODE_CACHE_H_
