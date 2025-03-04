// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/shared_css_fragment.h"

#include "core/base/json/json_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace encoder {
namespace test {

TEST(SharedCSSFragment, ConstructSharedCSSFragment) {
  encoder::SharedCSSFragment fragment(1, {1, 2, 3}, {}, {}, {});

  EXPECT_EQ(fragment.selector_tuple().size(), 0);
  EXPECT_EQ(fragment.GetKeyframesRuleMapForEncode().size(), 0);
  EXPECT_EQ(fragment.GetFontFaceTokenMapForEncode().size(), 0);

  std::vector<LynxCSSSelectorTuple> selector_tuple;
  selector_tuple.emplace_back(LynxCSSSelectorTuple());
  fragment.SetSelectorTuple(std::move(selector_tuple));
  EXPECT_EQ(fragment.selector_tuple().size(), 1);
}

}  // namespace test
}  // namespace encoder
}  // namespace lynx
