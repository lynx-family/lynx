// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/isolate.h"

#if defined(OS_ANDROID) || defined(OS_HARMONY)
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/no_destructor.h"
#include "clay/ui/resource/font_collection.h"

namespace clay {

#if defined(OS_ANDROID) || defined(OS_HARMONY)
void Setter(const lynx::fml::Thread::ThreadConfig& config) {
  if (::setpriority(PRIO_PROCESS, gettid(), 10) != 0) {
    FML_LOG(ERROR) << "Failed to set Workers task runner priority";
  }
}
#endif

// static
Isolate& Isolate::Instance() {
  static fml::NoDestructor<Isolate> manager;
  return *(manager.get());
}

Isolate::Isolate()
    : concurrent_message_loop_(
#if defined(OS_ANDROID) || defined(OS_HARMONY)
          fml::ConcurrentMessageLoop::Create(
              Setter, std::max(std::thread::hardware_concurrency() / 2, 1u))
#else
          fml::ConcurrentMessageLoop::Create(
              std::max(std::thread::hardware_concurrency() / 2, 1u))
#endif
              )
#ifndef ENABLE_SKITY
      ,
      skia_concurrent_executor_(
          [runner = concurrent_message_loop_->GetTaskRunner()](
              fml::closure work) { runner->PostTask(std::move(work)); })
#endif  // ENABLE_SKITY
{
  // Setting the executor.
#ifndef ENABLE_SKITY
  SkExecutor::SetDefault(&skia_concurrent_executor_);
#endif  // ENABLE_SKITY
  GraphicsIsolate::Instance().SetDelegate(this);
}

Isolate::~Isolate() {
#ifndef ENABLE_SKITY
  SkExecutor::SetDefault(nullptr);
#endif  // ENABLE_SKITY
}

std::shared_ptr<clay::FontCollection> Isolate::GetFontCollection() {
  return FontCollection::Instance();
}

std::shared_ptr<txt::FontCollection> Isolate::GetTxtFontCollection() {
  return FontCollection::Instance()->GetFontCollection();
}

std::shared_ptr<fml::ConcurrentTaskRunner>
Isolate::GetConcurrentWorkerTaskRunner() const {
  return concurrent_message_loop_->GetTaskRunner();
}

std::shared_ptr<fml::ConcurrentMessageLoop>
Isolate::GetConcurrentMessageLoop() {
  return concurrent_message_loop_;
}

GpuResourceCache* Isolate::GetResourceCache() {
  if (!resource_cache_) {
    resource_cache_ = std::make_unique<GpuResourceCache>();
  }
  return resource_cache_.get();
}

}  // namespace clay
