// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/message_loop.h"

#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/task_runner.h"
#include "base/include/no_destructor.h"

namespace lynx {
namespace fml {

namespace {
std::unique_ptr<MessageLoop>& GetThreadLocalLooper() {
  static thread_local base::NoDestructor<std::unique_ptr<MessageLoop>>
      tls_message_loop_instance;
  return *tls_message_loop_instance;
}
}  // namespace

MessageLoop& MessageLoop::GetCurrent() {
  auto* loop = GetThreadLocalLooper().get();
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(loop != nullptr);
  //     << "MessageLoop::EnsureInitializedForCurrentThread was not called on "
  //        "this thread prior to message loop use.";
  return *loop;
}

MessageLoop& MessageLoop::EnsureInitializedForCurrentThread(
    void* platform_loop) {
  auto& looper_storage = GetThreadLocalLooper();
  if (looper_storage == nullptr) {
    looper_storage.reset(new MessageLoop(platform_loop));
  }
  return *looper_storage;
}

MessageLoop* MessageLoop::IsInitializedForCurrentThread() {
  return GetThreadLocalLooper().get();
}

MessageLoop::MessageLoop(void* platform_loop)
    : loop_(MessageLoopImpl::Create(platform_loop)),
      task_runner_(fml::MakeRefCounted<fml::TaskRunner>(loop_)) {
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(loop_);
  LYNX_BASE_CHECK(task_runner_);

  // Cannot get the current MessageLoop in the constructor of this TaskRunner
  // because the MessageLoop constructor has not finished yet.
  // It should be explicitly bound to the current MessageLoop here.
  loop_->Bind(task_runner_->GetTaskQueueId());
}

MessageLoop::~MessageLoop() = default;

void MessageLoop::Run() { loop_->DoRun(); }

void MessageLoop::Terminate() { loop_->DoTerminate(); }

const fml::RefPtr<fml::TaskRunner>& MessageLoop::GetTaskRunner() const {
  return task_runner_;
}

const fml::RefPtr<MessageLoopImpl>& MessageLoop::GetLoopImpl() const {
  return loop_;
}

void MessageLoop::AddTaskObserver(intptr_t key, base::closure callback) {
  loop_->AddTaskObserver(key, std::move(callback));
}

void MessageLoop::RemoveTaskObserver(intptr_t key) {
  loop_->RemoveTaskObserver(key);
}

void MessageLoop::RunExpiredTasksNow() { loop_->RunExpiredTasksNow(); }

void MessageLoop::SetMessageLoopRestrictionDuration(
    fml::TimeDelta restriction_duration) {
  loop_->SetRestrictionDuration(restriction_duration);
}

TaskQueueId MessageLoop::GetCurrentTaskQueueId() {
  auto* loop = GetThreadLocalLooper().get();
  // TODO(zhengsenyao): Replace LYNX_BASE_CHECK with CHECK when CHECK available.
  LYNX_BASE_CHECK(loop != nullptr);
  //     << "MessageLoop::EnsureInitializedForCurrentThread was not called on "
  //        "this thread prior to message loop use.";
  return loop->GetTaskRunner()->GetTaskQueueId();
}

void MessageLoop::Bind(const TaskQueueId& queue_id) {
  // This TaskRunner should bind to the current MessageLoop in the constructor.
  if (queue_id == task_runner_->GetTaskQueueId()) {
    return;
  }

  loop_->Bind(queue_id);
}

void MessageLoop::UnBind(const TaskQueueId& queue_id) {
  // Unbinding this TaskRunner from the current MessageLoop is illegal.
  LYNX_BASE_CHECK(queue_id != task_runner_->GetTaskQueueId());

  loop_->UnBind(queue_id);
}
}  // namespace fml
}  // namespace lynx
