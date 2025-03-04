// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/value_utils.h"

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_error_helper.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/path_parser.h"
#include "core/runtime/vm/lepus/qjs_callback.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/ref_type.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

constexpr char foo[] = "foo";
constexpr char bar[] = "bar";
constexpr char bar_other[] = "bar_other";
extern LEPUSValue js_map_get_size(LEPUSContext* ctx, LEPUSValueConst this_val,
                                  int magic);

namespace lynx {
namespace base {

class LepusValueMethods : public ::testing::Test {
 public:
  class TestRefCountedClass : public lepus::RefCounted {
   public:
    static auto Create() { return fml::AdoptRef(new TestRefCountedClass()); }
    TestRefCountedClass() = default;
    void ReleaseSelf() const override { delete this; };
    ~TestRefCountedClass() override = default;

    lepus::RefType GetRefType() const override {
      return lepus::RefType::kOtherType;
    }
  };

 protected:
  LepusValueMethods() = default;
  ~LepusValueMethods() override = default;

  void SetUp() override {
    ctx_.Initialize();
    v1_ = lepus::Value(lepus::Dictionary::Create());
    lepus::Value v("hello");
    // {"prop1": "hello"}
    v1_.SetProperty(base::String("prop1"), v);
    ctx_.RegisterGlobalFunction(
        "assert",
        [](LEPUSContext* ctx, LEPUSValue this_obj, int32_t argc,
           LEPUSValue* argv) {
          assert(LEPUS_ToBool(ctx, argv[0]));
          return LEPUS_UNDEFINED;
        },
        1);
  }

  void TearDown() override {}

  lepus::QuickContext ctx_;
  lepus::Value v1_;
};

TEST(LepusShadowEqualTest, SameStringTable) {
  auto lepus_map = lepus::Dictionary::Create();

  lepus::Value v(bar);
  lepus_map.get()->SetValue(base::String(foo), v);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(foo), v);

  ASSERT_TRUE(!tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                             lepus::Value(target_map)));
}

TEST(LepusShadowEqualTest, SameIntTable) {
  auto lepus_map = lepus::Dictionary::Create();
  lepus_map.get()->SetValue(base::String(foo), lepus::Value(1));

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(foo), lepus::Value(1));

  ASSERT_TRUE(!tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                             lepus::Value(target_map)));
}

TEST(LepusShadowEqualTest, DiffSameStringTable) {
  auto lepus_map = lepus::Dictionary::Create();
  lepus::Value bar_value(bar);
  lepus_map.get()->SetValue(base::String(foo), bar_value);

  auto target_map = lepus::Dictionary::Create();
  lepus::Value bar_other_value(bar_other);
  target_map.get()->SetValue(base::String(foo), bar_other_value);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                            lepus::Value(target_map)));
}

TEST(LepusShadowEqualTest, HasNewKey) {
  auto lepus_map = lepus::Dictionary::Create();
  lepus::Value bar_value(bar);
  lepus_map.get()->SetValue(base::String(foo), bar_value);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(bar_other), bar_value);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                            lepus::Value(target_map)));
}

TEST(LepusShadowEqualTest, HasSameTableValueReturnTrue) {
  auto lepus_map = lepus::Dictionary::Create();
  lepus::Value dic = lepus::Value(lepus::Dictionary::Create());
  lepus_map.get()->SetValue(base::String(foo), dic);

  auto target_map = lepus::Dictionary::Create();
  target_map.get()->SetValue(base::String(foo), dic);

  ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                            lepus::Value(target_map)));
}

TEST(LepusValueTest, MKVAL) {
  LEPUSValue catch_offset = LEPUS_MKVAL(LEPUS_TAG_CATCH_OFFSET, 3);
  ASSERT_TRUE(LEPUS_VALUE_GET_CATCH_OFFSET(catch_offset) == 3);
  ASSERT_TRUE(LEPUS_VALUE_GET_TAG(catch_offset) == LEPUS_TAG_CATCH_OFFSET);
}

TEST_F(LepusValueMethods, ValuePrint) {
  std::ostringstream ss_;
  ss_ << v1_;
  ASSERT_TRUE(ss_.str() == "{prop1:hello}");
}

TEST_F(LepusValueMethods, IsEqual) {
  lepus::Value v2(ctx_.context(), v1_.ToJSValue(ctx_.context(), true));
  ASSERT_TRUE(v1_.IsEqual(v2));

  LEPUSValue v1_js_val_ = v1_.ToJSValue(ctx_.context());

  lepus::Value v3 =
      lepus::LEPUSValueHelper::ToLepusValue(ctx_.context(), v1_js_val_);

  ASSERT_TRUE(v1_.IsEqual(v3));
  if (!LEPUS_IsGCMode(ctx_.context()))
    LEPUS_FreeValue(ctx_.context(), v1_js_val_);
}

TEST_F(LepusValueMethods, Clone) {
  lepus::Value v2 = lepus::Value::Clone(v1_);
  ASSERT_TRUE(v2.IsEqual(v1_));
}

TEST_F(LepusValueMethods, SetGetProperty1) {
  lepus::Value v2 = lepus::Value::Clone(v1_);
  v2.SetProperty(base::String("prop1"), lepus::Value("world"));
  ASSERT_TRUE(
      v2.GetProperty(base::String("prop1")).IsEqual(lepus::Value("world")));
}
TEST_F(LepusValueMethods, SetGetProperty2RC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSValue v1 = v1_.ToJSValue(ctx_.context(), true);

  lepus::LEPUSValueHelper::SetProperty(
      ctx_.context(), v1, "prop1",
      lepus::LEPUSValueHelper::NewString(ctx_.context(), "world"));

  lepus::Value v2 = lepus::Value::Clone(v1_);
  v2.SetProperty(base::String("prop1"), lepus::Value("world"));

  lepus::Value v3 = lepus::Value(ctx_.context(), std::move(v1));
  ASSERT_TRUE(v2.IsEqual(v3));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue v2_ref = v2.ToJSValue(ctx);
  lepus::Value array_prop(lepus::CArray::Create());
  array_prop.Array()->push_back(lepus::Value("byte"));
  array_prop.Array()->push_back(lepus::Value("dance"));
  v2.SetProperty("prop2", array_prop);

  LEPUSValue prop1 = LEPUS_GetPropertyStr(ctx, v2_ref, "prop1");
  ASSERT_TRUE(lepus::Value(ctx, prop1) == lepus::Value("world"));
  LEPUSValue prop2 = LEPUS_GetPropertyStr(ctx, v2_ref, "prop2");
  ASSERT_TRUE(((LEPUSLepusRef*)(LEPUS_VALUE_GET_PTR(prop2)))->p ==
              array_prop.Array().get());

  LEPUSValue prop2_length = LEPUS_GetPropertyStr(ctx, prop2, "length");

  ASSERT_TRUE(LEPUS_VALUE_GET_INT(prop2_length) == 2);

  LEPUS_SetPropertyUint32(ctx, prop2, 2,
                          lepus::Value("x-engines").ToJSValue(ctx));
  ASSERT_TRUE(array_prop.Array()->size() == 3);

  LEPUS_FreeValue(ctx, v2_ref);
  LEPUS_FreeValue(ctx, prop1);
  LEPUS_FreeValue(ctx, prop2);
  LEPUS_FreeValue(ctx, prop2_length);
}
TEST_F(LepusValueMethods, SetGetProperty2GC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSValue v1 = v1_.ToJSValue(ctx_.context(), true);
  HandleScope func_scope(ctx_.context(), &v1, HANDLE_TYPE_LEPUS_VALUE);

  lepus::LEPUSValueHelper::SetProperty(
      ctx_.context(), v1, "prop1",
      lepus::LEPUSValueHelper::NewString(ctx_.context(), "world"));

  lepus::Value v2 = lepus::Value::Clone(v1_);
  v2.SetProperty(base::String("prop1"), lepus::Value("world"));

  lepus::Value v3 = lepus::Value(ctx_.context(), std::move(v1));
  ASSERT_TRUE(v2.IsEqual(v3));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue v2_ref = v2.ToJSValue(ctx);
  func_scope.PushHandle(&v2_ref, HANDLE_TYPE_LEPUS_VALUE);
  lepus::Value array_prop(lepus::CArray::Create());
  array_prop.Array()->push_back(lepus::Value("byte"));
  array_prop.Array()->push_back(lepus::Value("dance"));
  v2.SetProperty("prop2", array_prop);

  LEPUSValue prop1 = LEPUS_GetPropertyStr(ctx, v2_ref, "prop1");
  ASSERT_TRUE(lepus::Value(ctx, prop1) == lepus::Value("world"));
  LEPUSValue prop2 = LEPUS_GetPropertyStr(ctx, v2_ref, "prop2");
  ASSERT_TRUE(((LEPUSLepusRef*)(LEPUS_VALUE_GET_PTR(prop2)))->p ==
              array_prop.Array().get());

  LEPUSValue prop2_length = LEPUS_GetPropertyStr(ctx, prop2, "length");

  ASSERT_TRUE(LEPUS_VALUE_GET_INT(prop2_length) == 2);

  LEPUSValue x_engine = lepus::Value("x-engines").ToJSValue(ctx);
  func_scope.PushHandle(&x_engine, HANDLE_TYPE_LEPUS_VALUE);
  LEPUS_SetPropertyUint32(ctx, prop2, 2, x_engine);
  ASSERT_TRUE(array_prop.Array()->size() == 3);
}

TEST_F(LepusValueMethods, String) {
  LEPUSContext* ctx = ctx_.context();

  lepus::Value v2(ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  base::String str = v2.String();
  ASSERT_TRUE(str.IsEqual(base::String("hello world")));
}

TEST_F(LepusValueMethods, Iterator) {
  std::string str;
  std::string* pstr = &str;
  tasm::ForEachLepusValue(
      v1_, [pstr](const lepus::Value& key, const lepus::Value& val) {
        *pstr = *pstr + key.StdString() + val.StdString();
      });
  ASSERT_TRUE(str == "prop1hello");
}

TEST_F(LepusValueMethods, MarkConstRet) {
  v1_.MarkConst();
  lepus::Value v = v1_.GetProperty("prop1");
  ASSERT_TRUE(!v1_.SetProperty(base::String("prop1"), lepus::Value("word")));
}

TEST_F(LepusValueMethods, MarkConstSet) {
  v1_.MarkConst();
  lepus::Value v = v1_.GetProperty("prop1");
  v1_.SetProperty(base::String("prop1"), lepus::Value("word"));
  lepus::Value str = v1_.GetProperty(base::String("prop1"));
  ASSERT_TRUE(str == lepus::Value("hello"));
}

TEST_F(LepusValueMethods, MarkConstArray) {
  lepus::Value array = lepus::Value(lepus::CArray::Create());
  array.Array()->set(0, v1_);
  array.MarkConst();
  ASSERT_TRUE(!array.Array()->set(0, lepus::Value("hello")));
}

TEST_F(LepusValueMethods, MarkConstArray2) {
  lepus::Value array = lepus::Value(lepus::CArray::Create());
  array.Array()->set(0, v1_);
  array.MarkConst();
  array.Array()->set(0, lepus::Value("hello"));
  ASSERT_TRUE(array.Array()->get(0) == v1_);
}

TEST_F(LepusValueMethods, SetSelf) {
  lepus::Value table = lepus::Value(lepus::Dictionary::Create());
  {
    lepus::Value val = lepus::Value(lepus::Dictionary::Create());
    val.Table()->SetValue("key", lepus::Value("key"));
    lepus::Value val2 = lepus::Value(val.Table());
    table.Table()->SetValue("Hello", val);
    table.Table()->SetValue("Hello", val2);
  }
  ASSERT_TRUE(table.Table()->GetValue("Hello").Table()->GetValue("key") ==
              lepus::Value("key"));
}

TEST_F(LepusValueMethods, TestLepusJSValue) {
  lepus::Value array = lepus::Value(lepus::CArray::Create());
  // table.array = array
  // array[0] = "0";
  // array[1] = "1";
  // array[2] = "2";
  lepus::Value val = lepus::Value(lepus::Dictionary::Create());
  val.Table()->SetValue("array", array);
  array.SetProperty(0, lepus::Value("0"));
  array.SetProperty(1, lepus::Value("1"));
  array.SetProperty(2, lepus::Value("2"));

  lepus::Value val2 = lepus::Value(lepus::Dictionary::Create());
  LEPUSContext* ctx = ctx_.context();
  LEPUSValue array_jsvalue = array.ToJSValue(ctx);
  lepus::Value array_v(ctx, array_jsvalue);
  val2.Table()->SetValue("array", array_v);

  ASSERT_TRUE(val == val2);
  ASSERT_TRUE(val2 == val);
  if (!LEPUS_IsGCMode(ctx_.context())) LEPUS_FreeValue(ctx, array_jsvalue);
}

TEST_F(LepusValueMethods, TestLepusValueOperatorEqual) {
  lepus::QuickContext qctx;
  std::string src = "function entry() {}";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
  qctx.Execute();
  LEPUSValue entry = qctx.SearchGlobalData("entry");
  lepus::Value left(qctx.context(), entry);

  lepus::Value right;
  ASSERT_FALSE(left == right);

  lepus::Value right2;
  right2 = lepus::LEPUSValueHelper::ToLepusValue(qctx.context(), entry);
  ASSERT_TRUE(left == right2);
  if (!LEPUS_IsGCMode(ctx_.context())) LEPUS_FreeValue(qctx.context(), entry);
}
TEST_F(LepusValueMethods, TestToLepusValueRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::QuickContext qctx;
  std::string src = "let obj = {a: 3}; let obj2 = {b: 3};";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
  qctx.Execute();

  lepus::Value obj =
      lepus::Value(qctx.context(), qctx.SearchGlobalData("obj")).ToLepusValue();

  LEPUSValue obj_ref = obj.ToJSValue(qctx.context());
  auto obj2_wrap = qctx.SearchGlobalData("obj2");
  lepus::LEPUSValueHelper::SetProperty(qctx.context(), obj2_wrap, "test",
                                       obj_ref);
  // get a copy.
  lepus::Value obj_result =
      lepus::LEPUSValueHelper::ToLepusValue(qctx.context(), obj2_wrap, 1);

  auto p1 = obj_result.Table()->GetValue("test").Table();
  auto p2 = obj.Table();
  ASSERT_TRUE(p1 != p2);

  LEPUS_FreeValue(qctx.context(), obj2_wrap);
}
TEST_F(LepusValueMethods, TestToLepusValueGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::QuickContext qctx;
  std::string src = "let obj = {a: 3}; let obj2 = {b: 3};";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.1");
  qctx.Execute();

  lepus::Value obj =
      lepus::Value(qctx.context(), qctx.SearchGlobalData("obj")).ToLepusValue();

  LEPUSValue obj_ref = obj.ToJSValue(qctx.context());
  HandleScope func_scope(qctx.context(), &obj_ref, HANDLE_TYPE_LEPUS_VALUE);
  auto obj2_wrap = qctx.SearchGlobalData("obj2");
  lepus::LEPUSValueHelper::SetProperty(qctx.context(), obj2_wrap, "test",
                                       obj_ref);
  // get a copy.
  lepus::Value obj_result =
      lepus::LEPUSValueHelper::ToLepusValue(qctx.context(), obj2_wrap, 1);

  auto p1 = obj_result.Table()->GetValue("test").Table();
  auto p2 = obj.Table();
  ASSERT_TRUE(p1 != p2);
}

TEST(LepusWrapDestructTest, LepusWrapDestruct) {
  lepus::QuickContext qctx;
  lepus::Value number = lepus::Value(1);
  lepus::Value number_ref(qctx.context(), number.ToJSValue(qctx.context()));

  lepus::Value array = lepus::Value(lepus::CArray::Create());
  LEPUSValue lepusref = lepus::LEPUSValueHelper::CreateLepusRef(
      qctx.context(), array.Array().get(), lepus::Value_Array);
  if (!LEPUS_IsGCMode(qctx.context()))
    LEPUS_FreeValue(qctx.context(), lepusref);
}

TEST_F(LepusValueMethods, TestValueEqual1) {
  lepus::Value v1 = lepus::Value(lepus::Dictionary::Create());
  v1.SetProperty(base::String("h1"), lepus::Value(10));
  v1.SetProperty(base::String("h2"), lepus::Value(11));
  v1.SetProperty(base::String("h3"), lepus::Value(12));

  lepus::Value v = lepus::Value(lepus::Dictionary::Create());
  v.SetProperty(base::String("h1"), lepus::Value(10));
  v.SetProperty(base::String("h2"), lepus::Value(11));
  v.SetProperty(base::String("h3"), lepus::Value(12));

  LEPUSContext* ctx = ctx_.context();

  // LepusJSValue 1
  LEPUSValue v1_jsvalue = v1.ToJSValue(ctx);
  lepus::Value v2 = lepus::Value(ctx, v1_jsvalue);

  // LepusJSValue 2
  LEPUSValue v2_jsvalue = v2.ToJSValue(ctx);
  lepus::Value v3 = lepus::Value(ctx, v2_jsvalue);

  // LepusJSValue == LepusJSValue
  ASSERT_TRUE(v2 == v3);
  ASSERT_TRUE(v3 == v2);

  // Lepus::Value == LepusJSValue
  ASSERT_TRUE(v1 == v2);
  ASSERT_TRUE(v2 == v1);

  // Lepus::Value == LepusJSValue
  ASSERT_TRUE(v1 == v3);
  ASSERT_TRUE(v3 == v1);

  // lepus::Value == lepus::Value
  ASSERT_TRUE(v1 == v);
  ASSERT_TRUE(v == v1);

  lepus::Value js_v1(ctx, v1.ToJSValue(ctx, true));

  lepus::Value js_v2 = lepus::Value(ctx, v.ToJSValue(ctx));

  ASSERT_TRUE(js_v1 == js_v2);
  ASSERT_TRUE(js_v2 == js_v1);

  // JSValue vs Value
  ASSERT_TRUE(js_v1 == v2);
  ASSERT_TRUE(v2 == js_v1);

  ASSERT_TRUE(js_v2 == v);
  ASSERT_TRUE(v == js_v2);
  if (!LEPUS_IsGCMode(ctx_.context())) {
    LEPUS_FreeValue(ctx, v1_jsvalue);
    LEPUS_FreeValue(ctx, v2_jsvalue);
  }
}
TEST_F(LepusValueMethods, TestPropertyRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();
  lepus::Value lepus_object(lepus::Dictionary::Create());
  lepus::Value array_prop(lepus::CArray::Create());
  array_prop.Array()->push_back(lepus::Value("hello world"));

  lepus_object.SetProperty("prop", array_prop);

  ctx_.RegisterGlobalProperty("lepus_object", lepus_object.ToJSValue(ctx));
  std::string js_source =
      "let array = lepus_object.prop;  array[1] = "
      "'my lynx'; ";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source, "");
  ctx_.Execute();
  ASSERT_TRUE(array_prop.Array()->get(1) == lepus::Value("my lynx"));

  array_prop.Array()->push_back(lepus::Value("primjs"));

  std::string js_source_2 = "let array_prop3 = array[2];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source_2, "");
  ctx_.Execute();

  ASSERT_TRUE(
      lepus::Value(ctx_.context(), ctx_.SearchGlobalData("array_prop3")) ==
      array_prop.Array()->get(2));
}
TEST_F(LepusValueMethods, TestPropertyGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();
  lepus::Value lepus_object(lepus::Dictionary::Create());
  lepus::Value array_prop(lepus::CArray::Create());
  array_prop.Array()->push_back(lepus::Value("hello world"));

  lepus_object.SetProperty("prop", array_prop);

  LEPUSValue lepus_obj = lepus_object.ToJSValue(ctx);
  HandleScope func_scope(ctx, &lepus_obj, HANDLE_TYPE_LEPUS_VALUE);

  ctx_.RegisterGlobalProperty("lepus_object", lepus_obj);
  std::string js_source =
      "let array = lepus_object.prop;  array[1] = "
      "'my lynx'; ";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source, "");
  ctx_.Execute();
  ASSERT_TRUE(array_prop.Array()->get(1) == lepus::Value("my lynx"));

  array_prop.Array()->push_back(lepus::Value("primjs"));

  std::string js_source_2 = "let array_prop3 = array[2];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source_2, "");
  ctx_.Execute();

  ASSERT_TRUE(
      lepus::Value(ctx_.context(), ctx_.SearchGlobalData("array_prop3")) ==
      array_prop.Array()->get(2));
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"

