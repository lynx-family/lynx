// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_TASM_RUNTIME_BUNDLE_H_
#define CORE_RENDERER_TASM_RUNTIME_BUNDLE_H_

#include <string>
#include <utility>
#include <vector>

#include "core/renderer/data/template_data.h"
#include "core/runtime/piper/js/js_bundle.h"

namespace lynx {
namespace tasm {

/**
 * A struct containing the data that tasm runtime needs to post to js runtime
 * while loading template of card or components
 */
struct TasmRuntimeBundle {
  TasmRuntimeBundle() = default;
  ~TasmRuntimeBundle() = default;
  TasmRuntimeBundle(
      const std::string& name, const std::string& target_sdk_version,
      bool support_component_js, const lepus::Value& encoded_data,
      TemplateData init_data, std::vector<TemplateData> cache_data,
      const piper::JsBundle& js_bundle, bool enable_circular_data_check,
      bool enable_js_binding_api_throw_exception, bool enable_bind_icu,
      bool enable_microtask_promise_polyfill,
      const lepus::Value& custom_sections)
      : name(name),
        target_sdk_version(target_sdk_version),
        support_component_js(support_component_js),
        encoded_data(encoded_data),
        init_data(std::move(init_data)),
        cache_data(std::move(cache_data)),
        js_bundle(js_bundle),
        enable_circular_data_check(enable_circular_data_check),
        enable_js_binding_api_throw_exception(
            enable_js_binding_api_throw_exception),
        enable_bind_icu(enable_bind_icu),
        enable_microtask_promise_polyfill(enable_microtask_promise_polyfill),
        custom_sections(custom_sections) {}

  // move only
  TasmRuntimeBundle(const TasmRuntimeBundle&) = delete;
  TasmRuntimeBundle& operator=(const TasmRuntimeBundle&) = delete;
  TasmRuntimeBundle(TasmRuntimeBundle&&) = default;
  TasmRuntimeBundle& operator=(TasmRuntimeBundle&&) = default;

  std::string name;
  std::string target_sdk_version;
  bool support_component_js;
  lepus::Value encoded_data;

  TemplateData init_data;
  std::vector<TemplateData> cache_data;

  piper::JsBundle js_bundle;
  bool enable_circular_data_check;
  bool enable_js_binding_api_throw_exception;
  bool enable_bind_icu;
  bool enable_microtask_promise_polyfill;

  lepus::Value custom_sections{};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_TASM_RUNTIME_BUNDLE_H_
