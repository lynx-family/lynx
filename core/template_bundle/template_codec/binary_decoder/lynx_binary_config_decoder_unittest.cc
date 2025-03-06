// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_decoder_unittest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST_F(LynxBinaryConfigDecoderTest, ReadPipelineSchedulerConfig) {
  EXPECT_FALSE(page_config_->GetEnableParallelParseElementTemplate());
  config_decoder_->DecodePageConfig("{\n  \"pipelineSchedulerConfig\" : 1\n}",
                                    page_config_);
  EXPECT_TRUE(page_config_->GetEnableParallelParseElementTemplate());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
