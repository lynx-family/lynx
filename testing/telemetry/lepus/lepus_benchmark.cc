// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

constexpr char foo[] = "foo";
constexpr char bar[] = "bar";
constexpr char bar_other[] = "bar_other";

namespace lynx {
namespace lepusbenchmark {

static const std::string sdk_version = "2.0";

class MockContextDelegate : public lepus::Context::Delegate {
 public:
  const std::string& TargetSdkVersion() override { return sdk_version; }
  void ReportError(base::LynxError error) override {
    LOGE("lepus_benchmark_error: " << error.error_message_);
  }
  void OnBTSConsoleEvent(const std::string& func_name,
                         const std::string& args) override {}
  void ReportGCTimingEvent(const char* start, const char* end) override {}
  void OnRuntimeGC(
      std::unordered_map<std::string, std::string> mem_info) override {}

  fml::RefPtr<fml::TaskRunner> GetLepusTimedTaskRunner() override {
    return nullptr;
  }

  void OnScriptingStart() override {}
  void OnScriptingEnd() override {}
};

static MockContextDelegate mock_delegate_instance;

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
    LEPUSValue val = argv[i];
    if (LEPUS_IsLepusRef(val)) {
      new (largv + i) lepus::Value(
          lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx, val));
    } else {
      new (largv + i)
          lepus::Value(lepus::QuickContext::GetContextCellFromCtx(ctx)->env_,
                       LEPUS_VALUE_GET_INT64(val),
                       lepus::LEPUSValueHelper::CalculateTag(val));
    }
  }
}

