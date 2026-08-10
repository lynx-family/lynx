// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_PARSER_H_

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace lynx {
namespace lynxml {

// Identifies the top-level element that produced a source block.
enum class SourceBlockType {
  kStyle,
  // Script execution details, such as the target thread, are expressed by the
  // block's attributes and interpreted by the consumer.
  kScript,
};

struct Attribute {
  // The attribute name with its original spelling preserved.
  std::string name;
  // The value without surrounding quotes. Character references are not
  // decoded. A valueless attribute and an empty value are both represented by
  // an empty string.
  std::string value;
};

struct SourceBlock {
  // Whether this block came from a top-level <style> or <script> element.
  SourceBlockType type{SourceBlockType::kStyle};
  // Attributes from the source element's start tag, in source order.
  std::vector<Attribute> attributes;
  // Text between the start and end tags, preserved verbatim.
  std::string source;
};

// Describes the outcome reported through Observer::OnError.
enum class ParseErrorCode {
  kNone,
  kSyntaxError,
  kUnsupportedFeature,
};

struct ParseError {
  ParseErrorCode code{ParseErrorCode::kNone};
  // Zero-based byte offset in the original UTF-8 input.
  size_t offset{0};
  // Error detail without the error kind or source offset prefix.
  std::string message;

  // Returns a complete human-readable error message.
  std::string ToString() const;
};

// Synchronously parses a complete LynxML document and reports events in source
// order.
class LynxMLParser {
 public:
  class Observer {
   public:
    virtual ~Observer() = default;

    // Called after the doctype and <lynx> start tag have been parsed. The
    // attributes belong to the <lynx> element.
    virtual void OnDocumentStart(std::vector<Attribute> attributes) = 0;
    // Called once for each complete top-level <style> or <script> element.
    virtual void OnSourceBlock(SourceBlock source_block) = 0;
    // Called after a successful parse. It is not called if OnError is called.
    virtual void OnDocumentEnd() = 0;
    // Reports an error and terminates the current parse. Earlier document and
    // source-block callbacks may already have been delivered.
    virtual void OnError(ParseError error) = 0;
  };

  // The observer must outlive this parser.
  explicit LynxMLParser(Observer& observer) : observer_(observer) {}

  // Parses source synchronously. All observer callbacks occur before this
  // method returns, and source only needs to remain valid for the duration of
  // the call.
  void Parse(std::string_view source);

 private:
  Observer& observer_;
};

}  // namespace lynxml
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_LYNXML_LYNXML_PARSER_H_
