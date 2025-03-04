// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/common/jsi_object_wrapper.h"

#include <utility>

namespace lynx {
namespace piper {
JSIObjectProxyImpl::JSIObjectProxyImpl(
    int64_t obj_id, std::shared_ptr<JSIObjectWrapperManager> manager)
    : lepus::LEPUSObject::JSIObjectProxy(obj_id), manager_(manager) {}

JSIObjectProxyImpl::~JSIObjectProxyImpl() {
  std::shared_ptr<JSIObjectWrapperManager> manager = manager_.lock();
  if (manager) {
    manager->ReleaseJSIObjectByID(jsi_object_id_);
  }
}

JSIObjectWrapperManager::JSIObjectWrapperManager() : jsi_object_counter_(0) {}

std::shared_ptr<lepus::LEPUSObject::JSIObjectProxy>
JSIObjectWrapperManager::CreateJSIObjectWrapperOnJSThread(
    piper::Runtime& rt, piper::Object obj, const std::string group) {
  piper::Scope scope(rt);

  int64_t jsi_object_id = -1;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);

    JSIObjectWrapper* proxy = nullptr;

    // use previous wrapper if exeist
    std::pair<GROUPED_OBJECT_MAP::iterator, GROUPED_OBJECT_MAP::iterator>
        range = grouped_jsi_object_map_.equal_range(group);
    for (auto it = range.first; it != range.second; ++it) {
      JSIObjectWrapper* curr_obj = it->second;
      if (piper::Object::strictEquals(rt, curr_obj->jsi_object_, obj)) {
        jsi_object_id = curr_obj->id_;
        proxy = curr_obj;
        break;
      }
    }

    // create new wrapper
    if (jsi_object_id == -1) {
      jsi_object_id = jsi_object_counter_++;
      proxy = new JSIObjectWrapper(std::move(obj), jsi_object_id, group);
      jsi_object_map_.insert(std::make_pair(jsi_object_id, proxy));
      grouped_jsi_object_map_.insert(std::make_pair(group, proxy));
    }

    proxy->AddRef();
  }

  auto wrapper = std::shared_ptr<JSIObjectProxyImpl>(
      new JSIObjectProxyImpl(jsi_object_id, shared_from_this()));
  return std::static_pointer_cast<lepus::LEPUSObject::JSIObjectProxy>(wrapper);
}

JSIObjectWrapperManager::JSIObjectWrapper::JSIObjectWrapper(
    piper::Object obj, int64_t jsi_object_id, const std::string& group_id)
    : ref_count_(0),
      jsi_object_(std::move(obj)),
      id_(jsi_object_id),
      group_id_(group_id) {}

void JSIObjectWrapperManager::JSIObjectWrapper::AddRef() { ref_count_++; }

void JSIObjectWrapperManager::JSIObjectWrapper::Release() { ref_count_--; }

int JSIObjectWrapperManager::JSIObjectWrapper::ref_count() {
  return ref_count_;
}

void JSIObjectWrapperManager::ReleaseJSIObjectByID(int64_t jsi_object_id) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  JSI_OBJECT_MAP::iterator it = jsi_object_map_.find(jsi_object_id);
  if (it != jsi_object_map_.end()) {
    JSIObjectWrapper* jsi_object_wrapper = it->second;
    jsi_object_wrapper->Release();

    // move the wrapper  to dirty set if needed
    if (jsi_object_wrapper->ref_count() == 0) {
      dirty_jsi_object_set_.insert(jsi_object_wrapper);
      jsi_object_map_.erase(it);

      // remove from grouped_jsi_object_map_
      std::pair<GROUPED_OBJECT_MAP::iterator, GROUPED_OBJECT_MAP::iterator>
          range = grouped_jsi_object_map_.equal_range(
              jsi_object_wrapper->group_id_);
      for (auto itr_grouped_obj_map = range.first;
           itr_grouped_obj_map != range.second; ++itr_grouped_obj_map) {
        JSIObjectWrapper* curr_object_wrapper = itr_grouped_obj_map->second;
        if (curr_object_wrapper->id_ == jsi_object_wrapper->id_) {
          grouped_jsi_object_map_.erase(itr_grouped_obj_map);
          break;
        }
      }
    }
  }
}

piper::Value JSIObjectWrapperManager::GetJSIObjectByIDOnJSThread(
    piper::Runtime& rt, int64_t jsi_object_id) {
  piper::Scope scope(rt);

  std::lock_guard<std::mutex> lock(map_mutex_);
  JSI_OBJECT_MAP::iterator it = jsi_object_map_.find(jsi_object_id);
  if (it != jsi_object_map_.end()) {
    piper::Value obj = piper::Value(rt, it->second->jsi_object_);
    return obj;
  }
  return piper::Value::null();
}

void JSIObjectWrapperManager::ForceGcOnJSThread() {
  std::lock_guard<std::mutex> lock(map_mutex_);

  auto it = dirty_jsi_object_set_.begin();
  for (; it != dirty_jsi_object_set_.end(); it++) {
    JSIObjectWrapper* wrapper = *it;
    delete wrapper;
  }

  dirty_jsi_object_set_.clear();
}

void JSIObjectWrapperManager::DestroyOnJSThread() {
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto it_obj = jsi_object_map_.begin();
    for (; it_obj != jsi_object_map_.end(); it_obj++) {
      JSIObjectWrapper* wrapper = it_obj->second;
      delete wrapper;
    }

    jsi_object_map_.clear();
    grouped_jsi_object_map_.clear();
  }

  ForceGcOnJSThread();
}

}  // namespace piper
}  // namespace lynx
