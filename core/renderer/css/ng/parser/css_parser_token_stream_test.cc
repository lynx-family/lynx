// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/parser/css_parser_token_stream.h"

#include <string>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace css {

namespace {

TEST(CSSParserTokenStreamTest, EmptyStream) {
  CSSTokenizer tokenizer("");
  CSSParserTokenStream stream(tokenizer);
  EXPECT_TRUE(stream.Consume().IsEOF());
  EXPECT_TRUE(stream.Peek().IsEOF());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, PeekThenConsume) {
  CSSTokenizer tokenizer("A");  // kIdent
  CSSParserTokenStream stream(tokenizer);
  EXPECT_EQ(kIdentToken, stream.Peek().GetType());
  EXPECT_EQ(kIdentToken, stream.Consume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, ConsumeThenPeek) {
  CSSTokenizer tokenizer("A");  // kIdent
  CSSParserTokenStream stream(tokenizer);
  EXPECT_EQ(kIdentToken, stream.Consume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, ConsumeMultipleTokens) {
  CSSTokenizer tokenizer("A 1");  // kIdent kWhitespace kNumber
  CSSParserTokenStream stream(tokenizer);
  EXPECT_EQ(kIdentToken, stream.Consume().GetType());
  EXPECT_EQ(kWhitespaceToken, stream.Consume().GetType());
  EXPECT_EQ(kNumberToken, stream.Consume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, UncheckedPeekAndConsumeAfterPeek) {
  CSSTokenizer tokenizer("A");  // kIdent
  CSSParserTokenStream stream(tokenizer);
  EXPECT_EQ(kIdentToken, stream.Peek().GetType());
  EXPECT_EQ(kIdentToken, stream.UncheckedPeek().GetType());
  EXPECT_EQ(kIdentToken, stream.UncheckedConsume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, UncheckedPeekAndConsumeAfterAtEnd) {
  CSSTokenizer tokenizer("A");  // kIdent
  CSSParserTokenStream stream(tokenizer);
  EXPECT_FALSE(stream.AtEnd());
  EXPECT_EQ(kIdentToken, stream.UncheckedPeek().GetType());
  EXPECT_EQ(kIdentToken, stream.UncheckedConsume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, UncheckedConsumeComponentValue) {
  CSSTokenizer tokenizer("A{1}{2{3}}B");
  CSSParserTokenStream stream(tokenizer);

  EXPECT_EQ(kIdentToken, stream.Peek().GetType());
  stream.UncheckedConsumeComponentValue();
  EXPECT_EQ(kLeftBraceToken, stream.Peek().GetType());
  stream.UncheckedConsumeComponentValue();
  EXPECT_EQ(kLeftBraceToken, stream.Peek().GetType());
  stream.UncheckedConsumeComponentValue();
  EXPECT_EQ(kIdentToken, stream.Peek().GetType());
  stream.UncheckedConsumeComponentValue();

  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, ConsumeWhitespace) {
  CSSTokenizer tokenizer(" \t\n");  // kWhitespace
  CSSParserTokenStream stream(tokenizer);

  EXPECT_EQ(kWhitespaceToken, stream.Consume().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, ConsumeIncludingWhitespace) {
  CSSTokenizer tokenizer("A \t\n");  // kIdent kWhitespace
  CSSParserTokenStream stream(tokenizer);

  EXPECT_EQ(kIdentToken, stream.ConsumeIncludingWhitespace().GetType());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, RangesDoNotGetInvalidatedWhenConsuming) {
  std::string s;
  s.append("1 ");
  for (int i = 0; i < 100; i++) s.append("A ");

  CSSTokenizer tokenizer(s);
  CSSParserTokenStream stream(tokenizer);

  // Consume a single token range.
  auto range = stream.ConsumeUntilPeekedTypeIs<kIdentToken>();

  EXPECT_EQ(kNumberToken, range.Peek().GetType());

  // Consume remaining tokens to try to invalidate the range.
  while (!stream.AtEnd()) stream.ConsumeIncludingWhitespace();

  EXPECT_EQ(kNumberToken, range.ConsumeIncludingWhitespace().GetType());
  EXPECT_TRUE(range.AtEnd());
}

TEST(CSSParserTokenStreamTest, BlockErrorRecoveryConsumesRestOfBlock) {
  CSSTokenizer tokenizer("{B }1");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::BlockGuard guard(stream);
    EXPECT_EQ(kIdentToken, stream.Consume().GetType());
    EXPECT_FALSE(stream.AtEnd());
  }  // calls destructor

  EXPECT_EQ(kNumberToken, stream.Consume().GetType());
}

TEST(CSSParserTokenStreamTest, BlockErrorRecoveryOnSuccess) {
  CSSTokenizer tokenizer("{B }1");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::BlockGuard guard(stream);
    EXPECT_EQ(kIdentToken, stream.Consume().GetType());
    EXPECT_EQ(kWhitespaceToken, stream.Consume().GetType());
    EXPECT_TRUE(stream.AtEnd());
  }  // calls destructor

  EXPECT_EQ(kNumberToken, stream.Consume().GetType());
}

TEST(CSSParserTokenStreamTest, BlockErrorRecoveryConsumeComponentValue) {
  CSSTokenizer tokenizer("{{B} C}1");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::BlockGuard guard(stream);
    stream.EnsureLookAhead();
    stream.UncheckedConsumeComponentValue();
  }  // calls destructor

  EXPECT_EQ(kNumberToken, stream.Consume().GetType());
}

TEST(CSSParserTokenStreamTest, OffsetAfterPeek) {
  CSSTokenizer tokenizer("ABC");
  CSSParserTokenStream stream(tokenizer);

  EXPECT_EQ(0U, stream.Offset());
  EXPECT_EQ(kIdentToken, stream.Peek().GetType());
  EXPECT_EQ(0U, stream.Offset());
}

TEST(CSSParserTokenStreamTest, OffsetAfterConsumes) {
  CSSTokenizer tokenizer("ABC 1 {23 }");
  CSSParserTokenStream stream(tokenizer);

  EXPECT_EQ(0U, stream.Offset());
  EXPECT_EQ(kIdentToken, stream.Consume().GetType());
  EXPECT_EQ(3U, stream.Offset());
  EXPECT_EQ(kWhitespaceToken, stream.Consume().GetType());
  EXPECT_EQ(4U, stream.Offset());
  EXPECT_EQ(kNumberToken, stream.ConsumeIncludingWhitespace().GetType());
  EXPECT_EQ(6U, stream.Offset());
  stream.EnsureLookAhead();
  stream.UncheckedConsumeComponentValue();
  EXPECT_EQ(11U, stream.Offset());
}

TEST(CSSParserTokenStreamTest, LookAheadOffset) {
  CSSTokenizer tokenizer("ABC/* *//* */1");
  CSSParserTokenStream stream(tokenizer);

  stream.EnsureLookAhead();
  EXPECT_EQ(0U, stream.Offset());
  EXPECT_EQ(0U, stream.LookAheadOffset());
  EXPECT_EQ(kIdentToken, stream.Consume().GetType());

  stream.EnsureLookAhead();
  EXPECT_EQ(3U, stream.Offset());
  EXPECT_EQ(13U, stream.LookAheadOffset());
}

TEST(CSSParserTokenStreamTest, ConsumeUntilPeekedTypeIsEmpty) {
  CSSTokenizer tokenizer("{23 }");
  CSSParserTokenStream stream(tokenizer);

  auto range = stream.ConsumeUntilPeekedTypeIs<>();
  EXPECT_TRUE(stream.AtEnd());

  EXPECT_EQ(kLeftBraceToken, range.Consume().GetType());
  EXPECT_EQ(kNumberToken, range.Consume().GetType());
  EXPECT_EQ(kWhitespaceToken, range.Consume().GetType());
  EXPECT_EQ(kRightBraceToken, range.Consume().GetType());
  EXPECT_TRUE(range.AtEnd());
}

TEST(CSSParserTokenStreamTest, Boundary) {
  CSSTokenizer tokenizer("foo:red;bar:blue;asdf");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::Boundary boundary(stream, kSemicolonToken);
    CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
    EXPECT_EQ(u"foo", range.Consume().Value());
    EXPECT_EQ(kColonToken, range.Consume().GetType());
    EXPECT_EQ(u"red", range.Consume().Value());
    EXPECT_TRUE(stream.AtEnd());
  }

  EXPECT_FALSE(stream.AtEnd());
  EXPECT_EQ(kSemicolonToken, stream.Consume().GetType());

  {
    CSSParserTokenStream::Boundary boundary(stream, kSemicolonToken);
    CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
    EXPECT_EQ(u"bar", range.Consume().Value());
    EXPECT_EQ(kColonToken, range.Consume().GetType());
    EXPECT_EQ(u"blue", range.Consume().Value());
    EXPECT_TRUE(stream.AtEnd());
  }

  EXPECT_FALSE(stream.AtEnd());
  EXPECT_EQ(kSemicolonToken, stream.Consume().GetType());

  EXPECT_EQ(u"asdf", stream.Consume().Value());
  EXPECT_TRUE(stream.AtEnd());
}

TEST(CSSParserTokenStreamTest, MultipleBoundaries) {
  CSSTokenizer tokenizer("a:b,c;d:,;e");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::Boundary boundary_semicolon(stream, kSemicolonToken);

    {
      CSSParserTokenStream::Boundary boundary_comma(stream, kCommaToken);

      {
        CSSParserTokenStream::Boundary boundary_colon(stream, kColonToken);
        CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
        EXPECT_EQ(u"a", range.Consume().Value());
        EXPECT_TRUE(range.AtEnd());
        EXPECT_TRUE(stream.AtEnd());
      }

      EXPECT_FALSE(stream.AtEnd());
      EXPECT_EQ(kColonToken, stream.Consume().GetType());

      CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
      EXPECT_EQ(u"b", range.Consume().Value());
      EXPECT_TRUE(range.AtEnd());
      EXPECT_TRUE(stream.AtEnd());
    }

    EXPECT_FALSE(stream.AtEnd());
    EXPECT_EQ(kCommaToken, stream.Consume().GetType());

    CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
    EXPECT_EQ(u"c", range.Consume().Value());
    EXPECT_TRUE(range.AtEnd());
    EXPECT_TRUE(stream.AtEnd());
  }

  EXPECT_FALSE(stream.AtEnd());
  EXPECT_EQ(kSemicolonToken, stream.Consume().GetType());

  CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
  EXPECT_TRUE(stream.AtEnd());

  EXPECT_EQ(u"d", range.Consume().Value());
  EXPECT_EQ(kColonToken, range.Consume().GetType());
  EXPECT_EQ(kCommaToken, range.Consume().GetType());
  EXPECT_EQ(kSemicolonToken, range.Consume().GetType());
  EXPECT_EQ(u"e", range.Consume().Value());
}

TEST(CSSParserTokenStreamTest, IneffectiveBoundary) {
  CSSTokenizer tokenizer("a:b|");
  CSSParserTokenStream stream(tokenizer);

  {
    CSSParserTokenStream::Boundary boundary_colon(stream, kColonToken);

    {
      // It's valid to add another boundary, but it has no affect in this
      // case, since kColonToken appears first.
      CSSParserTokenStream::Boundary boundary_semicolon(stream,
                                                        kSemicolonToken);

      CSSParserTokenRange range = stream.ConsumeUntilPeekedTypeIs<>();
      EXPECT_EQ(u"a", range.Consume().Value());
      EXPECT_TRUE(range.AtEnd());

      EXPECT_EQ(kColonToken, stream.Peek().GetType());
      EXPECT_TRUE(stream.AtEnd());
    }

    EXPECT_TRUE(stream.AtEnd());
  }

  EXPECT_FALSE(stream.AtEnd());
}

}  // namespace
}  // namespace css
}  // namespace lynx
