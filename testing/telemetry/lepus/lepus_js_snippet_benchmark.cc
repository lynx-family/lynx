// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/bindings/lepus/renderer_functions.h"
#include "core/runtime/lepus_context/builtin.h"
#include "core/runtime/lepus_context/bytecode_generator.h"
#include "core/runtime/lepus_context/json_parser.h"
#include "core/runtime/lepus_context/jsvalue_helper.h"
#include "core/runtime/lepus_context/quick_context.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

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

static void BM_JSExecute_Fibonacci(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  function fibonacci(n) {
    var n1 = 1,
        n2 = 1,
        sum;
    for (let i = 2; i < n; i++) {
        sum = n1 + n2;
        n1 = n2;
        n2 = sum;
    }
    return sum;
  }

  for (let i = 1; i <= 25; ++i) {
    fibonacci(50 * i);
  }
  return 1;
}
    )";

  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_Fibonacci_Recursive(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  function fibonacci(n) {
    if (n <= 1) {
      return n;
    } else {
      return fibonacci(n - 1) + fibonacci(n - 2);
    }
  }

  for (let i = 1; i <= 16; ++i) {
    fibonacci(i);
  }
  return 1;
}
    )";

  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_NumericalCalculation(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  let total = 0;
  for (let i = 0; i < 10000; ++i) {
    total += i * 5 - i / 2 + (i % 10);
  }
  return total;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_ObjectPropertyAdd(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  for (let loop = 0; loop < 500; loop ++) {
    let obj = {};
    for (let i = 0; i < 4; ++i) {
      obj['prop' + i] = i;
    }
  }

  for (let loop = 0; loop < 250; loop ++) {
    let obj = {};
    for (let i = 0; i < 8; ++i) {
      obj['prop' + i] = i;
    }
  }

  for (let loop = 0; loop < 125; loop ++) {
    let obj = {};
    for (let i = 0; i < 16; ++i) {
      obj['prop' + i] = i;
    }
  }

  for (let loop = 0; loop < 83; loop ++) {
    let obj = {};
    for (let i = 0; i < 24; ++i) {
      obj['prop' + i] = i;
    }
  }

  for (let loop = 0; loop < 40; loop ++) {
    let obj = {};
    for (let i = 0; i < 48; ++i) {
      obj['prop' + i] = i;
    }
  }

  for (let loop = 0; loop < 20; loop ++) {
    let obj = {};
    for (let i = 0; i < 96; ++i) {
      obj['prop' + i] = i;
    }
  }
  return 1;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_ObjectPropertyLookup(benchmark::State& state) {
  const char* src = R"(
let testObj = {};
for (let i = 0; i < 2000; ++i) {
  testObj['prop' + i] = i;
}
export function execute() {
  let val = 0;
  for (let i = 0; i < 2000; ++i) {
    val = testObj['prop' + i];
  }
  return val;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_StringIndexOf(benchmark::State& state) {
  const char* src = R"(
let longStr = 'abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcxyzabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc';
let pattern = 'xyz';
export function execute() {
  let idx = -1;
  for (let i = 0; i < 100; ++i) {
    idx = String.indexOf(longStr, pattern);
  }
  return idx;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_ObjectAssign(benchmark::State& state) {
  const char* src = R"(
let source1 = { a: 1, b: 2 };
let source2 = { c: 3, d: 4 };
export function execute() {
  let target = {};
  for (let i = 0; i < 1000; ++i) {
    Object.assign(target, source1, source2);
  }
  return 1;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_ObjectFreeze(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  for (let i = 0; i < 1000; ++i) {
    let obj = { prop1: 42, prop2: 'hello' };
    Object.freeze(obj);
  }
  return 1;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_JsonParse(benchmark::State& state) {
  const char* src = R"(
let jsonStr = '{"a":1,"b":"hello","c":true,"d":[1,2,3],"e":{"f":"nested","g":null}}';
export function execute() {
  let count = 0;
  for (let i = 0; i < 1000; ++i) {
    let obj = JSON.parse(jsonStr);
    count += obj.a;
  }
  return count;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_RegExpTest(benchmark::State& state) {
  const char* src = R"(
let re = /^hello/;
let testStr = 'hello world';
export function execute() {
  let result = false;
  for (let i = 0; i < 1000; ++i) {
    result = re.test(testStr);
  }
  return result ? 1 : 0;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

static void BM_JSExecute_FunctionCall(benchmark::State& state) {
  const char* src = R"(
function f3(a, b) {
  return a + b;
}
function f2(a, b) {
  return f3(a, b) * 2;
}
function f1(a, b) {
  return f2(a, b) + 1;
}
export function execute() {
  let res = 0;
  for (let i = 0; i < 1000; ++i) {
    res = f1(i, i + 1);
  }
  return res;
}
    )";
  lepus::VMContext vctx;
  vctx.Initialize();
  vctx.SetGlobalData(
      "$kTemplateAssembler",
      lepus::Value((void*)&mock_delegate_instance));  // to mute error log
  lepus::BytecodeGenerator::GenerateBytecode(&vctx, src, sdk_version);
  vctx.Execute();
  volatile double total = 0;
  for (auto _ : state) {
    auto result = vctx.Call("execute");
    total += result.Number();
  }
}

BENCHMARK(BM_JSExecute_Fibonacci);
BENCHMARK(BM_JSExecute_Fibonacci_Recursive);
BENCHMARK(BM_JSExecute_NumericalCalculation);
BENCHMARK(BM_JSExecute_ObjectPropertyAdd);
BENCHMARK(BM_JSExecute_ObjectPropertyLookup);
BENCHMARK(BM_JSExecute_StringIndexOf);
BENCHMARK(BM_JSExecute_ObjectAssign);
BENCHMARK(BM_JSExecute_ObjectFreeze);
BENCHMARK(BM_JSExecute_JsonParse);
BENCHMARK(BM_JSExecute_RegExpTest);
BENCHMARK(BM_JSExecute_FunctionCall);

}  // namespace lepusbenchmark
}  // namespace lynx

#pragma clang diagnostic pop
