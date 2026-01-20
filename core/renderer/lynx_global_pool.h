// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_LYNX_GLOBAL_POOL_H_
#define CORE_RENDERER_LYNX_GLOBAL_POOL_H_

#include <memory>

#include "base/include/no_destructor.h"
#include "core/runtime/lepus/context_pool.h"

namespace lynx {
namespace tasm {

/**
 * A singleton class to store the global native cache.
 * Currently only contains quick_context_pool.
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

  lepus::LynxContextPool& GetQuickContextPool() { return *quick_context_pool_; }

 private:
  LynxGlobalPool()
      : quick_context_pool_(lepus::LynxContextPool::Create(true, false)){};

  std::shared_ptr<lepus::LynxContextPool> quick_context_pool_;

  friend class base::NoDestructor<LynxGlobalPool>;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_LYNX_GLOBAL_POOL_H_
