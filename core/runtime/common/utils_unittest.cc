// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/utils.h"

#include "core/renderer/tasm/config.h"
#include "core/runtime/common/jsi_object_wrapper.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {
class UtilTests : public JSITestBase {};

TEST_P(UtilTests, ValueFromLepusTest) {
  //   lepus::Value empty{};
  //   EXPECT_TRUE(valueFromLepus(rt, empty)->isNull());

  //   lepus::Value undef;
  //   undef.SetUndefined();
  //   EXPECT_TRUE(valueFromLepus(rt, undef)->isUndefined());

  //   // Use an array of sorted keys that acts as an approved list for
  //   selecting the
  //   // value and properties that will be stringified
  //   Function stable_stringify = function(R"(
  // function stableJSONStringify(value) {
  //   const allKeys = new Set();
  //   JSON.stringify(value, (key, value) => (allKeys.add(key), value));
  //   return JSON.stringify(value, Array.from(allKeys).sort());
  // }
  // )");
  //   lepus::Value value{lepus::Dictionary::Create()};

  //   auto ret = valueFromLepus(rt, value);

  //   EXPECT_TRUE(ret.has_value());
  //   EXPECT_TRUE(ret.value().isObject());
  //   EXPECT_EQ(stable_stringify.call(rt, ret.value())->getString(rt).utf8(rt),
  //             "{}");

  //   value = lepus::Value(lepus::Dictionary::Create());
  //   value.SetProperty("str", lepus::Value("bar"));
  //   value.SetProperty("num", lepus::Value(0xff));
  //   value.SetProperty("bool", lepus::Value(false));
  //   value.SetProperty("null", lepus::Value());

  //   ret = valueFromLepus(rt, value);

  //   EXPECT_TRUE(ret.has_value());
  //   EXPECT_TRUE(ret.value().isObject());
  //   EXPECT_EQ(stable_stringify.call(rt, ret.value())->getString(rt).utf8(rt),
  //             R"({"bool":false,"null":null,"num":255,"str":"bar"})");

  //   value = lepus::Value(lepus::Dictionary::Create());
  //   constexpr const size_t depth = 255;
  //   for (size_t i = 0; i < depth; ++i) {
  //     lepus::Value foo{lepus::Dictionary::Create()};
  //     foo.SetProperty("foo", value);
  //     value = foo;
  //   }

  //   ret = valueFromLepus(rt, value);
  //   EXPECT_TRUE(ret.has_value());
  //   EXPECT_TRUE(ret.value().isObject());
  //   EXPECT_EQ(function(R"(
  // function depth(value) {
  //   if (Object.keys(value).length === 0) {
  //     return 0;
  //   }
  //   return depth(value.foo) + 1;
  // }
  // )")
  //                 .call(rt, ret.value())
  //                 ->getNumber(),
  //             255.0);

  //   // circular value should cause crash
  //   lepus::Value circular{lepus::CArray::Create()};
  //   circular.SetProperty(0, circular);
  //   EXPECT_DEATH(valueFromLepus(rt, circular), "");
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, UtilTests, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<UtilTests::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
      }
    });

TEST_P(UtilTests, ConvertPiperValueToStringVector) {
  std::vector<std::string> output;

  // input = []
  piper::Value input = *piper::Array::createWithLength(rt, 0);
  EXPECT_TRUE(ConvertPiperValueToStringVector(rt, input, output));
  EXPECT_TRUE(output.empty());

  // input = ["#a", "#b"]
  auto array = piper::Array::createWithLength(rt, 2);
  array->setValueAtIndex(rt, 0, piper::String::createFromUtf8(rt, "#a"));
  array->setValueAtIndex(rt, 1, piper::String::createFromUtf8(rt, "#b"));
  input = *array;
  EXPECT_TRUE(ConvertPiperValueToStringVector(rt, input, output));
  bool res = output == std::vector<std::string>{"#a", "#b"};
  EXPECT_TRUE(res);

  // input = "#a"
  input = piper::String::createFromUtf8(rt, "#a");
  EXPECT_FALSE(ConvertPiperValueToStringVector(rt, input, output));

  // input = ["#a", 0]
  array = piper::Array::createWithLength(rt, 2);
  array->setValueAtIndex(rt, 0, piper::String::createFromUtf8(rt, "#a"));
  array->setValueAtIndex(rt, 2, piper::Value(0));
  input = *array;
  EXPECT_FALSE(ConvertPiperValueToStringVector(rt, input, output));
}

