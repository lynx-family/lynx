// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_parser.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// LynxML parsing is an experimental feature. Its syntax and behavior may change
// without backward compatibility guarantees.
namespace lynx {
namespace lynxml {
namespace {

constexpr std::string_view kCommentStart = "<!--";
constexpr std::string_view kCommentEnd = "-->";
constexpr std::string_view kDoctypeStart = "<!doctype";
constexpr std::string_view kEndTagStart = "</";
constexpr std::string_view kMarkupDeclarationStart = "<!";
constexpr std::string_view kProcessingInstructionStart = "<?";
constexpr std::string_view kSelfClosingTagEnd = "/>";
constexpr std::string_view kRootElementName = "lynx";
constexpr std::string_view kStyleElementName = "style";
constexpr std::string_view kScriptElementName = "script";
constexpr std::string_view kUTF8ByteOrderMark = "\xEF\xBB\xBF";

// Returns whether value is one of the ASCII whitespace characters recognized
// by LynxML syntax.
constexpr bool IsASCIIWhitespace(char value) {
  switch (value) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
    case '\f':
      return true;
    default:
      return false;
  }
}

// Returns whether value is an uppercase or lowercase ASCII letter.
constexpr bool IsASCIIAlpha(char value) {
  return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

// Returns whether value may occur in a LynxML element or attribute name. The
// caller separately requires the first character to be an ASCII letter.
constexpr bool IsNameCharacter(char value) {
  return IsASCIIAlpha(value) || (value >= '0' && value <= '9') || value == '-';
}

struct ParsedAttribute {
  std::string name;
  std::string value;
  bool has_value{false};
  size_t offset{0};
};

struct ParsedStartTag {
  std::string name;
  std::vector<ParsedAttribute> attributes;
};

class ParseContext {
 public:
  ParseContext(std::string_view source, LynxMLParser::Observer& observer)
      : source_(source), observer_(observer) {}

  void ParseDocument() {
    if (source_.substr(0, kUTF8ByteOrderMark.size()) == kUTF8ByteOrderMark) {
      position_ += kUTF8ByteOrderMark.size();
    }

    ConsumeWhitespace();
    if (!ConsumeDoctype()) {
      return;
    }
    if (!ConsumeIgnorable()) {
      return;
    }

    ParsedStartTag root;
    const size_t root_offset = position_;
    if (!ParseStartTag(&root)) {
      return;
    }
    if (root.name != kRootElementName) {
      Fail(root_offset, "expected '<lynx>' root element");
      return;
    }
    std::vector<Attribute> attributes;
    attributes.reserve(root.attributes.size());
    for (auto& attribute : root.attributes) {
      attributes.push_back(
          {std::move(attribute.name), std::move(attribute.value)});
    }
    observer_.OnDocumentStart(std::move(attributes));

    while (true) {
      if (!ConsumeIgnorable()) {
        return;
      }
      if (auto end = EndTagLengthAt(position_, kRootElementName)) {
        position_ += *end;
        break;
      }
      if (position_ == source_.size()) {
        Fail(position_, "missing closing tag '</lynx>'");
        return;
      }
      if (source_.substr(position_, kEndTagStart.size()) == kEndTagStart) {
        Fail(position_, "unexpected closing tag");
        return;
      }
      if (source_.substr(position_, kDoctypeStart.size()) == kDoctypeStart) {
        Fail(position_, "DOCTYPE must appear exactly once at the start");
        return;
      }
      if (!ConsumeSourceBlock()) {
        return;
      }
    }

    if (!ConsumeIgnorable()) {
      return;
    }
    if (position_ != source_.size()) {
      Fail(position_, "unexpected content after '</lynx>'");
      return;
    }
    observer_.OnDocumentEnd();
  }

 private:
  void ConsumeWhitespace() {
    while (position_ < source_.size() &&
           IsASCIIWhitespace(source_[position_])) {
      ++position_;
    }
  }

  bool ConsumeIgnorable() {
    while (true) {
      ConsumeWhitespace();
      if (source_.substr(position_, kCommentStart.size()) != kCommentStart) {
        return true;
      }
      if (!ConsumeComment()) {
        return false;
      }
    }
  }

  bool ConsumeComment() {
    const size_t comment_offset = position_;
    const size_t comment_end =
        source_.find(kCommentEnd, position_ + kCommentStart.size());
    if (comment_end == std::string_view::npos) {
      return Fail(comment_offset, "unterminated comment");
    }
    position_ = comment_end + kCommentEnd.size();
    return true;
  }

  bool ConsumeDoctype() {
    const size_t doctype_offset = position_;
    if (source_.substr(position_, kDoctypeStart.size()) != kDoctypeStart) {
      return Fail(position_, "expected '<!doctype lynx>'");
    }
    position_ += kDoctypeStart.size();
    if (position_ == source_.size() || !IsASCIIWhitespace(source_[position_])) {
      return Fail(doctype_offset, "DOCTYPE requires whitespace before 'lynx'");
    }
    ConsumeWhitespace();
    if (source_.substr(position_, kRootElementName.size()) !=
        kRootElementName) {
      return Fail(doctype_offset, "expected '<!doctype lynx>'");
    }
    position_ += kRootElementName.size();
    ConsumeWhitespace();
    if (position_ == source_.size() || source_[position_] != '>') {
      return Fail(doctype_offset,
                  "DOCTYPE does not accept identifiers or legacy strings");
    }
    ++position_;
    return true;
  }

  bool ParseStartTag(ParsedStartTag* tag) {
    const size_t tag_offset = position_;
    if (position_ == source_.size() || source_[position_] != '<' ||
        source_.substr(position_, kEndTagStart.size()) == kEndTagStart ||
        source_.substr(position_, kMarkupDeclarationStart.size()) ==
            kMarkupDeclarationStart ||
        source_.substr(position_, kProcessingInstructionStart.size()) ==
            kProcessingInstructionStart) {
      return Fail(position_, "expected an element start tag");
    }
    ++position_;

    const size_t name_start = position_;
    while (position_ < source_.size() && IsNameCharacter(source_[position_])) {
      ++position_;
    }
    if (position_ == name_start || !IsASCIIAlpha(source_[name_start])) {
      return Fail(tag_offset, "invalid element name");
    }
    tag->name.assign(source_.substr(name_start, position_ - name_start));

    while (true) {
      if (position_ == source_.size()) {
        return Fail(tag_offset, "unterminated start tag");
      }
      if (source_[position_] == '>') {
        ++position_;
        return true;
      }
      if (!IsASCIIWhitespace(source_[position_])) {
        if (source_.substr(position_, kSelfClosingTagEnd.size()) ==
            kSelfClosingTagEnd) {
          return Fail(position_, "self-closing tags are not supported");
        }
        return Fail(position_, "expected whitespace before an attribute");
      }
      ConsumeWhitespace();
      if (position_ == source_.size()) {
        return Fail(tag_offset, "unterminated start tag");
      }
      if (source_[position_] == '>') {
        ++position_;
        return true;
      }
      if (source_.substr(position_, kSelfClosingTagEnd.size()) ==
          kSelfClosingTagEnd) {
        return Fail(position_, "self-closing tags are not supported");
      }

      ParsedAttribute attribute;
      attribute.offset = position_;
      const size_t attribute_name_start = position_;
      while (position_ < source_.size() &&
             IsNameCharacter(source_[position_])) {
        ++position_;
      }
      if (position_ == attribute_name_start ||
          !IsASCIIAlpha(source_[attribute_name_start])) {
        return Fail(attribute.offset, "invalid attribute name");
      }
      attribute.name.assign(source_.substr(attribute_name_start,
                                           position_ - attribute_name_start));
      for (const auto& existing : tag->attributes) {
        if (existing.name == attribute.name) {
          return Fail(attribute.offset,
                      "duplicate attribute '" + attribute.name + "'");
        }
      }

      ConsumeWhitespace();
      if (position_ < source_.size() && source_[position_] == '=') {
        attribute.has_value = true;
        ++position_;
        ConsumeWhitespace();
        if (position_ == source_.size()) {
          return Fail(attribute.offset, "missing attribute value");
        }
        const char quote = source_[position_];
        size_t value_start = position_;
        size_t value_end = position_;
        if (quote == '\'' || quote == '"') {
          ++position_;
          value_start = position_;
          value_end = source_.find(quote, position_);
          if (value_end == std::string_view::npos) {
            return Fail(attribute.offset, "unterminated attribute value");
          }
          position_ = value_end + 1;
        } else {
          while (position_ < source_.size() &&
                 !IsASCIIWhitespace(source_[position_]) &&
                 source_[position_] != '>') {
            const char value = source_[position_];
            if (value == '"' || value == '\'' || value == '`' || value == '=' ||
                value == '<') {
              return Fail(position_, "invalid unquoted attribute value");
            }
            ++position_;
          }
          if (position_ == value_start) {
            return Fail(attribute.offset, "missing attribute value");
          }
          value_end = position_;
        }
        attribute.value.assign(
            source_.substr(value_start, value_end - value_start));
      }
      tag->attributes.push_back(std::move(attribute));
    }
  }

  bool ConsumeSourceBlock() {
    const size_t block_offset = position_;
    ParsedStartTag tag;
    if (!ParseStartTag(&tag)) {
      return false;
    }

    SourceBlockType type;
    if (tag.name == kStyleElementName) {
      type = SourceBlockType::kStyle;
    } else if (tag.name == kScriptElementName) {
      type = SourceBlockType::kScript;
    } else {
      return FailUnsupported(
          block_offset, "unsupported top-level element '<" + tag.name + ">'");
    }

    const size_t content_start = position_;
    const auto end_tag = FindRawTextEndTag(tag.name);
    if (!end_tag) {
      return Fail(content_start, "missing closing tag '</" + tag.name + ">'");
    }
    std::vector<Attribute> attributes;
    attributes.reserve(tag.attributes.size());
    for (auto& attribute : tag.attributes) {
      attributes.push_back(
          {std::move(attribute.name), std::move(attribute.value)});
    }
    observer_.OnSourceBlock(
        {type, std::move(attributes),
         std::string(
             source_.substr(content_start, end_tag->first - content_start))});
    position_ = end_tag->first + end_tag->second;
    return true;
  }

  std::optional<size_t> EndTagLengthAt(size_t position,
                                       std::string_view name) const {
    if (source_.substr(position, kEndTagStart.size()) != kEndTagStart) {
      return std::nullopt;
    }
    size_t cursor = position + kEndTagStart.size();
    if (source_.substr(cursor, name.size()) != name) {
      return std::nullopt;
    }
    cursor += name.size();
    if (cursor == source_.size() ||
        (!IsASCIIWhitespace(source_[cursor]) && source_[cursor] != '>')) {
      return std::nullopt;
    }
    while (cursor < source_.size() && IsASCIIWhitespace(source_[cursor])) {
      ++cursor;
    }
    if (cursor == source_.size() || source_[cursor] != '>') {
      return std::nullopt;
    }
    return cursor + 1 - position;
  }

  std::optional<std::pair<size_t, size_t>> FindRawTextEndTag(
      std::string_view name) const {
    size_t search = position_;
    while (search < source_.size()) {
      const size_t candidate = source_.find('<', search);
      if (candidate == std::string_view::npos) {
        return std::nullopt;
      }
      if (auto length = EndTagLengthAt(candidate, name)) {
        return std::make_pair(candidate, *length);
      }
      search = candidate + 1;
    }
    return std::nullopt;
  }

  bool Fail(size_t offset, std::string message) {
    observer_.OnError(
        {ParseErrorCode::kSyntaxError, offset, std::move(message)});
    return false;
  }

  bool FailUnsupported(size_t offset, std::string message) {
    observer_.OnError(
        {ParseErrorCode::kUnsupportedFeature, offset, std::move(message)});
    return false;
  }

  std::string_view source_;
  size_t position_{0};
  LynxMLParser::Observer& observer_;
};

}  // namespace

std::string ParseError::ToString() const {
  if (code == ParseErrorCode::kNone) {
    return {};
  }
  const char* prefix = code == ParseErrorCode::kUnsupportedFeature
                           ? "unsupported LynxML feature"
                           : "invalid LynxML";
  return std::string(prefix) + " at offset " + std::to_string(offset) + ": " +
         message;
}

void LynxMLParser::Parse(std::string_view source) {
  ParseContext(source, observer_).ParseDocument();
}

}  // namespace lynxml
}  // namespace lynx
