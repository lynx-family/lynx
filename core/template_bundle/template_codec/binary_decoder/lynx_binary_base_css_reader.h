// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_CSS_READER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_CSS_READER_H_

#include <memory>
#include <string>
#include <utility>

#include "core/renderer/css/css_value.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/runtime/vm/lepus/base_binary_reader.h"
#include "core/template_bundle/template_codec/template_binary.h"

namespace lynx {
namespace tasm {

#define DECODE_CSS_VALUE(name) \
  tasm::CSSValue name;         \
  ERROR_UNLESS(DecodeCSSValue(&name))

#define DECODE_CSS_VALUE_INTO(value) ERROR_UNLESS(DecodeCSSValue(&(value)))

class CSSParseToken;
class CSSSheet;
struct CSSRoute;

class LynxBinaryBaseCSSReader : public lepus::BaseBinaryReader {
 public:
  LynxBinaryBaseCSSReader(std::unique_ptr<lepus::InputStream> stream)
      : lepus::BaseBinaryReader(std::move(stream)){};

  ~LynxBinaryBaseCSSReader() override = default;

  LynxBinaryBaseCSSReader(const LynxBinaryBaseCSSReader&) = delete;
  LynxBinaryBaseCSSReader& operator=(const LynxBinaryBaseCSSReader&) = delete;

  LynxBinaryBaseCSSReader(LynxBinaryBaseCSSReader&&) = default;
  LynxBinaryBaseCSSReader& operator=(LynxBinaryBaseCSSReader&&) = default;

  static bool EnableCssVariable(const CompileOptions& options);
  static bool EnableCssParser(const CompileOptions& options);
  static bool EnableCssVariableMultiDefaultValue(const CompileOptions& options);

 protected:
  // Utils for decode css.
  bool DecodeCSSRoute(CSSRoute& css_router);
  bool DecodeCSSFragment(SharedCSSFragment* fragment, size_t descriptor_end);
  bool DecodeCSSParseToken(CSSParseToken*);
  bool DecodeCSSKeyframesToken(CSSKeyframesToken*);
  bool DecodeCSSSheet(CSSSheet* parent, CSSSheet* sheet);
  bool DecodeCSSAttributes(CSSParseToken* token);
  bool DecodeCSSAttributes(StyleMap&, RawStyleMap&, const CSSParserConfigs&);
  bool DecodeCSSStyleVariables(CSSVariableMap& style_variables);
  bool DecodeCSSKeyframesMap(CSSKeyframesContent*, CSSRawKeyframesContent*,
                             const CSSParserConfigs&);
  bool DecodeCSSFontFaceToken(CSSFontFaceRule* token);
  bool DecodeCSSSelector(css::LynxCSSSelector* selector);

  bool DecodeCSSValue(tasm::CSSValue*);
  bool DecodeCSSValue(tasm::CSSValue* result, bool enable_css_parser,
                      bool enable_css_variable,
                      bool enable_css_variable_multi_default_value);

  bool GetEnableNewImportRule();

 protected:
  Range css_section_range_;

  bool enable_css_font_face_extension_{false};
  bool enable_css_variable_{false};
  bool enable_css_parser_{false};
  bool enable_css_variable_multi_default_value_{false};
  std::string absetting_disable_css_lazy_decode_;
  bool enable_pre_process_attributes_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_BASE_CSS_READER_H_
