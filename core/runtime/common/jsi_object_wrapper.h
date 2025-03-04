// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_COMMON_JSI_OBJECT_WRAPPER_H_
#define CORE_RUNTIME_COMMON_JSI_OBJECT_WRAPPER_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "core/runtime/jsi/jsi.h"
#include "core/runtime/vm/lepus/js_object.h"

namespace lynx {
namespace piper {

// use component id( > 0) as the componen group id, -1 as the page group id
#define PAGE_GROUP_ID "-1"

class JSIObjectWrapperManager;

class JSIObjectProxyImpl : public lepus::LEPUSObject::JSIObjectProxy {
 public:
  virtual ~JSIObjectProxyImpl();

 private:
  JSIObjectProxyImpl(int64_t obj,
                     std::shared_ptr<JSIObjectWrapperManager> manager);

  std::weak_ptr<JSIObjectWrapperManager> manager_;

  friend class JSIObjectWrapperManager;
};

/*
 * manage the jsi object(data of the component / page) that referenced by lepus
 * engine lepus use the id of the jsi object, manager the jsi object by
 * component / page
 *
 */
class JSIObjectWrapperManager
    : public std::enable_shared_from_this<JSIObjectWrapperManager> {
 public:
  JSIObjectWrapperManager();

  std::shared_ptr<lepus::LEPUSObject::JSIObjectProxy>
  CreateJSIObjectWrapperOnJSThread(piper::Runtime& rt, piper::Object obj,
                                   const std::string group);

  piper::Value GetJSIObjectByIDOnJSThread(piper::Runtime& rt,
                                          int64_t jsi_object_id);

  void ReleaseJSIObjectByID(int64_t jsi_object_id);

  // release all the unreferenced jsi object
  void ForceGcOnJSThread();

  void DestroyOnJSThread();

 private:
  class JSIObjectWrapper {
   public:
    JSIObjectWrapper(piper::Object obj, int64_t jsi_object_id,
                     const std::string& group_id);
    ~JSIObjectWrapper() = default;
    void AddRef();
    void Release();
    int ref_count();

   private:
    friend class JSIObjectWrapperManager;

    int ref_count_;
    piper::Object jsi_object_;
    int64_t id_;
    std::string group_id_;
  };

  using GROUPED_OBJECT_MAP =
      std::unordered_multimap<std::string, JSIObjectWrapper*>;
  using JSI_OBJECT_MAP = std::unordered_map<int64_t, JSIObjectWrapper*>;
  GROUPED_OBJECT_MAP grouped_jsi_object_map_;
  JSI_OBJECT_MAP jsi_object_map_;
  std::unordered_set<JSIObjectWrapper*> dirty_jsi_object_set_;

  int64_t jsi_object_counter_;
  std::mutex map_mutex_;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_COMMON_JSI_OBJECT_WRAPPER_H_