TEST_F(LepusValueMethods, TestLepusValueMove) {
  lepus::Value source(lepus::Dictionary::Create());
  lepus::Value prop_array(lepus::CArray::Create());
  prop_array.Array()->push_back(lepus::Value(1));

  prop_array.Array()->push_back(lepus::Value("hello world"));
  source.SetProperty("prop", prop_array);
  lepus::Value obj(std::move(source));
  ASSERT_TRUE(source.IsNil());

  obj = std::move(obj);
  ASSERT_TRUE(obj.IsTable());
}

#pragma clang diagnostic pop
TEST_F(LepusValueMethods, TestLepusRefConstructRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();

  {
    LEPUSValue v1_jsvalue_ = v1_.ToJSValue(ctx);

    lepus::Value t1(ctx, v1_jsvalue_);

    ASSERT_TRUE(t1.Table().get() == v1_.Table().get());

    ASSERT_TRUE(LEPUS_IsLepusRef(v1_jsvalue_));

    ASSERT_TRUE(
        reinterpret_cast<LEPUSLepusRef*>(LEPUS_VALUE_GET_PTR(v1_jsvalue_))
            ->header.ref_count == 1);
    LEPUS_FreeValue(ctx, v1_jsvalue_);
  }
  lepus::Dictionary* v1_ptr = v1_.Table().get();
  ASSERT_TRUE(v1_ptr->HasOneRef());

  {
    lepus::Value t1(ctx, v1_.ToJSValue(ctx));

    ASSERT_TRUE(t1.Table().get() == v1_.Table().get());
  }
  ASSERT_TRUE(v1_ptr->HasOneRef());
}
TEST_F(LepusValueMethods, TestLepusRefConstructGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();

  {
    LEPUSValue v1_jsvalue_ = v1_.ToJSValue(ctx);

    lepus::Value t1(ctx, v1_jsvalue_);

    ASSERT_TRUE(t1.Table().get() == v1_.Table().get());

    ASSERT_TRUE(LEPUS_IsLepusRef(v1_jsvalue_));
  }
  /*lepus::Dictionary* v1_ptr =*/v1_.Table().get();
  // ASSERT_TRUE(v1_ptr->HasOneRef()); gc will decref during finalizer

  {
    lepus::Value t1(ctx, v1_.ToJSValue(ctx));

    ASSERT_TRUE(t1.Table().get() == v1_.Table().get());
  }
  // ASSERT_TRUE(v1_ptr->HasOneRef()); gc will decref during finalizer
}

TEST_F(LepusValueMethods, TestIteratorJsValue) {
  LEPUSContext* ctx = ctx_.context();

  std::string js_source =
      "let obj = { prop1: [1, 2, 'Lynx', { prop1_1: 'hello world' }, "
      "['nihaozhongguo', undefined, null,]], prop2: 'lepus' };";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source, "");
  ctx_.Execute();

  lepus::Value js_object(ctx, ctx_.SearchGlobalData("obj"));
  lepus::Value lepus_object = js_object.ToLepusValue();
  ASSERT_TRUE(js_object == lepus_object);

  lepus::Value temp1 = js_object.GetProperty("prop2");
  ASSERT_TRUE(temp1 == lepus::Value("lepus"));
  ASSERT_TRUE(js_object.GetProperty("prop1").GetProperty(3).GetProperty(
                  "prop1_1") == lepus::Value("hello world"));

  ASSERT_TRUE(
      js_object.GetProperty("prop1").GetProperty(4).GetProperty(2).IsNil());
  ASSERT_TRUE(js_object.GetProperty("prop1").GetProperty(4).GetProperty(1) ==
              lepus_object.GetProperty("prop1").GetProperty(4).GetProperty(1));

  ASSERT_TRUE(lepus::Value(ctx, lepus_object.ToJSValue(ctx, true)) ==
              js_object);
  ASSERT_TRUE(lepus::Value(ctx, lepus_object.ToJSValue(ctx)) == js_object);

  lepus::Value lepus_nested_jsobject(lepus::Dictionary::Create());
  lepus_nested_jsobject.SetProperty("jsvalue", js_object);

  lepus::Value js_nested_jsobject(lepus::Value::CreateObject(&ctx_));

  js_nested_jsobject.SetProperty("jsvalue", js_object);

  ASSERT_TRUE(lepus_nested_jsobject == js_nested_jsobject);

  LEPUSValue lepusref = v1_.ToJSValue(ctx);
  if (LEPUS_IsGCMode(ctx_.context())) {
    HandleScope func_scope(ctx, &lepusref, HANDLE_TYPE_LEPUS_VALUE);
    lepus::LEPUSValueHelper::SetProperty(ctx, js_nested_jsobject.WrapJSValue(),
                                         "lepusref", lepusref);
  } else {
    lepus::LEPUSValueHelper::SetProperty(ctx, js_nested_jsobject.WrapJSValue(),
                                         "lepusref", lepusref);
  }

  lepus_nested_jsobject.SetProperty("lepusref", v1_);

  ASSERT_TRUE(lepus_nested_jsobject == js_nested_jsobject);

  lepus::Value target(lepus::Dictionary::Create());

  tasm::ForEachLepusValue(
      js_nested_jsobject, [&target, this, &js_object](const lepus::Value& key,
                                                      const lepus::Value& val) {
        if (key.StdString() == "lepusref") {
          ASSERT_TRUE(val == v1_);
        } else if (key.StdString() == "jsvalue") {
          ASSERT_TRUE(val == js_object);
        } else {
          ASSERT_TRUE(false);
        }
        target.SetProperty(key.String(), lepus::Value::Clone(val));
      });

  ASSERT_TRUE(target == js_nested_jsobject);
  ASSERT_TRUE(lepus::Value(ctx, target.ToJSValue(ctx, true)) ==
              js_nested_jsobject);
  ASSERT_TRUE(lepus::Value(ctx, target.ToJSValue(ctx, true)) ==
              js_nested_jsobject.ToLepusValue());
  ASSERT_TRUE(lepus::Value(ctx, target.ToJSValue(ctx, false)) ==
              js_nested_jsobject);
  ASSERT_FALSE(lepus::Value(ctx, target.ToJSValue(ctx, true)) !=
               js_nested_jsobject);
}

