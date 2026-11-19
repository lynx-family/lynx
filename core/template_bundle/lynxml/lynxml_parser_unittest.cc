// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/lynxml/lynxml_parser.h"

#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace lynxml {
namespace {

struct RecordedDocument {
  std::vector<Attribute> attributes;
  std::vector<SourceBlock> source_blocks;
};

class RecordingObserver final : public LynxMLParser::Observer {
 public:
  void OnDocumentStart(std::vector<Attribute> attributes) override {
    document.attributes = std::move(attributes);
    events.emplace_back("document-start");
  }

  void OnSourceBlock(SourceBlock source_block) override {
    switch (source_block.type) {
      case SourceBlockType::kStyle:
        events.emplace_back("style");
        break;
      case SourceBlockType::kScript:
        events.emplace_back("script");
        break;
    }
    document.source_blocks.push_back(std::move(source_block));
  }

  void OnDocumentEnd() override {
    completed = true;
    events.emplace_back("document-end");
  }

  void OnError(ParseError parse_error) override {
    error = std::move(parse_error);
    events.emplace_back("error");
  }

  bool success() const {
    return completed && error.code == ParseErrorCode::kNone;
  }

  RecordedDocument document;
  ParseError error;
  std::vector<std::string> events;
  bool completed{false};
};

RecordingObserver ParseForTest(std::string_view source) {
  RecordingObserver observer;
  LynxMLParser parser(observer);
  parser.Parse(source);
  return observer;
}

TEST(LynxMLParserTest, PreservesSourceBlockOrderAndBoundaries) {
  const std::string source =
      "<!doctype lynx><lynx engine-version=5.4 feature-flag='enabled'>"
      "<style>.first { color: red; }</style>"
      "<script thread=main>initializeApp();</script>"
      "<script thread=background>globalThis.started = true;</script>"
      "<style>.second { color: blue; }</style>"
      "<script thread=main>renderPage();</script>"
      "</lynx>";

  auto result = ParseForTest(source);

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.attributes.size(), 2u);
  EXPECT_EQ(result.document.attributes[0].name, "engine-version");
  EXPECT_EQ(result.document.attributes[0].value, "5.4");
  EXPECT_EQ(result.document.attributes[1].name, "feature-flag");
  EXPECT_EQ(result.document.attributes[1].value, "enabled");
  ASSERT_EQ(result.document.source_blocks.size(), 5u);
  EXPECT_EQ(result.document.source_blocks[0].type, SourceBlockType::kStyle);
  EXPECT_EQ(result.document.source_blocks[0].source, ".first { color: red; }");
  EXPECT_EQ(result.document.source_blocks[1].type, SourceBlockType::kScript);
  ASSERT_EQ(result.document.source_blocks[1].attributes.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[1].attributes[0].name, "thread");
  EXPECT_EQ(result.document.source_blocks[1].attributes[0].value, "main");
  EXPECT_EQ(result.document.source_blocks[1].source, "initializeApp();");
  EXPECT_EQ(result.document.source_blocks[2].type, SourceBlockType::kScript);
  ASSERT_EQ(result.document.source_blocks[2].attributes.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[2].attributes[0].value, "background");
  EXPECT_EQ(result.document.source_blocks[3].type, SourceBlockType::kStyle);
  EXPECT_EQ(result.document.source_blocks[4].type, SourceBlockType::kScript);
  EXPECT_EQ(result.document.source_blocks[4].source, "renderPage();");
  EXPECT_EQ(result.events, (std::vector<std::string>{
                               "document-start", "style", "script", "script",
                               "style", "script", "document-end"}));
}

TEST(LynxMLParserTest, AcceptsCanonicalSyntaxAndComments) {
  auto result = ParseForTest(
      "\xEF\xBB\xBF  <!doctype lynx>\n"
      "<!-- before root --><lynx><!-- before block -->"
      "<style>view { width: 1px; }</style>"
      "<script thread=main>main();</script>"
      "</lynx><!-- after root -->");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.source_blocks.size(), 2u);
  EXPECT_EQ(result.document.source_blocks[0].type, SourceBlockType::kStyle);
  EXPECT_EQ(result.document.source_blocks[1].type, SourceBlockType::kScript);
}

