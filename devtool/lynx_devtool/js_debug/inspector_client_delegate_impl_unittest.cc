// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "devtool/lynx_devtool/js_debug/inspector_client_delegate_impl.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class InspectorClientDelegateImplTest : public ::testing::Test {
 public:
  InspectorClientDelegateImplTest() {}
  ~InspectorClientDelegateImplTest() override {}
  void SetUp() override {
    v8_delegate_ = InspectorClientDelegateProvider::GetInstance()->GetDelegate(
        kKeyEngineV8);
    qjs_delegate_ = InspectorClientDelegateProvider::GetInstance()->GetDelegate(
        kKeyEngineQuickjs);
    lepus_delegate_ =
        InspectorClientDelegateProvider::GetInstance()->GetDelegate(
            kKeyEngineLepus);
  }

 private:
  std::shared_ptr<InspectorClientDelegateImpl> v8_delegate_;
  std::shared_ptr<InspectorClientDelegateImpl> qjs_delegate_;
  std::shared_ptr<InspectorClientDelegateImpl> lepus_delegate_;
};

TEST_F(InspectorClientDelegateImplTest, GetDelegate) {
  std::shared_ptr<InspectorClientDelegateImpl> new_v8_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(kKeyEngineV8);
  std::shared_ptr<InspectorClientDelegateImpl> new_qjs_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(
          kKeyEngineQuickjs);
  std::shared_ptr<InspectorClientDelegateImpl> new_lepus_delegate =
      InspectorClientDelegateProvider::GetInstance()->GetDelegate(
          kKeyEngineLepus);

  EXPECT_EQ(v8_delegate_, new_v8_delegate);
  EXPECT_EQ(qjs_delegate_, new_qjs_delegate);
  EXPECT_NE(lepus_delegate_, new_lepus_delegate);
}
}  // namespace testing
}  // namespace devtool
}  // namespace lynx
