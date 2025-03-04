// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_runtime_actor_holder.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace shell {

void LynxRuntimeActorHolder::Hold(LynxRuntimeActor lynx_runtime_actor,
                                  const std::string& js_group_thread_name) {
  // This function must run in js thread!
  DCHECK(lynx_runtime_actor->CanRunNow());
  {
    std::lock_guard<std::mutex> lock(mutex_);
    runtime_actor_container_.emplace(lynx_runtime_actor->Impl()->GetRuntimeId(),
                                     lynx_runtime_actor);
  }
}

void LynxRuntimeActorHolder::PostDelayedRelease(
    int64_t runtime_id, const std::string& js_group_thread_name) {
  // This function must run in js thread!
  fml::RefPtr<fml::TaskRunner> js_runner =
      base::TaskRunnerManufactor::GetJSRunner(js_group_thread_name);
  DCHECK(js_runner->RunsTasksOnCurrentThread());
  js_runner->PostDelayedTask(
      [this, runtime_id]() { ReleaseInternal(runtime_id); },
      fml::TimeDelta::FromMilliseconds(kReleaseDelayedTime));
}

void LynxRuntimeActorHolder::Release(int64_t runtime_id,
                                     const std::string& js_group_thread_name) {
  // This function must run in js thread!
  DCHECK(base::TaskRunnerManufactor::GetJSRunner(js_group_thread_name)
             ->RunsTasksOnCurrentThread());
  ReleaseInternal(runtime_id);
}

void LynxRuntimeActorHolder::ReleaseInternal(int64_t runtime_id) {
  // This function must run in js thread!
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = runtime_actor_container_.find(runtime_id);
  if (it != runtime_actor_container_.end()) {
    (it->second)->Act([runtime_id](auto& runtime) {
      runtime = nullptr;
      tasm::report::FeatureCounter::Instance()->ClearAndReport(
          (int32_t)runtime_id);
    });
    runtime_actor_container_.erase(runtime_id);
  }
}

}  // namespace shell
}  // namespace lynx
