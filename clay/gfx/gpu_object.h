// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GPU_OBJECT_H_
#define CLAY_GFX_GPU_OBJECT_H_

#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gpu_ref_object.h"
#include "clay/gfx/rendering_backend.h"
namespace clay {

// A queue that holds gpu objects that must be destructed on the given task
// runner.
template <class T>
class UnrefQueue : public fml::RefCountedThreadSafe<UnrefQueue<T>> {
 public:
  using Context = T;
#ifndef ENABLE_SKITY
  using ContextPtr = sk_sp<Context>;
#else
  using ContextPtr = std::shared_ptr<Context>;
#endif  // ENABLE_SKITY

  bool IsShared() const { return is_shared_; }

  void Unref(GPURefObject* object) {
    std::scoped_lock lock(mutex_);
    objects_.push_back(object);
    if (start_auto_pending_drain_) {
      TriggerPendingDrainIfNeeded();
    }
  }

  void StartAutoPendingDrain() {
    std::scoped_lock lock(mutex_);
    if (start_auto_pending_drain_) {
      return;
    }
    start_auto_pending_drain_ = true;
#ifndef ENABLE_SKITY
    if (objects_.empty() && textures_.empty()) {
      return;
    }

#else
    if (objects_.empty()) {
      return;
    }
#endif  // ENABLE_SKITY
    TriggerPendingDrainIfNeeded();
  }

  fml::RefPtr<fml::TaskRunner> GetTaskRunner() { return task_runner_; }

#ifndef ENABLE_SKITY
  void DeleteTexture(GrBackendTexture texture) {
    std::scoped_lock lock(mutex_);
    textures_.push_back(texture);
    if (start_auto_pending_drain_) {
      TriggerPendingDrainIfNeeded();
    }
  }
#endif  // ENABLE_SKITY

  // Usually, the drain is called automatically. However, during IO manager
  // shutdown (when the platform side reference to the OpenGL context is about
  // to go away), we may need to pre-emptively drain the unref queue. It is the
  // responsibility of the caller to ensure that no further unref are queued
  // after this call.
  void Drain() {
#if defined(TRACE_EVENT)
    TRACE_EVENT("clay", "GPUUnrefQueue::Drain");
#endif  // TRACE_EVENT
    std::deque<GPURefObject*> objects;
#ifndef ENABLE_SKITY
    std::deque<GrBackendTexture> textures;
    {
      std::scoped_lock lock(mutex_);
      objects_.swap(objects);
      textures_.swap(textures);
      drain_pending_ = false;
    }
    DoDrain(objects, textures, context_);
#else
    {
      std::scoped_lock lock(mutex_);
      objects_.swap(objects);
      drain_pending_ = false;
    }
    DoDrain(objects, context_);
#endif  // ENABLE_SKITY
  }

  void SetContext(ContextPtr context) {
    std::scoped_lock lock(mutex_);
    context_ = context;
  }

  ContextPtr GetContext() {
    std::scoped_lock lock(mutex_);
    return context_;
  }

 private:
  const fml::RefPtr<fml::TaskRunner> task_runner_;
  bool is_shared_;
  const fml::TimeDelta drain_delay_;
  bool start_auto_pending_drain_ = false;
  bool drain_pending_ = false;
  std::recursive_mutex mutex_;
  std::deque<GPURefObject*> objects_;
  ContextPtr context_;
#ifndef ENABLE_SKITY
  std::deque<GrBackendTexture> textures_;
#endif  // ENABLE_SKITY
  explicit UnrefQueue(
      fml::RefPtr<fml::TaskRunner> task_runner, bool is_shared = true,
      fml::TimeDelta delay = fml::TimeDelta::FromMilliseconds(8))
      : task_runner_(task_runner), is_shared_(is_shared), drain_delay_(delay) {
    FML_DCHECK(task_runner_);
  }

  ~UnrefQueue() {
    // The Context must be deleted on the task runner thread.
    // Transfer ownership of the UnrefQueue's Context reference
    // into a task queued to that thread.
#ifndef ENABLE_SKITY
    Context* raw_context = context_.release();
    fml::TaskRunner::RunNowOrPostTask(
        task_runner_,
        [raw_context, objects = std::move(objects_),
         textures = std::move(textures_), task_runner = task_runner_]() {
          sk_sp<Context> context(raw_context);
          DoDrain(objects, textures, context);
          context.reset();
        });
#else
    fml::TaskRunner::RunNowOrPostTask(
        task_runner_,
        [context = context_, objects = std::move(objects_),
         task_runner = task_runner_]() { DoDrain(objects, context); });
#endif  // ENABLE_SKITY
  }

  void TriggerPendingDrainIfNeeded() {
    if (is_shared_) {
      if (!drain_pending_) {
        drain_pending_ = true;
        task_runner_->PostDelayedTask(
            [strong = fml::Ref(this)]() { strong->Drain(); }, drain_delay_);
      }
    } else {
      // On Windows or Mac, the GPUUnRefQueue is not shared, the raster task
      // runner will be terminated after the destruction of the Rasterizer
      // object.
      fml::TaskRunner::RunNowOrPostTask(
          task_runner_, [strong = fml::Ref(this)]() { strong->Drain(); });
    }
  }

#ifndef ENABLE_SKITY
  static void DoDrain(const std::deque<GPURefObject*>& gpu_objects,
                      const std::deque<GrBackendTexture>& textures,
                      ContextPtr context) {
    for (GPURefObject* gpu_object : gpu_objects) {
      gpu_object->Release();
    }
    if (context) {
      for (GrBackendTexture texture : textures) {
        context->deleteBackendTexture(texture);
      }

      if (!gpu_objects.empty()) {
        context->performDeferredCleanup(std::chrono::milliseconds(0));
      }
    }
  }
#else
  static void DoDrain(const std::deque<GPURefObject*>& gpu_objects,
                      ContextPtr context) {
    for (GPURefObject* gpu_object : gpu_objects) {
      gpu_object->Release();
    }
  }
#endif  // ENABLE_SKITY

  FML_FRIEND_REF_COUNTED_THREAD_SAFE(UnrefQueue);
  FML_FRIEND_MAKE_REF_COUNTED(UnrefQueue);
  BASE_DISALLOW_COPY_AND_ASSIGN(UnrefQueue);
};

using GPUUnrefQueue = UnrefQueue<GrContext>;

// An object whose deallocation needs to be performed on an specific unref
// queue. The template argument U need to have a call operator that returns
// that unref queue.
template <class T>
class GPUObject {
 public:
  using ObjectType = T;

  GPUObject() = default;
  GPUObject(fml::RefPtr<ObjectType> object, fml::RefPtr<GPUUnrefQueue> queue)
      : object_(std::move(object)), queue_(std::move(queue)) {
    FML_DCHECK(object_);
  }
  GPUObject(GPUObject&&) = default;
  ~GPUObject() { reset(); }

  GPUObject& operator=(GPUObject&&) = default;

  fml::RefPtr<ObjectType> object() const { return object_; }

  void reset() {
    if (object_ && queue_) {
      queue_->Unref(object_.AbandonRef());
    }
    queue_ = nullptr;
    FML_DCHECK(object_ == nullptr);
  }

 private:
  fml::RefPtr<ObjectType> object_;
  fml::RefPtr<GPUUnrefQueue> queue_;

  BASE_DISALLOW_COPY_AND_ASSIGN(GPUObject);
};

}  // namespace clay

#endif  // CLAY_GFX_GPU_OBJECT_H_
