// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>

#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

constexpr char foo[] = "foo";
constexpr char bar[] = "bar";
constexpr char bar_other[] = "bar_other";

namespace lynx {
namespace lepusbenchmark {

static const char* kCFuncEmptyFunc = "_EmptyFunc";
static const char* emptyFuncRetVal = "empty";

#define NORMAL_FUNCTION(name) \
  static lepus::Value name(lepus::Context* ctx, lepus::Value* argv, int argc)

class BenchmarkRendererFunctions {
 public:
  NORMAL_FUNCTION(emptyFunc) { return lepus::Value(emptyFuncRetVal); }
};

__attribute__((unused)) static void PrepareArgs(LEPUSContext* ctx,
                                                LEPUSValueConst* argv,
                                                lepus::Value* largv, int argc) {
  for (int i = 0; i < argc; i++) {
    new (largv + i) lepus::Value(ctx, std::move(argv[i]));
  }
}

static LEPUSValue emptyFuncNG(LEPUSContext* ctx, LEPUSValueConst this_val,
                              int argc, LEPUSValueConst* argv) {
  char args_buf[sizeof(lepus::Value) * argc];
  lepus::Value* largv = reinterpret_cast<lepus::Value*>(args_buf);
  PrepareArgs(ctx, argv, largv, argc);
  return BenchmarkRendererFunctions::emptyFunc(
             lepus::QuickContext::GetFromJsContext(ctx), largv, argc)
      .ToJSValue(ctx);
}

static void RegisterNGEmptyFunction(lepus::Context* ctx) {
  lepus::RegisterNGCFunction(ctx, kCFuncEmptyFunc, emptyFuncNG);
}

class LepusValueMethods {
 public:
  LepusValueMethods() = default;
  ~LepusValueMethods() = default;

  void SetUp() {
    ctx_.Initialize();
    v1_ = lepus::Value(lepus::Dictionary::Create());
    lepus::Value v("hello");
    v1_.SetProperty(base::String("prop1"), v);
  }

  void TearDown() {}

  lepus::QuickContext ctx_;
  lepus::Value v1_;
};

static void BM_ShadowEqualSameStringTable(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    auto lepus_map = lepus::Dictionary::Create();
    lepus::Value v(bar);
    lepus_map.get()->SetValue(base::String(foo), v);

    auto target_map = lepus::Dictionary::Create();
    target_map.get()->SetValue(base::String(foo), v);

    ASSERT_TRUE(!tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                               lepus::Value(target_map)));
  }
}

static void BM_ShadowEqualSameIntTable(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    auto lepus_map = lepus::Dictionary::Create();
    lepus_map.get()->SetValue(base::String(foo), lepus::Value(1));

    auto target_map = lepus::Dictionary::Create();
    target_map.get()->SetValue(base::String(foo), lepus::Value(1));

    ASSERT_TRUE(!tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                               lepus::Value(target_map)));
  }
}

static void BM_ShadowEqualDiffSameStringTable(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    auto lepus_map = lepus::Dictionary::Create();
    lepus::Value bar_value(bar);
    lepus_map.get()->SetValue(base::String(foo), bar_value);

    auto target_map = lepus::Dictionary::Create();
    lepus::Value bar_other_value(bar_other);
    target_map.get()->SetValue(base::String(foo), bar_other_value);

    ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                              lepus::Value(target_map)));
  }
}

static void BM_ShadowEqualHasNewKey(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    auto lepus_map = lepus::Dictionary::Create();
    lepus::Value bar_value(bar);
    lepus_map.get()->SetValue(base::String(foo), bar_value);

    auto target_map = lepus::Dictionary::Create();
    target_map.get()->SetValue(base::String(bar_other), bar_value);

    ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                              lepus::Value(target_map)));
  }
}

