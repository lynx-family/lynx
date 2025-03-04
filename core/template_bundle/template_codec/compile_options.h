// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_COMPILE_OPTIONS_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_COMPILE_OPTIONS_H_

#include <string>

#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {

constexpr uint8_t kTemplateDebugId = 12;

enum CompileOptionRadonMode : int32_t {
  RADON_MODE_NOT_RECORDED = 1,
  RADON_MODE_DOM_DIFF,
  RADON_MODE_RADON,
};

enum CompileOptionFrontEndDSL : int32_t {
  FRON_END_DSL_NOT_RECORDED = 1,
  FRON_END_DSL_MINI_APP,
  FRON_END_DSL_REACT,
};

enum FeOption : uint8_t {
  FE_OPTION_UNDEFINED = 1,
  FE_OPTION_ENABLE,
  FE_OPTION_DISABLE,
};

enum CompileOptionAirMode : uint8_t {
  AIR_MODE_OFF = 0,
  AIR_MODE_TTML_WITHOUT_JS,
  AIR_MODE_NATIVE_SCRIPT,
  AIR_MODE_STRICT,
  AIR_MODE_FIBER,
};

enum ArchOption : uint8_t { RADON_ARCH = 0, FIBER_ARCH, AIR_ARCH };

enum CompileOptionConfigType : int32_t {
  CONFIG_TYPE_EXPERIMENT_SETTINGS = 1,
};

/*
 * When adding or modifying some properties, please modify
 * oliver/type-lynx/compile/compile-options.d.ts at the same time.
 */
struct CompileOptions {
  std::string target_sdk_version_ = "";
  std::string template_debug_url_ = "";
  bool enable_css_parser_ = false;
  bool enable_css_external_class_ = true;
  bool enable_lepus_ng_ = false;
  bool enable_css_class_merge_ = false;
  bool enable_remove_css_scope_ = false;
  bool disable_multiple_cascade_css_ = false;
  bool remove_css_parser_log_ = false;
  bool enable_trial_options_ = false;
  bool lepusng_debuginfo_outside_ = false;
  // not used in runtime
  bool enable_dynamic_component_ = true;
  bool enable_css_strict_mode_ = false;
  bool default_overflow_visible_ = true;
  bool enable_css_variable_ = true;
  bool default_implicit_animation_ = false;
  int32_t radon_mode_ = RADON_MODE_NOT_RECORDED;
  int32_t front_end_dsl_ = FRON_END_DSL_NOT_RECORDED;
  bool enable_keep_page_data = false;
  bool default_display_linear_ = true;
  bool enable_lynx_air_ = false;
  FeOption enable_lazy_css_decode_ = FE_OPTION_UNDEFINED;
  FeOption enable_event_refactor_ = FE_OPTION_UNDEFINED;
  FeOption force_calc_new_style_ = FE_OPTION_UNDEFINED;
  FeOption enable_async_css_decode_ = FE_OPTION_UNDEFINED;
  bool enable_css_engine = true;
  bool enable_component_config_ = false;
  uint8_t lynx_air_mode_ = AIR_MODE_OFF;
  int32_t config_type = CONFIG_TYPE_EXPERIMENT_SETTINGS;
  bool enable_fiber_arch_ = false;

  // If enable this value, the templaet will be encoded as flexible template.
  // See issue: #7974.
  bool enable_flexible_template_ = false;

  // Default arch is RADON_ARCH. If enable_fiber_arch, it is FIBER_ARCH. If
  // lynx_air_mode != AIR_MODE_OFF, it is AIR_ARCH.
  ArchOption arch_option_ = RADON_ARCH;
  // this switch will enable the css module in blink standard mode
  bool enable_css_selector_ = false;
  // enable this switch to reuse lepus::Context between DynamicComponent & page
  // can only be used in lepusng.
  bool enable_reuse_context = false;
  // If enable CSS invalidation we use RuleInvalidationSet to gather the
  // selector invalidation
  bool enable_css_invalidation_ = false;
  // enable this switch to use all raw css styles(no parse in encode), not used
  // in runtime.
  bool enable_air_raw_css_ = true;
  // allow encoding quickjs bytecode instead of source code in template.
  bool encode_quickjs_bytecode_ = false;
  // allow async decode lepus chunk
  bool enable_async_lepus_chunk_decode_ = false;
};

#define FOREACH_FIXED_LENGTH_FIELD(V)       \
  V(UINT8, enable_css_parser_, 1);          \
  V(UINT8, enable_css_external_class_, 2);  \
  V(UINT8, enable_css_strict_mode_, 3);     \
  V(UINT8, enable_lepus_ng_, 4);            \
  V(UINT8, default_overflow_visible_, 5);   \
  V(UINT8, enable_css_variable_, 6);        \
  V(UINT8, default_implicit_animation_, 7); \
  V(INT32, radon_mode_, 8);                 \
  V(INT32, front_end_dsl_, 9);              \
  V(UINT8, enable_keep_page_data, 10);      \
  V(UINT8, enable_remove_css_scope_, 11);   \
  V(UINT8, enable_css_class_merge_, 13);    \
  V(UINT8, default_display_linear_, 14);    \
  V(UINT8, remove_css_parser_log_, 15);     \
  V(UINT8, enable_lynx_air_, 16);           \
  V(UINT8, enable_lazy_css_decode_, 17);    \
  V(UINT8, enable_event_refactor_, 18);     \
  V(UINT8, force_calc_new_style_, 19);      \
  V(UINT8, enable_trial_options_, 20);      \
  V(UINT8, enable_async_css_decode_, 21);   \
  V(UINT8, enable_css_engine, 22);          \
  V(UINT8, enable_component_config_, 23);   \
  V(UINT8, lynx_air_mode_, 24);             \
  V(UINT8, enable_fiber_arch_, 25);         \
  V(UINT8, lepusng_debuginfo_outside_, 26); \
  V(UINT8, enable_flexible_template_, 27);  \
  V(UINT8, arch_option_, 28);               \
  V(UINT8, enable_css_selector_, 29);       \
  V(UINT8, enable_reuse_context, 30);       \
  V(UINT8, enable_css_invalidation_, 31);   \
  V(UINT8, enable_async_lepus_chunk_decode_, 32);

#define FOREACH_STRING_FIELD(V) \
  V(target_sdk_version_, 0);    \
  V(template_debug_url_, 12);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_COMPILE_OPTIONS_H_
