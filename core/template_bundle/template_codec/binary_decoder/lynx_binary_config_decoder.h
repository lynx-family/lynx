// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/dom/component_config.h"
#include "core/renderer/page_config.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_helper.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace tasm {

// Utils class for decode Lynx Config.
class LynxBinaryConfigDecoder {
 public:
  LynxBinaryConfigDecoder(const tasm::CompileOptions& compile_option,
                          const std::string& target_sdk_version,
                          bool is_lepusng_binary, bool enable_css_parser)
      : compile_options_(compile_option),
        target_sdk_version_(target_sdk_version),
        is_lepusng_binary_(is_lepusng_binary),
        enable_css_parser_(enable_css_parser){};

  LynxBinaryConfigDecoder(const LynxBinaryConfigDecoder&) = delete;
  LynxBinaryConfigDecoder& operator=(const LynxBinaryConfigDecoder&) = delete;
  LynxBinaryConfigDecoder(LynxBinaryConfigDecoder&&) = delete;
  LynxBinaryConfigDecoder& operator=(LynxBinaryConfigDecoder&&) = delete;

  bool DecodePageConfig(const std::string& config_str,
                        std::shared_ptr<PageConfig>& page_config);
  bool DecodeComponentConfig(
      const std::string& config_str,
      std::shared_ptr<ComponentConfig>& component_config);

  void SetAbSettingDisableCSSLazyDecode(
      std::string& absetting_disable_css_lazy_decode) {
    absetting_disable_css_lazy_decode_ = absetting_disable_css_lazy_decode;
  }

 private:
  /// TODO(limeng.amer): move to report thread.
  /// Upload global feature switches in PageConfig with common data about lynx
  /// view. If you add a new  global feature switch, you should add it to report
  /// event.
  void ReportGlobalFeatureSwitch(
      const std::shared_ptr<PageConfig>& page_config);

  void UpdateCSSConfigs(const std::shared_ptr<PageConfig>& page_config);

  tasm::CompileOptions compile_options_;
  std::string target_sdk_version_;
  bool is_lepusng_binary_{false};
  bool enable_css_parser_{false};
  std::string absetting_disable_css_lazy_decode_{};
  LynxBinaryConfigHelper config_helper_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_H_