TEST_F(LepusValueMethods, CreateJsObject) {
  lepus::Value jsobject = lepus::Value::CreateObject(&ctx_);

  jsobject.SetProperty("child_object", lepus::Value::CreateObject(&ctx_));

  ASSERT_TRUE(jsobject.IsObject());
}
TEST_F(LepusValueMethods, GetArrayPropertyRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value lepus_array(lepus::CArray::Create());

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue js_array = lepus_array.ToJSValue(ctx);

  LEPUSValue prop_0 = LEPUS_GetPropertyUint32(ctx, js_array, 0);
  ASSERT_TRUE(LEPUS_IsUndefined(prop_0));

  lepus_array.Array()->push_back(lepus::Value(1));

  prop_0 = LEPUS_GetPropertyUint32(ctx, js_array, 0);

  ASSERT_TRUE(LEPUS_VALUE_GET_INT(prop_0) == 1 &&
              LEPUS_VALUE_GET_TAG(prop_0) == LEPUS_TAG_INT);

  LEPUSValue prop_1 = LEPUS_GetPropertyUint32(ctx, js_array, 1);
  ASSERT_TRUE(LEPUS_IsUndefined(prop_1));

  LEPUS_FreeValue(ctx, js_array);
}
TEST_F(LepusValueMethods, GetArrayPropertyGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value lepus_array(lepus::CArray::Create());

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue js_array = lepus_array.ToJSValue(ctx);
  HandleScope func_scope(ctx, &js_array, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSValue prop_0 = LEPUS_GetPropertyUint32(ctx, js_array, 0);
  ASSERT_TRUE(LEPUS_IsUndefined(prop_0));

  lepus_array.Array()->push_back(lepus::Value(1));

  prop_0 = LEPUS_GetPropertyUint32(ctx, js_array, 0);

  ASSERT_TRUE(LEPUS_VALUE_GET_INT(prop_0) == 1 &&
              LEPUS_VALUE_GET_TAG(prop_0) == LEPUS_TAG_INT);

  LEPUSValue prop_1 = LEPUS_GetPropertyUint32(ctx, js_array, 1);
  ASSERT_TRUE(LEPUS_IsUndefined(prop_1));
}

TEST_F(LepusValueMethods, SetArrayProperty) {
  lepus::Value array(lepus::CArray::Create());

  array.Array()->push_back(lepus::Value("hello"));
  array.Array()->push_back(lepus::Value("world"));

  LEPUSValue arr = array.ToJSValue(ctx_.context());
  if (LEPUS_IsGCMode(ctx_.context())) {
    HandleScope func_scope(ctx_.context(), &arr, HANDLE_TYPE_LEPUS_VALUE);
    ctx_.RegisterGlobalProperty("arr", arr);
  } else {
    ctx_.RegisterGlobalProperty("arr", arr);
  }

  std::string js_source =
      "arr[4] = 'lynx'; let ele2 = arr[2]; let array_length = arr.length; "
      "arr.length = 10; let "
      "array_length2 = arr.length; let arr_idx5 = arr[5]; let arr_idx4 = "
      "arr[4];";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source, "");
  ctx_.Execute();

  lepus::Value arr_element2(ctx_.context(), ctx_.SearchGlobalData("ele2"));

  ASSERT_TRUE(arr_element2.IsUndefined());

  lepus::Value js_array_length1(ctx_.context(),
                                ctx_.SearchGlobalData("array_length"));
  lepus::Value js_array_length2(ctx_.context(),
                                ctx_.SearchGlobalData("array_length2"));
  lepus::Value js_array_idx4(ctx_.context(), ctx_.SearchGlobalData("arr_idx4"));

  ASSERT_TRUE(js_array_idx4 == lepus::Value("lynx"));

  ASSERT_TRUE(js_array_length1 == lepus::Value(5));

  ASSERT_TRUE(js_array_length2 == lepus::Value(10));

  lepus::Value arr_idx4 = array.GetProperty(4);

  ASSERT_TRUE(arr_idx4 == lepus::Value("lynx"));

  ASSERT_TRUE(array.Array()->size() == 10);

  LEPUSValue arr_idx5 = ctx_.SearchGlobalData("arr_idx5");

  ASSERT_TRUE(LEPUS_IsUndefined(arr_idx5));

  js_source = "arr.length = 1";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, js_source, "");
  ctx_.Execute();

  ASSERT_TRUE(array.Array()->size() == 1);

  ASSERT_TRUE(array.Array()->get(1).IsNil());
}
TEST_F(LepusValueMethods, PrintJsvalueRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value src(lepus::Dictionary::Create());

  lepus::Value prop1_lepusvalue(lepus::CArray::Create());

  prop1_lepusvalue.Array()->push_back(lepus::Value("hello"));

  prop1_lepusvalue.Array()->push_back(lepus::Value("world"));

  src.Table()->SetValue("prop1", prop1_lepusvalue);

  /**
   * lepus::value -> jsobject -> jsarray -> jslepusref
   */

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue prop2_jsvalue = LEPUS_NewObject(ctx);
  LEPUSValue prop2_jsvalue_child_prop1 = LEPUS_NewArray(ctx);

  LEPUS_SetPropertyUint32(ctx, prop2_jsvalue_child_prop1, 0,
                          LEPUS_NewString(ctx, "array_prop1"));

  LEPUS_SetPropertyUint32(ctx, prop2_jsvalue_child_prop1, 1,
                          v1_.ToJSValue(ctx));

  LEPUS_SetPropertyStr(ctx, prop2_jsvalue, "child_prop1",
                       prop2_jsvalue_child_prop1);

  src.Table()->SetValue("prop2", lepus::Value(ctx_.context(), prop2_jsvalue));
  std::ostringstream ss_;
  lepus::LEPUSValueHelper::PrintValue(ss_, ctx, prop2_jsvalue);
  std::cout << ss_.str() << std::endl;
  ASSERT_TRUE(
      ss_.str() ==
      "{\n  child_prop1: [\n    array_prop1,\n    {prop1:hello}\n  ]\n}");

  LEPUS_FreeValue(ctx, prop2_jsvalue);
}
TEST_F(LepusValueMethods, PrintJsvalueGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value src(lepus::Dictionary::Create());

  lepus::Value prop1_lepusvalue(lepus::CArray::Create());

  prop1_lepusvalue.Array()->push_back(lepus::Value("hello"));

  prop1_lepusvalue.Array()->push_back(lepus::Value("world"));

  src.Table()->SetValue("prop1", prop1_lepusvalue);

  /**
   * lepus::value -> jsobject -> jsarray -> jslepusref
   */

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue prop2_jsvalue = LEPUS_NewObject(ctx);
  HandleScope func_scope(ctx, &prop2_jsvalue, HANDLE_TYPE_LEPUS_VALUE);
  LEPUSValue prop2_jsvalue_child_prop1 = LEPUS_NewArray(ctx);
  func_scope.PushHandle(&prop2_jsvalue_child_prop1, HANDLE_TYPE_LEPUS_VALUE);

  LEPUS_SetPropertyUint32(ctx, prop2_jsvalue_child_prop1, 0,
                          LEPUS_NewString(ctx, "array_prop1"));

  LEPUS_SetPropertyUint32(ctx, prop2_jsvalue_child_prop1, 1,
                          v1_.ToJSValue(ctx));

  LEPUS_SetPropertyStr(ctx, prop2_jsvalue, "child_prop1",
                       prop2_jsvalue_child_prop1);

  src.Table()->SetValue("prop2", lepus::Value(ctx_.context(), prop2_jsvalue));
  std::ostringstream ss_;
  lepus::LEPUSValueHelper::PrintValue(ss_, ctx, prop2_jsvalue);
  std::cout << ss_.str() << std::endl;
  ASSERT_TRUE(
      ss_.str() ==
      "{\n  child_prop1: [\n    array_prop1,\n    {prop1:hello}\n  ]\n}");
}
TEST_F(LepusValueMethods, ToJSONString) {
  lepus::VMContext vctx;
  std::string src = lepus::readFile("./core/runtime/vm/lepus/big_object.js");
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, "");
  vctx.Execute();
  lepus::Value obj;
  bool ret = vctx.GetTopLevelVariableByName("obj", &obj);
  ASSERT_TRUE(ret);
  lepus::Value jsobj(ctx_.context(), obj.ToJSValue(ctx_.context(), true));

  std::stringstream s1, s2, s3, s4;
  lepus::lepusValueToJSONString(s1, obj, false);
  lepus::lepusValueToJSONString(s2, jsobj, false);

  ASSERT_TRUE(s1.str().length() == s2.str().length());

  lepus::lepusValueToJSONString(s3, obj, true);
  lepus::lepusValueToJSONString(s4, jsobj, true);

  ASSERT_FALSE(s3.str().compare(s4.str()));
  ASSERT_TRUE(s2.str().compare(s4.str()));
}
TEST_F(LepusValueMethods, SetConstValuePropertyRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value obj(lepus::Dictionary::Create());

  obj.SetProperty("prop1", lepus::Value("helloworld"));

  LEPUSValue objjs = obj.ToJSValue(ctx_.context());

  ctx_.RegisterGlobalProperty("obj", objjs);

  obj.Table()->MarkConst();

  lepus::Value arr(lepus::CArray::Create());
  arr.Array()->push_back(lepus::Value(2));
  LEPUSValue arrjs = arr.ToJSValue(ctx_.context());
  ctx_.RegisterGlobalProperty("arr", arrjs);

  std::string src = R"(console.log(obj.prop2); obj.prop2 = "changed";)";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();

  ASSERT_TRUE(obj.GetProperty("prop2").IsNil());
}
TEST_F(LepusValueMethods, SetConstValuePropertyGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value obj(lepus::Dictionary::Create());

  obj.SetProperty("prop1", lepus::Value("helloworld"));

  LEPUSValue objjs = obj.ToJSValue(ctx_.context());
  HandleScope func_scope(ctx_.context(), &objjs, HANDLE_TYPE_LEPUS_VALUE);

  ctx_.RegisterGlobalProperty("obj", objjs);

  obj.Table()->MarkConst();

  lepus::Value arr(lepus::CArray::Create());
  arr.Array()->push_back(lepus::Value(2));
  LEPUSValue arrjs = arr.ToJSValue(ctx_.context());
  func_scope.PushHandle(&arrjs, HANDLE_TYPE_LEPUS_VALUE);
  ctx_.RegisterGlobalProperty("arr", arrjs);

  std::string src = R"(console.log(obj.prop2); obj.prop2 = "changed";)";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();

  ASSERT_TRUE(obj.GetProperty("prop2").IsNil());
}

class LeakVisitor {
 public:
  LeakVisitor() : out_sum(0), rc_sum(0) {}
  void ClearData() {
    visit_table.clear();
    visit_array.clear();
    table_edge.clear();
    array_edge.clear();
    out_sum = 0;
    rc_sum = 0;
  }
  std::unordered_set<lepus::Dictionary*> visit_table;
  std::unordered_set<lepus::CArray*> visit_array;
  std::set<std::pair<lepus::Dictionary*, base::String>> table_edge;
  std::set<std::pair<lepus::CArray*, int>> array_edge;
  int out_sum;
  int rc_sum;
};

static void TraverseCArray(lepus::CArray* array, LeakVisitor& visitor);

static void TraverseTable(lepus::Dictionary* table, LeakVisitor& visitor) {
  if (!table) return;
  visitor.visit_table.insert(table);
  int out = 0;
  lepus::Dictionary* table_ptr = nullptr;
  lepus::CArray* array_ptr = nullptr;
  for (auto it = table->begin(); it != table->end(); it++) {
    switch (it->second.Type()) {
      case lepus::Value_Table: {
        out++;
        table_ptr = reinterpret_cast<lepus::Dictionary*>(it->second.Ptr());
        auto ele =
            visitor.visit_table.find(const_cast<lepus::Dictionary*>(table_ptr));
        if (ele != visitor.visit_table.end()) {
          visitor.table_edge.insert(std::make_pair(table, it->first));
        } else {
          TraverseTable(table_ptr, visitor);
        }
        break;
      }
      case lepus::Value_Array: {
        out++;
        array_ptr = reinterpret_cast<lepus::CArray*>(it->second.Ptr());
        auto ele =
            visitor.visit_array.find(const_cast<lepus::CArray*>(array_ptr));
        if (ele != visitor.visit_array.end()) {
          visitor.table_edge.insert(std::make_pair(table, it->first));
        } else {
          TraverseCArray(array_ptr, visitor);
        }
        break;
      }
      default:
        break;
    }
  }
  visitor.rc_sum += table->SubtleRefCountForDebug();
  visitor.out_sum += out;
}

static void TraverseCArray(lepus::CArray* array, LeakVisitor& visitor) {
  if (!array) return;
  visitor.visit_array.insert(array);
  size_t size = array->size();
  int out = 0;
  lepus::Dictionary* table_ptr = nullptr;
  lepus::CArray* array_ptr = nullptr;
  for (size_t i = 0; i < size; i++) {
    switch (array->get(i).Type()) {
      case lepus::Value_Table: {
        out++;
        table_ptr = reinterpret_cast<lepus::Dictionary*>(array->get(i).Ptr());
        auto ele =
            visitor.visit_table.find(const_cast<lepus::Dictionary*>(table_ptr));
        if (ele != visitor.visit_table.end()) {
          visitor.array_edge.insert(std::make_pair(array, i));
        } else {
          TraverseTable(table_ptr, visitor);
        }
        break;
      }
      case lepus::Value_Array: {
        out++;
        array_ptr = reinterpret_cast<lepus::CArray*>(array->get(i).Ptr());
        auto ele =
            visitor.visit_array.find(const_cast<lepus::CArray*>(array_ptr));
        if (ele != visitor.visit_array.end()) {
          visitor.array_edge.insert(std::make_pair(array, i));
        } else {
          TraverseCArray(array_ptr, visitor);
        }
        break;
      }
      default:
        break;
    }
  }
  visitor.rc_sum += array->SubtleRefCountForDebug();
  visitor.out_sum += out;
}

TEST_F(LepusValueMethods, TestTraverseTable) {
  LEPUSContext* lctx = ctx_.context();
  lepus::Value tableA(lepus::Dictionary::Create());
  lepus::Value refTableA(
      lctx, lepus::LEPUSValueHelper::CreateLepusRef(lctx, tableA.Table().get(),
                                                    lepus::Value_Table));
  lepus::Value arrayA(lepus::CArray::Create());
  refTableA.SetProperty("refTableA_b", arrayA);
  lepus::Value tableB(lepus::Dictionary::Create());
  arrayA.SetProperty(1, tableB);
  tableB.SetProperty("tableB_arrayA", arrayA);
  lepus::Value tableC(lepus::Dictionary::Create());
  tableB.SetProperty("tableB_tableC", tableC);
  tableC.SetProperty("tableC_refTableA", refTableA);

  lepus::Value tableD(lepus::Dictionary::Create());

  LeakVisitor visitor;
  TraverseTable(reinterpret_cast<lepus::Dictionary*>(tableA.Ptr()), visitor);
  lepus::CArray* arr = reinterpret_cast<lepus::CArray*>(arrayA.Ptr());
  ASSERT_TRUE(visitor.visit_array.find(arr) != visitor.visit_array.end());
  lepus::Dictionary* table = reinterpret_cast<lepus::Dictionary*>(tableD.Ptr());
  ASSERT_FALSE(visitor.visit_table.find(table) != visitor.visit_table.end());
}

TEST_F(LepusValueMethods, TestTraverseCArray) {
  LEPUSContext* lctx = ctx_.context();
  lepus::Value array1(lepus::CArray::Create());
  array1.SetProperty(0, lepus::Value("table_v1"));
  lepus::Value array2(lepus::CArray::Create());
  lepus::Value table1(lepus::Dictionary::Create());
  table1.SetProperty("name", lepus::Value("table_1"));
  lepus::Value refArray2(lctx,
                         lepus::LEPUSValueHelper::CreateLepusRef(
                             lctx, (array2.Array().get()), lepus::Value_Array));
  array1.SetProperty(3, refArray2);
  array2.SetProperty(0, table1);
  lepus::Value table2(lepus::Dictionary::Create());
  table2.SetProperty("name", lepus::Value("table_2"));
  table2.SetProperty("table2_refArray2", refArray2);
  array2.SetProperty(2, table2);
  table1.SetProperty("table1_array1", array1);
  lepus::Value table3(lepus::Dictionary::Create());
  table1.SetProperty("table1_table3", table3);
  table3.SetProperty("table3_table1", table1);
  table3.SetProperty("table3_table2", table2);
  lepus::Value refTable1(
      lctx, lepus::LEPUSValueHelper::CreateLepusRef(lctx, table1.Table().get(),
                                                    lepus::Value_Table));

  LeakVisitor visitor;
  TraverseCArray(reinterpret_cast<lepus::CArray*>(array1.Ptr()), visitor);
  lepus::CArray* arr = reinterpret_cast<lepus::CArray*>(array2.Ptr());
  ASSERT_TRUE(visitor.visit_array.find(arr) != visitor.visit_array.end());
  lepus::Dictionary* table = reinterpret_cast<lepus::Dictionary*>(table1.Ptr());
  ASSERT_TRUE(visitor.visit_table.find(table) != visitor.visit_table.end());
}

TEST_F(LepusValueMethods, ToLepusValueNested) {
  LEPUSContext* ctx = ctx_.context();
  LEPUSValue obj1 = LEPUS_NewObject(ctx);

  LEPUS_SetPropertyStr(ctx, obj1, "prop1", LEPUS_NewString(ctx, "hello world"));

  lepus::Value lepus_table(lepus::Dictionary::Create());
  lepus_table.SetProperty("js_prop1", lepus::Value(ctx, std::move(obj1)));

  lepus::Value tolepusvalue = lepus_table.ToLepusValue();

  ASSERT_TRUE(tolepusvalue == lepus_table);

  ASSERT_TRUE(!tolepusvalue.GetProperty("js_prop1").IsJSValue());

  std::string src = R"(function ftest() {console.log('hello world')})";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();

  LEPUSValue js_func = ctx_.SearchGlobalData("ftest");

  lepus_table.SetProperty("js_func_prop", lepus::Value(ctx, js_func));

  tolepusvalue = lepus_table.ToLepusValue();

  ASSERT_TRUE(tolepusvalue == lepus_table);
  ASSERT_TRUE(lepus_table.GetProperty("js_func_prop").IsJSValue());
  if (!LEPUS_IsGCMode(ctx_.context())) LEPUS_FreeValue(ctx, js_func);
}

