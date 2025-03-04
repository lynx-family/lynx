// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/parser/css_string_scanner.h"

#include <array>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/parser/css_string_parser.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

TEST(CSSStringScanner, EmptyString) {
  std::string input = "";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));
  EXPECT_EQ(scanner.ScanToken().type, TokenType::TOKEN_EOF);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, Consume) {
  std::string input = "A";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));
  EXPECT_EQ(scanner.ScanToken().type, TokenType::IDENTIFIER);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeMultipleTokens) {
  std::string input = "A 1";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));
  EXPECT_EQ(scanner.ScanToken().type, TokenType::IDENTIFIER);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::WHITESPACE);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::NUMBER);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeComponentValue) {
  std::string input = "A(1)(2(3))B";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  EXPECT_EQ(scanner.ScanToken().type, TokenType::IDENTIFIER);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::LEFT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::NUMBER);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::RIGHT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::LEFT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::NUMBER);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::LEFT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::NUMBER);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::RIGHT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::RIGHT_PAREN);
  EXPECT_EQ(scanner.ScanToken().type, TokenType::IDENTIFIER);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeWhitespace) {
  std::string input = " \t\n";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  EXPECT_EQ(scanner.ScanToken().type, TokenType::WHITESPACE);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeString) {
  {
    std::string input = "\"A\"";
    Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

    auto token = scanner.ScanToken();

    EXPECT_EQ(token.type, TokenType::STRING);
    EXPECT_EQ(std::string(token.start, token.length), "A");
    EXPECT_TRUE(scanner.IsAtEnd());
  }
  {
    std::string input = "'a'";
    Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

    auto token = scanner.ScanToken();

    EXPECT_EQ(token.type, TokenType::STRING);
    EXPECT_EQ(std::string(token.start, token.length), "a");
    EXPECT_TRUE(scanner.IsAtEnd());
  }
  {
    std::string input = "'as\"";
    Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

    auto token = scanner.ScanToken();

    EXPECT_EQ(token.type, TokenType::ERROR);
    EXPECT_TRUE(scanner.IsAtEnd());
  }
}

TEST(CSSStringScanner, ConsumeHex) {
  std::string input = "#00214";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  auto token = scanner.ScanToken();

  EXPECT_EQ(token.type, TokenType::HEX);
  EXPECT_EQ(std::string(token.start, token.length), "00214");
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeFunction) {
  std::string input = "calc()";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  auto token = scanner.ScanToken();

  EXPECT_EQ(token.type, TokenType::CALC);
  EXPECT_EQ(std::string(token.start, token.length), input);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeFunctionWithArguments) {
  std::string input = "calc(2px)";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  auto token = scanner.ScanToken();

  EXPECT_EQ(token.type, TokenType::CALC);
  EXPECT_EQ(std::string(token.start, token.length), input);
  EXPECT_TRUE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, ConsumeFunctionError) {
  std::string input = "calc((2px)))";
  Scanner scanner(input.c_str(), static_cast<uint32_t>(input.size()));

  auto token = scanner.ScanToken();

  EXPECT_EQ(token.type, TokenType::CALC);
  EXPECT_FALSE(scanner.IsAtEnd());
}

TEST(CSSStringScanner, Dimension) {
  std::string input = "20px";
  Scanner scanner1(input.c_str(), static_cast<uint32_t>(input.size()));
  auto token = scanner1.ScanToken();
  EXPECT_EQ(token.type, TokenType::DIMENSION);
  EXPECT_EQ(token.unit, TokenType::PX);
  EXPECT_TRUE(scanner1.IsAtEnd());

  input = "20 px";
  Scanner scanner2(input.c_str(), static_cast<uint32_t>(input.size()));
  auto number = scanner2.ScanToken();
  EXPECT_EQ(number.type, TokenType::NUMBER);
  EXPECT_FALSE(scanner2.IsAtEnd());
}

TEST(CSSStringScanner, Ident) {
  std::string input = "item1-ani-frames";
  Scanner scanner1(input.c_str(), static_cast<uint32_t>(input.size()));
  auto token = scanner1.ScanToken();
  EXPECT_EQ(token.type, TokenType::IDENTIFIER);
  EXPECT_EQ(std::string(token.start, token.length), input);
  EXPECT_TRUE(scanner1.IsAtEnd());
}

}  // namespace tasm
}  // namespace lynx