TEST_P(UtilTests, ParseNullJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();

  piper::Value null_data = piper::Value::null();
  auto lepus_value_opt =
      ParseJSValue(rt, null_data, jsi_object_wrapper_manager.get(),
                   jsi_object_group_id, target_sdk_version, pre_object_vector);
  EXPECT_TRUE(lepus_value_opt.has_value());
  EXPECT_TRUE(lepus_value_opt->IsNil());
}

TEST_P(UtilTests, ParseUndefinedJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();

  piper::Value undefined_data = piper::Value::undefined();
  auto lepus_value_opt =
      ParseJSValue(rt, undefined_data, jsi_object_wrapper_manager.get(),
                   jsi_object_group_id, target_sdk_version, pre_object_vector);
  EXPECT_TRUE(lepus_value_opt.has_value());
  EXPECT_TRUE(lepus_value_opt->IsUndefined());
}

TEST_P(UtilTests, ParseBoolJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();

  {
    piper::Value true_data(true);
    auto lepus_value_opt = ParseJSValue(
        rt, true_data, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsTrue());
  }

  {
    piper::Value false_data(false);
    auto lepus_value_opt = ParseJSValue(
        rt, false_data, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsFalse());
  }
}

TEST_P(UtilTests, ParseNumberJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();

  {
    int foo = 10;
    piper::Value int_data(foo);
    auto lepus_value_opt = ParseJSValue(
        rt, int_data, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsNumber());
    EXPECT_EQ(lepus_value_opt->Number(), foo);
  }

  {
    double foo = 10.0;
    piper::Value double_data(foo);
    auto lepus_value_opt = ParseJSValue(
        rt, double_data, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsDouble());
    EXPECT_EQ(lepus_value_opt->Double(), foo);
  }
}

TEST_P(UtilTests, ParseStringJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  {
    std::string foo_str = "foo";
    auto foo = piper::String::createFromAscii(rt, foo_str);
    piper::Value string_data(foo);
    auto lepus_value_opt = ParseJSValue(
        rt, string_data, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsString());
    EXPECT_EQ(lepus_value_opt->ToString(), foo_str);
  }
}

TEST_P(UtilTests, ParseSymbolJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  {
    Function make_symbol = function(R"(
function makeSymbol() {
  return Symbol('foo');
}
)");
    auto symbol_opt = make_symbol.call(rt);
    EXPECT_TRUE(symbol_opt.has_value());
    EXPECT_TRUE(symbol_opt->isSymbol());
    auto lepus_value_opt = ParseJSValue(
        rt, *symbol_opt, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    // TODO(liyanbo): support symbol type.
    EXPECT_FALSE(lepus_value_opt.has_value());
  }
}

TEST_P(UtilTests, ParseArrayJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  {
    uint8_t empty_array_json[] = "[]";
    auto empty_array_data_opt = piper::Value::createFromJsonUtf8(
        rt, empty_array_json, sizeof(empty_array_json) - 1);
    auto lepus_value_opt = ParseJSValue(
        rt, *empty_array_data_opt, jsi_object_wrapper_manager.get(),
        jsi_object_group_id, target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsArray());
    EXPECT_EQ(lepus_value_opt->Array()->size(), 0);
  }

  {
    const int element_count = 5;
    uint8_t array_json[] = "[\"foo\", true, 10, null, {}]";
    auto array_data_opt = piper::Value::createFromJsonUtf8(
        rt, array_json, sizeof(array_json) - 1);
    auto lepus_value_opt = ParseJSValue(
        rt, *array_data_opt, jsi_object_wrapper_manager.get(),
        jsi_object_group_id, target_sdk_version, pre_object_vector);
    std::stringstream ss;
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsArray());
    EXPECT_EQ(lepus_value_opt->Array()->size(), element_count);
    auto array = lepus_value_opt->Array();
    {
      lepus::Value element = array->get(0);
      EXPECT_TRUE(element.IsString());
      EXPECT_TRUE(element.String().IsEqual("foo"));
    }
    {
      lepus::Value element = array->get(1);
      EXPECT_TRUE(element.IsBool());
      EXPECT_EQ(element.Bool(), true);
    }
    {
      lepus::Value element = array->get(2);
      EXPECT_TRUE(element.IsNumber());
      EXPECT_EQ(element.Number(), 10);
    }
    {
      lepus::Value element = array->get(3);
      EXPECT_TRUE(element.IsNil());
    }
    {
      lepus::Value element = array->get(4);
      EXPECT_TRUE(element.IsObject());
    }
  }
}