TEST_F(LepusValueMethods, UpdateTopLevelVariable) {
  lepus::Value gobj(lepus::Dictionary::Create());
  gobj.SetProperty("prop1", lepus::Value("hello world"));

  ctx_.UpdateTopLevelVariable("obj", gobj);

  lepus::Value arr(lepus::CArray::Create());

  arr.SetProperty(0, lepus::Value("Lynx"));

  ctx_.UpdateTopLevelVariable("obj.array", arr);

  ASSERT_TRUE(gobj.GetProperty("array") == arr);
}

static void LepusGetIdxFromAtom(LEPUSAtom prop, int32_t& idx) {
  idx = -1;
  if (prop & (1U << 31)) {
    idx = prop & ~(1U << 31);
  }
}
TEST_F(LepusValueMethods, DictioanryHasPropertyTestRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value dictionary(lepus::Dictionary::Create());
  dictionary.SetProperty("prop", lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue dic_js = dictionary.ToJSValue(ctx_.context());

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;

  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtom(ctx, "porp_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);
  LEPUS_FreeValue(ctx, dic_js);
}
TEST_F(LepusValueMethods, DictioanryHasPropertyTestGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value dictionary(lepus::Dictionary::Create());
  dictionary.SetProperty("prop", lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue dic_js = dictionary.ToJSValue(ctx_.context());
  HandleScope func_scope(ctx, &dic_js, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;

  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, dic_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "porp_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, dic_js, prop, idx));
}
TEST_F(LepusValueMethods, ArrayHasPropertyTestRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value array(lepus::CArray::Create());
  array.Array()->push_back(lepus::Value("hello"));
  array.Array()->push_back(lepus::Value("world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = array.ToJSValue(ctx_.context());

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;

  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom();
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 0);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtomUInt32(ctx, 1);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  LEPUS_FreeValue(ctx, arr_js);
}
TEST_F(LepusValueMethods, ArrayHasPropertyTestGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value array(lepus::CArray::Create());
  array.Array()->push_back(lepus::Value("hello"));
  array.Array()->push_back(lepus::Value("world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = array.ToJSValue(ctx_.context());
  HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;

  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "length");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 0);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 1);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusHasProperty(ctx, arr_js, prop, idx));
}
TEST_F(LepusValueMethods, DeleteDictionaryPropertyRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value dictionary(lepus::Dictionary::Create());
  dictionary.SetProperty("prop", lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue dic_js = dictionary.ToJSValue(ctx);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;
  LepusGetIdxFromAtom(prop, idx);

  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtom(ctx, "prop_notfound");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_FALSE(dictionary.Contains("prop"));
  dictionary.SetProperty("prop2", lepus::Value("primjs"));
  dictionary.MarkConst();

  prop = LEPUS_NewAtom(ctx, "prop2");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_TRUE(dictionary.Contains("prop2"));

  prop = LEPUS_NewAtom(ctx, "prop_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  LEPUS_FreeValue(ctx, dic_js);
}
TEST_F(LepusValueMethods, DeleteDictionaryPropertyGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value dictionary(lepus::Dictionary::Create());
  dictionary.SetProperty("prop", lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue dic_js = dictionary.ToJSValue(ctx);
  HandleScope func_scope(ctx, &dic_js, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;
  LepusGetIdxFromAtom(prop, idx);

  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "prop_notfound");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));

  ASSERT_FALSE(dictionary.Contains("prop"));
  dictionary.SetProperty("prop2", lepus::Value("primjs"));
  dictionary.MarkConst();

  prop = LEPUS_NewAtom(ctx, "prop2");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));

  ASSERT_TRUE(dictionary.Contains("prop2"));

  prop = LEPUS_NewAtom(ctx, "prop_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, dic_js, prop, idx));
}
TEST_F(LepusValueMethods, DeleteArrayPropertyRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value("hello"));
  arr.Array()->push_back(lepus::Value("world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom();
  LepusGetIdxFromAtom(prop, idx);

  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "prop_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtomUInt32(ctx, 1);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_TRUE(arr.GetLength() == 2);
  ASSERT_EQ(arr.GetProperty(0), lepus::Value("hello"));
  ASSERT_TRUE(arr.GetProperty(1).IsUndefined());

  prop = LEPUS_NewAtomUInt32(ctx, 0);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_TRUE(arr.GetLength() == 2);
  ASSERT_TRUE(arr.GetProperty(0).IsUndefined());
  ASSERT_TRUE(arr.GetProperty(1).IsUndefined());

  arr.Array()->push_back(lepus::Value("hello world"));

  arr.MarkConst();
  prop = LEPUS_NewAtomUInt32(ctx, 10);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_TRUE(arr.GetLength() == 3);

  prop = lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom();
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "prop_no_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));
  LEPUS_FreeAtom(ctx, prop);

  ASSERT_TRUE(arr.GetLength() == 3);

  LEPUS_FreeValue(ctx, arr_js);
}
TEST_F(LepusValueMethods, DeleteArrayPropertyGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value("hello"));
  arr.Array()->push_back(lepus::Value("world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  int32_t idx = -1;
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "length");
  LepusGetIdxFromAtom(prop, idx);

  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "prop_not_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 1);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  ASSERT_TRUE(arr.GetLength() == 2);
  ASSERT_EQ(arr.GetProperty(0), lepus::Value("hello"));
  ASSERT_TRUE(arr.GetProperty(1).IsUndefined());

  prop = LEPUS_NewAtomUInt32(ctx, 0);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  ASSERT_TRUE(arr.GetLength() == 2);
  ASSERT_TRUE(arr.GetProperty(0).IsUndefined());
  ASSERT_TRUE(arr.GetProperty(1).IsUndefined());

  arr.Array()->push_back(lepus::Value("hello world"));

  arr.MarkConst();
  prop = LEPUS_NewAtomUInt32(ctx, 10);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  ASSERT_TRUE(arr.GetLength() == 3);

  prop = LEPUS_NewAtom(ctx, "length");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtom(ctx, "prop_no_found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  prop = LEPUS_NewAtomUInt32(ctx, 2);
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, arr_js, prop, idx));

  ASSERT_TRUE(arr.GetLength() == 3);
}
static void lepus_free_prop_enum(LEPUSContext* ctx, LEPUSPropertyEnum* tab,
                                 uint32_t len) {
  uint32_t i;
  if (tab) {
    for (i = 0; i < len; i++) LEPUS_FreeAtom(ctx, tab[i].atom);
    lepus_free(ctx, tab);
  }
}
TEST_F(LepusValueMethods, DictionaryGetOwnPropertyNamesRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();

  lepus::Value dic(lepus::Dictionary::Create());

  LEPUSValue dic_js = dic.ToJSValue(ctx);
  uint32_t prop_count = 0;
  LEPUSPropertyEnum* props = nullptr;

  dic.SetProperty("prop1", lepus::Value(10));
  dic.SetProperty("prop2", lepus::Value("Lynx"));

  ASSERT_EQ(
      lepus::LEPUSValueGetOwnPropertyNames(ctx, dic_js, &prop_count, &props, 0),
      0);
  ASSERT_TRUE(prop_count == 2 && props);
  uint32_t i = 0;
  for (auto& itr : *dic.Table()) {
    LEPUSAtom prop_test = LEPUS_NewAtom(ctx, itr.first.c_str());
    ASSERT_TRUE(props[i].atom == prop_test);
    ASSERT_TRUE(props[i].is_enumerable);
    LEPUS_FreeAtom(ctx, prop_test);
    ++i;
  }
  lepus_free_prop_enum(ctx, props, prop_count);
  LEPUS_FreeValue(ctx, dic_js);
}
TEST_F(LepusValueMethods, DictionaryGetOwnPropertyNamesGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  LEPUSContext* ctx = ctx_.context();

  lepus::Value dic(lepus::Dictionary::Create());

  LEPUSValue dic_js = dic.ToJSValue(ctx);
  HandleScope func_scope(ctx, &dic_js, HANDLE_TYPE_LEPUS_VALUE);
  uint32_t prop_count = 0;
  LEPUSPropertyEnum* props = nullptr;

  dic.SetProperty("prop1", lepus::Value(10));
  dic.SetProperty("prop2", lepus::Value("Lynx"));

  ASSERT_EQ(
      lepus::LEPUSValueGetOwnPropertyNames(ctx, dic_js, &prop_count, &props, 0),
      0);
  ASSERT_TRUE(prop_count == 2 && props);
  func_scope.PushHandle((void*)&props, HANDLE_TYPE_HEAP_OBJ);
  uint32_t i = 0;
  for (auto& itr : *dic.Table()) {
    LEPUSAtom prop_test = LEPUS_NewAtom(ctx, itr.first.c_str());
    ASSERT_TRUE(props[i].atom == prop_test);
    ASSERT_TRUE(props[i].is_enumerable);
    ++i;
  }
}
TEST_F(LepusValueMethods, ArrayGetOwnPropertyNamesRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value("string"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);

  uint32_t prop_count = 0;
  LEPUSPropertyEnum* ptab = nullptr;
  ASSERT_EQ(
      lepus::LEPUSValueGetOwnPropertyNames(ctx, arr_js, &prop_count, &ptab, 0),
      0);

  ASSERT_TRUE((prop_count == 3) && ptab);

  for (size_t i = 0; i < arr.Array()->size(); ++i) {
    ASSERT_TRUE(ptab[i].is_enumerable);
    LEPUSAtom prop_test = LEPUS_NewAtomUInt32(ctx, i);
    ASSERT_EQ(prop_test, (ptab[i].atom));
    LEPUS_FreeAtom(ctx, prop_test);
  }
  ASSERT_FALSE(ptab[2].is_enumerable);
  LEPUSAtom len_prop =
      lepus::QuickContext::GetFromJsContext(ctx)->GetLengthAtom();
  ASSERT_TRUE(len_prop = ptab[2].atom);

  lepus_free_prop_enum(ctx, ptab, prop_count);

  ASSERT_EQ(lepus::LEPUSValueGetOwnPropertyNames(ctx, arr_js, &prop_count,
                                                 &ptab, LEPUS_GPN_ENUM_ONLY),
            0);

  ASSERT_TRUE((prop_count == 2) && ptab);

  for (size_t i = 0; i < arr.Array()->size(); ++i) {
    ASSERT_TRUE(ptab[i].is_enumerable);
    LEPUSAtom prop_test = LEPUS_NewAtomUInt32(ctx, i);
    ASSERT_EQ(prop_test, (ptab[i].atom));
    LEPUS_FreeAtom(ctx, prop_test);
  }

  lepus_free_prop_enum(ctx, ptab, prop_count);
  LEPUS_FreeValue(ctx, arr_js);
}
TEST_F(LepusValueMethods, ArrayGetOwnPropertyNamesGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value("string"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);

  uint32_t prop_count = 0;
  LEPUSPropertyEnum* ptab = nullptr;
  ASSERT_EQ(
      lepus::LEPUSValueGetOwnPropertyNames(ctx, arr_js, &prop_count, &ptab, 0),
      0);

  ASSERT_TRUE((prop_count == 3) && ptab);
  func_scope.PushHandle((void*)&ptab, HANDLE_TYPE_HEAP_OBJ);

  for (size_t i = 0; i < arr.Array()->size(); ++i) {
    ASSERT_TRUE(ptab[i].is_enumerable);
    LEPUSAtom prop_test = LEPUS_NewAtomUInt32(ctx, i);
    ASSERT_EQ(prop_test, (ptab[i].atom));
  }
  ASSERT_FALSE(ptab[2].is_enumerable);
  LEPUSAtom len_prop = LEPUS_NewAtom(ctx, "length");
  ASSERT_TRUE(len_prop = ptab[2].atom);

  ASSERT_EQ(lepus::LEPUSValueGetOwnPropertyNames(ctx, arr_js, &prop_count,
                                                 &ptab, LEPUS_GPN_ENUM_ONLY),
            0);

  ASSERT_TRUE((prop_count == 2) && ptab);

  for (size_t i = 0; i < arr.Array()->size(); ++i) {
    ASSERT_TRUE(ptab[i].is_enumerable);
    LEPUSAtom prop_test = LEPUS_NewAtomUInt32(ctx, i);
    ASSERT_EQ(prop_test, (ptab[i].atom));
  }
}
TEST_F(LepusValueMethods, ArrayPushAndPopRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  arr.Array()->push_back(lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);

  std::vector<LEPUSValue> args;

  for (uint32_t i = 0; i < 5; ++i) {
    args.push_back(LEPUS_NewString(ctx, std::to_string(i).c_str()));
  }
  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPushCallBack(
                                 ctx, arr_js, args.size(), args.data(), 0),
                             LEPUS_NewInt64(ctx, arr.GetLength())));

  ASSERT_TRUE(arr.GetLength() == 6);

  ASSERT_TRUE(arr.GetProperty(5).ToString() == "4");

  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPushCallBack(
                                 ctx, arr_js, args.size(), args.data(), 1),
                             LEPUS_NewInt64(ctx, arr.GetLength())));

  ASSERT_TRUE(arr.GetLength() == 11);

  ASSERT_TRUE(arr.GetProperty(0).ToString() == "0");
  ASSERT_TRUE(arr.GetProperty(5).StringView() == "hello world");

  for (auto& itr : args) {
    LEPUS_FreeValue(ctx, itr);
  }

  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPopCallBack(ctx, arr_js, 1),
                             LEPUS_NewString(ctx, "0")));

  ASSERT_TRUE(arr.GetLength() == 10);

  ASSERT_TRUE(arr.GetProperty(0).StringView() == "1");

  ASSERT_TRUE(arr.GetProperty(4).StringView() == "hello world");

  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPopCallBack(ctx, arr_js, 0),
                             LEPUS_NewString(ctx, "4")));

  ASSERT_TRUE(arr.GetLength() == 9);

  ASSERT_TRUE(arr.GetProperty(4).StringView() == "hello world");
  ASSERT_TRUE(arr.GetProperty(8).StringView() == "3");
  LEPUS_FreeValue(ctx, arr_js);
}
TEST_F(LepusValueMethods, ArrayPushAndPopGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  arr.Array()->push_back(lepus::Value("hello world"));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSValue args[5];

  for (uint32_t i = 0; i < 5; ++i) {
    args[i] = LEPUS_NewString(ctx, std::to_string(i).c_str());
  }
  func_scope.PushLEPUSValueArrayHandle(args, 5, false);
  ASSERT_TRUE(LEPUS_StrictEq(
      ctx, lepus::LEPUSRefArrayPushCallBack(ctx, arr_js, 5, args, 0),
      LEPUS_NewInt64(ctx, arr.GetLength())));

  ASSERT_TRUE(arr.GetLength() == 6);

  ASSERT_TRUE(arr.GetProperty(5).ToString() == "4");

  ASSERT_TRUE(LEPUS_StrictEq(
      ctx, lepus::LEPUSRefArrayPushCallBack(ctx, arr_js, 5, args, 1),
      LEPUS_NewInt64(ctx, arr.GetLength())));

  ASSERT_TRUE(arr.GetLength() == 11);

  ASSERT_TRUE(arr.GetProperty(0).ToString() == "0");
  ASSERT_TRUE(arr.GetProperty(5).StringView() == "hello world");

  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPopCallBack(ctx, arr_js, 1),
                             LEPUS_NewString(ctx, "0")));

  ASSERT_TRUE(arr.GetLength() == 10);

  ASSERT_TRUE(arr.GetProperty(0).StringView() == "1");

  ASSERT_TRUE(arr.GetProperty(4).StringView() == "hello world");

  ASSERT_TRUE(LEPUS_StrictEq(ctx,
                             lepus::LEPUSRefArrayPopCallBack(ctx, arr_js, 0),
                             LEPUS_NewString(ctx, "4")));

  ASSERT_TRUE(arr.GetLength() == 9);

  ASSERT_TRUE(arr.GetProperty(4).StringView() == "hello world");
  ASSERT_TRUE(arr.GetProperty(8).StringView() == "3");
}
TEST_F(LepusValueMethods, ArrayFindRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  arr.Array()->push_back(lepus::Value("hello world"));

  arr.Array()->push_back(lepus::Value("lepus_string"));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value("lepus_string"));

  LEPUSValue op;
  op = lepus::Value("lepus_string").ToJSValue(ctx);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 1);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, -1), -1);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 3, -1), 3);

  LEPUS_FreeValue(ctx, op);
  op = LEPUS_NewString(ctx, "lepus_string");
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 2, 1), 3);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 2, -1), 1);
  LEPUS_FreeValue(ctx, op);

  arr.SetProperty(2, lepus::Value(+0));

  op = LEPUS_NewInt64(ctx, +0);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);
  LEPUS_FreeValue(ctx, op);

  op = LEPUS_NewInt64(ctx, -0);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);
  LEPUS_FreeValue(ctx, op);

  arr.SetProperty(2, lepus::Value(double(+0.0)));

  op = lepus::Value(double(+0.0)).ToJSValue(ctx);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);
  LEPUS_FreeValue(ctx, op);

  lepus::Value table(lepus::Dictionary::Create());
  arr.SetProperty(2, table);

  op = LEPUS_NewObject(ctx);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), -1);
  LEPUS_FreeValue(ctx, op);
  LEPUS_FreeValue(ctx, arr_js);
}
TEST_F(LepusValueMethods, ArrayFindGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);
  arr.Array()->push_back(lepus::Value("hello world"));

  arr.Array()->push_back(lepus::Value("lepus_string"));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value("lepus_string"));

  LEPUSValue op;
  op = lepus::Value("lepus_string").ToJSValue(ctx);
  func_scope.PushHandle(&op, HANDLE_TYPE_LEPUS_VALUE);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 1);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, -1), -1);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 3, -1), 3);

  op = LEPUS_NewString(ctx, "lepus_string");
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 2, 1), 3);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 2, -1), 1);

  arr.SetProperty(2, lepus::Value(+0));

  op = LEPUS_NewInt64(ctx, +0);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);

  op = LEPUS_NewInt64(ctx, -0);

  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);

  arr.SetProperty(2, lepus::Value(double(+0.0)));

  op = lepus::Value(double(+0.0)).ToJSValue(ctx);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), 2);

  lepus::Value table(lepus::Dictionary::Create());
  arr.SetProperty(2, table);

  op = LEPUS_NewObject(ctx);
  ASSERT_EQ(lepus ::LEPUSRefArrayFindCallBack(ctx, arr_js, op, 0, 1), -1);
}