static void BM_ShadowEqualSameTableValue(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    auto lepus_map = lepus::Dictionary::Create();
    lepus::Value dic = lepus::Value(lepus::Dictionary::Create());
    lepus_map.get()->SetValue(base::String(foo), dic);

    auto target_map = lepus::Dictionary::Create();
    target_map.get()->SetValue(base::String(foo), dic);

    ASSERT_TRUE(tasm::CheckTableShadowUpdated(lepus::Value(lepus_map),
                                              lepus::Value(target_map)));
  }
}

static void BM_ValueMethodsValuePrint(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    std::ostringstream ss_;
    ss_ << test.v1_;
    ASSERT_TRUE(ss_.str() == "{prop1:hello}");
  }
}

static void BM_ValueMethodsIsEqual(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    lepus::Value v2(test.ctx_.context(),
                    test.v1_.ToJSValue(test.ctx_.context()));
    ASSERT_TRUE(test.v1_.IsEqual(v2));
  }
}

static void BM_ValueMethodsClone(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    lepus::Value v2 = lepus::Value::Clone(test.v1_);
    ASSERT_TRUE(v2.IsEqual(test.v1_));
  }
}

static void BM_ValueMethodsSetGetProperty1(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    lepus::Value v2 = lepus::Value::Clone(test.v1_);
    v2.SetProperty(base::String("prop1"), lepus::Value("world"));
    ASSERT_TRUE(
        v2.GetProperty(base::String("prop1")).IsEqual(lepus::Value("world")));
  }
}

static void BM_ValueMethodsSetGetProperty2(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    LEPUSContext* ctx = test.ctx_.context();

    lepus::Value lepusv1(ctx, test.v1_.ToJSValue(ctx));

    lepusv1.SetProperty(
        "prop",
        lepus::Value(ctx, lepus::LEPUSValueHelper::NewString(ctx, "world")));

    lepus::Value v2 = lepus::Value::Clone(test.v1_);
    v2.SetProperty(base::String("prop"), lepus::Value("world"));

    ASSERT_TRUE(v2.IsEqual(lepusv1));
  }
}

static void BM_ValueMethodsString(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    LEPUSContext* ctx = test.ctx_.context();
    LEPUSValue lepusv1 = lepus::LEPUSValueHelper::NewString(ctx, "hello world");
    lepus::Value v2 = lepus::Value(ctx, lepusv1);
    base::String str = v2.String();
    ASSERT_TRUE(str.IsEqual(base::String("hello world")));
    LEPUS_FreeValue(ctx, lepusv1);
  }
}

static void BM_ValueMethodsIterator(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    std::string str;
    std::string* pstr = &str;
    tasm::ForEachLepusValue(
        test.v1_, [pstr](const lepus::Value& key, const lepus::Value& val) {
          *pstr = *pstr + key.StdString() + val.StdString();
        });
    ASSERT_TRUE(str == "prop1hello");
  }
}

static void BM_ValueMethodsSetSelf(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
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
}

static void BM_ValueMethodsTestLepusJSValue(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    LepusValueMethods test;
    test.SetUp();
    state.ResumeTiming();
    lepus::Value array = lepus::Value(lepus::CArray::Create());
    lepus::Value val = lepus::Value(lepus::Dictionary::Create());
    val.Table()->SetValue("array", array);
    array.SetProperty(0, lepus::Value("0"));
    array.SetProperty(1, lepus::Value("1"));
    array.SetProperty(2, lepus::Value("2"));

    lepus::Value val2 = lepus::Value(lepus::Dictionary::Create());
    LEPUSContext* ctx = test.ctx_.context();
    LEPUSValue array_jsvalue = array.ToJSValue(ctx);
    lepus::Value array_v(ctx, array_jsvalue);
    val2.Table()->SetValue("array", array_v);

    ASSERT_TRUE(val == val2);
    ASSERT_TRUE(val2 == val);
    state.PauseTiming();
    LEPUS_FreeValue(ctx, array_jsvalue);

    lepus::Value val3 = lepus::Value(lepus::Dictionary::Create());
    lepus::Value array_deeptojs(test.ctx_.context(),
                                array.ToJSValue(test.ctx_.context(), true));
    val3.Table()->SetValue("array", array_deeptojs);
    ASSERT_TRUE(val == val3);
    state.ResumeTiming();
  }
}

