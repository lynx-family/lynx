// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "platform/embedder/lynx_service/lynx_service_base.h"
#include "platform/embedder/lynx_template_bundle_priv.h"
#include "platform/embedder/public/capi/lynx_security_service_capi.h"
#include "platform/embedder/public/capi/lynx_service_center_capi.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace {

constexpr char kMinimalTemplate[] = R"({
  "compilerOptions": {
    "enableFiberArch": true,
    "targetSdkVersion": "2.8",
    "bundleModuleMode": "ReturnByFunction",
    "useLepusNG": true
  },
  "sourceContent": {
    "dsl": "tt",
    "appType": "card",
    "config": {}
  },
  "css": {
    "cssMap": {"0": []},
    "cssSource": {"0": "index.css"}
  },
  "lepusCode": {"root": ""},
  "manifest": {}
})";

class LynxTemplateBundleTest : public ::testing::TestWithParam<bool> {
 protected:
  using Bundle = std::unique_ptr<lynx_template_bundle_t,
                                 decltype(&lynx_template_bundle_release)>;

  void SetUp() override {
    center_ = lynx_service_get_center_instance();
    previous_service_ = static_cast<lynx::embedder::LynxServiceBase*>(
        lynx_service_get_service(center_, kServiceTypeSecurity));
    if (previous_service_) {
      previous_service_->AddRef();
      lynx_service_unregister_service(center_, kServiceTypeSecurity,
                                      previous_service_);
    }

    // Generate a fixture using the encoder selected by this build.
    auto result = lynx::tasm::encode(kMinimalTemplate);
    ASSERT_EQ(result.status, 0) << result.error_msg;
    ASSERT_GT(result.buffer.size(), 4u);
    valid_content_ = std::move(result.buffer);
  }

  void TearDown() override {
    if (security_service_) {
      lynx_service_unregister_service(center_, kServiceTypeSecurity,
                                      security_service_);
      lynx_security_service_release(security_service_);
    }
    if (previous_service_) {
      lynx_service_register_service(center_, kServiceTypeSecurity,
                                    previous_service_);
      previous_service_->Release();
    }
  }

  Bundle Create(const std::vector<uint8_t>& content) {
    input_length_ = content.size();
    input_ = std::make_unique<uint8_t[]>(input_length_);
    std::copy(content.begin(), content.end(), input_.get());
    Bundle bundle(
        lynx_template_bundle_create(input_.get(), input_length_,
                                    GetParam() ? DestroyInput : nullptr, this),
        lynx_template_bundle_release);
    EXPECT_EQ(destructor_calls_, GetParam() ? 1 : 0);
    // Without a callback the caller still owns the input. Both paths free it
    // before inspecting the bundle, which must retain its own decoded data.
    input_.reset();
    return bundle;
  }

  static void DestroyInput(uint8_t* content, size_t length, void* opaque) {
    auto* test = static_cast<LynxTemplateBundleTest*>(opaque);
    EXPECT_EQ(content, test->input_.get());
    EXPECT_EQ(length, test->input_length_);
    ++test->destructor_calls_;
    test->input_.reset();
  }

  void BindVerifier(int result, const char* error) {
    verification_result_ = result;
    verification_error_ = error;
    security_service_ = lynx_security_service_create(this);
    lynx_security_service_bind(
        security_service_,
        [](lynx_security_service_t* service, uint8_t* content, size_t length,
           const char* url, lynx_tasm_type_e type, const char** error_msg) {
          auto* test = static_cast<LynxTemplateBundleTest*>(
              lynx_security_service_get_user_data(service));
          EXPECT_EQ(content, test->input_.get());
          EXPECT_EQ(length, test->input_length_);
          EXPECT_EQ(test->destructor_calls_, 0);
          EXPECT_STREQ(url, "");
          EXPECT_EQ(type, kTypeTemplate);
          ++test->verification_calls_;
          *error_msg = test->verification_error_;
          return test->verification_result_;
        });
    lynx_service_register_service(center_, kServiceTypeSecurity,
                                  security_service_);
  }

  void CheckAndRelease(Bundle bundle, bool valid,
                       const char* expected_error = nullptr) {
    ASSERT_NE(bundle, nullptr);
    EXPECT_EQ(lynx_template_bundle_is_valid(bundle.get()), valid ? 1 : 0);
    const char* error = lynx_template_bundle_get_error_message(bundle.get());
    if (valid) {
      EXPECT_STREQ(error, "");
    } else if (expected_error) {
      EXPECT_STREQ(error, expected_error);
    } else {
      ASSERT_NE(error, nullptr);
      EXPECT_NE(error[0], '\0');
    }
    bundle.reset();
    EXPECT_EQ(destructor_calls_, GetParam() ? 1 : 0);
  }

  std::vector<uint8_t> valid_content_;
  int verification_calls_ = 0;

 private:
  lynx_service_center_t* center_ = nullptr;
  lynx::embedder::LynxServiceBase* previous_service_ = nullptr;
  lynx_security_service_t* security_service_ = nullptr;
  std::unique_ptr<uint8_t[]> input_;
  size_t input_length_ = 0;
  int destructor_calls_ = 0;
  int verification_result_ = 0;
  const char* verification_error_ = nullptr;
};

TEST_P(LynxTemplateBundleTest, DecodesValidBundle) {
  CheckAndRelease(Create(valid_content_), true);
}

TEST_P(LynxTemplateBundleTest, DecodesAfterSecurityApproval) {
  BindVerifier(0, nullptr);
  CheckAndRelease(Create(valid_content_), true);
  EXPECT_EQ(verification_calls_, 1);
}

TEST_P(LynxTemplateBundleTest, RejectsMalformedData) {
  CheckAndRelease(Create({0x00, 0x00, 0x00, 0x00}), false);
}

TEST_P(LynxTemplateBundleTest, RejectsTruncatedBundle) {
  // Truncate a valid bundle without assuming a particular binary layout.
  valid_content_.resize(valid_content_.size() / 2);
  CheckAndRelease(Create(valid_content_), false);
}

TEST_P(LynxTemplateBundleTest, ReportsSecurityError) {
  BindVerifier(1, "rejected by test verifier");
  CheckAndRelease(Create(valid_content_), false, "rejected by test verifier");
  EXPECT_EQ(verification_calls_, 1);
}

TEST_P(LynxTemplateBundleTest, UsesDefaultSecurityError) {
  BindVerifier(1, nullptr);
  CheckAndRelease(Create(valid_content_), false,
                  "template verification failed.");
  EXPECT_EQ(verification_calls_, 1);
}

INSTANTIATE_TEST_SUITE_P(InputOwnership, LynxTemplateBundleTest,
                         ::testing::Bool(),
                         [](const ::testing::TestParamInfo<bool>& info) {
                           return info.param ? "WithDestructor"
                                             : "WithoutDestructor";
                         });

}  // namespace
