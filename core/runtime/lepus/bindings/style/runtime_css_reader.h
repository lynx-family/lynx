// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_BINDINGS_STYLE_RUNTIME_CSS_READER_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_STYLE_RUNTIME_CSS_READER_H_

#include <memory>
#include <utility>

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_css_reader.h"

namespace lynx {
namespace tasm {

class RuntimeCSSReader : public LynxBinaryBaseCSSReader {
 public:
  RuntimeCSSReader(std::unique_ptr<lepus::InputStream> stream,
                   const CompileOptions& options,
                   bool enable_inline_css_variable)
      : LynxBinaryBaseCSSReader(std::move(stream)) {
    compile_options_ = options;
    enable_css_font_face_extension_ = Config::IsHigherOrEqual(
        compile_options_.target_sdk_version_, FEATURE_CSS_FONT_FACE_EXTENSION);
    enable_css_variable_ = EnableCssVariable(compile_options_);
    enable_css_variable_multi_default_value_ =
        EnableCssVariableMultiDefaultValue(compile_options_);
    enable_css_parser_ = EnableCssParser(compile_options_);
    enable_css_inline_variables_ = enable_inline_css_variable;
    decode_string_directly_ = true;
  }

  using LynxBinaryBaseCSSReader::DecodeCSSFragment;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_STYLE_RUNTIME_CSS_READER_H_
