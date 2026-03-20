// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_builder_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

TEST(LynxViewBuilder, Create) {
  lynx_view_builder_t* builder = lynx_view_builder_create();

  lynx_view_builder_set_screen_size(builder, 100, 200, 1.0);
  lynx_view_builder_set_frame(builder, 0, 0, 100, 200);
  lynx_view_builder_set_font_scale(builder, 1.0);

  lynx_group_t* group = lynx_group_create("group");
  lynx_view_builder_set_lynx_group(builder, group);

  lynx_view_builder_register_native_module(
      builder, "test",
      [](napi_env, napi_value exports, const char* module_name, void* opaque) {
        return exports;
      },
      nullptr);

  EXPECT_EQ(builder->screen_size.width, 100);
  EXPECT_EQ(builder->screen_size.height, 200);
  EXPECT_EQ(builder->screen_size.pixel_ratio, 1.0);
  EXPECT_EQ(builder->frame.x, 0);
  EXPECT_EQ(builder->frame.y, 0);
  EXPECT_EQ(builder->frame.width, 100);
  EXPECT_EQ(builder->frame.height, 200);
  EXPECT_EQ(builder->font_scale, 1.0);
  EXPECT_EQ(builder->group, group);
  EXPECT_EQ(builder->native_modules.size(), 1);

  lynx_view_builder_release(builder);
  lynx_group_release(group);
}
