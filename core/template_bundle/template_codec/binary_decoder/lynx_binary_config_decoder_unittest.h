// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_UNITTEST_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_UNITTEST_H_

#include <memory>

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_decoder.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
class LynxBinaryConfigDecoderTest : public ::testing::Test {
 public:
  LynxBinaryConfigDecoderTest() {
    config_decoder_ = std::make_unique<LynxBinaryConfigDecoder>(
        tasm::CompileOptions(), "3.2", true, false);

    page_config_ = std::make_shared<PageConfig>();
  };
  ~LynxBinaryConfigDecoderTest() = default;
  void SetUp() override {}
  void TearDown() override {}

 private:
  std::unique_ptr<LynxBinaryConfigDecoder> config_decoder_{nullptr};
  std::shared_ptr<PageConfig> page_config_{nullptr};
};
}  // namespace test
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_DECODER_LYNX_BINARY_CONFIG_DECODER_UNITTEST_H_
