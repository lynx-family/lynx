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

static void BM_ValueMoveAssign(benchmark::State& state) {
  std::vector<lepus::Value> source_arr;
  source_arr.emplace_back((int)5);
  source_arr.emplace_back((uint64_t)0xFFFFFFFFFF);
  source_arr.emplace_back("abcdefg");
  source_arr.emplace_back(lepus::Dictionary::Create());
  source_arr.emplace_back((void*)&BM_ValueMoveAssign);
  source_arr.emplace_back(true);
  for (auto _ : state) {
    state.PauseTiming();
    std::vector<lepus::Value> dest_arr1;
    std::vector<lepus::Value> dest_arr2;
    dest_arr1.reserve(source_arr.size() * 1000);
    dest_arr2.resize(source_arr.size() * 1000);  // default constructed
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr1.emplace_back(source_arr[j]);
      }
    }
    // Set some values as refcounted.
    for (size_t i = 0; i < dest_arr2.size(); i++) {
      if (i % 20 == 0) {
        dest_arr2[i] = lepus::Value(lepus::Dictionary::Create());
      }
    }
    state.ResumeTiming();
    for (int i = 0; i < 1000; i++) {
      for (size_t j = 0; j < source_arr.size(); j++) {
        dest_arr2[i * source_arr.size() + j] =
            std::move(dest_arr1[i * source_arr.size() + j]);
      }
    }
    state.PauseTiming();  // ~Value not measured
  }
}

static void BM_JSExecute_ArrayJoin(benchmark::State& state) {
  const char* src = R"(
let arr = [];
for (let i = 0; i < 5000; ++i) {
  arr.push('item' + i);
}
export function execute() {
  let s = arr.join(',');
  return s.length > 0 ? 1 : 0;
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

static void BM_JSExecute_StringSplit(benchmark::State& state) {
  const char* src = R"(
let longStr = '';
for (let i = 0; i < 500; ++i) {
  longStr += 'word' + i + ',';
}
export function execute() {
  let arr = longStr.split(',');
  return arr.length > 0 ? 1 : 0;
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

static void BM_JSExecute_StringSearch(benchmark::State& state) {
  const char* src = R"(
let longStr = 'abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcxyzabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc';
let pattern = /xyz/;
export function execute() {
  let idx = -1;
  for (let i = 0; i < 100; ++i) {
    idx = longStr.search(pattern);
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

static void BM_JSExecute_StringReplace(benchmark::State& state) {
  const char* src = R"(
let baseStr = 'The quick brown fox jumps over the lazy dog.';
let regex = /fox/g;
export function execute() {
  let count = 0;
  for (let i = 0; i < 100; i ++) {
    let s = baseStr.replace('dog', 'cat');
    s = s.replace(regex, 'wolf');
    count += s.length;
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

static void BM_JSExecute_StringSlice(benchmark::State& state) {
  const char* src = R"(
let longStr = 'abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz';
export function execute() {
  let s = '';
  for (let i = 0; i < 500; ++i) {
    s = longStr.slice(i, i + 10);
  }
  return s.length > 0 ? 1 : 0;
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

static void BM_JSExecute_StringTrim(benchmark::State& state) {
  const char* src = R"(
let strWithSpaces = '   hello world   ';
export function execute() {
  let s = '';
  for (let i = 0; i < 5000; ++i) {
    s = strWithSpaces.trim();
  }
  return s.length > 0 ? 1 : 0;
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

static void BM_JSExecute_StringSubstring(benchmark::State& state) {
  const char* src = R"(
let longStr = 'abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz';
export function execute() {
  let s = '';
  for (let i = 0; i < 500; ++i) {
    s = longStr.substring(i, i + 10);
  }
  return s.length > 0 ? 1 : 0;
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

static void BM_JSExecute_ObjectKeys(benchmark::State& state) {
  const char* src = R"(
let obj = {};
for (let i = 0; i < 12; ++i) {
  obj['prop' + i] = i;
}
export function execute() {
  let count = 0;
  for (let i = 0; i < 100; i++) {
    let keys = Object.keys(obj);
    count += keys.length;
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

static void BM_JSExecute_JsonStringify(benchmark::State& state) {
  const char* src = R"(
let complexObj = {
  a: 1,
  b: 'hello',
  c: true,
  d: [1, 2, 3],
  e: { f: 'nested', g: null }
};
export function execute() {
  let count = 0;
  for (let i = 0; i < 500; ++i) {
    let jsonStr = JSON.stringify(complexObj);
    count += jsonStr.length;
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

BENCHMARK(BM_ValueMoveAssign);
BENCHMARK(BM_JSExecute_ArrayJoin);
BENCHMARK(BM_JSExecute_StringSplit);
BENCHMARK(BM_JSExecute_StringSearch);
BENCHMARK(BM_JSExecute_StringReplace);
BENCHMARK(BM_JSExecute_StringSlice);
BENCHMARK(BM_JSExecute_StringTrim);
BENCHMARK(BM_JSExecute_StringSubstring);
BENCHMARK(BM_JSExecute_ObjectKeys);
BENCHMARK(BM_JSExecute_JsonStringify);

}  // namespace lepusbenchmark
}  // namespace lynx

#pragma clang diagnostic pop
