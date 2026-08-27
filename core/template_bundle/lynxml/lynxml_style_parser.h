// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_STYLE_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_STYLE_PARSER_H_

#include <memory>
#include <string>

#include "core/renderer/css/shared_css_fragment.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace tasm {

// Contains the result of parsing a LynxML style source.
struct LynxMLStyleParseResult {
  // Whether parsing completed successfully.
  bool success = false;

  // The parsed CSS fragment. This may be null when the source produces no
  // style rules.
  std::unique_ptr<SharedCSSFragment> fragment;

  // The parsing error. This is empty when success is true.
  std::string error;
};

// Parses the contents of a LynxML <style> source block. Each parser instance
// may be parsed at most once and may not be copied or moved.
class LynxMLStyleParser {
 public:
  // Creates a parser for style using the supplied template compile options.
  LynxMLStyleParser(const std::string& style,
                    const CompileOptions& compile_options);
  ~LynxMLStyleParser() = default;

  LynxMLStyleParser(const LynxMLStyleParser&) = delete;
  LynxMLStyleParser& operator=(const LynxMLStyleParser&) = delete;
  LynxMLStyleParser(LynxMLStyleParser&&) = delete;
  LynxMLStyleParser& operator=(LynxMLStyleParser&&) = delete;

  // Parses the style source and returns either a CSS fragment or an error.
  // Calling this more than once returns an error.
  LynxMLStyleParseResult Parse();

 private:
  std::string style_;
  CompileOptions compile_options_;
  bool parsed_ = false;
};

// Parses a LynxML style source without retaining a parser object.
LynxMLStyleParseResult ParseLynxMLStyle(const std::string& style,
                                        const CompileOptions& compile_options);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_STYLE_PARSER_H_
