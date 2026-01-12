// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/v8/v8_isolate_wrapper_node_impl.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "libplatform/libplatform.h"
#include "third_party/node/src/node.h"
#include "third_party/node/src/node_internals.h"
#include "uv.h"
#include "v8-cppgc.h"

namespace lynx {
namespace piper {

namespace {

const uint64_t kInitNodeEnvFlags = node::EnvironmentFlags::kDefaultFlags &
                                   node::EnvironmentFlags::kNoCreateInspector;

class V8PlatformHelper : public V8PlatformData {
 public:
  V8PlatformHelper() {}
  ~V8PlatformHelper() override { onDestory(); }

  static void RunUVTaskLoop(std::weak_ptr<V8PlatformHelper> helper);

  v8::Isolate* CreateIsolate();
  v8::Global<v8::Context> CreateNodeEnvContext(v8::Isolate* isolate);

  void onDestory();

 private:
  void InitUVLoop();

  static node::MultiIsolatePlatform* GetPlatform() {
    node::MultiIsolatePlatform* platform =
        static_cast<node::MultiIsolatePlatform*>(
            V8PlatformData::GetV8Platform());
    DCHECK(platform);
    return platform;
  }

  bool is_destory_ = {false};
  uv_loop_t uv_loop_;
  node::Environment* env_ = nullptr;
  node::IsolateData* isolate_data_ = nullptr;
};

void V8PlatformHelper::onDestory() {
  if (is_destory_) {
    return;
  }

  uv_run(&uv_loop_, UV_RUN_NOWAIT);
  uv_loop_close(&uv_loop_);

  if (env_) {
    node::FreeEnvironment(env_);
  }

  if (isolate_data_) {
    node::FreeIsolateData(isolate_data_);
  }
}

v8::Isolate* V8PlatformHelper::CreateIsolate() {
  InitUVLoop();

  std::shared_ptr<node::ArrayBufferAllocator> allocator =
      node::ArrayBufferAllocator::Create();
  return node::NewIsolate(allocator, &uv_loop_, GetPlatform());
}

v8::Global<v8::Context> V8PlatformHelper::CreateNodeEnvContext(
    v8::Isolate* isolate) {
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);

  auto context = node::NewContext(isolate);
  DCHECK(!context.IsEmpty());
  v8::Context::Scope context_scope(context);

  isolate_data_ = node::CreateIsolateData(isolate, &uv_loop_, GetPlatform());

  env_ = node::CreateEnvironment(
      isolate_data_, context, {}, {},
      static_cast<node::EnvironmentFlags::Flags>(kInitNodeEnvFlags));

  // load global data.
  static constexpr const char* script =
      "globalThis.node_exports={require:require, process:process};";
  node::LoadEnvironment(env_, script);

  v8::Global<v8::Context> global_context;
  global_context.Reset(isolate, context);
  return std::move(global_context);
}

void V8PlatformHelper::InitUVLoop() { uv_loop_init(&uv_loop_); }

void V8PlatformHelper::RunUVTaskLoop(std::weak_ptr<V8PlatformHelper> helper) {
  auto helper_spr = helper.lock();
  if (!helper_spr || helper_spr->is_destory_) {
    return;
  }

  uv_run(&helper_spr->uv_loop_, UV_RUN_NOWAIT);

  fml::MessageLoop::GetCurrent().GetTaskRunner()->PostDelayedTask(
      [helper]() { RunUVTaskLoop(helper); },
      fml::TimeDelta::FromMilliseconds(10));
}

}  // namespace

std::shared_ptr<V8IsolateInstance>
V8IsolateInstance::CreateV8IsolateInstance() {
  return std::make_shared<V8IsolateInstanceNodeImpl>();
}

V8IsolateInstanceNodeImpl::V8IsolateInstanceNodeImpl() = default;

V8IsolateInstanceNodeImpl::~V8IsolateInstanceNodeImpl() {
  if (isolate_ != nullptr) {
    auto helper = dynamic_cast<V8PlatformHelper*>(platform_data_.get());
    helper->onDestory();

    node_ctx_.Reset();
    node_exports_.Reset();
    isolate_->Dispose();
    LOGI("lynx ~V8IsolateInstance");
  }
}

void V8IsolateInstanceNodeImpl::InitIsolate(const char* arg, bool useSnapshot) {
  LOGI("lynx V8IsolateInstanceImpl::InitIsolate");
  auto helper = std::make_shared<V8PlatformHelper>();

  isolate_ = helper->CreateIsolate();

  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  node_ctx_ = helper->CreateNodeEnvContext(isolate_);
  {
    auto context = node_ctx_.Get(isolate_);
    v8::Context::Scope context_scope(context);

    auto global = context->Global();
    auto node_exports = global
                            ->Get(context, v8::String::NewFromUtf8Literal(
                                               isolate_, "node_exports"))
                            .ToLocalChecked();

    node_exports_.Reset(isolate_, node_exports);
  }

  V8PlatformHelper::RunUVTaskLoop(helper);
  platform_data_ = std::move(helper);
}

v8::Isolate* V8IsolateInstanceNodeImpl::Isolate() const { return isolate_; }

void V8IsolateInstanceNodeImpl::AddExtensionDataToGlobal(
    v8::Local<v8::Context> ctx, v8::Local<v8::Object> data) {
  data->Set(ctx, v8::String::NewFromUtf8Literal(isolate_, "node"),
            node_exports_.Get(isolate_))
      .FromJust();
}

}  // namespace piper
}  // namespace lynx
