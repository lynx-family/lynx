// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/fetcher/lynx_resource_response_priv.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

TEST(LynxResourceResponse, Create) {
  lynx_resource_response_t* response =
      lynx_resource_response_create(nullptr, nullptr);
  EXPECT_TRUE(response != nullptr);
  EXPECT_EQ(response->code, -1);
  lynx_resource_response_set_code(response, 0);
  EXPECT_EQ(response->code, 0);
  EXPECT_TRUE(response->error_message.empty());
  lynx_resource_response_set_error_message(response, "error");
  EXPECT_STREQ(response->error_message.c_str(), "error");

  lynx_resource_response_release(response);
}

TEST(LynxResourceResponse, Callback) {
  int callbacks = 0;
  lynx_resource_response_t* response = lynx_resource_response_create_internal(
      [&](lynx_resource_response_t* response) {
        ++callbacks;
        EXPECT_EQ(response->code, -1);
        EXPECT_STREQ(response->error_message.c_str(), "error");
      });
  EXPECT_TRUE(response != nullptr);
  lynx_resource_response_set_code(response, -1);
  lynx_resource_response_set_error_message(response, "error");
  lynx_resource_response_callback(response);
  lynx_resource_response_callback(response);
  EXPECT_EQ(callbacks, 1);
  lynx_resource_response_release(response);
  EXPECT_EQ(callbacks, 1);
}

TEST(LynxResourceResponse, ReplacingAndClearingDataReleasesOnce) {
  for (bool null_content : {false, true}) {
    SCOPED_TRACE(null_content);
    uint8_t bytes[] = {1, 2};
    struct Owner {
      uint8_t* bytes;
      int releases = 0;
    } owner{bytes};
    auto release = [](uint8_t* bytes, size_t length, void* data) {
      auto* owner = static_cast<Owner*>(data);
      EXPECT_EQ(bytes, owner->bytes);
      EXPECT_EQ(length, 2u);
      ++owner->releases;
    };
    auto* response = lynx_resource_response_create(nullptr, nullptr);
    lynx_resource_response_set_data(response, bytes, 2, nullptr, nullptr);
    EXPECT_NE(response->data.content, bytes);
    bytes[0] = 3;
    ASSERT_NE(response->data.content, nullptr);
    ASSERT_EQ(response->data.length, 2u);
    EXPECT_EQ(response->data.content[0], 1);
    lynx_resource_response_set_data(response, bytes, 2, release, &owner);
    EXPECT_EQ(response->data.content, bytes);
    EXPECT_EQ(owner.releases, 0);
    lynx_resource_response_set_data(response, bytes, 2, nullptr, nullptr);
    EXPECT_EQ(owner.releases, 1);
    EXPECT_NE(response->data.content, bytes);
    EXPECT_EQ(response->data.dtor, nullptr);
    lynx_resource_response_set_data(response, bytes, 2, release, &owner);
    lynx_resource_response_set_data(response, null_content ? nullptr : bytes,
                                    null_content ? 2 : 0, nullptr, nullptr);
    EXPECT_EQ(owner.releases, 2);
    EXPECT_EQ(response->data.content, nullptr);
    EXPECT_EQ(response->data.length, 0u);
    lynx_resource_response_release(response);
    EXPECT_EQ(owner.releases, 2);
  }
}

TEST(LynxResourceResponse, SwapTransfersDataAndBundleOwnership) {
  int releases = 0;
  auto* source = lynx_resource_response_create(nullptr, nullptr);
  lynx_resource_response_set_code(source, 200);
  lynx_resource_response_set_error_message(source, "message");
  lynx_resource_response_set_data(
      source, new uint8_t[2]{1, 2}, 2,
      [](uint8_t* data, size_t length, void* opaque) {
        EXPECT_EQ(length, 2u);
        ++*static_cast<int*>(opaque);
        delete[] data;
      },
      &releases);
  lynx_template_bundle_t bundle;
  bundle.template_bundle = std::make_shared<lynx::tasm::LynxTemplateBundle>();
  std::weak_ptr<lynx::tasm::LynxTemplateBundle> weak = bundle.template_bundle;
  lynx_resource_response_set_template_bundle(source, &bundle);
  bundle.template_bundle.reset();
  auto* target = lynx_resource_response_create_swap(source);
  ASSERT_NE(target, nullptr);
  EXPECT_EQ(source->data.content, nullptr);
  EXPECT_EQ(source->data.length, 0u);
  EXPECT_EQ(source->template_bundle, nullptr);
  lynx_resource_response_release(source);
  EXPECT_EQ(releases, 0);
  EXPECT_EQ(target->code, 200);
  EXPECT_EQ(target->error_message, "message");
  ASSERT_NE(target->data.content, nullptr);
  ASSERT_EQ(target->data.length, 2u);
  EXPECT_EQ(target->data.content[1], 2);
  EXPECT_EQ(target->template_bundle, weak.lock());
  EXPECT_FALSE(weak.expired());
  lynx_resource_response_release(target);
  EXPECT_EQ(releases, 1);
  EXPECT_TRUE(weak.expired());
}

namespace {
void TestCallback(lynx_resource_response_t* response, void* user_data) {
  int* value = static_cast<int*>(user_data);
  EXPECT_EQ(*value, 123);
  EXPECT_EQ(response->code, 200);
}
}  // namespace

TEST(LynxResourceResponse, CapiCreateWithCallback) {
  int user_data = 123;
  lynx_resource_response_t* response =
      lynx_resource_response_create(TestCallback, &user_data);
  EXPECT_TRUE(response != nullptr);
  lynx_resource_response_set_code(response, 200);
  lynx_resource_response_release(response);
}