TEST_F(LepusValueMethods, ArrayReverse) {
  lepus::Value arr(lepus::CArray::Create());

  for (uint32_t i = 0; i < 5; ++i) {
    arr.Array()->push_back(lepus::Value(i));
  }

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue arr_js = arr.ToJSValue(ctx);
  if (LEPUS_IsGCMode(ctx_.context())) {
    HandleScope func_scope(ctx, &arr_js, HANDLE_TYPE_LEPUS_VALUE);
    ASSERT_EQ(lepus::Value(ctx, lepus::LEPUSRefArrayReverse(ctx, arr_js)), arr);
  } else {
    ASSERT_EQ(lepus::Value(ctx, lepus::LEPUSRefArrayReverse(ctx, arr_js)), arr);
  }
  for (uint32_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(arr.Array()->get(i) == lepus::Value(4 - i));
  }
}

namespace checkUpdateTopLevelVariable {

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextSameString) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 'helloworld'";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value("helloworld"));
  ASSERT_FALSE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextDiffString) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 'helloworld'";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value("helloworld2"));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextSameNumber) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 123";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value(123));
  ASSERT_FALSE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextDiffNumber) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 123";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value(1234));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextSameTable) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let obj = { prop1: 123, prop2: 'lepus' };";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  auto sub_table = lepus::Dictionary::Create();
  sub_table->SetValue(base::String("prop1"), lepus::Value(123));
  sub_table->SetValue(base::String("prop2"), lepus::Value("lepus"));
  table->SetValue(base::String("obj"), lepus::Value(sub_table));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextDiffTable) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let obj = { prop1: 123, prop2: 'lepus' };";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  auto sub_table = lepus::Dictionary::Create();
  sub_table->SetValue(base::String("prop1"), lepus::Value(1));
  sub_table->SetValue(base::String("prop2"), lepus::Value("lepus"));
  table->SetValue(base::String("obj"), lepus::Value(sub_table));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextSameArray) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let arr = [1, 2, 3];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(1));
  array->push_back(lepus::Value(2));
  array->push_back(lepus::Value(3));
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("arr"), lepus::Value(array));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableQuickContextDiffArray) {
  lepus::QuickContext ctx;
  ctx.Initialize();
  std::string js_source = "let arr = [1, 2, 3];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(1));
  array->push_back(lepus::Value(2));
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("arr"), lepus::Value(array));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextSameString) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 'helloworld'";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value("helloworld"));
  ASSERT_FALSE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextDiffString) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 'helloworld'";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value("helloworld2"));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextSameNumber) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 123";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"),
                  lepus::Value(static_cast<int64_t>(123)));
  ASSERT_FALSE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextDiffNumber) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let prop1 = 123";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("prop1"), lepus::Value(1234));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextSameTable) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let obj = { prop1: 123, prop2: 'lepus' };";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  auto sub_table = lepus::Dictionary::Create();
  sub_table->SetValue(base::String("prop1"), lepus::Value(123));
  sub_table->SetValue(base::String("prop2"), lepus::Value("lepus"));
  table->SetValue(base::String("obj"), lepus::Value(sub_table));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextDiffTable) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let obj = { prop1: 123, prop2: 'lepus' };";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();
  auto table = lepus::Dictionary::Create();
  auto sub_table = lepus::Dictionary::Create();
  sub_table->SetValue(base::String("prop1"), lepus::Value(1));
  sub_table->SetValue(base::String("prop2"), lepus::Value("lepus"));
  table->SetValue(base::String("obj"), lepus::Value(sub_table));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextSameArray) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let arr = [1, 2, 3];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(1));
  array->push_back(lepus::Value(2));
  array->push_back(lepus::Value(3));
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("arr"), lepus::Value(array));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}