TEST_P(UtilTests, ParseBigIntJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  {
    auto big_int_opt = BigInt::createWithString(rt, "1234567890");
    EXPECT_TRUE(big_int_opt.has_value());
    auto lepus_value_opt = ParseJSValue(
        rt, *big_int_opt, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsInt64());
    EXPECT_EQ(lepus_value_opt->Int64(), 1234567890);
  }
  // TODO(liyanbo): fix this Precision loss problem
  //  {
  //      auto big_int_opt = BigInt::createWithString(rt,
  //      "9223372036854775808"); EXPECT_TRUE(big_int_opt.has_value()); auto
  //      lepus_value_opt =
  //          ParseJSValue(rt, *big_int_opt, jsi_object_wrapper_manager.get(),
  //                  jsi_object_group_id, target_sdk_version,
  //                  *pre_object_vector);
  //      EXPECT_TRUE(lepus_value_opt.has_value());
  //      EXPECT_TRUE(lepus_value_opt->IsInt64());
  //      EXPECT_EQ(lepus_value_opt->Int64(), 9223372036854775808);
  //  }
}

TEST_P(UtilTests, ParseFunctionJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
  Function make_func = function(R"(
function make() {
  return function() { return 1; };
}
)");
  auto func_opt = make_func.call(rt);
  EXPECT_TRUE(func_opt.has_value());
  {
    auto lepus_value_opt = ParseJSValue(
        rt, *func_opt, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsJSObject());
  }
  {
    auto lepus_value_opt =
        ParseJSValue(rt, *func_opt, nullptr, jsi_object_group_id,
                     target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsNil());
  }
}

TEST_P(UtilTests, ParseObjectJSValueTest) {
  JSValueCircularArray pre_object_vector;
  const std::string jsi_object_group_id = "1";
  auto jsi_object_wrapper_manager = std::make_shared<JSIObjectWrapperManager>();
  Function make_func = function(R"(
function make() {
  return {"foo":1, [Symbol("bar")]: 2, "bar": undefined};
}
)");
  const int object_key_count = 3;
  auto obj_opt = make_func.call(rt);
  EXPECT_TRUE(obj_opt.has_value());
  EXPECT_TRUE(obj_opt->isObject());
  {
    const std::string target_sdk_version = LYNX_VERSION_1_6.ToString();
    auto lepus_value_opt = ParseJSValue(
        rt, *obj_opt, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsObject());
    auto table = lepus_value_opt->Table();
    // TODO(liyanbo): support symbol. (-2) is symbol key and undefined value.
    EXPECT_EQ(table->size(), object_key_count - 2);
    auto foo_value = table->GetValue("foo");
    EXPECT_TRUE(foo_value.IsNumber());
    EXPECT_EQ(static_cast<int>(foo_value.Number()), 1);
  }

  {
    const std::string target_sdk_version = LYNX_VERSION_2_3.ToString();
    auto lepus_value_opt = ParseJSValue(
        rt, *obj_opt, jsi_object_wrapper_manager.get(), jsi_object_group_id,
        target_sdk_version, pre_object_vector);
    EXPECT_TRUE(lepus_value_opt.has_value());
    EXPECT_TRUE(lepus_value_opt->IsObject());
    auto table = lepus_value_opt->Table();
    // TODO(liyanbo): support symbol. (-1) is symbol key.
    EXPECT_EQ(table->size(), object_key_count - 1);
    auto foo_value = table->GetValue("foo");
    EXPECT_TRUE(foo_value.IsNumber());
    EXPECT_EQ(static_cast<int>(foo_value.Number()), 1);

    auto bar_value = table->GetValue("bar");
    EXPECT_TRUE(bar_value.IsUndefined());
  }
}

}  // namespace test
}  // namespace piper
}  // namespace lynx
