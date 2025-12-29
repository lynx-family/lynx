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

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct CachedData {
  CachedData() : length_(0), data_(nullptr), file_name_("") {}

  CachedData(int length, const uint8_t* data, const std::string& name)
      : length_(length), data_(data), file_name_(name) {}

  // carefully copy and move objects of this class
  ~CachedData() {
    if (data_) delete[] data_;
  }

  static bool compare(CachedData* left, CachedData* right) {
    if (left->used_times_ > right->used_times_) {
      return true;
    } else if (left->used_times_ < right->used_times_) {
      return false;
    } else {
      return left->length_ > right->length_;
    }
  }

  int used_times_ = 0;
  int length_;
  const uint8_t* data_;
  const std::string file_name_;
};

typedef std::unordered_map<std::string, CachedData*> CacheMap;
typedef std::vector<CachedData*> CacheVector;
#define SHORT_SIZE 2
#define INT_SIZE 4
#define DOUBLE_SIZE 8

class CacheBlob {
 private:
  enum CacheMode { kWriting, kAppending };
  static constexpr double MAGIC = 3.14159265;

 public:
  explicit CacheBlob(const std::string& path, int max_cap = 1 << 20)
      : current_size_(0),
        target_path_(path),
        max_capacity_(max_cap),
        empty_cache_(new CachedData) {}

  virtual ~CacheBlob() {
    for (auto it : cache_map_) delete it.second;
    if (append_vec_) delete append_vec_;
  }

  // NOTE: modifications on CacheBlob can only happen in worker Thread.
  bool insert(const std::string& filename, const uint8_t* data, int length);
  const CachedData* find(const std::string& filename, int* len) const;
  void remove(const std::string& filename);
  void output();
  bool input();
  int size() const { return current_size_; }

#ifdef PROFILE_CODECACHE
  void dump_status(void* p);
#endif  // PROFILE_CODECACHE

 private:
  void write_cache_unit(FILE* file_out, const CachedData* unit);
  void read_cache_unit(FILE* file_in);
  void remove_from_ranking_list(CachedData* target);
  // This is a vast-time-costing function
  bool get_enough_space(int data_size);

  CacheMap cache_map_;
  // ranking list for caches' frequency of being used.
  CacheVector heat_ranking_;
  int current_size_;
  const std::string target_path_;
  int max_capacity_;
  mutable std::mutex write_mutex_;
  std::unique_ptr<CachedData> empty_cache_;

#ifdef PROFILE_CODECACHE
  mutable int total_query_ = 0;
  mutable int missed_query_ = 0;
  mutable int expired_query_ = 0;
#endif  // PROFILE_CODECACHE
  CacheMode mode_ = kWriting;
  CacheVector* append_vec_ = nullptr;
};

#endif  // SRC_NAPI_COMMON_CODE_CACHE_H_
