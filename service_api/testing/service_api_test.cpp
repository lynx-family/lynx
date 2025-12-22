// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <dlfcn.h>
#include <service_api/service_api.h>
#include <third_party/googletest/googletest/include/gtest/gtest.h>

#include <iostream>

#ifndef NO_IMPL
#define NO_IMPL 0
#endif

#ifndef STATIC_LINK
#define STATIC_LINK 0
#endif

using lynx::service::a_service::AService;
using lynx::service::b_service::BService;

class CommonTest : public testing::Test {
 public:
  void SetUp() override {}
};

class WithImplTest : public testing::Test {
 public:
  void SetUp() override {
    if constexpr (NO_IMPL) {
      GTEST_SKIP() << "skip tests which need service impl loaded";
    }
  }
};

class WithoutImplTest : public testing::Test {
 public:
  void SetUp() override {
    if constexpr (!NO_IMPL) {
      GTEST_SKIP();
    }
  }
};

TEST_F(CommonTest, service_name) {
  auto name_a = lynx::service::get_service_name<AService>();
  auto name_b = lynx::service::get_service_name<BService>();
  EXPECT_EQ(name_a, "lynx::service::a_service::AService");
  EXPECT_EQ(name_b, "lynx::service::b_service::BService");
}

TEST_F(WithImplTest, get_service) {
  auto aService = lynx::service::get_service<AService>();
  EXPECT_NE(aService, nullptr);
  EXPECT_EQ(aService->get_descriptor(), "func_a");
  auto bService = lynx::service::get_service<BService>();
  EXPECT_NE(bService, nullptr);
  EXPECT_EQ(bService->get_descriptor(), "with creator");
}

TEST_F(WithoutImplTest, get_service) {
  auto aService = lynx::service::get_service<AService>();
  EXPECT_EQ(aService, nullptr);
  auto bService = lynx::service::get_service<BService>();
  EXPECT_EQ(bService, nullptr);
}
