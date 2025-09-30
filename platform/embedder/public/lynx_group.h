// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_GROUP_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_GROUP_H_

#include <string>
#include <utility>
#include <vector>

#include "capi/lynx_group_capi.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxGroup` is a class that represents a group of Lynx views, allowing
 * developers to share JSContext between different Lynx views within the same
 * group.
 */
class LynxGroup {
 public:
  explicit LynxGroup(const std::string& name)
      : group_(lynx_group_create(name.c_str())), name_(name) {}
  LynxGroup(const std::string& name, const std::string& id)
      : group_(lynx_group_create_with_id(name.c_str(), id.c_str())),
        name_(name),
        id_(id) {}

  ~LynxGroup() { lynx_group_release(group_); }

  std::string GetGroupName() const { return name_; }

  std::string GetID() const { return id_; }

  bool EnableJSGroupThread() const { return enable_js_group_thread_; }

  void SetEnableJSGroupThread(bool enable) {
    enable_js_group_thread_ = enable;
    lynx_group_set_enable_js_group_thread(group_, enable);
  }

  void SetPreloadJSPaths(std::vector<std::string> paths) {
    preload_js_paths_ = std::move(paths);
    std::vector<const char*> c_paths(preload_js_paths_.size());
    for (size_t i = 0; i < preload_js_paths_.size(); i++) {
      c_paths[i] = preload_js_paths_[i].c_str();
    }
    lynx_group_set_preload_js_paths(group_, c_paths.data(),
                                    preload_js_paths_.size());
  }

  lynx_group_t* Impl() { return group_; }

  LynxGroup(const LynxGroup&) = delete;
  LynxGroup& operator=(const LynxGroup&) = delete;

 private:
  lynx_group_t* group_;
  std::string name_;
  std::string id_;
  std::vector<std::string> preload_js_paths_;
  bool enable_js_group_thread_ = false;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_GROUP_H_
