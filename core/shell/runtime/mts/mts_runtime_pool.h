// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_
#define CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/closure.h"
#include "base/include/vector.h"
#if ENABLE_TRACE_PERFETTO
#include "base/include/notification_center.h"
#include "core/runtime/js/runtime_constant.h"
#endif
#include "core/shell/runtime/mts/mts_runtime.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {

namespace devtool {
class DevToolPool;
}
namespace shell {

class MTSRuntimePool : public std::enable_shared_from_this<MTSRuntimePool> {
 public:
  static std::shared_ptr<MTSRuntimePool> Create(
      runtime::ContextType context_type, bool disable_tracing_gc);
  // ContextPool must check its own life cycle asynchronously when
  // replenishing the cache, so it can only exist in the form of shared_ptr
  static std::shared_ptr<MTSRuntimePool> Create(
      runtime::ContextType context_type, const std::string& template_url,
      bool disable_tracing_gc,
      const std::shared_ptr<runtime::ContextBundle>& context_bundle,
      const tasm::CompileOptions& compile_options,
      tasm::PageConfig* page_configs);

  ~MTSRuntimePool();

  MTSRuntimePool(const MTSRuntimePool&) = delete;
  MTSRuntimePool& operator=(const MTSRuntimePool&) = delete;

  MTSRuntimePool(MTSRuntimePool&&) = delete;
  MTSRuntimePool& operator=(MTSRuntimePool&&) = delete;

  void FillPool(int32_t count);
  void FillPoolSync(int32_t count);

  // Loads bytecode into every runtime currently owned by this pool. The work
  // is serialized with FillPool on the normal-priority worker. Preload is only
  // accepted after auto-refill has been disabled. The callback is invoked
  // after every pooled runtime has successfully loaded the bytecode.
  bool Preload(std::string url, std::vector<uint8_t> bytecode,
               base::MoveOnlyClosure<> callback);

  std::shared_ptr<runtime::MTSRuntime> TakeMTSRuntimeSafely();

  void SetEnableAutoGenerate(bool enable);

  void SetDevToolPool(
      const std::shared_ptr<devtool::DevToolPool>& devtool_pool) {
    devtool_pool_ = devtool_pool;
  }

  const std::shared_ptr<devtool::DevToolPool>& GetDevToolPool() {
    return devtool_pool_;
  }

 private:
  // The global pool doesn't hold context_bundle_ and need to check settings to
  // determine its size.
  // The local pool in TemplateBundle hold context_bundle_ and have no need to
  // check settings.
  MTSRuntimePool(runtime::ContextType context_type, bool disable_tracing_gc)
      : context_type_(context_type),
        is_global_pool_(true),
        disable_tracing_gc_(disable_tracing_gc) {
    InitReportPoolState();
    ReportPoolState();
  }

  MTSRuntimePool(runtime::ContextType context_type,
                 const std::string& template_url, bool disable_tracing_gc,
                 const std::shared_ptr<runtime::ContextBundle>& context_bundle,
                 const tasm::CompileOptions& compile_options,
                 tasm::PageConfig* page_configs)
      : context_type_(context_type),
        arch_option_(compile_options.arch_option_),
        disable_tracing_gc_(disable_tracing_gc),
        enable_signal_api_(
            page_configs ? page_configs->GetEnableSignalAPIBoolValue() : false),
        enable_mts_pre_execute_(
            page_configs ? page_configs->GetEnableMTSPreExecute() : false),
        enable_element_api_new_registration_(
            page_configs ? page_configs->GetEnableElementApiNewRegistration()
                         : false),
        template_url_(template_url),
        target_sdk_version_(compile_options.target_sdk_version_),
        context_bundle_(context_bundle),
        debug_info_url_(compile_options.template_debug_url_) {
    InitReportPoolState();
    ReportPoolState();
  }

  void AddMTSRuntimeSafely(int32_t count);
  void InitReportPoolState();
  void ReportPoolState();

  const runtime::ContextType context_type_{
      runtime::ContextType::LepusNGContextType};
  const bool is_global_pool_{false};
  const tasm::ArchOption arch_option_{tasm::RADON_ARCH};
  bool enable_auto_generate_{true};
  const bool disable_tracing_gc_{false};
  const bool enable_signal_api_{false};
  const bool enable_mts_pre_execute_{false};
  const bool enable_element_api_new_registration_{false};

  std::string template_url_;
  const std::string target_sdk_version_;
  const std::shared_ptr<runtime::ContextBundle> context_bundle_{nullptr};

  std::atomic<bool> is_destroying_{false};
  std::mutex mtx_;
  base::InlineVector<std::shared_ptr<runtime::MTSRuntime>, 8> mts_runtimes_;

  std::shared_ptr<devtool::DevToolPool> devtool_pool_;
  std::string debug_info_url_;

  int32_t pool_instance_id_{-1};
  int64_t created_at_ms_{0};
#if ENABLE_TRACE_PERFETTO
  std::unique_ptr<base::NotificationCallback> report_pool_state_;
#endif
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_MTS_MTS_RUNTIME_POOL_H_