static void BM_ValueMethodsTestLepusValueOperatorEqual(
    benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = "function entry() {}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    state.ResumeTiming();
    lepus::Value left;
    LEPUSContext* ctx = qctx.context();
    left = lepus::Value(ctx, qctx.SearchGlobalData("entry"));

    lepus::Value right;
    ASSERT_FALSE(left == right);

    lepus::Value right2 =
        lepus::Value(ctx, qctx.SearchGlobalData("entry")).ToLepusValue();
    ASSERT_TRUE(left.ToLepusValue() == right2);
  }
}

static void BM_ValueMethodsTestToLepusValue(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = "let obj = {a: 3}; let obj2 = {b: 3};";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.1");
    qctx.Execute();

    state.ResumeTiming();
    lepus::Value obj =
        lepus::Value(qctx.context(), qctx.SearchGlobalData("obj"))
            .ToLepusValue();
    LEPUSValue obj_ref = obj.ToJSValue(qctx.context(), false);
    auto obj2_wrap =
        lepus::Value(qctx.context(), qctx.SearchGlobalData("obj2"));
    obj2_wrap.SetProperty("test", lepus::Value(qctx.context(), obj_ref));

    // get a copy.
    lepus::Value obj_result = obj2_wrap.ToLepusValue();

    auto p1 = obj_result.Table()->GetValue("test").Table();
    auto p2 = obj.Table();
    ASSERT_TRUE(p1 == p2);
    LEPUS_FreeValue(qctx.context(), obj_ref);
  }
}

static void BM_LepusWrapDestruct(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    state.ResumeTiming();
    lepus::QuickContext qctx;
    lepus::Value number = lepus::Value(1);
    lepus::Value number_wrap(qctx.context(), number.ToJSValue(qctx.context()));
    lepus::Value array = lepus::Value(lepus::CArray::Create());

    lepus::Value ref(qctx.context(), lepus::LEPUSValueHelper::CreateLepusRef(
                                         qctx.context(), array.Array().get(),
                                         lepus::Value_Array));
  }
}

static void BM_TestDefaultStackSize(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = "function entry(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.1");
    qctx.Execute();
    state.ResumeTiming();
    LEPUSValue res = qctx.GetAndCall("entry", nullptr, 0);
    LEPUSValue expected_res = LEPUS_MKVAL(LEPUS_TAG_INT, 2);
    EXPECT_EQ(LEPUS_VALUE_GET_TAG(res), LEPUS_VALUE_GET_TAG(expected_res));
    EXPECT_EQ(LEPUS_VALUE_GET_INT64(res), LEPUS_VALUE_GET_INT64(expected_res));
  }
}

static void BM_TestSetNormalStackSize(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetStackSize(1024 * 1024);
    std::string src = "function sayHello(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.1");
    qctx.Execute();
    state.ResumeTiming();
    LEPUSValue res = qctx.GetAndCall("sayHello", nullptr, 0);
    LEPUSValue expected_res = LEPUS_MKVAL(LEPUS_TAG_INT, 2);
    EXPECT_EQ(LEPUS_VALUE_GET_TAG(res), LEPUS_VALUE_GET_TAG(expected_res));
    EXPECT_EQ(LEPUS_VALUE_GET_INT64(res), LEPUS_VALUE_GET_INT64(expected_res));
  }
}

static void BM_FromLepusDeepConvert(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::VMContext vctx;
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, "2.0");
    vctx.Execute();
    lepus::Value* obj_ptr = new lepus::Value();
    bool ret = vctx.GetTopLevelVariableByName("obj", obj_ptr);
    ASSERT_TRUE(ret);
    lepus::QuickContext qctx;
    state.ResumeTiming();
    LEPUSValue obj_ref = obj_ptr->ToJSValue(qctx.context(), true);
    state.PauseTiming();
    LEPUS_FreeValue(qctx.context(), obj_ref);
    state.ResumeTiming();
  }
}

