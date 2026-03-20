// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/tasm/config.h"
#include "platform/embedder/public/lynx_env.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

#if ENABLE_INSPECTOR
TEST(LynxEnv, Devtool) {
  lynx_env_enable_devtool(0);
  EXPECT_EQ(lynx_env_is_devtool_enabled(), 0);
  lynx_env_enable_devtool(1);
  EXPECT_EQ(lynx_env_is_devtool_enabled(), 1);
}

TEST(LynxEnv, DevtoolCpp) {
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetDevtoolEnabled(false);
  EXPECT_EQ(lynx_env.IsDevtoolEnabled(), false);
  lynx_env.SetDevtoolEnabled(true);
  EXPECT_EQ(lynx_env.IsDevtoolEnabled(), true);
}

TEST(LynxEnv, Logbox) {
  lynx_env_enable_logbox(0);
  EXPECT_EQ(lynx_env_is_logbox_enabled(), 0);
  lynx_env_enable_logbox(1);
  EXPECT_EQ(lynx_env_is_logbox_enabled(), 1);
}

TEST(LynxEnv, LogboxCpp) {
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetLogboxEnabled(false);
  EXPECT_EQ(lynx_env.IsLogboxEnabled(), false);
  lynx_env.SetLogboxEnabled(true);
  EXPECT_EQ(lynx_env.IsLogboxEnabled(), true);
}
#endif

TEST(LynxEnv, Version) {
  EXPECT_STREQ(lynx_env_get_sdk_version(),
               lynx::tasm::Config::GetCurrentLynxVersion().c_str());
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  EXPECT_STREQ(lynx_env.GetVersion(),
               lynx::tasm::Config::GetCurrentLynxVersion().c_str());
}

TEST(LynxEnv, RegisterNativeModule) {
  lynx_env_register_native_module(
      "test",
      [](napi_env, napi_value exports, const char* module_name, void* opaque) {
        return exports;
      },
      nullptr);

  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.RegisterNativeModule(
      "test",
      [](napi_env, napi_value exports, const char* module_name, void* opaque) {
        return exports;
      },
      nullptr);
}