static LEPUSValue emptyFuncNG(LEPUSContext* ctx, LEPUSValueConst this_val,
                              int argc, LEPUSValueConst* argv) {
  char args_buf[sizeof(lepus::Value) * argc];
  lepus::Value* largv = reinterpret_cast<lepus::Value*>(args_buf);
  PrepareArgs(ctx, argv, largv, argc);
  return lepus::LEPUSValueHelper::ToJsValue(
      ctx, BenchmarkRendererFunctions::emptyFunc(
               lepus::QuickContext::GetFromJsContext(ctx), largv, argc));
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

static void BM_ValueCopyConstruct(benchmark::State& state) {
  std::vector<lepus::Value> source_arr;
  source_arr.emplace_back((int)5);
  source_arr.emplace_back((uint64_t)0xFFFFFFFFFF);
  source_arr.emplace_back("abcdefg");
  source_arr.emplace_back(lepus::Dictionary::Create());
  source_arr.emplace_back((void*)&BM_ValueCopyConstruct);
  source_arr.emplace_back(true);
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr;
    dest_arr.reserve(source_arr.size() * 1000);
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr.emplace_back(source_arr[j]);
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueCopyAssign(benchmark::State& state) {
  std::vector<lepus::Value> source_arr;
  source_arr.emplace_back((int)5);
  source_arr.emplace_back((uint64_t)0xFFFFFFFFFF);
  source_arr.emplace_back("abcdefg");
  source_arr.emplace_back(lepus::Dictionary::Create());
  source_arr.emplace_back((void*)&BM_ValueCopyAssign);
  source_arr.emplace_back(true);
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr;
    dest_arr.resize(source_arr.size() * 1000);  // default constructed
    // Set some values as refcounted.
    for (size_t i = 0; i < dest_arr.size(); i++) {
      if (i % 20 == 0) {
        dest_arr[i] = lepus::Value(lepus::Dictionary::Create());
      }
    }
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr[i * source_arr.size() + j] = source_arr[j];
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueMoveConstruct(benchmark::State& state) {
  std::vector<lepus::Value> source_arr;
  source_arr.emplace_back((int)5);
  source_arr.emplace_back((uint64_t)0xFFFFFFFFFF);
  source_arr.emplace_back("abcdefg");
  source_arr.emplace_back(lepus::Dictionary::Create());
  source_arr.emplace_back((void*)&BM_ValueMoveConstruct);
  source_arr.emplace_back(true);
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr1;
    std::vector<lepus::Value> dest_arr2;
    dest_arr1.reserve(source_arr.size() * 1000);
    dest_arr2.reserve(source_arr.size() * 1000);
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr1.emplace_back(source_arr[j]);
      }
    }
    state.ResumeTiming();
    for (size_t i = 0; i < dest_arr1.size(); i++) {
      dest_arr2.emplace_back(std::move(dest_arr1[i]));
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueConstructFromDict(benchmark::State& state) {
  std::vector<fml::RefPtr<lepus::Dictionary>> source_arr;
  for (int i = 0; i < 20; i++) {
    source_arr.emplace_back(lepus::Dictionary::Create());
  }
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr;
    dest_arr.reserve(source_arr.size() * 1000);
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr.emplace_back(source_arr[j]);
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueDictVisit(benchmark::State& state) {
  std::vector<lepus::Value> arr;
  for (int j = 0; j < 2000; j++) {
    arr.emplace_back(lepus::Dictionary::Create());
  }
  for (auto _ : state) {
    for (size_t i = 0; i < arr.size(); i++) {
      auto ptr = arr[i].Table();
      benchmark::DoNotOptimize(ptr);
    }
  }
}

static void BM_ValueConstructFromArray(benchmark::State& state) {
  std::vector<fml::RefPtr<lepus::CArray>> source_arr;
  for (int i = 0; i < 20; i++) {
    source_arr.emplace_back(lepus::CArray::Create());
  }
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr;
    dest_arr.reserve(source_arr.size() * 1000);
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr.emplace_back(source_arr[j]);
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueArrayVisit(benchmark::State& state) {
  std::vector<lepus::Value> arr;
  for (int j = 0; j < 2000; j++) {
    arr.emplace_back(lepus::CArray::Create());
  }
  for (auto _ : state) {
    for (size_t i = 0; i < arr.size(); i++) {
      auto ptr = arr[i].Array();
      benchmark::DoNotOptimize(ptr);
    }
  }
}

static void BM_ValueConstructCopyString(benchmark::State& state) {
  std::vector<base::String> source_arr;
  for (int i = 0; i < 26; i++) {
    source_arr.emplace_back(std::string(10, 'a' + i));
  }
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr;
    dest_arr.reserve(source_arr.size() * 1000);
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr.emplace_back(source_arr[j]);
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueConstructMoveString(benchmark::State& state) {
  std::vector<base::String> source_arr;
  for (int i = 0; i < 26; i++) {
    source_arr.emplace_back(std::string(10, 'a' + i));
  }
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<base::String> temp_arr;
    std::vector<lepus::Value> dest_arr;
    temp_arr.reserve(source_arr.size() * 1000);
    dest_arr.reserve(source_arr.size() * 1000);
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        temp_arr.emplace_back(source_arr[j]);
      }
    }
    state.ResumeTiming();
    for (size_t i = 0; i < temp_arr.size(); i++) {
      dest_arr.emplace_back(std::move(temp_arr[i]));
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_ValueStringVisit(benchmark::State& state) {
  std::vector<const lepus::Value> arr;
  for (int j = 0; j < 1000; j++) {
    for (int i = 0; i < 26; i++) {
      arr.emplace_back(std::string(10, 'a' + i));
    }
  }
  volatile int64_t total = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < arr.size(); i++) {
      total += arr[i].String().length();
    }
  }
}

static void BM_ValueStringRValueVisit(benchmark::State& state) {
  std::vector<lepus::Value> arr;
  for (int j = 0; j < 1000; j++) {
    for (int i = 0; i < 26; i++) {
      arr.emplace_back(std::string(10, 'a' + i));
    }
  }
  volatile int64_t total = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < arr.size(); i++) {
      total += std::move(arr[i]).String().length();
    }
  }
}

static void BM_ValueStringStdStringVisit(benchmark::State& state) {
  std::vector<lepus::Value> arr;
  for (int j = 0; j < 1000; j++) {
    for (int i = 0; i < 26; i++) {
      arr.emplace_back(std::string(10, 'a' + i));
    }
  }
  volatile int64_t total = 0;
  for (auto _ : state) {
    for (size_t i = 0; i < arr.size(); i++) {
      total += arr[i].StdString().length();
    }
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
    lepus::Value v2 =
        MK_JS_LEPUS_VALUE_WITH_CONVERT(test.ctx_.context(), test.v1_, false);
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

    lepus::Value lepusv1 = MK_JS_LEPUS_VALUE_WITH_CONVERT(ctx, test.v1_, false);

    lepusv1.SetProperty(
        "prop", MK_JS_LEPUS_VALUE(
                    ctx, lepus::LEPUSValueHelper::NewString(ctx, "world")));

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
    lepus::Value v2 = MK_JS_LEPUS_VALUE(ctx, lepusv1);
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
    LEPUSValue array_jsvalue = lepus::LEPUSValueHelper::ToJsValue(ctx, array);
    lepus::Value array_v = MK_JS_LEPUS_VALUE(ctx, array_jsvalue);
    val2.Table()->SetValue("array", array_v);

    ASSERT_TRUE(val == val2);
    ASSERT_TRUE(val2 == val);
    state.PauseTiming();
    LEPUS_FreeValue(ctx, array_jsvalue);

    lepus::Value val3 = lepus::Value(lepus::Dictionary::Create());
    lepus::Value array_deeptojs =
        MK_JS_LEPUS_VALUE_WITH_CONVERT(test.ctx_.context(), array, true);
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
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = "function entry() {}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
    qctx.Execute();
    state.ResumeTiming();
    lepus::Value left;
    LEPUSContext* ctx = qctx.context();
    left = MK_JS_LEPUS_VALUE(ctx, qctx.SearchGlobalData("entry"));

    lepus::Value right;
    ASSERT_FALSE(left == right);

    lepus::Value right2 =
        MK_JS_LEPUS_VALUE(ctx, qctx.SearchGlobalData("entry")).ToLepusValue();
    ASSERT_TRUE(left.ToLepusValue() == right2);
  }
}

static void BM_ValueMethodsTestToLepusValue(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = "let obj = {a: 3}; let obj2 = {b: 3};";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
    qctx.Execute();

    state.ResumeTiming();
    lepus::Value obj =
        MK_JS_LEPUS_VALUE(qctx.context(), qctx.SearchGlobalData("obj"))
            .ToLepusValue();
    LEPUSValue obj_ref =
        lepus::LEPUSValueHelper::ToJsValue(qctx.context(), obj, false);
    auto obj2_wrap =
        MK_JS_LEPUS_VALUE(qctx.context(), qctx.SearchGlobalData("obj2"));
    obj2_wrap.SetProperty("test", MK_JS_LEPUS_VALUE(qctx.context(), obj_ref));

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
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    lepus::Value number = lepus::Value(1);
    lepus::Value number_wrap =
        MK_JS_LEPUS_VALUE_WITH_CONVERT(qctx.context(), number, false);
    lepus::Value array = lepus::Value(lepus::CArray::Create());

    lepus::Value ref = MK_JS_LEPUS_VALUE(
        qctx.context(),
        lepus::LEPUSValueHelper::CreateLepusRef(
            qctx.context(), array.Array().get(), lepus::Value_Array));
  }
}

static void BM_TestDefaultStackSize(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = "function entry(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
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
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    qctx.SetStackSize(1024 * 1024);
    std::string src = "function sayHello(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
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
    vctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
    vctx.Execute();
    lepus::Value* obj_ptr = new lepus::Value();
    bool ret = vctx.GetTopLevelVariableByName("obj", obj_ptr);
    ASSERT_TRUE(ret);
    lepus::QuickContext qctx;
    state.ResumeTiming();
    LEPUSValue obj_ref =
        lepus::LEPUSValueHelper::ToJsValue(qctx.context(), *obj_ptr, true);
    state.PauseTiming();
    LEPUS_FreeValue(qctx.context(), obj_ref);
    delete obj_ptr;
  }
}