static void BM_FromLepusShallowConvert(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::VMContext vctx;
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, "2.0");
    vctx.Execute();
    lepus::Value* obj_ptr = new lepus::Value();
    bool ret = vctx.GetTopLevelVariableByName("obj", obj_ptr);
    ASSERT_TRUE(ret);
    lepus::QuickContext qctx;
    state.ResumeTiming();
    LEPUSValue obj_ref = obj_ptr->ToJSValue(qctx.context());
    state.PauseTiming();
    LEPUS_FreeValue(qctx.context(), obj_ref);
    state.ResumeTiming();
  }
}

static void BM_ToLepusValueDeepConvert(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    lepus::Value obj_wrap_lepus_value(qctx.context(),
                                      qctx.SearchGlobalData("obj"));
    state.ResumeTiming();
    lepus::Value obj = obj_wrap_lepus_value.ToLepusValue();
  }
}

static void BM_ToLepusValueShallowConvert(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    LEPUSValue obj_wrap = qctx.SearchGlobalData("obj");
    state.ResumeTiming();
    lepus::Value obj = lepus::Value(qctx.context(), obj_wrap);
    state.PauseTiming();
    LEPUS_FreeValue(qctx.context(), obj_wrap);
    state.ResumeTiming();
  }
}

static void BM_ValueTestCloneBigObject(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    lepus::Value obj =
        lepus::Value(qctx.context(), qctx.SearchGlobalData("obj"))
            .ToLepusValue();
    state.ResumeTiming();
    lepus::Value obj_clone = lepus::Value::Clone(obj);
  }
}

static void BM_TestEmptyRenderNGFunction(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    RegisterNGEmptyFunction(&qctx);
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    state.ResumeTiming();
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();

    lepus::Value ret =
        qctx.Call(kCFuncEmptyFunc,
                  lepus::Value(qctx.context(), qctx.SearchGlobalData("obj")));
    lepus::Value emp = lepus::Value(emptyFuncRetVal);
    ASSERT_TRUE(emp == ret);
  }
}

static void BM_TestCollectLeak(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::VMContext vctx;
    std::string src = lepus::readFile("./benchmark_test_files/leak_objects.js");
    lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, "2.0");
    vctx.Execute();
    lepus::Value* obj_ptr = new lepus::Value();
    bool ret = vctx.GetTopLevelVariableByName("obj", obj_ptr);
    ASSERT_TRUE(ret);
    lepus::Dictionary* obj =
        reinterpret_cast<lepus::Dictionary*>(obj_ptr->Ptr());
    lepus::Value* value_arr[10000];
    lepus::QuickContext qctx;
    for (int i = 0; i < 10000; i++) {
      value_arr[i] = const_cast<lepus::Value*>(
          &(obj->GetValue("ele" + std::to_string(i))));
      if (i % 1000 == 0) {
        LEPUSValue obj_ref = value_arr[i]->ToJSValue(qctx.context());
        state.ResumeTiming();
        lepus::Value ref_value(qctx.context(), obj_ref);
        state.PauseTiming();
        LEPUS_FreeValue(qctx.context(), obj_ref);
      }
    }
    state.ResumeTiming();
  }
}

static void BM_ArrayPushBack(benchmark::State& state) {
  for (auto _ : state) {
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    array->reserve(30000);
    for (int i = 0; i < 10000; i++) {
      array->push_back(lepus::Value(i));
    }
    for (int i = 0; i < 10000; i++) {
      array->push_back(lepus::Value(sValue));
    }
    for (int i = 0; i < 10000; i++) {
      array->push_back(lepus::Value(child_array));
    }
  }
}

static void BM_ArrayEmplaceBack(benchmark::State& state) {
  for (auto _ : state) {
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    array->reserve(30000);
    for (int i = 0; i < 10000; i++) {
      array->emplace_back(i);
    }
    for (int i = 0; i < 10000; i++) {
      array->emplace_back(sValue);
    }
    for (int i = 0; i < 10000; i++) {
      array->emplace_back(child_array);
    }
  }
}

