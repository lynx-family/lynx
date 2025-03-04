// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_RUNTIME_ACTOR_HOLDER_H_
#define CORE_SHELL_LYNX_RUNTIME_ACTOR_HOLDER_H_

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/runtime/piper/js/lynx_runtime.h"
#include "core/shell/lynx_actor_specialization.h"
namespace lynx {

namespace shell {

/*
 * When LynxShell::Destroy() be called, lynx_runtime actor in lynx_shell will be
 * put in this holder. This holder will hold lynx_runtime actor until ths JSB
 * task called in onDestroy() done.
 */
class LynxRuntimeActorHolder {
  friend class base::NoDestructor<LynxRuntimeActorHolder>;

 public:
  ~LynxRuntimeActorHolder() = default;
  LynxRuntimeActorHolder(const LynxRuntimeActorHolder&) = delete;
  LynxRuntimeActorHolder& operator=(const LynxRuntimeActorHolder&) = delete;
  LynxRuntimeActorHolder(LynxRuntimeActorHolder&&) = delete;
  LynxRuntimeActorHolder& operator=(LynxRuntimeActorHolder&&) = delete;

  static LynxRuntimeActorHolder* GetInstance() {
    static base::NoDestructor<LynxRuntimeActorHolder> sLynxRuntimeActorHolder;
    return sLynxRuntimeActorHolder.get();
  }

  using LynxRuntimeActor = std::shared_ptr<LynxActor<runtime::LynxRuntime>>;

  void Hold(LynxRuntimeActor lynx_runtime_actor,
            const std::string& js_group_thread_name = "");

  void PostDelayedRelease(int64_t,
                          const std::string& js_group_thread_name = "");

  void Release(int64_t runtime_id,
               const std::string& js_group_thread_name = "");

 private:
  LynxRuntimeActorHolder() = default;

  void ReleaseInternal(int64_t runtime_id);

  std::mutex mutex_;

  std::unordered_map<int64_t, LynxRuntimeActor> runtime_actor_container_;

  static constexpr int64_t kReleaseDelayedTime = 2000;  // ms
};

}  // namespace shell
}  // namespace lynx
#endif  // CORE_SHELL_LYNX_RUNTIME_ACTOR_HOLDER_H_
