// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_update_meta_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxUpdateMeta, Create) {
  lynx_update_meta_t* meta = lynx_update_meta_create();

  lynx_template_data_t* update_data =
      lynx_template_data_create_from_json("{\"key\": \"value\"}");
  lynx_update_meta_set_update_data(meta, update_data);
  EXPECT_TRUE(meta->update_data != nullptr);

  lynx_template_data_t* global_props =
      lynx_template_data_create_from_json("{\"key\": \"value\"}");
  lynx_update_meta_set_global_props(meta, global_props);
  EXPECT_TRUE(meta->global_props != nullptr);

  lynx_update_meta_release(meta);
}
