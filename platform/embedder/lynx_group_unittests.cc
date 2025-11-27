// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_group_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxGroup, Create) {
  lynx_group_t* group = lynx_group_create("group");
  EXPECT_TRUE(group != nullptr);
  EXPECT_STREQ(group->name.c_str(), "group");
  EXPECT_STREQ(group->id.c_str(), "0");
  lynx_group_release(group);
}

TEST(LynxGroup, CreateWithId) {
  lynx_group_t* group = lynx_group_create_with_id("group", LYNX_SINGLE_GROUP);
  EXPECT_TRUE(group != nullptr);
  EXPECT_STREQ(group->name.c_str(), "group");
  EXPECT_STREQ(group->id.c_str(), LYNX_SINGLE_GROUP);
  lynx_group_release(group);
}

TEST(LynxGroup, SetEnableJsGroupThread) {
  lynx_group_t* group = lynx_group_create_with_id("group", LYNX_SINGLE_GROUP);
  EXPECT_TRUE(group != nullptr);
  lynx_group_set_enable_js_group_thread(group, 1);
  EXPECT_TRUE(group->enable_js_group_thread);
  lynx_group_set_enable_js_group_thread(group, 0);
  EXPECT_FALSE(group->enable_js_group_thread);
  lynx_group_release(group);
}