static void BM_TableSetValueNoEmplace(benchmark::State& state) {
  std::vector<base::String> keys;
  keys.reserve(30000);
  for (int i = 0; i < 30000; i++) {
    keys.emplace_back(std::to_string(i) + "_key");
  }

  for (auto _ : state) {
    auto dict = lepus::Dictionary::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], lepus::Value(i));
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i + 10000], lepus::Value(sValue));
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i + 20000], lepus::Value(child_array));
    }
  }
}

static void BM_TableSetValueEmplace(benchmark::State& state) {
  std::vector<base::String> keys;
  keys.reserve(30000);
  for (int i = 0; i < 30000; i++) {
    keys.emplace_back(std::to_string(i) + "_key");
  }

  for (auto _ : state) {
    auto dict = lepus::Dictionary::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], i);
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i + 10000], sValue);
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i + 20000], child_array);
    }
  }
}

static void BM_TableSetValueNoEmplaceKeyConflict(benchmark::State& state) {
  std::vector<base::String> keys;
  keys.reserve(10000);
  for (int i = 0; i < 10000; i++) {
    keys.emplace_back(std::to_string(i) + "_key");
  }

  for (auto _ : state) {
    auto dict = lepus::Dictionary::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], lepus::Value(i));
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], lepus::Value(sValue));
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], lepus::Value(child_array));
    }
  }
}

static void BM_TableSetValueEmplaceKeyConflict(benchmark::State& state) {
  std::vector<base::String> keys;
  keys.reserve(10000);
  for (int i = 0; i < 10000; i++) {
    keys.emplace_back(std::to_string(i) + "_key");
  }

  for (auto _ : state) {
    auto dict = lepus::Dictionary::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], i);
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], sValue);
    }
    for (int i = 0; i < 10000; i++) {
      dict->SetValue(keys[i], child_array);
    }
  }
}

BENCHMARK(BM_ShadowEqualSameStringTable);
BENCHMARK(BM_ShadowEqualSameIntTable);
BENCHMARK(BM_ShadowEqualDiffSameStringTable);
BENCHMARK(BM_ShadowEqualHasNewKey);
BENCHMARK(BM_ShadowEqualSameTableValue);
BENCHMARK(BM_ValueMethodsValuePrint);
BENCHMARK(BM_ValueMethodsIsEqual);
BENCHMARK(BM_ValueMethodsClone);
BENCHMARK(BM_ValueMethodsSetGetProperty1);
BENCHMARK(BM_ValueMethodsSetGetProperty2);
BENCHMARK(BM_ValueMethodsString);
BENCHMARK(BM_ValueMethodsIterator);
BENCHMARK(BM_ValueMethodsSetSelf);
BENCHMARK(BM_ValueMethodsTestLepusJSValue);
BENCHMARK(BM_ValueMethodsTestLepusValueOperatorEqual);
BENCHMARK(BM_ValueMethodsTestToLepusValue);
BENCHMARK(BM_LepusWrapDestruct);
BENCHMARK(BM_TestDefaultStackSize);
BENCHMARK(BM_TestSetNormalStackSize);
BENCHMARK(BM_FromLepusDeepConvert);
BENCHMARK(BM_FromLepusShallowConvert);
BENCHMARK(BM_ToLepusValueDeepConvert);
BENCHMARK(BM_ToLepusValueShallowConvert);
BENCHMARK(BM_ValueTestCloneBigObject);
BENCHMARK(BM_TestEmptyRenderNGFunction);
BENCHMARK(BM_TestCollectLeak)->Iterations(5);
BENCHMARK(BM_ArrayPushBack);
BENCHMARK(BM_ArrayEmplaceBack);
BENCHMARK(BM_TableSetValueNoEmplace);
BENCHMARK(BM_TableSetValueEmplace);
BENCHMARK(BM_TableSetValueNoEmplaceKeyConflict);
BENCHMARK(BM_TableSetValueEmplaceKeyConflict);
}  // namespace lepusbenchmark
}  // namespace lynx
