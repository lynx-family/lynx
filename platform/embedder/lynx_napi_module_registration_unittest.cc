// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/public/lynx_env.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxNativeModuleRegistration, RegistersGlobalAndInstanceModules) {
  auto creator = [](auto, auto exports, const char*, void*) { return exports; };
  lynx_env_register_native_module("test", creator, nullptr);
  lynx::pub::LynxEnv::GetInstance().RegisterNativeModule("test", creator,
                                                         nullptr);

  auto* builder = lynx_view_builder_create();
  lynx_view_builder_register_native_module(builder, "test", creator, nullptr);
  EXPECT_EQ(builder->native_modules.size(), 1u);
  lynx_view_builder_release(builder);
}
