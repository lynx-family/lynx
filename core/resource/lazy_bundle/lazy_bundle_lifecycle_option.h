// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LIFECYCLE_OPTION_H_
#define CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LIFECYCLE_OPTION_H_

#include <memory>
#include <string>

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/resource/lazy_bundle/lazy_bundle_utils.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

struct LazyBundleLifecycleOption {
  LazyBundleLifecycleOption(const std::string &url, int instance_id);

  // Move Only.
  LazyBundleLifecycleOption(const LazyBundleLifecycleOption &) = delete;
  LazyBundleLifecycleOption &operator=(const LazyBundleLifecycleOption &) =
      delete;
  LazyBundleLifecycleOption(LazyBundleLifecycleOption &&) = default;
  LazyBundleLifecycleOption &operator=(LazyBundleLifecycleOption &&) = default;

  ~LazyBundleLifecycleOption();

  bool OnLazyBundleLifecycleEnd(TemplateAssembler *tasm);

  void SyncOption(const LazyBundleLifecycleOption &option);

  lepus::Value GetPerfInfo();

  std::string component_url{};
  uint32_t component_uid{0};
  RadonLazyComponent *component_instance{nullptr};
  int instance_id{0};

  bool sync{false};  // whether the lazy bundle is loaded sync.
  bool is_success{
      true};  // true means loaded success, false means loaded failed.
  int64_t binary_size{0};  //  the binary size of lazy bundle

  lazy_bundle::LazyBundleState mode{
      LazyBundleState::STATE_UNKNOWN};  // preload, cache or normal.

  // perf timing
  int64_t start_require_time{0};  //  start require time of lazy bundle.
  int64_t end_require_time{0};    //  end require time of lazy bundle.
  int64_t start_decode_time{0};   //  start decode time of lazy bundle.
  int64_t end_decode_time{0};     //  end decode time of lazy bundle.

  // info
  int32_t error_code{error::E_SUCCESS};  //  if the lazy bundle loaded failed,
  //  error_code indicates the reason.
  lepus::Value message{};  //  if the lazy bundle loaded end, the
                           //  error_msg/success_info needs to be reported.

  // lepus closure for callback.
  lepus::Value callback{};  // callback of `QueryComponent` api.
  bool enable_fiber_arch{false};

 private:
  bool HandleLoadSuccess(TemplateAssembler *tasm);

  bool HandleLoadFailure(TemplateAssembler *tasm);

  lepus::Value GetPerfEventMessage();

  bool enable_report_event_{false};
  lepus::Value perf_info_{};
};

}  // namespace tasm
}  // namespace lynx

#endif  //  CORE_RESOURCE_LAZY_BUNDLE_LAZY_BUNDLE_LIFECYCLE_OPTION_H_
