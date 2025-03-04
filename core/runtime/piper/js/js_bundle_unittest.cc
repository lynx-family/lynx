// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/piper/js/js_bundle.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace testing {
TEST(JsBundle, JsContent) {
  auto src = std::make_shared<StringBuffer>("src");
  JsContent source(src, JsContent::Type::SOURCE);
  EXPECT_EQ(source.GetBuffer(), src);
  EXPECT_TRUE(source.IsSourceCode());
  EXPECT_FALSE(source.IsByteCode());
  EXPECT_FALSE(source.IsError());

  source = JsContent("src", JsContent::Type::BYTECODE);
  EXPECT_FALSE(source.IsSourceCode());
  EXPECT_TRUE(source.IsByteCode());
  EXPECT_FALSE(source.IsError());

  source = JsContent("error", JsContent::Type::ERROR);
  EXPECT_FALSE(source.IsSourceCode());
  EXPECT_FALSE(source.IsByteCode());
  EXPECT_TRUE(source.IsError());
}

TEST(JsBundle, JsBundle) {
  auto src1 = std::make_shared<StringBuffer>("src1");
  auto src2 = std::make_shared<StringBuffer>("src2");
  auto src3 = std::make_shared<StringBuffer>("src3");
  JsBundle bundle;
  bundle.AddJsContent("path1", {src1, JsContent::Type::SOURCE});
  bundle.AddJsContent("path2", {src2, JsContent::Type::BYTECODE});
  bundle.AddJsContent("path3", {src3, JsContent::Type::BYTECODE});
  EXPECT_EQ(bundle.GetAllJsFiles().size(), 3);
  EXPECT_EQ(bundle.GetJsContent("path1")->get().GetBuffer(), src1);
  EXPECT_EQ(bundle.GetJsContent("path2")->get().GetBuffer(), src2);
  EXPECT_FALSE(bundle.GetJsContent("path0").has_value());
}
}  // namespace testing
}  // namespace piper
}  // namespace lynx
