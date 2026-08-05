// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_LYNX_GLOBAL_POOL_H_
#define CORE_RENDERER_LYNX_GLOBAL_POOL_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/shell/runtime/mts/mts_runtime_pool.h"

namespace lynx {
namespace tasm {

/**
 * A singleton class to store the global native cache.
 * Contains global pools for MTS runtimes that can be prepared before pages
 * claim them.
 */
class LynxGlobalPool {
 public:
  static LynxGlobalPool& GetInstance();

  ~LynxGlobalPool() = default;
  LynxGlobalPool(const LynxGlobalPool&) = delete;
  LynxGlobalPool& operator=(const LynxGlobalPool&) = delete;
  LynxGlobalPool(LynxGlobalPool&&) = delete;
  LynxGlobalPool& operator=(LynxGlobalPool&&) = delete;

  // Only called when LynxEnv is initialized
  void PreparePool();
  void PreparePool(runtime::ContextType context_type, int32_t count);
  void PreparePoolSync(runtime::ContextType context_type, int32_t count);

  shell::MTSRuntimePool& GetQuickContextPool() {
    return *GetPool(runtime::ContextType::LepusNGContextType);
  }
  shell::MTSRuntimePool* GetPool(runtime::ContextType context_type);

 private:
  LynxGlobalPool() = default;

  std::mutex context_pools_mutex_;
  std::unordered_map<runtime::ContextType,
                     std::shared_ptr<shell::MTSRuntimePool>>
      context_pools_;

  friend class base::NoDestructor<LynxGlobalPool>;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_LYNX_GLOBAL_POOL_H_
