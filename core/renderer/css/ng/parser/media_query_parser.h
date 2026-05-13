// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
//
// Recursive-descent parser that turns the textual condition attached to a
// CSS @media at-rule into the structured MediaQuerySet / MediaQueryExpNode
// object graph defined in this directory. The grammar follows CSS Media
// Queries Level 4:
//
//   <media-query-list>     = <media-query> ( "," <media-query> )*
//   <media-query>          = <media-condition>
//                          | [ not | only ]? <media-type>
//                            [ "and" <media-condition-without-or> ]?
//   <media-condition>      = <media-not> | <media-in-parens>
//                            ( ( "and" <media-in-parens> )*
//                            | ( "or"  <media-in-parens> )* )
//   <media-condition-wo-or>= <media-not>
//                          | <media-in-parens> ( "and" <media-in-parens> )*
//   <media-not>            = "not" <media-in-parens>
//   <media-in-parens>      = "(" <media-condition> ")" | <media-feature>
//   <media-feature>        = plain | boolean | range
//
// Only the pure data shape is produced here; evaluation is the eventual
// responsibility of a MediaQueryEvaluator that consumes this AST.

#ifndef CORE_RENDERER_CSS_NG_PARSER_MEDIA_QUERY_PARSER_H_
#define CORE_RENDERER_CSS_NG_PARSER_MEDIA_QUERY_PARSER_H_

#include <string>

#include "base/include/fml/memory/ref_counted.h"
#include "core/renderer/css/ng/media_query/media_query.h"
#include "core/renderer/css/ng/media_query/media_query_exp.h"
#include "core/renderer/css/ng/media_query/media_query_set.h"

namespace lynx {
namespace css {

class CSSParserTokenStream;

class MediaQueryParser {
 public:
  // Parses a full media-query-list. Never returns null.
  //   * Empty / whitespace-only input -> empty set (spec: matches all).
  //   * Wholly invalid input          -> single `not all` entry.
  //   * Mixed list                    -> invalid entries dropped.
  static fml::RefPtr<MediaQuerySet> ParseMediaQuerySet(const std::string& text);

  // Parses a single <media-condition> (without a media-type). Returns null
  // when the input cannot be parsed.
  static fml::RefPtr<const MediaQueryExpNode> ParseMediaCondition(
      const std::string& text);

 private:
  explicit MediaQueryParser(CSSParserTokenStream& stream) : stream_(stream) {}

  fml::RefPtr<MediaQuery> ConsumeMediaQuery();
  fml::RefPtr<const MediaQueryExpNode> ConsumeCondition(bool allow_or);
  fml::RefPtr<const MediaQueryExpNode> ConsumeInParens();
  fml::RefPtr<const MediaQueryExpNode> ConsumeFeature();

  // Small utilities to keep the recursive descent readable.
  bool ConsumeIdentIgnoringCase(const char* lowercase_ascii);
  bool PeekIdentIgnoringCase(const char* lowercase_ascii);
  bool ConsumeFeatureOperator(MediaFeatureOperator& out);
  bool ConsumeFeatureValue(MediaFeatureValue& out);
  bool ConsumeFeatureName(std::string& out);

  CSSParserTokenStream& stream_;
};

}  // namespace css
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_NG_PARSER_MEDIA_QUERY_PARSER_H_