TEST_F(LepusValueMethods,
       CheckTableShadowUpdatedWithTopLevelVariableVMContextDiffArray) {
  lepus::VMContext ctx;
  ctx.Initialize();
  std::string js_source = "let arr = [1, 2, 3];";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx, js_source, "");
  ctx.Execute();

  auto array = lepus::CArray::Create();
  array->push_back(lepus::Value(1));
  array->push_back(lepus::Value(2));
  auto table = lepus::Dictionary::Create();
  table->SetValue(base::String("arr"), lepus::Value(array));
  ASSERT_TRUE(
      ctx.CheckTableShadowUpdatedWithTopLevelVariable(lepus::Value(table)));
}
TEST_F(LepusValueMethods, Array_EraseRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  arr.Array()->Erase(0, 1);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));

  arr.Array()->Erase(1, 0);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));

  arr.Array()->Erase(5, 2);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));

  arr.Array()->Erase(4, 2);
  ASSERT_TRUE(arr.Array()->size() == 4);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));
  ASSERT_TRUE(arr.Array()->get(3) == lepus::Value(4));

  arr.Array()->Erase(0, 4);

  ASSERT_TRUE(arr.Array()->size() == 0);

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();

  LEPUSValue args[] = {LEPUS_NewString(ctx, "1"), LEPUS_NewString(ctx, "2"),
                       LEPUS_NewString(ctx, "3")};

  arr.Array()->Insert(0, sizeof(args) / sizeof(args[0]), ctx, args);

  ASSERT_TRUE(arr.Array()->size() == 9);

  for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); ++i) {
    ASSERT_TRUE(arr.Array()->get(i).IsJSValue());
    ASSERT_TRUE(arr.Array()->get(i).ToString() == std::to_string(i + 1));
  }

  arr.Array()->Erase(0, sizeof(args) / sizeof(args[0]));

  arr.Array()->Insert(5, sizeof(args) / sizeof(args[0]), ctx, args);
  ASSERT_TRUE(arr.Array()->size() == 9);

  ASSERT_TRUE(arr.Array()->get(5).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(5) == lepus::Value("1"));

  ASSERT_TRUE(!arr.Array()->get(8).IsJSValue());

  arr.Array()->Insert(9, sizeof(args) / sizeof(args[0]), ctx, args);

  ASSERT_TRUE(arr.Array()->size() == 12);
  ASSERT_TRUE(arr.Array()->get(10) == lepus::Value("2"));
  ASSERT_TRUE(arr.Array()->get(11) == lepus::Value("3"));

  for (auto& arg : args) {
    LEPUS_FreeValue(ctx, arg);
  }
}
TEST_F(LepusValueMethods, Array_EraseGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  arr.Array()->Erase(0, 1);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));

  arr.Array()->Erase(1, 0);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));

  arr.Array()->Erase(5, 2);
  ASSERT_TRUE(arr.Array()->size() == 5);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));

  arr.Array()->Erase(4, 2);
  ASSERT_TRUE(arr.Array()->size() == 4);
  ASSERT_TRUE(arr.Array()->get(0) == lepus::Value(1));
  ASSERT_TRUE(arr.Array()->get(1) == lepus::Value(2));
  ASSERT_TRUE(arr.Array()->get(2) == lepus::Value(3));
  ASSERT_TRUE(arr.Array()->get(3) == lepus::Value(4));

  arr.Array()->Erase(0, 4);

  ASSERT_TRUE(arr.Array()->size() == 0);

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue args[3];
  HandleScope func_scope(ctx);
  func_scope.PushLEPUSValueArrayHandle(args, 3, true);
  for (int i = 0; i < 3; i++) {
    args[i] = LEPUS_NewString(ctx, std::to_string(i + 1).c_str());
  }

  arr.Array()->Insert(0, sizeof(args) / sizeof(args[0]), ctx, args);

  ASSERT_TRUE(arr.Array()->size() == 9);

  for (size_t i = 0; i < sizeof(args) / sizeof(args[0]); ++i) {
    ASSERT_TRUE(arr.Array()->get(i).IsJSValue());
    ASSERT_TRUE(arr.Array()->get(i).ToString() == std::to_string(i + 1));
  }

  arr.Array()->Erase(0, sizeof(args) / sizeof(args[0]));

  arr.Array()->Insert(5, sizeof(args) / sizeof(args[0]), ctx, args);

  ASSERT_TRUE(arr.Array()->size() == 9);

  ASSERT_TRUE(arr.Array()->get(5).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(5) == lepus::Value("1"));

  ASSERT_TRUE(!arr.Array()->get(8).IsJSValue());

  arr.Array()->Insert(9, sizeof(args) / sizeof(args[0]), ctx, args);

  ASSERT_TRUE(arr.Array()->size() == 12);
  ASSERT_TRUE(arr.Array()->get(10) == lepus::Value("2"));
  ASSERT_TRUE(arr.Array()->get(11) == lepus::Value("3"));
}
TEST_F(LepusValueMethods, ArrayPrototypeSliceRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue ref = arr.ToJSValue(ctx);

  auto ret = lepus::LEPUSRefArraySlice(ctx, ref, 0, 6, 0, nullptr, 0);

  ASSERT_TRUE(LEPUS_IsArray(ctx, ret));
  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 6);

  ASSERT_TRUE(lepus::Value(ctx, ret) == arr);

  LEPUS_FreeValue(ctx, ret);

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 1, 5, 0, nullptr, 0);
  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 5);
  for (int32_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(i) == lepus::Value(1 + i));
  }
  LEPUS_FreeValue(ctx, ret);
  LEPUS_FreeValue(ctx, ref);
}
TEST_F(LepusValueMethods, ArrayPrototypeSliceGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue ref = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &ref, HANDLE_TYPE_LEPUS_VALUE);

  auto ret = lepus::LEPUSRefArraySlice(ctx, ref, 0, 6, 0, nullptr, 0);
  func_scope.PushHandle(&ret, HANDLE_TYPE_LEPUS_VALUE);

  ASSERT_TRUE(LEPUS_IsArray(ctx, ret));
  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 6);

  ASSERT_TRUE(lepus::Value(ctx, ret) == arr);

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 1, 5, 0, nullptr, 0);
  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 5);
  for (int32_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(i) == lepus::Value(1 + i));
  }
}
TEST_F(LepusValueMethods, ArrayPrototypeSpliceRC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue ref = arr.ToJSValue(ctx);

  LEPUSValue args[] = {LEPUS_NewString(ctx, "1"), LEPUS_NewString(ctx, "2"),
                       LEPUS_NewString(ctx, "3")};

  auto ret = lepus::LEPUSRefArraySlice(ctx, ref, 1, 0,
                                       sizeof(args) / sizeof(args[0]), args, 1);
  ASSERT_TRUE(arr.Array()->size() == 9);
  ASSERT_TRUE(!arr.Array()->get(0).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(1).IsJSValue() &&
              arr.Array()->get(1) == lepus::Value("1"));
  ASSERT_TRUE(arr.Array()->get(2).IsJSValue() &&
              arr.Array()->get(2) == lepus::Value("2"));
  ASSERT_TRUE(arr.Array()->get(3).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(4).IsJSValue() == false);

  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 0);
  LEPUS_FreeValue(ctx, ret);

  arr.Array()->Erase(0, 9);
  ASSERT_TRUE(arr.GetLength() == 0);
  for (auto& arg : args) {
    LEPUS_FreeValue(ctx, arg);
  }

  arr.Array()->push_back(lepus::Value("angel"));
  arr.Array()->push_back(lepus::Value("clown"));
  arr.Array()->push_back(lepus::Value("mandarin"));
  arr.Array()->push_back(lepus::Value("sturgeon"));

  args[0] = LEPUS_NewString(ctx, "drum");

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 2, 0, 1, args, 1);

  ASSERT_EQ(LEPUS_GetLength(ctx, ret), 0);

  ASSERT_EQ(arr.Array()->size(), 5U);

  std::vector<lepus::Value> expected = {
      lepus::Value("angel"), lepus::Value("clown"), lepus::Value("drum"),
      lepus::Value("mandarin"), lepus::Value("sturgeon")};

  size_t i = 0;
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }
  LEPUS_FreeValue(ctx, ret);
  LEPUS_FreeValue(ctx, args[0]);

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 3, 1, 0, nullptr, 1);
  i = 0;
  expected = {lepus::Value("angel"), lepus::Value("clown"),
              lepus::Value("drum"), lepus::Value("sturgeon")};
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }

  ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(0) ==
              lepus::Value("mandarin"));

  LEPUS_FreeValue(ctx, ret);

  args[0] = LEPUS_NewString(ctx, "trumpet");
  ret = lepus::LEPUSRefArraySlice(ctx, ref, 2, 1, 1, args, 1);

  expected = {lepus::Value("angel"), lepus::Value("clown"),
              lepus::Value("trumpet"), lepus::Value("sturgeon")};
  i = 0;
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }
  ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(0) == lepus::Value("drum"));
  LEPUS_FreeValue(ctx, ret);
  LEPUS_FreeValue(ctx, args[0]);
  LEPUS_FreeValue(ctx, ref);
}
TEST_F(LepusValueMethods, ArrayPrototypeSpliceGC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());

  arr.Array()->push_back(lepus::Value(0));
  arr.Array()->push_back(lepus::Value(1));
  arr.Array()->push_back(lepus::Value(2));
  arr.Array()->push_back(lepus::Value(3));
  arr.Array()->push_back(lepus::Value(4));
  arr.Array()->push_back(lepus::Value(5));

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue ref = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &ref, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSValue args[3];
  func_scope.PushLEPUSValueArrayHandle(args, 3, true);
  for (int i = 0; i < 3; i++) {
    args[i] = LEPUS_NewString(ctx, std::to_string(i + 1).c_str());
  }

  auto ret = lepus::LEPUSRefArraySlice(ctx, ref, 1, 0,
                                       sizeof(args) / sizeof(args[0]), args, 1);
  ASSERT_TRUE(arr.Array()->size() == 9);
  ASSERT_TRUE(!arr.Array()->get(0).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(1).IsJSValue() &&
              arr.Array()->get(1) == lepus::Value("1"));
  ASSERT_TRUE(arr.Array()->get(2).IsJSValue() &&
              arr.Array()->get(2) == lepus::Value("2"));
  ASSERT_TRUE(arr.Array()->get(3).IsJSValue());
  ASSERT_TRUE(arr.Array()->get(4).IsJSValue() == false);

  ASSERT_TRUE(LEPUS_GetLength(ctx, ret) == 0);

  arr.Array()->Erase(0, 9);
  ASSERT_TRUE(arr.GetLength() == 0);

  arr.Array()->push_back(lepus::Value("angel"));
  arr.Array()->push_back(lepus::Value("clown"));
  arr.Array()->push_back(lepus::Value("mandarin"));
  arr.Array()->push_back(lepus::Value("sturgeon"));

  args[0] = LEPUS_NewString(ctx, "drum");

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 2, 0, 1, args, 1);

  ASSERT_EQ(LEPUS_GetLength(ctx, ret), 0);

  ASSERT_EQ(arr.Array()->size(), 5U);

  std::vector<lepus::Value> expected = {
      lepus::Value("angel"), lepus::Value("clown"), lepus::Value("drum"),
      lepus::Value("mandarin"), lepus::Value("sturgeon")};

  size_t i = 0;
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }

  ret = lepus::LEPUSRefArraySlice(ctx, ref, 3, 1, 0, nullptr, 1);
  i = 0;
  expected = {lepus::Value("angel"), lepus::Value("clown"),
              lepus::Value("drum"), lepus::Value("sturgeon")};
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }

  ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(0) ==
              lepus::Value("mandarin"));

  args[0] = LEPUS_NewString(ctx, "trumpet");
  ret = lepus::LEPUSRefArraySlice(ctx, ref, 2, 1, 1, args, 1);

  expected = {lepus::Value("angel"), lepus::Value("clown"),
              lepus::Value("trumpet"), lepus::Value("sturgeon")};
  i = 0;
  for (auto& itr : expected) {
    ASSERT_TRUE(arr.Array()->get(i++) == itr);
  }
  ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(0) == lepus::Value("drum"));
}
TEST_F(LepusValueMethods, ArrayPrototypeSplice2RC) {
  if (LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  std::vector<std::string> sources = {"angel", "clown", "trumpet", "sturgeon"};

  for (auto& itr : sources) {
    arr.Array()->push_back(lepus::Value(itr.c_str()));
  }

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue this_val = arr.ToJSValue(ctx);

  std::vector<LEPUSValue> args = {LEPUS_NewString(ctx, "parrot"),
                                  LEPUS_NewString(ctx, "anemone"),
                                  LEPUS_NewString(ctx, "blue")};
  LEPUSValue ret = lepus::LEPUSRefArraySlice(ctx, this_val, 0, 2, args.size(),
                                             args.data(), 1);

  std::vector<lepus::Value> ret_expected = {lepus::Value("angel"),
                                            lepus::Value("clown")};
  std::vector<lepus::Value> source_expcected = {
      lepus::Value("parrot"), lepus::Value("anemone"), lepus::Value("blue"),
      lepus::Value("trumpet"), lepus::Value("sturgeon")};

  ASSERT_EQ(source_expcected.size(), arr.Array()->size());

  size_t i = 0;
  for (auto& itr : source_expcected) {
    ASSERT_EQ(arr.Array()->get(i), itr);
    ++i;
  }

  ASSERT_EQ(ret_expected.size(), (size_t)LEPUS_GetLength(ctx, ret));
  i = 0;

  for (auto& itr : ret_expected) {
    ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(i++) == itr);
  }

  for (auto& itr : args) {
    LEPUS_FreeValue(ctx, itr);
  }

  LEPUS_FreeValue(ctx, ret);
  LEPUS_FreeValue(ctx, this_val);
}
TEST_F(LepusValueMethods, ArrayPrototypeSplice2GC) {
  if (!LEPUS_IsGCMode(ctx_.context())) return;
  lepus::Value arr(lepus::CArray::Create());
  std::vector<std::string> sources = {"angel", "clown", "trumpet", "sturgeon"};

  for (auto& itr : sources) {
    arr.Array()->push_back(lepus::Value(itr.c_str()));
  }

  LEPUSContext* ctx = ctx_.context();
  LEPUSValue this_val = arr.ToJSValue(ctx);
  HandleScope func_scope(ctx, &this_val, HANDLE_TYPE_LEPUS_VALUE);

  LEPUSValue args[3];
  func_scope.PushLEPUSValueArrayHandle(args, 3, true);
  args[0] = LEPUS_NewString(ctx, "parrot");
  args[1] = LEPUS_NewString(ctx, "anemone");
  args[2] = LEPUS_NewString(ctx, "blue");

  LEPUSValue ret = lepus::LEPUSRefArraySlice(ctx, this_val, 0, 2, 3, args, 1);

  std::vector<lepus::Value> ret_expected = {lepus::Value("angel"),
                                            lepus::Value("clown")};
  std::vector<lepus::Value> source_expcected = {
      lepus::Value("parrot"), lepus::Value("anemone"), lepus::Value("blue"),
      lepus::Value("trumpet"), lepus::Value("sturgeon")};

  ASSERT_EQ(source_expcected.size(), arr.Array()->size());

  size_t i = 0;
  for (auto& itr : source_expcected) {
    ASSERT_EQ(arr.Array()->get(i), itr);
    ++i;
  }

  ASSERT_EQ(ret_expected.size(), (size_t)LEPUS_GetLength(ctx, ret));
  i = 0;

  for (auto& itr : ret_expected) {
    ASSERT_TRUE(lepus::Value(ctx, ret).GetProperty(i++) == itr);
  }
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueJSString) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  lepus::Value res = tasm::ConvertJSValueToLepusValue(js_string_val);
  lepus::Value expected_val("hello world");
  ASSERT_TRUE(res == expected_val);
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueJSNumber) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value js_number_val(ctx, LEPUS_NewFloat64(ctx, 2023.3));
  lepus::Value res = tasm::ConvertJSValueToLepusValue(js_number_val);
  lepus::Value expected_val(2023.3);
  ASSERT_TRUE(res == expected_val);
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueJSInt) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value js_number_val(ctx, LEPUS_NewInt64(ctx, 2023));
  lepus::Value res = tasm::ConvertJSValueToLepusValue(js_number_val);
  lepus::Value expected_val(2023);
  ASSERT_TRUE(res == expected_val);
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueJSBool) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value expected_val(true);
  lepus::Value js_bool_val(ctx, expected_val.ToJSValue(ctx));
  lepus::Value res = tasm::ConvertJSValueToLepusValue(js_bool_val);
  ASSERT_TRUE(res == expected_val);
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueArray) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value array(lepus::CArray::Create());

  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  array.Array()->push_back(js_string_val);
  array.Array()->push_back(lepus::Value("hello world"));

  lepus::Value res = tasm::ConvertJSValueToLepusValue(array);

  lepus::Value expected_val("hello world");
  lepus::Value expected_array(lepus::CArray::Create());
  expected_array.Array()->push_back(expected_val);
  expected_array.Array()->push_back(lepus::Value("hello world"));

  ASSERT_TRUE(res == expected_array);
  ASSERT_TRUE(res.GetProperty(0) == expected_val);
  ASSERT_TRUE(res.GetProperty(1) == lepus::Value("hello world"));
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueTable) {
  LEPUSContext* ctx = ctx_.context();

  lepus::Value table(lepus::Dictionary::Create());
  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  table.Table()->SetValue("key1", js_string_val);
  table.Table()->SetValue("key2", lepus::Value("hello world"));

  lepus::Value res = tasm::ConvertJSValueToLepusValue(table);

  lepus::Value expected_val("hello world");
  lepus::Value expected_table(lepus::Dictionary::Create());
  expected_table.Table()->SetValue("key1", expected_val);
  expected_table.Table()->SetValue("key2", lepus::Value("hello world"));

  ASSERT_TRUE(res == expected_table);
  ASSERT_TRUE(res.GetProperty("key1") == expected_val);
  ASSERT_TRUE(res.GetProperty("key2") == lepus::Value("hello world"));
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValuJSArray) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value array(lepus::CArray::Create());

  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  array.Array()->push_back(js_string_val);
  array.Array()->push_back(lepus::Value("hello world"));

  lepus::Value js_array(ctx, array.ToJSValue(ctx));

  lepus::Value res = tasm::ConvertJSValueToLepusValue(js_array);

  lepus::Value expected_val("hello world");
  lepus::Value expected_array(lepus::CArray::Create());
  expected_array.Array()->push_back(expected_val);
  expected_array.Array()->push_back(lepus::Value("hello world"));

  ASSERT_TRUE(res == expected_array);
  ASSERT_TRUE(res.GetProperty(0) == expected_val);
  ASSERT_TRUE(res.GetProperty(1) == lepus::Value("hello world"));
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueJSTable) {
  LEPUSContext* ctx = ctx_.context();

  lepus::Value table(lepus::Dictionary::Create());
  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  table.Table()->SetValue("key1", js_string_val);
  table.Table()->SetValue("key2", lepus::Value("hello world"));

  lepus::Value js_table(ctx, table.ToJSValue(ctx));

  lepus::Value res = tasm::ConvertJSValueToLepusValue(table);

  lepus::Value expected_val("hello world");
  lepus::Value expected_table(lepus::Dictionary::Create());
  expected_table.Table()->SetValue("key1", expected_val);
  expected_table.Table()->SetValue("key2", lepus::Value("hello world"));

  ASSERT_TRUE(res == expected_table);
  ASSERT_TRUE(res.GetProperty("key1") == expected_val);
  ASSERT_TRUE(res.GetProperty("key2") == lepus::Value("hello world"));
}

TEST_F(LepusValueMethods, ConvertJSValueToLepusValueArrayWithSubArray) {
  LEPUSContext* ctx = ctx_.context();
  lepus::Value array(lepus::CArray::Create());

  lepus::Value js_string_val(
      ctx, lepus::LEPUSValueHelper::NewString(ctx, "hello world"));
  array.Array()->push_back(js_string_val);
  lepus::Value sub_array(lepus::CArray::Create());
  sub_array.Array()->push_back(js_string_val);
  array.Array()->push_back(sub_array);

  lepus::Value res = tasm::ConvertJSValueToLepusValue(array);

  lepus::Value expected_val("hello world");
  lepus::Value expected_array(lepus::CArray::Create());
  expected_array.Array()->push_back(expected_val);
  lepus::Value expected_sub_array(lepus::CArray::Create());
  expected_sub_array.Array()->push_back(expected_val);
  expected_array.Array()->push_back(expected_sub_array);

  ASSERT_TRUE(res == expected_array);
  ASSERT_TRUE(res.GetProperty(0) == lepus::Value("hello world"));
  ASSERT_TRUE(res.GetProperty(1) == expected_sub_array);
}

TEST_F(LepusValueMethods, UpdateValueByPath) {
  /**
   * target:
   * { data : [0, {a : {b : {name : "lynx"}}}]}
   *
   * updata:
   * { data[1].a.b.name : "new name"}
   * { data[1].a.value : "new value"}
   * { data[3] : "new array value"}
   *
   * expected:
   * { data : [
   *           0,
   *           {a : {
   *                  b : { name : "new name"},
   *                  value : "new value",
   *                 }
   *            },
   *            undefined,
   *            "new array value"
   *           ]
   * }
   */

  const std::vector<int> range = {0, 1, 2};

  // create update paths
  std::vector<lepus::PathVector> paths = {
      lepus::ParseValuePath("data[1].a.b.name"),
      lepus::ParseValuePath("data[1].a.value"),
      lepus::ParseValuePath("data[3]")};

  // 1. test Lepus Value

  // create target value
  lepus::Value array(lepus::CArray::Create());
  array.Array()->push_back(lepus::Value(0));
  array.Array()->push_back(lepus::Value(lepus::Dictionary::Create({
      {base::String("a"),
       lepus::Value(lepus::Dictionary::Create({
           {base::String("b"), lepus::Value(lepus::Dictionary::Create({
                                   {base::String("name"), lepus::Value("lynx")},
                               }))},
       }))},
  })));
  lepus::Value target(lepus::Dictionary::Create({
      {base::String("data"), lepus::Value(array)},
  }));

  // create the values which will update
  std::vector<lepus::Value> values = {lepus::Value("new name"),
                                      lepus::Value("new value"),
                                      lepus::Value("new array value")};

  // create check function
  auto check = [&range, &paths](lepus::Value& target,
                                const std::vector<lepus::Value>& update) {
    // update:
    // data[1].a.b.name = "new name"
    // data[1].a.value = "new value"
    // data[3] = "new array value"
    for (int i : range) {
      lepus::Value::UpdateValueByPath(target, update[i], paths[i]);
    }

    // check if the data is updated successfully
    ASSERT_TRUE(target.GetProperty("data")
                    .GetProperty(1)
                    .GetProperty("a")
                    .GetProperty("b")
                    .GetProperty("name") == update[0]);
    ASSERT_TRUE(
        target.GetProperty("data").GetProperty(1).GetProperty("a").GetProperty(
            "value") == update[1]);
    ASSERT_TRUE(target.GetProperty("data").GetProperty(3) == update[2]);
  };

  // check
  check(target, values);

  // 2. test JS Value
  LEPUSContext* ctx = ctx_.context();

  // convert target and updates to JSValue
  lepus::Value js_target(ctx, target.ToJSValue(ctx, true));
  std::vector<lepus::Value> js_values;
  for (int i : range) {
    js_values.emplace_back(lepus::Value(ctx, values[i].ToJSValue(ctx, true)));
  }

  // check JSValue
  check(js_target, js_values);
}

