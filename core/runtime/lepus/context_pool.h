// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_CONTEXT_POOL_H_
#define CORE_RUNTIME_LEPUS_CONTEXT_POOL_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/vector.h"
#include "core/runtime/lepus/context.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace lepus {

class LynxContextPool : public std::enable_shared_from_this<LynxContextPool> {
 public:
  static std::shared_ptr<LynxContextPool> Create(bool is_lepus_ng,
                                                 bool disable_tracing_gc);
  // ContextPool must check its own life cycle asynchronously when
  // replenishing the cache, so it can only exist in the form of shared_ptr
  static std::shared_ptr<LynxContextPool> Create(
      bool is_lepus_ng, bool disable_tracing_gc,
      const std::shared_ptr<ContextBundle>& context_bundle,
      const tasm::CompileOptions& compile_options,
      tasm::PageConfig* page_configs);

  ~LynxContextPool() = default;

  LynxContextPool(const LynxContextPool&) = delete;
  LynxContextPool& operator=(const LynxContextPool&) = delete;

  LynxContextPool(LynxContextPool&&) = delete;
  LynxContextPool& operator=(LynxContextPool&&) = delete;

  void FillPool(int32_t count);

  std::shared_ptr<lepus::Context> TakeContextSafely();

  void SetEnableAutoGenerate(bool enable);

 private:
  // The global pool doesn't hold context_bundle_ and need to check settings to
  // determine its size.
  // The local pool in TemplateBundle hold context_bundle_ and have no need to
  // check settings.
  LynxContextPool(bool is_lepus_ng, bool disable_tracing_gc)
      : is_lepus_ng_(is_lepus_ng), disable_tracing_gc_(disable_tracing_gc) {}

  LynxContextPool(bool is_lepus_ng, bool disable_tracing_gc,
                  const std::shared_ptr<ContextBundle>& context_bundle,
                  const tasm::CompileOptions& compile_options,
                  tasm::PageConfig* page_configs)
      : is_lepus_ng_(is_lepus_ng),
        disable_tracing_gc_(disable_tracing_gc),
        enable_signal_api_(
            page_configs ? page_configs->GetEnableSignalAPIBoolValue() : false),
        target_sdk_version_(compile_options.target_sdk_version_),
        context_bundle_(context_bundle),
        arch_option_(compile_options.arch_option_),
        enable_mts_pre_execute_(
            page_configs ? page_configs->GetEnableMTSPreExecute() : false) {}

  void AddContextSafely(int32_t count);

  bool enable_auto_generate_{true};

  const bool is_lepus_ng_{true};
  const bool disable_tracing_gc_{false};
  const bool enable_signal_api_{false};
  const std::string target_sdk_version_;
  const std::shared_ptr<ContextBundle> context_bundle_{nullptr};
  const tasm::ArchOption arch_option_{tasm::RADON_ARCH};
  const bool enable_mts_pre_execute_{false};

  std::mutex mtx_;
  base::InlineVector<std::shared_ptr<lepus::Context>, 8> contexts_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_CONTEXT_POOL_H_
