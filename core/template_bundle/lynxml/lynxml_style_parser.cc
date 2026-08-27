// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_style_parser.h"

namespace lynx {
namespace tasm {
namespace {

class StyleParserImpl {
 public:
  StyleParserImpl(const std::string& style,
                  const CompileOptions& compile_options);

  LynxMLStyleParseResult Parse();

 private:
  [[maybe_unused]] const std::string& style_;
  [[maybe_unused]] const CompileOptions& compile_options_;
};

StyleParserImpl::StyleParserImpl(const std::string& style,
                                 const CompileOptions& compile_options)
    : style_(style), compile_options_(compile_options) {}

LynxMLStyleParseResult StyleParserImpl::Parse() { return {true, nullptr, {}}; }

}  // namespace

LynxMLStyleParser::LynxMLStyleParser(const std::string& style,
                                     const CompileOptions& compile_options)
    : style_(style), compile_options_(compile_options) {}

LynxMLStyleParseResult LynxMLStyleParser::Parse() {
  if (parsed_) {
    return {false, nullptr, "LynxMLStyleParser::Parse may only be called once"};
  }
  parsed_ = true;
  return StyleParserImpl(style_, compile_options_).Parse();
}

LynxMLStyleParseResult ParseLynxMLStyle(const std::string& style,
                                        const CompileOptions& compile_options) {
  return LynxMLStyleParser(style, compile_options).Parse();
}

}  // namespace tasm
}  // namespace lynx