TEST_F(LepusValueMethods, PrintErrorObject) {
  lepus::QuickContext qctx;
  std::string src = R"(
    let err_res;
    try {
      let obj = {

      };
      throw new Error('error test');
    } catch(e) {
      err_res = e;
    }
  )";

  ASSERT_TRUE(
      lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "").empty());
  ASSERT_TRUE(ctx_.Execute());

  auto err_ret = ctx_.SearchGlobalData("err_res");
  ASSERT_TRUE(LEPUS_IsError(ctx_.context(), err_ret));

  std::stringstream print_res;
  lepus::Value(ctx_.context(), std::move(err_ret)).PrintValue(print_res);
  ASSERT_TRUE(print_res.str().find("eval") != std::string::npos);
}

TEST_F(LepusValueMethods, TestRefCountedValueConvertToJSValue) {
  auto* ctx = ctx_.context();
  auto value = lepus::Value(TestRefCountedClass::Create());
  auto array = lepus::Value(lepus::CArray::Create());
  array.SetProperty(0, value);
  ctx_.RegisterGlobalProperty("array", array.ToJSValue(ctx));

  static auto lepus_func = [&](lepus::Context* ctx, lepus::Value* argv,
                               int32_t argc) {
    auto idx = argv[1].Number();
    if (array.GetProperty(idx) != argv[0]) {
      return lepus::Value(false);
    };
    return lepus::Value(true);
  };

  auto lepusng_func = [](LEPUSContext* ctx, LEPUSValue this_val, int32_t argc,
                         LEPUSValue* argv) {
    char args_buf[sizeof(lepus::Value) * argc];
    auto* largv = reinterpret_cast<lepus::Value*>(args_buf);
    for (int i = 0; i < argc; ++i) {
      new (largv + i) lepus::Value(ctx, argv[i]);
    }
    auto ret =
        lepus_func(lepus::QuickContext::GetFromJsContext(ctx), largv, argc)
            .ToJSValue(ctx);
    for (int i = 0; i < argc; ++i) {
      largv[i].FreeValue();
    }
    return ret;
  };

  ctx_.RegisterGlobalFunction("test_func", lepusng_func, 2);
  std::string src = R"(
    let result = true;
    array.forEach((element, idx) => result = (result && test_func(element, idx)));
  )";

  ASSERT_TRUE(lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "") == "");
  ASSERT_TRUE((ctx_.Execute()));

  auto result = ctx_.SearchGlobalData("result");
  ASSERT_TRUE(LEPUS_VALUE_GET_BOOL(result));
  LEPUS_FreeValue(ctx, result);
  return;
}

TEST_F(LepusValueMethods, TestRefCountedConstruct) {
  auto* ctx = ctx_.context();

  auto value = lepus::Value(TestRefCountedClass::Create());
  auto jsvalue = value.ToJSValue(ctx);

  auto lepus_value = lepus::Value(ctx, jsvalue);

  ASSERT_TRUE(lepus_value == value);

  LEPUS_FreeValue(ctx, jsvalue);
}

TEST_F(LepusValueMethods, TestRefCountedMoveConstruct) {
  auto* ctx = ctx_.context();
  auto value = lepus::Value(TestRefCountedClass::Create());
  auto lepus_value = lepus::Value(ctx, value.ToJSValue(ctx));

  ASSERT_TRUE(lepus_value == value);
  value.SetUndefined();
  auto* ref_ptr = lepus_value.RefCounted().get();
  ASSERT_TRUE(ref_ptr->HasOneRef());
}

TEST_F(LepusValueMethods, TestTableCheckAndGetProperty) {
  lepus::Value table(lepus::Dictionary::Create());

  table.SetProperty("prop", lepus::Value("world"));

  auto result = table.Table()->GetProperty("prop");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(result.value() == lepus::Value("world"));

  result = table.Table()->GetProperty("no_prop");
  ASSERT_FALSE(result.has_value());
}
TEST_F(LepusValueMethods, CheckValueIsNumber) {
  auto* ctx = ctx_.context();
  auto v1 = lepus::Value(ctx, LEPUS_NewInt32(ctx, INT32_MAX));
  auto v2 = lepus::Value(ctx, LEPUS_NewBigUint64(ctx, UINT64_MAX));
  auto v3 = lepus::Value(ctx, LEPUS_NewInt64(ctx, std::pow(2, 53) - 1));
  auto v4 = lepus::Value(ctx, LEPUS_NewFloat64(ctx, 0.0001));

  ASSERT_TRUE(v1.IsNumber());
  ASSERT_TRUE(v2.IsNumber());
  ASSERT_TRUE(v3.IsNumber());
  ASSERT_TRUE(v4.IsJSNumber());
}

TEST_F(LepusValueMethods, TestGetUtf8StrFast) {
  std::string str1 = "hello world";
  auto* ctx = ctx_.context();
  auto str1_js = LEPUS_NewString(ctx, str1.c_str());
  ASSERT_TRUE(LEPUS_VALUE_IS_STRING(str1_js));
  auto* utf8_str = LEPUS_GetStringUtf8(ctx, LEPUS_VALUE_GET_STRING(str1_js));
  ASSERT_TRUE(utf8_str);
  ASSERT_TRUE(!strcmp(utf8_str, str1.c_str()));
  LEPUS_FreeValue(ctx, str1_js);
}

TEST_F(LepusValueMethods, TestGetUtf16StrFast) {
  std::string str1 = "\u4f60\u597d\u4e16\u754c";
  auto* ctx = ctx_.context();
  auto str1_js = LEPUS_NewString(ctx, str1.c_str());
  ASSERT_TRUE(LEPUS_VALUE_IS_STRING(str1_js));
  auto* utf8_str = LEPUS_GetStringUtf8(ctx, LEPUS_VALUE_GET_STRING(str1_js));
  ASSERT_FALSE(utf8_str);
  LEPUS_FreeValue(ctx, str1_js);
}

TEST_F(LepusValueMethods, ValueToString) {
  auto* ctx = ctx_.context();
  constexpr const char* case1 = "hello world";
  constexpr const char* case2 = "\u4f60\u597d\u4e16\u754c";
  lepus::Value str1 = lepus::Value(ctx, LEPUS_NewString(ctx, case1));
  auto str1_ret = str1.ToString();
  ASSERT_EQ(str1_ret, case1);
  lepus::Value str2 = lepus::Value(ctx, LEPUS_NewString(ctx, case2));
  ASSERT_EQ(str2.ToString(), case2);
  lepus::Value str3 = lepus::Value(ctx, LEPUS_UNDEFINED);
  ASSERT_EQ(str3.ToString(), "");

  ASSERT_EQ(lepus::LEPUSValueHelper::ToStdString(ctx, str1.WrapJSValue()),
            case1);
  ASSERT_EQ(lepus::LEPUSValueHelper::ToStdString(ctx, str2.WrapJSValue()),
            case2);

  ASSERT_EQ(str1.ToLepusValue(), lepus::Value(case1));
  ASSERT_EQ(str2.ToLepusValue(), lepus::Value(case2));
}

TEST_F(LepusValueMethods, TableToJSObject) {
  lepus::Value dic(lepus::Dictionary::Create());
  dic.SetProperty("prop", lepus::Value(lepus::Dictionary::Create()));
  dic.SetProperty("prop2", lepus::Value(lepus::CArray::Create()));
  auto* ctx = ctx_.context();
  lepus::Value dic_jsvalue = lepus::Value(ctx, dic.ToJSValue(ctx, true));
  ASSERT_TRUE(dic_jsvalue.IsObject());
  ASSERT_FALSE(dic_jsvalue.GetProperty("prop").Ptr() ==
               dic.GetProperty("prop").Ptr());
  ASSERT_EQ(dic_jsvalue, dic);
}

TEST_F(LepusValueMethods, ArrayToJSArray) {
  lepus::Value arr(lepus::CArray::Create());
  auto* ctx = ctx_.context();
  arr.Array()->push_back(lepus::Value(lepus::Dictionary::Create()));
  arr.Array()->push_back(lepus::Value(lepus::CArray::Create()));
  lepus::Value arr_js(ctx, arr.ToJSValue(ctx, true));
  ASSERT_TRUE(arr_js == arr);
  ASSERT_TRUE(arr_js.IsJSArray());
  ASSERT_TRUE(arr_js.GetJSLength() == 2);
}

TEST_F(LepusValueMethods, DeleteObjectProperty) {
  auto* ctx = ctx_.context();
  lepus::Value table(lepus::Dictionary::Create());
  table.SetProperty("prop", lepus::Value("prop"));
  auto jsvalue = table.ToJSValue(ctx);
  LEPUSAtom prop = LEPUS_NewAtom(ctx, "prop");
  // deleted successfully
  int32_t idx = 0;
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, jsvalue, prop, idx));
  // prop is not found
  ASSERT_TRUE(lepus::LepusDeleteProperty(ctx, jsvalue, prop, idx));

  table.SetProperty("prop", lepus::Value("hello world"));
  table.MarkConst();
  // table is const
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, jsvalue, prop, idx));
  LEPUS_FreeAtom(ctx, prop);
  prop = LEPUS_NewAtom(ctx, "prop not found");
  LepusGetIdxFromAtom(prop, idx);
  ASSERT_FALSE(lepus::LepusDeleteProperty(ctx, jsvalue, prop, idx));
  LEPUS_FreeAtom(ctx, prop);
  LEPUS_FreeValue(ctx, jsvalue);
}

TEST(ReportFatalError, VMContextTest) {
  auto report_fatal_error = [](lepus::Context* ctx) {
    auto* arg = ctx->GetParam(0);
    if (!arg->IsString()) {
      ctx->ReportFatalError("args is not string", false,
                            error::E_MTS_RENDERER_FUNCTION_FATAL);
    }
    return *arg;
  };
  lepus::VMContext ctx;
  ctx.Initialize();
  lepus::RegisterCFunction(&ctx, "TestReportFatal", report_fatal_error);

  std::string src = R"(
   let result = false;
   let error = "";
    function ThrowFatalError() {
      // will throw exception
      return TestReportFatal(100);
    }

    function ThrowNormalException() {
        let obj = {}
        return obj(); // throw not a function exception
    }

    function End() {
      return TestReportFatal("end");
    }

    function reportTest() {
        try {
            ThrowNormalException();
        } catch (e) {

        }

        try {
          ThrowFatalError();
        } catch (e) {
          error = e;
        }

        try {
          ThrowNormalException();
        } catch (e) {}
    }
    reportTest();
    result = End();
  )";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx, src, "null");
  ctx.Execute();

  lepus::Value result, error;
  ctx.GetTopLevelVariableByName("result", &result);
  ctx.GetTopLevelVariableByName("error", &error);

  ASSERT_EQ(result, lepus::Value("end"));
  ASSERT_TRUE(error.StringView().find("args is not string") !=
              std::string_view::npos);
}

TEST(ReportFatalError, VMContextTest2) {
  auto report_fatal_error = [](lepus::Context* ctx) {
    auto* arg = ctx->GetParam(0);
    if (!arg->IsString()) {
      return ctx->ReportFatalError("args is not string", false,
                                   error::E_MTS_RENDERER_FUNCTION_FATAL);
    }
    return *arg;
  };
  lepus::VMContext ctx;
  ctx.Initialize();
  lepus::RegisterCFunction(&ctx, "TestReportFatal", report_fatal_error);

  std::string src = R"(
   let result = false;
   let error = "";
    function ThrowFatalError() {
      // will throw exception
      return TestReportFatal(100);
    }

    function ThrowNormalException() {
        let obj = {}
        return obj(); // throw not a function exception
    }

    function End() {
      return TestReportFatal("end");
    }

    function reportTest() {
        try {
            ThrowNormalException();
        } catch (e) {

        }
        ThrowFatalError();
    }
    reportTest();
    result = End();
  )";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx, src, "null");
  ctx.Execute();

  lepus::Value result, error;
  ctx.GetTopLevelVariableByName("result", &result);
  ctx.GetTopLevelVariableByName("error", &error);

  ASSERT_EQ(result, lepus::Value(false));
  ASSERT_TRUE(error == lepus::Value(""));
}

TEST(ReportFatalError, QuickContextTest) {
  auto report_fatal_error = [](LEPUSContext* ctx, LEPUSValue this_obj,
                               int32_t argc, LEPUSValue* argv) {
    if (!LEPUS_IsString(argv[0])) {
      return lepus::QuickContext::GetFromJsContext(ctx)
          ->ReportFatalError("args is not string", false,
                             error::E_MTS_RENDERER_FUNCTION_FATAL)
          .ToJSValue(ctx);
    }
    return LEPUS_DupValue(ctx, argv[0]);
  };

  lepus::QuickContext qctx;
  qctx.Initialize();
  qctx.RegisterGlobalFunction("TestReportFatal", report_fatal_error);

  std::string src = R"(
    result = false;
    error = undefined;
    function ThrowFatalError() {
      // will throw exception
      return TestReportFatal(100);
    }

    function ThrowNormalException() {
        let obj = {}
        return obj(); // throw not a function exception
    }

    function End() {
      return TestReportFatal("end");
    }

    function reportTest() {
        try {
            ThrowNormalException();
        } catch (e) {

        }
        if (enable_catch) {
          try {
            ThrowFatalError();
          } catch (e) {
            error = e;
          }
        } else {
          ThrowFatalError();
        }
    }
    reportTest();
    result = End();
  )";

  qctx.RegisterGlobalProperty("result", LEPUS_FALSE);
  qctx.RegisterGlobalProperty("error", LEPUS_UNDEFINED);
  qctx.RegisterGlobalProperty("enable_catch", LEPUS_TRUE);

  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "");
  lepus::Value ret;
  ASSERT_TRUE(qctx.Execute(&ret));
  ASSERT_FALSE(LEPUS_IsException(ret.WrapJSValue()));
  auto* ctx = qctx.context();
  auto error = lepus::Value(ctx, qctx.SearchGlobalData("error"));
  auto result = lepus::Value(ctx, qctx.SearchGlobalData("result"));
  ASSERT_TRUE(error.ToString().find("args is not string") != std::string::npos);
  ret = qctx.Call("reportTest");
  ASSERT_TRUE(ret.IsUndefined());

  ASSERT_EQ(result, lepus::Value("end"));
  qctx.SetGlobalData("enable_catch", lepus::Value(false));
  qctx.Execute(&ret);
  ASSERT_TRUE(LEPUS_IsException(ret.WrapJSValue()));
  error = lepus::Value(ctx, qctx.SearchGlobalData("erro"));
  result = lepus::Value(ctx, qctx.SearchGlobalData("result"));
  ASSERT_TRUE(error.IsJSUndefined());
  ASSERT_EQ(result, lepus::Value(false));
}

