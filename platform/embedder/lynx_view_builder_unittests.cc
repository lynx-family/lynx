// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/lynx_vsync_monitor_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

TEST(LynxViewBuilder, DefaultBackgroundColor) {
  auto* builder = lynx_view_builder_create();
  auto* other = lynx_view_builder_create();
  EXPECT_FALSE(builder->default_background_color.has_value());
  lynx_view_builder_set_default_background_color(builder, 0x00000000u);
  ASSERT_TRUE(builder->default_background_color.has_value());
  EXPECT_EQ(*builder->default_background_color, 0x00000000u);
  lynx_view_builder_set_default_background_color(builder, 0x80402010u);
  EXPECT_EQ(*builder->default_background_color, 0x80402010u);
  EXPECT_FALSE(other->default_background_color.has_value());
  lynx_view_builder_release(other);
  lynx_view_builder_release(builder);
}

TEST(LynxViewBuilder, Create) {
  lynx_view_builder_t* builder = lynx_view_builder_create();

  lynx_view_builder_set_screen_size(builder, 100, 200, 1.0);
  lynx_view_builder_set_frame(builder, 0, 0, 100, 200);
  lynx_view_builder_set_font_scale(builder, 1.0);
  lynx_view_builder_set_enable_js_runtime(builder, false);

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
  EXPECT_FALSE(builder->enable_js_runtime);
  EXPECT_EQ(builder->group, group);
  EXPECT_EQ(builder->native_modules.size(), 1);

  lynx_view_builder_release(builder);
  lynx_group_release(group);
}

TEST(LynxViewBuilder, CustomVSyncFinalizesUnusedAndReplacedMonitors) {
  int finalized = 0;
  auto create = [&] {
    return lynx_vsync_monitor_create_with_finalizer(
        &finalized, [](auto*, void* data) { ++*static_cast<int*>(data); });
  };
  auto* builder = lynx_view_builder_create();
  auto* first = create();
  lynx_view_builder_set_custom_vsync_monitor(builder, first);
  lynx_view_builder_set_custom_vsync_monitor(builder, first);
  EXPECT_EQ(finalized, 0);
  lynx_view_builder_set_custom_vsync_monitor(builder, create());
  EXPECT_EQ(finalized, 1);
  lynx_view_builder_set_custom_vsync_monitor(builder, nullptr);
  EXPECT_EQ(finalized, 2);
  lynx_view_builder_set_custom_vsync_monitor(builder, create());
  lynx_view_builder_release(builder);
  EXPECT_EQ(finalized, 3);
}

TEST(LynxViewBuilder, CustomVSyncSurvivesBuilderReplacementAndSharing) {
  struct State {
    int requests = 0;
    int finalized = 0;
  } first, replacement;
  auto create = [](State* state) {
    auto* monitor = lynx_vsync_monitor_create_with_finalizer(
        state,
        [](auto*, void* data) { ++static_cast<State*>(data)->finalized; });
    lynx_vsync_monitor_bind_request_vsync_func(
        monitor, [](auto* monitor, auto callback, void* context) {
          auto* state =
              static_cast<State*>(lynx_vsync_monitor_get_user_data(monitor));
          ++state->requests;
          callback(context, 10, 20);
        });
    return monitor;
  };
  std::shared_ptr<lynx::embedder::LynxVSyncMonitor> source;
  std::shared_ptr<lynx::embedder::LynxVSyncMonitor> second_source;
  std::shared_ptr<lynx::embedder::LynxVSyncMonitor> replacement_source;
  {
    auto* builder = lynx_view_builder_create();
    lynx_view_builder_set_custom_vsync_monitor(builder, create(&first));
    // Retain the source as views do when copying the builder settings.
    source = builder->custom_vsync_monitor;
    second_source = builder->custom_vsync_monitor;
    lynx_view_builder_set_custom_vsync_monitor(builder, create(&replacement));
    replacement_source = builder->custom_vsync_monitor;
    lynx_view_builder_set_custom_vsync_monitor(builder, nullptr);
    lynx_view_builder_release(builder);
  }
  EXPECT_EQ(first.finalized, 0);
  EXPECT_EQ(replacement.finalized, 0);
  int signals = 0;
  {
    ASSERT_NE(source, nullptr);
    ASSERT_EQ(source, second_source);
    ASSERT_NE(source, replacement_source);
    ASSERT_NE(replacement_source, nullptr);
    for (const auto& monitor : {source, second_source, replacement_source}) {
      monitor->RequestVSync([&](int64_t start, int64_t target) {
        EXPECT_EQ(start, 10);
        EXPECT_EQ(target, 20);
        ++signals;
      });
    }
  }
  EXPECT_EQ(first.requests, 2);
  EXPECT_EQ(replacement.requests, 1);
  EXPECT_EQ(signals, 3);
  source.reset();
  EXPECT_EQ(first.finalized, 0);
  second_source.reset();
  EXPECT_EQ(first.finalized, 1);
  EXPECT_EQ(replacement.finalized, 0);
  replacement_source.reset();
  EXPECT_EQ(replacement.finalized, 1);
}