TEST(LynxMLParserTest, RejectsCaseMismatchedSyntax) {
  const char* documents[] = {
      "<!Doctype lynx><lynx></lynx>",
      "<!doctype Lynx><lynx></lynx>",
      "<!doctype lynx><Lynx></Lynx>",
      "<!doctype lynx><lynx><Style>style</Style></lynx>",
      "<!doctype lynx><lynx><style>style</Style></lynx>",
      "<!doctype lynx><lynx></Lynx>",
  };

  for (const char* document : documents) {
    auto result = ParseForTest(document);
    EXPECT_FALSE(result.success()) << document;
  }
}

TEST(LynxMLParserTest, PreservesSourceBlockAttributes) {
  auto result = ParseForTest(
      "<!doctype lynx><lynx><style scoped>style</style>"
      "<script ThReAd=worker flag>script</script></lynx>");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.source_blocks.size(), 2u);
  ASSERT_EQ(result.document.source_blocks[0].attributes.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[0].attributes[0].name, "scoped");
  EXPECT_TRUE(result.document.source_blocks[0].attributes[0].value.empty());
  ASSERT_EQ(result.document.source_blocks[1].attributes.size(), 2u);
  EXPECT_EQ(result.document.source_blocks[1].attributes[0].name, "ThReAd");
  EXPECT_EQ(result.document.source_blocks[1].attributes[0].value, "worker");
  EXPECT_EQ(result.document.source_blocks[1].attributes[1].name, "flag");
  EXPECT_TRUE(result.document.source_blocks[1].attributes[1].value.empty());
}

TEST(LynxMLParserTest, PreservesAttributeValuesVerbatim) {
  auto result = ParseForTest(
      "<!doctype lynx><lynx page-config='A&amp;B &#65;'>"
      "<script thread=main>main</script>"
      "</lynx>");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.attributes.size(), 1u);
  EXPECT_EQ(result.document.attributes[0].value, "A&amp;B &#65;");
  ASSERT_EQ(result.document.source_blocks.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[0].type, SourceBlockType::kScript);
}

TEST(LynxMLParserTest, TreatsSourceBlockContentAsRawText) {
  const std::string raw =
      "<!-- not a comment --> &amp; <![CDATA[not special]]> "
      "const tag = '</script-like>';";
  auto result = ParseForTest("<!doctype lynx><lynx><script thread=main>" + raw +
                             "</script></lynx>");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.source_blocks.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[0].source, raw);
}

TEST(LynxMLParserTest, AcceptsEmptyDocumentAndRepeatedBlocks) {
  auto empty = ParseForTest("<!doctype lynx><lynx></lynx>");
  ASSERT_TRUE(empty.success()) << empty.error.ToString();
  EXPECT_TRUE(empty.document.source_blocks.empty());

  auto repeated = ParseForTest(
      "<!doctype lynx><lynx>"
      "<script thread=background>one</script>"
      "<script thread=background>two</script>"
      "</lynx>");
  ASSERT_TRUE(repeated.success()) << repeated.error.ToString();
  ASSERT_EQ(repeated.document.source_blocks.size(), 2u);
  EXPECT_EQ(repeated.document.source_blocks[0].source, "one");
  EXPECT_EQ(repeated.document.source_blocks[1].source, "two");
}

TEST(LynxMLParserTest, RequiresExactlyOneLeadingDoctype) {
  const char* documents[] = {
      "<lynx></lynx>",
      "<?xml version='1.0'?><!doctype lynx><lynx></lynx>",
      "<!-- before --><!doctype lynx><lynx></lynx>",
      "<!doctype html><lynx></lynx>",
      "<!doctype lynx PUBLIC 'id'><lynx></lynx>",
      "<!doctype lynx><!doctype lynx><lynx></lynx>",
      "<!doctype lynx><lynx><!doctype lynx></lynx>",
  };

  for (const char* document : documents) {
    auto result = ParseForTest(document);
    EXPECT_FALSE(result.success()) << document;
    EXPECT_EQ(result.error.code, ParseErrorCode::kSyntaxError) << document;
  }
}

