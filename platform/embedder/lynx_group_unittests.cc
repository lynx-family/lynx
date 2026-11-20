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

TEST(LynxGroup, SetEnableWhiteBoard) {
  lynx_group_t* group = lynx_group_create_with_id("group", LYNX_SINGLE_GROUP);
  EXPECT_TRUE(group != nullptr);
  EXPECT_EQ(group->white_board, nullptr);

  lynx_group_set_enable_white_board(group, 1);
  EXPECT_NE(group->white_board, nullptr);
  auto white_board = group->white_board;

  lynx_group_set_enable_white_board(group, 1);
  EXPECT_EQ(group->white_board, white_board);

  lynx_group_set_enable_white_board(group, 0);
  EXPECT_EQ(group->white_board, nullptr);
  lynx_group_release(group);
}

TEST(LynxGroup, SetJSGroupThreadName) {
  auto* group = lynx_group_create_with_id("group", "shared-group");
  EXPECT_TRUE(group->js_group_thread_name.empty());
  std::string name = "shared-thread";
  lynx_group_set_js_group_thread_name(group, name.c_str());
  name.clear();
  EXPECT_EQ(group->js_group_thread_name, "shared-thread");
  EXPECT_EQ(group->id, "shared-group");
  EXPECT_FALSE(group->enable_js_group_thread);
  lynx_group_set_js_group_thread_name(group, "");
  EXPECT_TRUE(group->js_group_thread_name.empty());
  lynx_group_set_js_group_thread_name(group, "other-thread");
  lynx_group_set_js_group_thread_name(group, nullptr);
  EXPECT_TRUE(group->js_group_thread_name.empty());
  lynx_group_release(group);
}

TEST(LynxGroup, SetPreloadJSPathsCopiesStrings) {
  auto* group = lynx_group_create_with_id("group", "shared-group");
  std::string path = "test://preload.js";
  const char* paths[] = {path.c_str()};
  lynx_group_set_preload_js_paths(group, paths, 1);
  path.clear();
  ASSERT_EQ(group->preload_js_paths.size(), 1u);
  EXPECT_EQ(group->preload_js_paths[0], "test://preload.js");
  lynx_group_set_preload_js_paths(group, nullptr, 0);
  EXPECT_TRUE(group->preload_js_paths.empty());
  lynx_group_release(group);
}