static void BM_ToLepusValueDeepConvert(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
    qctx.Execute();
    lepus::Value obj_wrap_lepus_value =
        MK_JS_LEPUS_VALUE(qctx.context(), qctx.SearchGlobalData("obj"));
    state.ResumeTiming();
    lepus::Value obj = obj_wrap_lepus_value.ToLepusValue(true);
    state.PauseTiming();  // pause for not counting time of destruction of
                          // obj_wrap_lepus_value and obj
  }
}

static void BM_ValueTestCloneBigObject(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
    qctx.Execute();
    lepus::Value obj =
        MK_JS_LEPUS_VALUE(qctx.context(), qctx.SearchGlobalData("obj"))
            .ToLepusValue();
    state.ResumeTiming();
    lepus::Value obj_clone = lepus::Value::Clone(obj);
  }
}

static void BM_TestEmptyRenderNGFunction(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::QuickContext qctx;
    qctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    RegisterNGEmptyFunction(&qctx);
    std::string src = lepus::readFile("./benchmark_test_files/big_object.js");
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, sdk_version);
    qctx.Execute();
    state.ResumeTiming();
    lepus::Value ret = qctx.Call(
        kCFuncEmptyFunc,
        MK_JS_LEPUS_VALUE(qctx.context(), qctx.SearchGlobalData("obj")));
    state.PauseTiming();  // ignore destruction time of local objects.
  }
}