TEST(LynxMLParserTest, RejectsDuplicateAttributes) {
  const char* documents[] = {
      "<!doctype lynx><lynx config=one config=two></lynx>",
      "<!doctype lynx><lynx><script thread=main thread=background>source"
      "</script></lynx>",
  };

  for (const char* document : documents) {
    auto result = ParseForTest(document);
    EXPECT_FALSE(result.success()) << document;
    EXPECT_EQ(result.error.code, ParseErrorCode::kSyntaxError) << document;
  }
}

TEST(LynxMLParserTest, TreatsDifferentlyCasedAttributesAsDistinct) {
  auto result = ParseForTest(
      "<!doctype lynx><lynx config=one CONFIG=two>"
      "<script thread=main THREAD=background>source</script></lynx>");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.attributes.size(), 2u);
  EXPECT_EQ(result.document.attributes[0].name, "config");
  EXPECT_EQ(result.document.attributes[1].name, "CONFIG");
  ASSERT_EQ(result.document.source_blocks.size(), 1u);
  ASSERT_EQ(result.document.source_blocks[0].attributes.size(), 2u);
  EXPECT_EQ(result.document.source_blocks[0].attributes[0].name, "thread");
  EXPECT_EQ(result.document.source_blocks[0].attributes[1].name, "THREAD");
}

TEST(LynxMLParserTest, DoesNotEnforceAttributeNamingConvention) {
  auto result = ParseForTest(
      "<!doctype lynx><lynx camelCase=one page--config=two "
      "page-config-=three></lynx>");

  ASSERT_TRUE(result.success()) << result.error.ToString();
  ASSERT_EQ(result.document.attributes.size(), 3u);
  EXPECT_EQ(result.document.attributes[0].name, "camelCase");
  EXPECT_EQ(result.document.attributes[1].name, "page--config");
  EXPECT_EQ(result.document.attributes[2].name, "page-config-");
}

TEST(LynxMLParserTest, RejectsUnsupportedTopLevelElements) {
  auto result = ParseForTest("<!doctype lynx><lynx><view></view></lynx>");

  EXPECT_FALSE(result.success());
  EXPECT_EQ(result.error.code, ParseErrorCode::kUnsupportedFeature);
  EXPECT_EQ(result.error.ToString().find("unsupported LynxML feature"), 0u);
}

TEST(LynxMLParserTest, RejectsMalformedTagsAndComments) {
  const char* documents[] = {
      "<!doctype lynx><lynx/>",
      "<!doctype lynx><lynx><style>source</lynx>",
      "<!doctype lynx><lynx><script thread=main>source</script worker></lynx>",
      "<!doctype lynx><lynx><!-- unterminated</lynx>",
      "<!doctype lynx><lynx></lynx>trailing",
  };

  for (const char* document : documents) {
    auto result = ParseForTest(document);
    EXPECT_FALSE(result.success()) << document;
    EXPECT_EQ(result.error.code, ParseErrorCode::kSyntaxError) << document;
  }
}

TEST(LynxMLParserTest, KeepsProducedBlocksWhenLaterInputFails) {
  auto result = ParseForTest(
      "<!doctype lynx><lynx><style>valid</style><view></view></lynx>");

  ASSERT_FALSE(result.success());
  ASSERT_EQ(result.document.source_blocks.size(), 1u);
  EXPECT_EQ(result.document.source_blocks[0].source, "valid");
  EXPECT_EQ(result.error.code, ParseErrorCode::kUnsupportedFeature);
  EXPECT_EQ(result.events,
            (std::vector<std::string>{"document-start", "style", "error"}));
}

}  // namespace
}  // namespace lynxml
}  // namespace lynx
