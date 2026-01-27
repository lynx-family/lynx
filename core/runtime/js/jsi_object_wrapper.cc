// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi_object_wrapper.h"

#include <utility>

namespace lynx {
namespace runtime {
namespace js {
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
    Runtime& rt, Object obj, const std::string group) {
  Scope scope(rt);

  int64_t jsi_object_id = -1;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);

    JSIObjectWrapper* proxy = nullptr;

    // use previous wrapper if exeist
    std::pair<GROUPED_OBJECT_MAP::iterator, GROUPED_OBJECT_MAP::iterator>
        range = grouped_jsi_object_map_.equal_range(group);
    for (auto it = range.first; it != range.second; ++it) {
      JSIObjectWrapper* curr_obj = it->second;
      if (Object::strictEquals(rt, curr_obj->jsi_object_, obj)) {
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
    Object obj, int64_t jsi_object_id, const std::string& group_id)
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
  if (it == jsi_object_map_.end()) {
    return;
  }
  JSIObjectWrapper* wrapper = it->second;
  if (wrapper->ref_count() == 0) {
    grouped_jsi_object_map_.erase(wrapper->group_id_);
    dirty_jsi_object_set_.erase(wrapper);
    delete wrapper;
    jsi_object_map_.erase(it);
  } else {
    dirty_jsi_object_set_.insert(wrapper);
  }
}

void JSIObjectWrapperManager::ForceGcOnJSThread() {
  for (auto iter = dirty_jsi_object_set_.begin();
       iter != dirty_jsi_object_set_.end();) {
    if ((*iter)->ref_count() == 0) {
      grouped_jsi_object_map_.erase((*iter)->group_id_);
      JSI_OBJECT_MAP::iterator it = jsi_object_map_.find((*iter)->id_);
      delete *iter;
      iter = dirty_jsi_object_set_.erase(iter);
      jsi_object_map_.erase(it);
    } else {
      ++iter;
    }
  }
}

Value JSIObjectWrapperManager::GetJSIObjectByIDOnJSThread(
    Runtime& rt, int64_t jsi_object_id) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  JSI_OBJECT_MAP::iterator it = jsi_object_map_.find(jsi_object_id);
  if (it == jsi_object_map_.end()) {
    return Value::null();
  }

  auto wrapper = it->second;
  if (!wrapper) {
    return Value::null();
  }
  return Value(wrapper->jsi_object_);
}

void JSIObjectWrapperManager::DestroyOnJSThread() {
  std::lock_guard<std::mutex> lock(map_mutex_);
  for (auto it : jsi_object_map_) {
    delete it.second;
  }
  jsi_object_map_.clear();
  grouped_jsi_object_map_.clear();
  dirty_jsi_object_set_.clear();
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
