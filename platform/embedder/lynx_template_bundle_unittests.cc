// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_template_bundle_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxTemplateBundle, Invalid) {
  uint8_t content[] = {0x00, 0x00, 0x00, 0x00};
  lynx_template_bundle_t* bundle =
      lynx_template_bundle_create(content, sizeof(content), nullptr, nullptr);
  EXPECT_EQ(lynx_template_bundle_is_valid(bundle), 0);
  lynx_template_bundle_release(bundle);
}
