// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>
#include <utility>
#include <vector>

#include "base/include/value/base_value.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/lepus/bindings/renderer_functions.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

namespace lynx {
namespace lepusbenchmark {

static const std::string sdk_version = "2.0";

class MockContextDelegate : public runtime::MTSRuntime::Delegate {
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

static void BM_JSExecute_ArrayPushPop(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  let arr = [];
  for (let i = 0; i < 5000; ++i) {
    arr.push(i);
  }
  for (let i = 0; i < 5000; ++i) {
    arr.pop();
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

static void BM_JSExecute_ArrayGetIndex(benchmark::State& state) {
  const char* src = R"(
let arr = [];
for (let i = 0; i < 2000; ++i) {
  arr.push(i);
}
export function execute() {
  let sum = 0;
  for (let i = 0; i < 2000; ++i) {
    sum += arr[i];
  }
  return sum;
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

static void BM_JSExecute_ArrayShift(benchmark::State& state) {
  const char* src = R"(
export function execute() {
  let arr = [];
  for(let i = 0; i < 1000; ++i) {
    arr.push(i);
  }
  for(let i = 0; i < 1000; ++i) {
    arr.shift();
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

static void BM_JSExecute_ArraySlice(benchmark::State& state) {
  const char* src = R"(
let arr = [];
for (let i = 0; i < 1000; ++i) {
  arr.push(i);
}
export function execute() {
  for (let i = 0; i < 500; ++i) {
    let subArr = arr.slice(i, i + 100);
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

static void BM_JSExecute_ArrayForEach(benchmark::State& state) {
  const char* src = R"(
let arr = [];
for (let i = 0; i < 2000; ++i) {
  arr.push(i);
}
export function execute() {
  let sum = 0;
  arr.forEach(function(item) {
    sum += item;
  });
  return sum > 0 ? 1 : 0;
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

static void BM_JSExecute_ArrayMap(benchmark::State& state) {
  const char* src = R"(
let arr = [];
for (let i = 0; i < 2000; ++i) {
  arr.push(i);
}
export function execute() {
  let newArr = arr.map(function(item) {
    return item * 2;
  });
  return newArr.length > 0 ? 1 : 0;
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

BENCHMARK(BM_JSExecute_ArrayPushPop);
BENCHMARK(BM_JSExecute_ArrayGetIndex);
BENCHMARK(BM_JSExecute_ArrayShift);
BENCHMARK(BM_JSExecute_ArraySlice);
BENCHMARK(BM_JSExecute_ArrayForEach);
BENCHMARK(BM_JSExecute_ArrayMap);

}  // namespace lepusbenchmark
}  // namespace lynx

#pragma clang diagnostic pop
