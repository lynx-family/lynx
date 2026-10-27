// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <future>
#include <utility>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "clay/gfx/gpu_object.h"
#include "clay/testing/thread_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace clay {
namespace testing {

using clay::GPUUnrefQueue;

class TestGPUObject : public clay::GPURefObject {
 public:
  TestGPUObject(std::shared_ptr<fml::AutoResetWaitableEvent> latch,
                fml::TaskQueueId* dtor_task_queue_id)
      : latch_(std::move(latch)), dtor_task_queue_id_(dtor_task_queue_id) {}

  virtual ~TestGPUObject() {
    if (dtor_task_queue_id_) {
      *dtor_task_queue_id_ = fml::MessageLoop::GetCurrentTaskQueueId();
    }
    latch_->Signal();
  }

 private:
  std::shared_ptr<fml::AutoResetWaitableEvent> latch_;
  fml::TaskQueueId* dtor_task_queue_id_;
};

class TestContext : public SkRefCnt {
 public:
  TestContext(std::shared_ptr<fml::AutoResetWaitableEvent> latch,
              fml::TaskQueueId* dtor_task_queue_id)
      : latch_(std::move(latch)), dtor_task_queue_id_(dtor_task_queue_id) {}
  virtual ~TestContext() {
    if (dtor_task_queue_id_) {
      *dtor_task_queue_id_ = fml::MessageLoop::GetCurrentTaskQueueId();
    }
    latch_->Signal();
  }
#ifndef ENABLE_SKITY
  void performDeferredCleanup(std::chrono::milliseconds msNotUsed) {}
  void deleteBackendTexture(const GrBackendTexture& texture) {}
#endif  // ENABLE_SKITY

 private:
  std::shared_ptr<fml::AutoResetWaitableEvent> latch_;
  fml::TaskQueueId* dtor_task_queue_id_;
};

class GpuObjectTest : public ThreadTest {
 public:
  GpuObjectTest()
      : unref_task_runner_(CreateNewThread()),
#if OS_WIN || OS_MAC
        unref_queue_(fml::MakeRefCounted<GPUUnrefQueue>(
            unref_task_runner(), false, fml::TimeDelta::FromMilliseconds(0))) {
#else
        unref_queue_(fml::MakeRefCounted<GPUUnrefQueue>(
            unref_task_runner(), true, fml::TimeDelta::FromMilliseconds(0))) {
#endif
    // The unref queues must be created in the same thread of the
    // unref_task_runner so the queue can access the same-thread-only WeakPtr
    // of the GrContext constructed during the creation.
    std::promise<bool> queues_created;
    unref_task_runner_->PostTask([this, &queues_created]() {
      unref_queue_ = fml::MakeRefCounted<GPUUnrefQueue>(unref_task_runner());
      queues_created.set_value(true);
    });
    queues_created.get_future().wait();
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
  }

  fml::RefPtr<fml::TaskRunner> unref_task_runner() {
    return unref_task_runner_;
  }
  fml::RefPtr<GPUUnrefQueue> unref_queue() { return unref_queue_; }

 private:
  fml::RefPtr<fml::TaskRunner> unref_task_runner_;
  fml::RefPtr<GPUUnrefQueue> unref_queue_;
};

TEST_F(GpuObjectTest, QueueSimple) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::TaskQueueId dtor_task_queue_id(0);
  clay::GPURefObject* ref_object =
      new TestGPUObject(latch, &dtor_task_queue_id);
  unref_queue()->Unref(ref_object);

  // Drain the queue on the unref task runner thread.
  unref_task_runner()->PostTask(
      [unref_queue = unref_queue(), ref_object]() { unref_queue->Drain(); });
  latch->Wait();
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

TEST_F(GpuObjectTest, ObjectDestructor) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::TaskQueueId dtor_task_queue_id(0);
  auto object = fml::MakeRefCounted<TestGPUObject>(latch, &dtor_task_queue_id);
  {
    clay::GPUObject<TestGPUObject> gpu_object(std::move(object), unref_queue());
  }

  // Drain the queue on the unref task runner thread.
  unref_task_runner()->PostTask(
      [unref_queue = unref_queue()]() { unref_queue->Drain(); });

  latch->Wait();
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

TEST_F(GpuObjectTest, ObjectReset) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::TaskQueueId dtor_task_queue_id(0);
  clay::GPUObject<TestGPUObject> gpu_object(
      fml::MakeRefCounted<TestGPUObject>(latch, &dtor_task_queue_id),
      unref_queue());
  // Verify that explicitly resetting the GPU object queues and unref.
  gpu_object.reset();
  ASSERT_EQ(gpu_object.object(), nullptr);

  // Drain the queue on the unref task runner thread.
  unref_task_runner()->PostTask(
      [unref_queue = unref_queue()]() { unref_queue->Drain(); });

  latch->Wait();
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

TEST_F(GpuObjectTest, AutoPendingDrain) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::TaskQueueId dtor_task_queue_id(0);
  unref_queue()->StartAutoPendingDrain();
  auto object = fml::MakeRefCounted<TestGPUObject>(latch, &dtor_task_queue_id);
  {
    // This object will be auto drained.
    clay::GPUObject<TestGPUObject> gpu_object(std::move(object), unref_queue());
  }

  latch->Wait();
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

TEST_F(GpuObjectTest, ObjectResetTwice) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::TaskQueueId dtor_task_queue_id(0);
  clay::GPUObject<TestGPUObject> gpu_object(
      fml::MakeRefCounted<TestGPUObject>(latch, &dtor_task_queue_id),
      unref_queue());

  gpu_object.reset();
  ASSERT_EQ(gpu_object.object(), nullptr);
  gpu_object.reset();
  ASSERT_EQ(gpu_object.object(), nullptr);

  // Drain the queue on the unref task runner thread.
  unref_task_runner()->PostTask(
      [unref_queue = unref_queue()]() { unref_queue->Drain(); });

  latch->Wait();
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

TEST_F(GpuObjectTest, UnrefContextInTaskRunnerThread) {
  std::shared_ptr<fml::AutoResetWaitableEvent> latch =
      std::make_shared<fml::AutoResetWaitableEvent>();
  fml::RefPtr<clay::UnrefQueue<TestContext>> unref_queue;
  fml::TaskQueueId dtor_task_queue_id(0);
  unref_task_runner()->PostTask([&]() {
    auto context = sk_make_sp<TestContext>(latch, &dtor_task_queue_id);
    unref_queue =
        fml::MakeRefCounted<clay::UnrefQueue<TestContext>>(unref_task_runner());
    unref_queue->SetContext(std::move(context));
    latch->Signal();
  });
  latch->Wait();

  // Delete the unref queue, it will schedule a task to unref the context in
  // the task runner's thread.
  unref_queue = nullptr;
  latch->Wait();
  // Verify that the context was destroyed in the task runner's thread.
  ASSERT_EQ(dtor_task_queue_id, unref_task_runner()->GetTaskQueueId());
}

}  // namespace testing
}  // namespace clay
