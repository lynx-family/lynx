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
  uv_loop_t* uv_loop_;
};

void V8PlatformHelper::onDestory() {
  if (is_destory_) {
    return;
  }
}

v8::Isolate* V8PlatformHelper::CreateIsolate() {
  InitUVLoop();

  std::shared_ptr<node::ArrayBufferAllocator> allocator =
      node::ArrayBufferAllocator::Create();
  return node::NewIsolate(allocator, uv_loop_, GetPlatform());
}

void V8PlatformHelper::InitUVLoop() {
  auto* messageloop = reinterpret_cast<fml::MessageLoopNode*>(
      fml::MessageLoop::GetCurrent().GetLoopImpl().get());
  uv_loop_ = messageloop->GetUVLoop();
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
