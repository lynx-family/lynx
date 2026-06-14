// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_NG_PARSER_SUPPORTS_CONDITION_PARSER_H_
#define CORE_RENDERER_CSS_NG_PARSER_SUPPORTS_CONDITION_PARSER_H_

#include <string>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/supports/supports_condition.h"

namespace lynx {
namespace css {

class CSSParserTokenStream;

class SupportsConditionParser {
 public:
  // Returns null when input is completely unparseable.
  static fml::RefPtr<const SupportsConditionNode> Parse(
      const std::string& text);

 private:
  explicit SupportsConditionParser(CSSParserTokenStream& stream)
      : stream_(stream) {}

  fml::RefPtr<const SupportsConditionNode> ConsumeCondition();
  fml::RefPtr<const SupportsConditionNode> ConsumeInParens();
  fml::RefPtr<const SupportsConditionNode> ConsumeDeclaration();
  fml::RefPtr<const SupportsConditionNode> ConsumeFunction();
  fml::RefPtr<const SupportsConditionNode> ConsumeEngineVersion();

  // Helpers
  bool PeekIdentIgnoringCase(const char* lowercase_ascii);
  std::string ConsumeRemainingAsString();
  fml::RefPtr<const SupportsConditionNode> MakeGeneralEnclosed(
      size_t content_start);

  CSSParserTokenStream& stream_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_SUPPORTS_CONDITION_PARSER_H_
