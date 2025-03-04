// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "jsbridge/bindings/gen_test/napi_gen_test_command_buffer.h"

#include "jsbridge/bindings/gen_test/napi_async_object.h"
#include "base/include/no_destructor.h"
#include "third_party/binding/napi/napi_base_wrap.h"

using Napi::Array;
using Napi::ArrayBuffer;
using Napi::CallbackInfo;
using Napi::Function;
using Napi::FunctionReference;
using Napi::Number;
using Napi::Object;
using Napi::ObjectReference;
using Napi::ObjectWrap;
using Napi::ScriptWrappable;
using Napi::String;
using Napi::Value;

namespace lynx {
namespace gen_test {

namespace {
const uint64_t kNapiGenTestCommandBufferID =
    reinterpret_cast<uint64_t>(&kNapiGenTestCommandBufferID);
const uint64_t kNapiGenTestCommandBufferCtorID =
    reinterpret_cast<uint64_t>(&kNapiGenTestCommandBufferCtorID);
constexpr size_t kCommandBufferSize = 200 * 1024;

using Wrapped = binding::NapiBaseWrapped<NapiGenTestCommandBuffer>;
typedef Value (NapiGenTestCommandBuffer::*InstanceCallback)(
    const CallbackInfo& info);

void AddInstanceMethod(std::vector<Wrapped::PropertyDescriptor>& props,
                       const char* name, InstanceCallback method) {
  props.push_back(
      Wrapped::InstanceMethod(name, method, napi_default_jsproperty));
}
}  // namespace

NapiGenTestCommandBuffer::NapiGenTestCommandBuffer(const CallbackInfo& info)
    : env_(info.Env()) {
  auto obj = info.This().ToObject();
  // This effectively leaks the object until current Env is destroyed.
  js_object_.Reset(obj, 1);

  env_.SetInstanceData(kNapiGenTestCommandBufferID, this, nullptr, nullptr);

  auto buffer = ArrayBuffer::New(env_, kCommandBufferSize);
  buffer_ = static_cast<uint32_t*>(buffer.Data());
  buffer_[0] = 1u;
  obj["buffer"] = buffer;

  auto objects = Array::New(env_);
  objects_.Reset(objects, 1);
  obj["objects"] = objects;

  auto active = Array::New(env_);
  active_contexts_.Reset(active, 1);
  obj["active"] = active;
}

// static
uint32_t NapiGenTestCommandBuffer::RegisterBufferedObject(
    ScriptWrappable* wrapped) {
  // Start with a random number in [1000, 2000).
  static uint32_t s_incremental_id = std::abs(std::rand() % 1000) + 1000;
  // We really don't want the id to overflow. This also helps differentiate null
  // objects.
  CHECK(s_incremental_id);
  ObjectRegistry()[s_incremental_id] = wrapped;
  return s_incremental_id++;
}

// static
void NapiGenTestCommandBuffer::UnregisterBufferedObject(
    ScriptWrappable* wrapped, uint32_t id) {
  DCHECK(ObjectRegistry().count(id));
  DCHECK(ObjectRegistry()[id] == wrapped);
  ObjectRegistry().erase(id);
}

// static
void NapiGenTestCommandBuffer::RegisterAsyncObject(
    NapiAsyncObject* async_object) {
  NapiGenTestCommandBuffer* cb =
      async_object->NapiEnv().GetInstanceData<NapiGenTestCommandBuffer>(
          kNapiGenTestCommandBufferID);
  DCHECK(cb);
  if (cb) {
    DCHECK(!cb->async_object_registry_.count(async_object->id()));
    cb->async_object_registry_.insert({async_object->id(), async_object});
  }
}

// static
void NapiGenTestCommandBuffer::UnregisterAsyncObject(
    NapiAsyncObject* async_object) {
  NapiGenTestCommandBuffer* cb =
      async_object->NapiEnv().GetInstanceData<NapiGenTestCommandBuffer>(
          kNapiGenTestCommandBufferID);
  DCHECK(cb);
  if (cb) {
    DCHECK(cb->async_object_registry_.count(async_object->id()));
    DCHECK(cb->async_object_registry_[async_object->id()] ==
                   async_object);
    cb->async_object_registry_.erase(async_object->id());
  }
}

NapiAsyncObject* NapiGenTestCommandBuffer::FromAsyncId(uint32_t async_id) {
  // Caller may pass in an invalid value (0), except for the case of creation.
  if (auto it = async_object_registry_.find(async_id);
      it != async_object_registry_.end()) {
    return static_cast<NapiAsyncObject*>(it->second);
  }
  return nullptr;
}

// static
void NapiGenTestCommandBuffer::OrphanImpl(Napi::Env env,
                                          std::unique_ptr<ImplBase> impl,
                                          uint32_t id) {
  NapiGenTestCommandBuffer* cb = env.GetInstanceData<NapiGenTestCommandBuffer>(
      kNapiGenTestCommandBufferID);
  DCHECK(cb);
  if (cb) {
    DCHECK(!cb->orphans_.count(id));
    LOGI("Orphaning id: " << id << ", env: " << env
                                  << ", impl: " << impl);
    cb->orphans_.insert({id, std::move(impl)});
    cb->RequestVSync();
  }
}

// static
std::map<uint32_t, ScriptWrappable*>&
NapiGenTestCommandBuffer::ObjectRegistry() {
  static base::NoDestructor<std::map<uint32_t, ScriptWrappable*>> registry_;
  return *registry_;
}

// static
Napi::Value NapiGenTestCommandBuffer::GetCommandBufferInstance(
    const CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (NapiGenTestCommandBuffer* cb =
          env.GetInstanceData<NapiGenTestCommandBuffer>(
              kNapiGenTestCommandBufferID)) {
    return cb->js_object_.Value();
  }

  // Run once per Env.
  std::vector<Wrapped::PropertyDescriptor> props;

  // Methods
  AddInstanceMethod(props, "call", &NapiGenTestCommandBuffer::Call);
  AddInstanceMethod(props, "flush", &NapiGenTestCommandBuffer::Flush);

  auto* clazz =
      new Napi::Class(Wrapped::DefineClass(env, "GenTestCommandBuffer", props));
  env.SetInstanceData<Napi::Class>(kNapiGenTestCommandBufferCtorID, clazz);
  return clazz->Get(env).New({});
}

// static
void NapiGenTestCommandBuffer::Install(Napi::Env env, Napi::Object& target) {
  if (target.Has("getCommandBuffer").FromMaybe(false)) {
    return;
  }
  target.Set("getCommandBuffer",
             Napi::Function::New(
                 env, &NapiGenTestCommandBuffer::GetCommandBufferInstance));
}

// static
void NapiGenTestCommandBuffer::FlushCommandBuffer(Napi::Env env,
                                                  bool is_frame) {
  if (auto* cb = env.GetInstanceData<NapiGenTestCommandBuffer>(
          kNapiGenTestCommandBufferID)) {
    cb->DoFlush(env);
  }
}

Value NapiGenTestCommandBuffer::Call(const CallbackInfo& info) {
  return DoFlush(info.Env(), false);
}

Value NapiGenTestCommandBuffer::Flush(const CallbackInfo& info) {
  return DoFlush(info.Env());
}

Value NapiGenTestCommandBuffer::DoFlush(Napi::Env env,
                                        bool reset_argument_cache) {
  if (is_in_flush_) {
    return Value();
  }
  AutoReset<bool> flushing(&is_in_flush_, true);

  uint32_t length = buffer_[0];
  if (length == 1u) {
    // Recycle orphans even if there are no commands.
    orphans_.clear();
    return Value();
  }

  RequestVSync();

  for (const auto& [key, val] : orphans_) {
    LOGI("Flushing with orphan id: " << key << ", impl: " << val);
  }

  Value rv = RunBuffer(buffer_, length, env);

  // Reset buffer length.
  buffer_[0] = 1u;

  if (reset_argument_cache) {
    objects_.Value()["length"] = 0;
  }

  // Recycle orphans after executing all possible commands from them.
  orphans_.clear();

  return rv;
}

void NapiGenTestCommandBuffer::RequestVSync() {}

// static
void NapiGenTestCommandBuffer::LogTypeError(const char* method,
                                            const char* arg_name, uint32_t id) {
  LOGE("Method " << method << " called with invalid " << arg_name
                         << " argument id: " << id);
}

}  // namespace gen_test
}  // namespace lynx
