// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider.h"

#include "core/runtime/jscache/quickjs/bytecode/quickjs_bytecode_provider_src.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/interface.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace quickjs {
namespace testing {
TEST(QuickjsBytecodeProvider, IsBytecodePass) {
  Bytecode::HeaderV1 header(0, base::Version(""));
  std::string str;
  str.append(reinterpret_cast<char *>(&header), sizeof(header));
  EXPECT_TRUE(
      QuickjsBytecodeProvider::IsBytecode(std::make_shared<StringBuffer>(str)));
}

TEST(QuickjsBytecodeProvider, IsBytecodeFail) {
  EXPECT_FALSE(QuickjsBytecodeProvider::IsBytecode(nullptr));
  EXPECT_FALSE(
      QuickjsBytecodeProvider::IsBytecode(std::make_shared<StringBuffer>("")));
  EXPECT_FALSE(QuickjsBytecodeProvider::IsBytecode(
      std::make_shared<StringBuffer>("wrong")));

  // invalid magic
  {
    Bytecode::HeaderV1 header(0, base::Version(""));
    const_cast<uint32_t &>(header.base_header.magic) += 1;
    std::string buffer;
    buffer.append(reinterpret_cast<char *>(&header), sizeof(header));
    EXPECT_FALSE(QuickjsBytecodeProvider::IsBytecode(
        std::make_shared<StringBuffer>(buffer.data())));
  }

  // invalid size
  {
    Bytecode::HeaderV1 header(0, base::Version(""));
    std::string buffer;
    buffer.append(reinterpret_cast<char *>(&header), sizeof(header));
    buffer.resize(sizeof(uint32_t));
    EXPECT_FALSE(QuickjsBytecodeProvider::IsBytecode(
        std::make_shared<StringBuffer>(buffer.data())));
  }
}

TEST(QuickjsBytecodeProvider, IsBytecodeValidPass) {
  {
    Bytecode::HeaderV1 header(0, base::Version(""));
    std::string str;
    str.append(reinterpret_cast<char *>(&header), sizeof(header));
    EXPECT_TRUE(QuickjsBytecodeProvider::ValidateBytecode(
                    std::make_shared<StringBuffer>(str))
                    .first);
  }

  {
    std::string str = "";
    std::string data = "test_data";
    Bytecode::HeaderV1 header(data.size(), base::Version("2.1"));
    str.append(reinterpret_cast<char *>(&header), sizeof(header));
    str.append(data.data(), data.size());
    EXPECT_TRUE(QuickjsBytecodeProvider::ValidateBytecode(
                    std::make_shared<StringBuffer>(str))
                    .first);
  }
}

TEST(QuickjsBytecodeProvider, IsBytecodeValidFail) {
  EXPECT_FALSE(QuickjsBytecodeProvider::ValidateBytecode(nullptr).first);
  EXPECT_FALSE(
      QuickjsBytecodeProvider::IsBytecode(std::make_shared<StringBuffer>("")));
  EXPECT_FALSE(QuickjsBytecodeProvider::IsBytecode(
      std::make_shared<StringBuffer>("wrong")));

  {
    Bytecode::HeaderV1 header(1, base::Version(""));
    std::string str;
    str.append(reinterpret_cast<char *>(&header), sizeof(header));
    EXPECT_FALSE(QuickjsBytecodeProvider::ValidateBytecode(
                     std::make_shared<StringBuffer>(str))
                     .first);
  }

  {
    Bytecode::HeaderV1 header(0, base::Version("2.1"));
    std::string str;
    str.append(reinterpret_cast<char *>(&header), sizeof(header));
    str.resize(str.size() - 1);
    EXPECT_FALSE(QuickjsBytecodeProvider::ValidateBytecode(
                     std::make_shared<StringBuffer>(str))
                     .first);
  }

  {
    Bytecode::HeaderV1 header(0, base::Version(""));
    const_cast<uint32_t &>(header.base_header.header_version) += 1;

    std::string str;
    str.append(reinterpret_cast<char *>(&header), sizeof(header));
    EXPECT_FALSE(QuickjsBytecodeProvider::ValidateBytecode(
                     std::make_shared<StringBuffer>(str))
                     .first);
  }
}

TEST(QuickjsBytecodeProviderSrc, CompileToProviderPass) {
  auto source = QuickjsBytecodeProvider::FromSource(
      "url", std::make_shared<StringBuffer>("var v = 0;"));
  auto provider = source.Compile(base::Version("2.10"), {});
  EXPECT_TRUE(provider.has_value());
  EXPECT_TRUE(provider->GetPackedBytecodeBuffer()->size());
  EXPECT_TRUE(provider->GetRawBytecode()->size());
}

TEST(QuickjsBytecodeProviderSrc, CompileToProviderFail) {
  std::shared_ptr<Buffer> invalid_srcs[3] = {
      nullptr, std::make_shared<StringBuffer>(""),
      std::make_shared<StringBuffer>("????")};

  for (auto &invalid_src : invalid_srcs) {
    auto source = QuickjsBytecodeProvider::FromSource("url", invalid_src);
    auto provider = source.Compile(base::Version("2.10"), {});
    EXPECT_FALSE(provider);
  }
}

TEST(QuickjsBytecodeProvider, FromPackedBytecodeFail) {
  {
    auto bytecode = QuickjsBytecodeProvider::FromPackedBytecode(nullptr);
    EXPECT_FALSE(bytecode.has_value());
  }

  {
    auto bytecode = QuickjsBytecodeProvider::FromPackedBytecode(
        std::make_shared<StringBuffer>(""));
    EXPECT_FALSE(bytecode.has_value());
  }

  {
    auto source = QuickjsBytecodeProvider::FromSource(
        "url", std::make_shared<StringBuffer>("var v = 0;"));
    auto provider = source.Compile(base::Version("2.10"), {});
    auto buffer = provider->GetPackedBytecodeBuffer();

    *const_cast<uint32_t *>(
        &const_cast<Bytecode::HeaderV1 *>(
             reinterpret_cast<const Bytecode::HeaderV1 *>(buffer->data()))
             ->base_header.header_version) =
        Bytecode::LATEST_HEADER_VERSION + 1;
    auto bytecode = QuickjsBytecodeProvider::FromPackedBytecode(buffer);
    EXPECT_FALSE(bytecode.has_value());
  }
}

TEST(QuickjsBytecodeProvider, GetPackagedBytecodeBufferPass) {
  auto source = QuickjsBytecodeProvider::FromSource(
      "url", std::make_shared<StringBuffer>("var v = 0;"));
  auto provider = source.Compile(base::Version("2.10"), {});
  EXPECT_TRUE(provider);
  EXPECT_TRUE(provider->GetPackedBytecodeBuffer()->size());

  auto bytecode = QuickjsBytecodeProvider::FromPackedBytecode(
      provider->GetPackedBytecodeBuffer());

  EXPECT_TRUE(bytecode->GetPackedBytecodeBuffer()->size());
  EXPECT_TRUE(bytecode->GetTargetSdkVersion() == base::Version("2.10"));
  EXPECT_TRUE(bytecode->GetRawBytecode()->size() ==
              provider->GetRawBytecode()->size());
  EXPECT_TRUE(0 == std::memcmp(bytecode->GetRawBytecode()->data(),
                               provider->GetRawBytecode()->data(),
                               bytecode->GetRawBytecode()->size()));
}

}  // namespace testing
}  // namespace quickjs
}  // namespace piper
}  // namespace lynx
