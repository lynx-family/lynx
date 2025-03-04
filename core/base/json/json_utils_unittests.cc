// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/json/json_util.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace base {

TEST(JSON, all) {
  {
    std::string json_str = "Is an illegal json string";
    rapidjson::Document value = strToJson(json_str.c_str());
    EXPECT_TRUE(value.IsNull());
    EXPECT_TRUE(IsNull(value));
    EXPECT_FALSE(IsArray(value));
    EXPECT_FALSE(IsNumber(value));
    EXPECT_STREQ(TypeName(value), "null");
    std::string raw_str = ToJson(value);
    EXPECT_STREQ(raw_str.c_str(), "null");
  }

  {
    std::string json_str = "[1,2]";
    rapidjson::Document value = strToJson(json_str.c_str());
    EXPECT_TRUE(value.IsArray());
    EXPECT_STREQ(TypeName(value), "array");
    int target_result[] = {1, 2};
    for (unsigned int index = 0; index < value.GetArray().Size(); index++) {
      EXPECT_TRUE(IsNumber((value.GetArray())[index]));
      EXPECT_STREQ(TypeName((value.GetArray())[index]), "number");
      EXPECT_TRUE((value.GetArray())[index].IsNumber());
      EXPECT_EQ((value.GetArray())[index].GetInt(), target_result[index]);
    }
    EXPECT_FALSE(IsNull(value));
    EXPECT_TRUE(IsArray(value));
    EXPECT_FALSE(IsNumber(value));
    std::string raw_str = ToJson(value);
    EXPECT_STREQ(raw_str.c_str(), json_str.c_str());
  }

  {
    std::string json_str = "{\"name\":\"lynx\",\"age\":10000,\"enable\":true}";
    rapidjson::Document value = strToJson(json_str.c_str());
    EXPECT_FALSE(IsNull(value));
    EXPECT_FALSE(IsArray(value));
    EXPECT_FALSE(IsNumber(value));
    EXPECT_TRUE(value.IsObject());
    EXPECT_STREQ(TypeName(value), "object");
    EXPECT_TRUE(value.GetObject().MemberCount() == 3);
    EXPECT_TRUE(value.GetObject().HasMember("name"));
    EXPECT_TRUE(value.GetObject()["name"].IsString());
    EXPECT_STREQ(TypeName(value.GetObject()["name"]), "string");
    EXPECT_STREQ(value.GetObject()["name"].GetString(), "lynx");
    EXPECT_TRUE(value.GetObject().HasMember("age"));
    EXPECT_TRUE(IsNumber(value.GetObject()["age"]));
    EXPECT_EQ(value.GetObject()["age"].GetInt(), 10000);

    EXPECT_TRUE(value.GetObject().HasMember("enable"));
    EXPECT_TRUE(value.GetObject()["enable"].IsBool());
    EXPECT_STREQ(TypeName(value.GetObject()["enable"]), "bool");
    EXPECT_TRUE(value.GetObject()["enable"].GetBool());

    std::string raw_str = ToJson(value);
    EXPECT_STREQ(raw_str.c_str(), json_str.c_str());
  }
}

}  // namespace base
}  // namespace lynx
