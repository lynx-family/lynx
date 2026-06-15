// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/v8/v8_isolate_wrapper_node_impl.h"

#include <memory>
#include <mutex>
#include <string>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/platform/node/message_loop_node.h"
#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "core/renderer/utils/lynx_env.h"
#include "libplatform/libplatform.h"
#include "third_party/node/src/node.h"
#include "third_party/node/src/node_internals.h"
#include "uv.h"
#include "v8.h"
#include "v8-cppgc.h"

namespace lynx {
namespace runtime {
namespace js {

namespace {

class V8PlatformHelper : public V8PlatformData {
 public:
  V8PlatformHelper() {}
  virtual ~V8PlatformHelper() { onDestory(); }

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
  fml::MessageLoopNode* message_loop_ = nullptr;
  uv_loop_t* uv_loop_;
  std::unique_ptr<v8::Locker> node_loop_locker_;
  std::unique_ptr<v8::Isolate::Scope> node_loop_isolate_scope_;
};

void V8PlatformHelper::onDestory() {
  if (is_destory_) {
    return;
  }
  if (message_loop_) {
    message_loop_->SetUVRunCallback(nullptr);
    message_loop_ = nullptr;
  }
  node_loop_isolate_scope_.reset();
  node_loop_locker_.reset();
  is_destory_ = true;
}

v8::Isolate* V8PlatformHelper::CreateIsolate() {
  InitUVLoop();

  std::shared_ptr<node::ArrayBufferAllocator> allocator =
      node::ArrayBufferAllocator::Create();
  auto* isolate = node::NewIsolate(allocator, uv_loop_, GetPlatform());
  node_loop_locker_ = std::make_unique<v8::Locker>(isolate);
  node_loop_isolate_scope_ = std::make_unique<v8::Isolate::Scope>(isolate);
  if (message_loop_) {
    message_loop_->SetUVRunCallback(
        [isolate](uv_loop_t* loop, uv_run_mode mode) {
          v8::Locker locker(isolate);
          v8::Isolate::Scope isolate_scope(isolate);
          return uv_run(loop, mode);
        });
  }
  return isolate;
}

void V8PlatformHelper::InitUVLoop() {
  message_loop_ = reinterpret_cast<fml::MessageLoopNode*>(
      fml::MessageLoop::GetCurrent().GetLoopImpl().get());
  uv_loop_ = message_loop_->GetUVLoop();
}

}  // namespace

V8IsolateInstanceNodeImpl::V8IsolateInstanceNodeImpl() = default;

V8IsolateInstanceNodeImpl::~V8IsolateInstanceNodeImpl() {
  if (isolate_ != nullptr) {
    auto helper = static_cast<V8PlatformHelper*>(platform_data_.get());
    helper->onDestory();
    isolate_->Dispose();
    LOGI("lynx ~V8IsolateInstance");
  }
}

void V8IsolateInstanceNodeImpl::InitIsolate(const char* arg, bool useSnapshot) {
  LOGI("lynx V8IsolateInstanceImpl::InitIsolate");
  auto helper = std::make_shared<V8PlatformHelper>();

  isolate_ = helper->CreateIsolate();

  platform_data_ = std::move(helper);
}

v8::Isolate* V8IsolateInstanceNodeImpl::Isolate() const { return isolate_; }

}  // namespace js
}  // namespace runtime
}  // namespace lynx
