// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_POOL_H_
#define CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_POOL_H_

#include <memory>
#include <mutex>
#include <utility>

#include "base/include/vector.h"

namespace lynx {
namespace lepus {

class ContextBundle;
class QuickContext;

class QuickContextPool : public std::enable_shared_from_this<QuickContextPool> {
 public:
  // QuickContextPool must check its own life cycle asynchronously when
  // replenishing the cache, so it can only exist in the form of shared_ptr
  static std::shared_ptr<QuickContextPool> Create(
      const std::shared_ptr<ContextBundle>& context_bundle = nullptr);

  ~QuickContextPool() = default;

  QuickContextPool(const QuickContextPool&) = delete;
  QuickContextPool& operator=(const QuickContextPool&) = delete;

  QuickContextPool(QuickContextPool&&) = delete;
  QuickContextPool& operator=(QuickContextPool&&) = delete;

  void FillPool(int32_t count);

  std::shared_ptr<lepus::QuickContext> TakeContextSafely();

  void SetEnableAutoGenerate(bool enable);

 private:
  // The global pool doesn't hold context_bundle_ and need to check settings to
  // determine its size.
  // The local pool in TemplateBundle hold context_bundle_ and have no need to
  // check settings.
  explicit QuickContextPool(
      const std::shared_ptr<ContextBundle>& context_bundle)
      : need_check_settings_(context_bundle == nullptr),
        context_bundle_(context_bundle) {}

  int32_t TryCheckSettings(int32_t default_value);

  void AddContextSafely(int32_t count);

  base::InlineVector<std::shared_ptr<lepus::QuickContext>, 8> contexts_;
  std::mutex mtx_;
  bool need_check_settings_{true};
  std::shared_ptr<ContextBundle> context_bundle_{nullptr};
  bool enable_auto_generate_{true};
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_QUICK_CONTEXT_POOL_H_
