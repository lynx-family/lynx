// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_load_meta_priv.h"
#include "platform/embedder/lynx_template_bundle_priv.h"
#include "platform/embedder/lynx_template_data_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxLoadMeta, Create) {
  lynx_load_meta_t* meta = lynx_load_meta_create();
  lynx_load_meta_set_url(meta, "assets://main.lynx.bundle");
  EXPECT_STREQ(meta->url.c_str(), "assets://main.lynx.bundle");
  uint8_t content[] = {0x01, 0x02, 0x03};
  lynx_load_meta_set_binary_data(meta, content, sizeof(content), nullptr,
                                 nullptr);
  EXPECT_EQ(meta->binary_data.length, sizeof(content));
  EXPECT_EQ(meta->binary_data.data[0], 0x01);
  EXPECT_EQ(meta->binary_data.data[1], 0x02);
  EXPECT_EQ(meta->binary_data.data[2], 0x03);
  lynx_load_meta_release(meta);
}

TEST(LynxLoadMeta, TemplateBundle) {
  lynx_load_meta_t* meta = lynx_load_meta_create();
  uint8_t content[] = {0x00, 0x00, 0x00, 0x00};
  lynx_template_bundle_t* bundle =
      lynx_template_bundle_create(content, sizeof(content), nullptr, nullptr);
  lynx_load_meta_set_template_bundle(meta, bundle);
  EXPECT_EQ(meta->template_bundle, bundle->template_bundle);
  EXPECT_EQ(lynx_load_meta_is_template_bundle_valid(meta), 0);

  lynx_load_meta_release(meta);
}