static void BM_TestCollectLeak(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    lepus::VMContext vctx;
    vctx.SetGlobalData(
        "$kTemplateAssembler",
        lepus::Value((void*)&mock_delegate_instance));  // to mute error log
    std::string src = lepus::readFile("./benchmark_test_files/leak_objects.js");
    lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
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
          &(*obj->GetValue("ele" + std::to_string(i))));
      if (i % 1000 == 0) {
        LEPUSValue obj_ref =
            lepus::LEPUSValueHelper::ToJsValue(qctx.context(), *value_arr[i]);
        state.ResumeTiming();
        lepus::Value ref_value = MK_JS_LEPUS_VALUE(qctx.context(), obj_ref);
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

static void BM_ArrayPushBackNoReserve(benchmark::State& state) {
  for (auto _ : state) {
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
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

static void BM_ArrayEmplaceBackNoReserve(benchmark::State& state) {
  for (auto _ : state) {
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
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

// Test array of small size
static void BM_ArrayPushBackNoReserveSmall(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    base::Vector<fml::RefPtr<lepus::CArray>> arrays;
    for (int i = 0; i < 500; i++) {
      arrays.push_back(lepus::CArray::Create());
    }
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");

    state.ResumeTiming();
    for (int i = 0; i < 500; i++) {
      auto& array = arrays[i];
      for (int i = 0; i < 10; i++) {
        array->push_back(lepus::Value(i));
      }
      for (int i = 0; i < 10; i++) {
        array->push_back(lepus::Value(sValue));
      }
      for (int i = 0; i < 10; i++) {
        array->push_back(lepus::Value(child_array));
      }
    }
    state.PauseTiming();
  }
}

static void BM_ArrayEmplaceBackNoReserveSmall(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    base::Vector<fml::RefPtr<lepus::CArray>> arrays;
    for (int i = 0; i < 500; i++) {
      arrays.push_back(lepus::CArray::Create());
    }
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");

    state.ResumeTiming();
    for (int i = 0; i < 500; i++) {
      auto& array = arrays[i];
      for (int i = 0; i < 10; i++) {
        array->emplace_back(i);
      }
      for (int i = 0; i < 10; i++) {
        array->emplace_back(sValue);
      }
      for (int i = 0; i < 10; i++) {
        array->emplace_back(child_array);
      }
    }
    state.PauseTiming();
  }
}

static void BM_ArrayInsert(benchmark::State& state) {
  for (auto _ : state) {
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    array->reserve(2000);
    for (int i = 0; i < 300; i++) {
      array->Insert(0, lepus::Value(i));
    }
    for (int i = 0; i < 300; i++) {
      array->Insert(0, lepus::Value(sValue));
    }
    for (int i = 0; i < 300; i++) {
      array->Insert(0, lepus::Value(child_array));
    }
  }
}

static void BM_ArrayErase(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto array = lepus::CArray::Create();
    auto child_array = lepus::CArray::Create();
    base::String sValue("benchmark");
    array->reserve(2000);
    for (int i = 0; i < 500; i++) {
      array->emplace_back(i);
    }
    for (int i = 0; i < 500; i++) {
      array->emplace_back(sValue);
    }
    for (int i = 0; i < 500; i++) {
      array->emplace_back(child_array);
    }
    state.ResumeTiming();
    while (array->size() > 0) {
      array->Erase(0);
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
BENCHMARK(BM_ValueCopyConstruct);
BENCHMARK(BM_ValueCopyAssign);
BENCHMARK(BM_ValueMoveConstruct);
BENCHMARK(BM_ValueConstructFromDict);
// BENCHMARK(BM_ValueDictVisit); // Not stable
BENCHMARK(BM_ValueConstructFromArray);
// BENCHMARK(BM_ValueArrayVisit); // Not stable
BENCHMARK(BM_ValueConstructCopyString);
BENCHMARK(BM_ValueConstructMoveString);
BENCHMARK(BM_ValueStringVisit);
BENCHMARK(BM_ValueStringRValueVisit);
BENCHMARK(BM_ValueStringStdStringVisit);
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
BENCHMARK(BM_ToLepusValueDeepConvert);
BENCHMARK(BM_ValueTestCloneBigObject);
BENCHMARK(BM_TestEmptyRenderNGFunction);
BENCHMARK(BM_TestCollectLeak)->Iterations(5);
BENCHMARK(BM_ArrayPushBack);
BENCHMARK(BM_ArrayEmplaceBack);
BENCHMARK(BM_ArrayPushBackNoReserve);
BENCHMARK(BM_ArrayEmplaceBackNoReserve);
BENCHMARK(BM_ArrayPushBackNoReserveSmall);
BENCHMARK(BM_ArrayEmplaceBackNoReserveSmall);
BENCHMARK(BM_ArrayInsert);
BENCHMARK(BM_ArrayErase);
BENCHMARK(BM_TableSetValueNoEmplace);
BENCHMARK(BM_TableSetValueEmplace);
BENCHMARK(BM_TableSetValueNoEmplaceKeyConflict);
BENCHMARK(BM_TableSetValueEmplaceKeyConflict);

}  // namespace lepusbenchmark
}  // namespace lynx

#pragma clang diagnostic pop
