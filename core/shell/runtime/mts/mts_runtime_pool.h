// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_
#define CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "base/include/vector.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace shell {

class MTSRuntimePool : public std::enable_shared_from_this<MTSRuntimePool> {
 public:
  static std::shared_ptr<MTSRuntimePool> Create(
      runtime::ContextType context_type, bool disable_tracing_gc);
  // ContextPool must check its own life cycle asynchronously when
  // replenishing the cache, so it can only exist in the form of shared_ptr
  static std::shared_ptr<MTSRuntimePool> Create(
      runtime::ContextType context_type, bool disable_tracing_gc,
      const std::shared_ptr<runtime::ContextBundle>& context_bundle,
      const tasm::CompileOptions& compile_options,
      tasm::PageConfig* page_configs);

  ~MTSRuntimePool() = default;

  MTSRuntimePool(const MTSRuntimePool&) = delete;
  MTSRuntimePool& operator=(const MTSRuntimePool&) = delete;

  MTSRuntimePool(MTSRuntimePool&&) = delete;
  MTSRuntimePool& operator=(MTSRuntimePool&&) = delete;

  void FillPool(int32_t count);

  std::shared_ptr<runtime::MTSRuntime> TakeMTSRuntimeSafely();

  void SetEnableAutoGenerate(bool enable);

 private:
  // The global pool doesn't hold context_bundle_ and need to check settings to
  // determine its size.
  // The local pool in TemplateBundle hold context_bundle_ and have no need to
  // check settings.
  MTSRuntimePool(runtime::ContextType context_type, bool disable_tracing_gc)
      : context_type_(context_type), disable_tracing_gc_(disable_tracing_gc) {}

  MTSRuntimePool(runtime::ContextType context_type, bool disable_tracing_gc,
                 const std::shared_ptr<runtime::ContextBundle>& context_bundle,
                 const tasm::CompileOptions& compile_options,
                 tasm::PageConfig* page_configs)
      : context_type_(context_type),
        disable_tracing_gc_(disable_tracing_gc),
        enable_signal_api_(
            page_configs ? page_configs->GetEnableSignalAPIBoolValue() : false),
        target_sdk_version_(compile_options.target_sdk_version_),
        context_bundle_(context_bundle),
        arch_option_(compile_options.arch_option_),
        enable_mts_pre_execute_(
            page_configs ? page_configs->GetEnableMTSPreExecute() : false) {}

  void AddMTSRuntimeSafely(int32_t count);

  bool enable_auto_generate_{true};

  const runtime::ContextType context_type_{
      runtime::ContextType::LepusNGContextType};
  const bool disable_tracing_gc_{false};
  const bool enable_signal_api_{false};
  const std::string target_sdk_version_;
  const std::shared_ptr<runtime::ContextBundle> context_bundle_{nullptr};
  const tasm::ArchOption arch_option_{tasm::RADON_ARCH};
  const bool enable_mts_pre_execute_{false};

  std::mutex mtx_;
  base::InlineVector<std::shared_ptr<runtime::MTSRuntime>, 8> mts_runtimes_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_
