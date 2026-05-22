// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/lynx_module/type_utils.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(LynxModuleTypes, TypesUtils) {
  LynxModuleValues module_values;
  auto& values = module_values.values;

  values.emplace_back(10);
  values.emplace_back(20);

  clay::Value::Array array;
  const char* strs[] = {"0", "1", "2", "3", "4"};
  for (int i = 0; i < 5; ++i) {
    array.emplace_back(strs[i]);
  }
  values.emplace_back(std::move(array));

  int int_value_to_cast_1;
  int int_value_to_cast_2;
  std::vector<std::string> str_to_cast;
  CastLynxModuleArgs(module_values, int_value_to_cast_1, int_value_to_cast_2,
                     str_to_cast);
  EXPECT_EQ(int_value_to_cast_1, 10);
  EXPECT_EQ(int_value_to_cast_2, 20);
  EXPECT_EQ(static_cast<int>(str_to_cast.size()), 5);
  EXPECT_STREQ(str_to_cast[2].c_str(), "2");
}

}  // namespace clay