TEST(ReportFatalError, SetErrorCode) {
  auto report_fatal_error = [](LEPUSContext* ctx, LEPUSValue, int32_t,
                               LEPUSValue* argv) {
    if (!LEPUS_IsString(argv[0])) {
      return lepus::QuickContext::GetFromJsContext(ctx)
          ->ReportFatalError("arg is not string", false,
                             error::E_MTS_RENDERER_FUNCTION_FATAL)
          .ToJSValue(ctx);
    }
    return LEPUS_DupValue(ctx, argv[0]);
  };

  lepus::QuickContext ctx;
  ctx.Initialize();
  ctx.RegisterGlobalFunction("ReportFatal", report_fatal_error);

  std::string src = R"(
    function test() {
      ReportFatal("100");
      return ReportFatal(100);
    }
    
    function test2() {
      try {
        ReportFatal(100);
      } catch (e) {
        
      }
      throw new Error('testerror');
    }
  )";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx, src, "");
  ctx.Execute();
  auto* lepus_context = ctx.context();
  auto global = LEPUS_GetGlobalObject(lepus_context);
  auto caller = ctx.SearchGlobalData("test");
  auto ret = LEPUS_Call(lepus_context, caller, global, 0, nullptr);

  ASSERT_TRUE(LEPUS_IsException(ret));
  auto exception_val = LEPUS_GetException(lepus_context);
  ASSERT_EQ(lepus::LepusErrorHelper::GetErrorCode(lepus_context, exception_val),
            error::E_MTS_RENDERER_FUNCTION_FATAL);
  ASSERT_EQ(
      lepus::LepusErrorHelper::GetErrorMessage(lepus_context, exception_val),
      "Error: arg is not string");
  LEPUS_FreeValue(lepus_context, exception_val);
  LEPUS_FreeValue(lepus_context, caller);
  caller = ctx.SearchGlobalData("test2");
  ret = LEPUS_Call(lepus_context, caller, global, 0, nullptr);
  ASSERT_TRUE(LEPUS_IsException(ret));
  exception_val = LEPUS_GetException(lepus_context);
  ASSERT_EQ(lepus::LepusErrorHelper::GetErrorCode(lepus_context, exception_val),
            error::E_MTS_RUNTIME_ERROR);
  ASSERT_EQ(
      lepus::LepusErrorHelper::GetErrorMessage(lepus_context, exception_val),
      "Error: testerror");
  LEPUS_FreeValue(lepus_context, exception_val);
  LEPUS_FreeValue(lepus_context, caller);
  LEPUS_FreeValue(lepus_context, global);
}

TEST_F(LepusValueMethods, ToJSValueConvert) {
  lepus::Value dic{lepus::Dictionary::Create()};
  dic.SetProperty("prop", lepus::Value(lepus::CArray::Create()));
  auto* ctx = ctx_.context();
  auto jsvalue = dic.ToJSValue(ctx, true);
  auto js_prop = LEPUS_GetPropertyStr(ctx, jsvalue, "prop");
  ASSERT_TRUE(LEPUS_IsObject(js_prop));
  LEPUS_FreeValue(ctx, js_prop);
  LEPUS_FreeValue(ctx, jsvalue);
}

TEST_F(LepusValueMethods, ArrayToJSValueDeepConvert) {
  lepus::Value arr(lepus::CArray::Create());
  auto* ctx = ctx_.context();
  auto arr_jsvalue = arr.ToJSValue(ctx, true);
  ASSERT_EQ(lepus::Value(ctx, arr_jsvalue), arr);
  ASSERT_TRUE(LEPUS_IsArray(ctx, arr_jsvalue));
  ASSERT_TRUE(LEPUS_GetLength(ctx, arr_jsvalue) == 0);
  ASSERT_TRUE(
      lepus::Value(ctx, LEPUS_GetPropertyStr(ctx, arr_jsvalue, "length"))
          .Number() == 0);

  LEPUS_SetPropertyInt64(ctx, arr_jsvalue, 0, LEPUS_NewString(ctx, "ele0"));
  ASSERT_TRUE(LEPUS_GetLength(ctx, arr_jsvalue) == 1);
  ASSERT_TRUE(
      lepus::Value(ctx, LEPUS_GetPropertyStr(ctx, arr_jsvalue, "length"))
          .Number() == 1);
  LEPUS_SetPropertyInt64(ctx, arr_jsvalue, 99, LEPUS_NewString(ctx, "ele1"));
  ASSERT_TRUE(LEPUS_GetLength(ctx, arr_jsvalue) == 100);
  ASSERT_TRUE(
      lepus::Value(ctx, LEPUS_GetPropertyStr(ctx, arr_jsvalue, "length"))
          .Number() == 100);
  LEPUS_FreeValue(ctx, arr_jsvalue);

  arr.Array()->push_back(lepus::Value(lepus::Dictionary::Create()));
  arr.Array()->push_back(lepus::Value(lepus::CArray::Create()));
  arr.Array()->push_back(lepus::Value("hello"));

  arr_jsvalue = arr.ToJSValue(ctx, true);
  ASSERT_TRUE(lepus::Value(ctx, arr_jsvalue) == arr);
  ASSERT_EQ(LEPUS_GetLength(ctx, arr_jsvalue), 3);
  ASSERT_TRUE(
      lepus::Value(ctx, LEPUS_GetPropertyStr(ctx, arr_jsvalue, "length"))
          .Number() == 3);
  ASSERT_EQ(lepus::Value(ctx, LEPUS_GetPropertyUint32(ctx, arr_jsvalue, 2)),
            lepus::Value("hello"));

  LEPUS_FreeValue(ctx, arr_jsvalue);
}

TEST_F(LepusValueMethods, GetEmptyStringHashValue) {
  auto str1 = base::String("");
  auto str2 = base::String(std::string(""));
  auto str3 = base::String("", strlen(""));
  ASSERT_GT(str1.hash(), 0);
  ASSERT_EQ(str1.hash(), str2.hash());
  ASSERT_EQ(str2.hash(), str3.hash());
}

TEST_F(LepusValueMethods, TestUnhandleRejection) {
  std::string src = R"(
  function test() {
    const rejectedPromise = Promise.reject(
      new Error('This is an unhandled rejection')
    );
  }
  )";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();
  auto test = ctx_.GetAndCall("test", nullptr, 0);
  ASSERT_FALSE(LEPUS_IsException(test));
  ASSERT_FALSE(LEPUS_MoveUnhandledRejectionToException(ctx_.context()));
}

TEST_F(LepusValueMethods, WeakMapLepusValue) {
  std::string src = R"(
    assert("new weakmap");
    const wm = new WeakMap();
    assert(!wm.has(element));
    assert(wm.get(element) === undefined);

    assert("insert text element");
    assert(element);
    wm.set(element, "ELEMENT-1");
    assert(wm.has(element));
    assert(wm.get(element) === "ELEMENT-1");

    assert("query text element");
    assert(queriedElement);
    assert(wm.has(queriedElement));
    assert(wm.get(queriedElement) === "ELEMENT-1");

    assert("query view element");
    assert(anotherQueriedElement);
    assert(!wm.has(anotherQueriedElement));
    assert(wm.get(anotherQueriedElement) === undefined);

    assert("delete view element");
    assert(!wm.delete(anotherQueriedElement));
    assert(!wm.delete(""));

    assert("insert view element");
    assert(element);
    wm.set(element, "ELEMENT-2");
    assert(wm.has(element));
    assert(wm.get(element) === "ELEMENT-2");

    assert("delete text element");
    assert(wm.delete(queriedElement));
    assert(!wm.has(element));
    assert(!wm.has(queriedElement));
    assert(wm.get(element) === undefined);
    assert(wm.get(anotherQueriedElement) === undefined);
  )";

  auto element = lepus::Value(TestRefCountedClass::Create());
  auto queriedElement = element;
  auto anotherQueriedElement = lepus::Value(TestRefCountedClass::Create());
  ctx_.RegisterGlobalProperty("element", element.ToJSValue(ctx_.context()));
  ctx_.RegisterGlobalProperty("queriedElement",
                              queriedElement.ToJSValue(ctx_.context()));
  ctx_.RegisterGlobalProperty("anotherQueriedElement",
                              anotherQueriedElement.ToJSValue(ctx_.context()));
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();
}

TEST_F(LepusValueMethods, DISABLED_LepusValueJsObjectCacheConsistent) {
  std::string src = R"(
    function Process(rawData) {
      {
        const { ad_data } = rawData;
        const { prop } = ad_data;
        assert(prop == undefined);
      }
      rawData.ad_data.log_extra = {
        prop: 'hello world',
      };
      {
        const { ad_data } = rawData;
        const { log_extra } = ad_data;
        const { prop } = log_extra;
        assert(prop == "hello world");
      }
    }
    Process(rawData);
  )";
  lepus::Value rawData = lepus::Value(lepus::Dictionary::Create());
  lepus::Value ad_data = lepus::Value(lepus::Dictionary::Create());
  rawData.SetProperty("ad_data", ad_data);
  ctx_.RegisterGlobalProperty("rawData", rawData.ToJSValue(ctx_.context()));
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();
}

TEST_F(LepusValueMethods, RefCountedWeakMap) {
  auto element_value = lepus::Value(TestRefCountedClass::Create());
  LEPUSContext* ctx = ctx_.context();
  ctx_.RegisterGlobalProperty("rc_obj", element_value.ToJSValue(ctx, false));

  ctx_.RegisterGlobalProperty("rc_obj2", element_value.ToJSValue(ctx));
  std::string src = R"(
   {
    let weakMap = new WeakMap();
    assert(weakMap.has(rc_obj) == false);
    weakMap.set(rc_obj, "primjs");
    assert(weakMap.has(rc_obj));
    assert(weakMap.get(rc_obj) == "primjs");
    assert(weakMap.get(rc_obj2) == 'primjs');

    let weakMap2 = new WeakMap();
    weakMap2.set(rc_obj2, "primjs2");
    assert(weakMap2.has(rc_obj));
    assert(weakMap2.get(rc_obj) == 'primjs2');
    assert(weakMap.has(rc_obj));
    weakMap.delete(rc_obj);
    assert(weakMap.has(rc_obj) == false);
    assert(weakMap2.has(rc_obj));
    assert(weakMap2.get(rc_obj) == 'primjs2');
    assert(weakMap2.get(rc_obj2) == 'primjs2');
   }
  )";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();
  ASSERT_TRUE(element_value.RefCounted()->js_object_cache.has_value());
  ASSERT_TRUE(LEPUS_VALUE_IS_OBJECT(
      element_value.RefCounted()->js_object_cache->WrapJSValue()));

  element_value = lepus::Value();
  src = R"(
    var weakmap1 = new WeakMap();
    var weakmap2 = new WeakMap();
    {
      assert(weakmap1.has(rc_obj) == false);
      weakmap1.set(rc_obj, "primjs");
      assert(weakmap1.has(rc_obj));
      assert(weakmap1.get(rc_obj) == "primjs");
      assert(weakmap1.get(rc_obj2) == 'primjs');

      weakmap2.set(rc_obj2, "primjs2");
      assert(weakmap2.has(rc_obj));
      assert(weakmap2.get(rc_obj) == 'primjs2');
      assert(weakmap1.has(rc_obj));
      weakmap1.delete(rc_obj);
      assert(weakmap1.has(rc_obj) == false);
      assert(weakmap2.has(rc_obj));
      assert(weakmap2.get(rc_obj) == 'primjs2');
      assert(weakmap2.get(rc_obj2) == 'primjs2');
      rc_obj = null;
      rc_obj2 = null;
    }
  )";

  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();

  auto weakmap1 = ctx_.SearchGlobalData("weakmap1");
  auto weakmap2 = ctx_.SearchGlobalData("weakmap2");

  ASSERT_EQ(LEPUS_VALUE_GET_INT(js_map_get_size(ctx, weakmap1, 1 << 1)), 0);

  LEPUS_FreeValue(ctx, weakmap2);
  LEPUS_FreeValue(ctx, weakmap1);
}

TEST_F(LepusValueMethods, RefCountedWeakSet) {
  auto element_value = lepus::Value(TestRefCountedClass::Create());
  LEPUSContext* ctx = ctx_.context();
  ctx_.RegisterGlobalProperty("rc_obj", element_value.ToJSValue(ctx, false));

  ctx_.RegisterGlobalProperty("rc_obj2", element_value.ToJSValue(ctx));

  std::string src = R"(
    let weakset = new WeakSet();
    assert(weakset.has(rc_obj) == false);
    weakset.add(rc_obj, "primjs");
    assert(weakset.has(rc_obj));
    assert(weakset.has(rc_obj2));

    weakset.delete(rc_obj2);
    assert(weakset.has(rc_obj) == false);

    weakset.add(rc_obj2);
    let weakMap2 = new WeakSet();
    weakMap2.add(rc_obj2);
    assert(weakMap2.has(rc_obj));
    assert(weakMap2.has(rc_obj2));

    assert(weakset.has(rc_obj));
    weakset.delete(rc_obj);
    assert(weakset.has(rc_obj) == false);
    assert(weakset.has(rc_obj2) == false);
    assert(weakMap2.has(rc_obj));
  )";
  lepus::BytecodeGenerator::GenerateBytecode(&ctx_, src, "");
  ctx_.Execute();
}

TEST_F(LepusValueMethods, MarkConstJSValue) {
  auto table = lepus::Dictionary::Create();
  auto* ctx = ctx_.context();
  std::unordered_map<std::string, lepus::Value> tests = {
      {"number", lepus::Value(ctx, LEPUS_NewInt32(ctx, 100))},
      {"bool", lepus::Value(ctx, LEPUS_TRUE)},
      {"null", lepus::Value(ctx, LEPUS_NULL)},
      {"undef", lepus::Value(ctx, LEPUS_UNDEFINED)},
  };

  for (const auto& [key, val] : tests) {
    table->SetValue(key, val);
  }
  ASSERT_TRUE(table->MarkConst());
  ASSERT_TRUE(table->IsConst());

  for (const auto& [key, val] : tests) {
    ASSERT_EQ(table->GetValue(key), val);
    ASSERT_FALSE(table->GetValue(key).IsJSValue());
  }
}

TEST_F(LepusValueMethods, CallNotFunction) {
  ASSERT_TRUE(
      LEPUS_IsException(ctx_.Call("notfunction").ToJSValue(ctx_.context())));
}

}  // namespace checkUpdateTopLevelVariable

}  // namespace base
}  // namespace lynx
