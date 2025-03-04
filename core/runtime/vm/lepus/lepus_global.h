// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_GLOBAL_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_GLOBAL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace lepus {
class Context;

class Global {
 public:
  Global() : global_() { global_content_.reserve(256); }

  ~Global() = default;

  inline Value* Get(std::size_t index) {
    if (index < global_content_.size()) {
      return &global_content_[index];
    }
    return nullptr;
  }

  int Search(const base::String& name) {
    auto iter = global_.find(name);
    if (iter != global_.end()) {
      return iter->second;
    }
    return -1;
  }

  Value* Find(const base::String& name) {
    auto iter = global_.find(name);
    if (iter != global_.end()) {
      return &global_content_[iter->second];
    }
    return nullptr;
  }

  std::size_t Add(const base::String& name, Value value) {
    auto iter = global_.find(name);
    if (iter != global_.end()) {
      return iter->second;
    }
    global_content_.push_back(std::move(value));
    global_.insert(std::make_pair(name, global_content_.size() - 1));
    return global_content_.size() - 1;
  }

  void Set(const base::String& name, Value value) {
    auto iter = global_.find(name);
    if (iter != global_.end()) {
      global_content_[iter->second] = std::move(value);
      return;
    }
    global_content_.push_back(std::move(value));
    global_.insert(std::make_pair(name, global_content_.size() - 1));
  }

  void Replace(const base::String& name, Value value) {
    auto iter = global_.find(name);
    if (iter != global_.end()) {
      global_content_[iter->second] = std::move(value);
    } else {
      Add(name, std::move(value));
    }
  }

  bool Update(size_t index, Value value) {
    if (index >= global_content_.size()) {
      return false;
    }
    global_content_[index] = std::move(value);
    return true;
  }

  bool Update(const base::String& name, Value value) {
    printf("global update:%s\n", name.c_str());
    auto iter = global_.find(name);
    int index = -1;
    if (iter != global_.end()) {
      index = iter->second;
    } else {
      return false;
    }
    global_content_[index] = std::move(value);
    return true;
  }

  std::size_t size() { return global_content_.size(); }

 private:
  friend class ContextBinaryWriter;
  std::unordered_map<base::String, int> global_;
  std::vector<Value> global_content_;
};

class JsonData {
 public:
  JsonData(const char* json, lepus::Value* value)
      : source_(json), value_(value){};
  ~JsonData() {
    if (value_ != nullptr) delete value_;
  }
  // std::shared_ptr<JsonData> Clone();
  bool Parse();
  std::string source_;
  lepus::Value* value_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_GLOBAL_H_
