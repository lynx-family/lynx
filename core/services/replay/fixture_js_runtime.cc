// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/fixture_js_runtime.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <utility>

#include "core/runtime/js/jsi/quickjs/quickjs_api.h"
#include "core/runtime/js/jsi/quickjs/quickjs_runtime.h"
#include "quickjs/include/quickjs.h"

namespace lynx {
namespace tasm {
namespace replay {
namespace {

std::string WrapFixtureSource(std::string source) {
  constexpr char kExportDefaultFunction[] = "export default function";
  size_t export_position = source.find(kExportDefaultFunction);
  if (export_position == std::string::npos) {
    return "(function(ctx) {\n" + source + "\n})(ctx);\n";
  }
  source.replace(export_position, std::strlen(kExportDefaultFunction),
                 "var __fixture_main__ = function");
  source += "\n__fixture_main__(ctx);\n";
  return source;
}

class QuickJsFixtureRuntime final : public FixtureJsRuntime {
 public:
  QuickJsFixtureRuntime(std::unique_ptr<runtime::js::Runtime> runtime,
                        const FixtureRuntimeLimits& limits)
      : FixtureJsRuntime(std::move(runtime)), limits_(limits) {}

  bool timed_out() const override { return timed_out_; }

 private:
  LEPUSRuntime* GetRuntime() {
    return static_cast<runtime::js::QuickjsRuntime&>(runtime()).getJSRuntime();
  }

  static int Interrupt(LEPUSRuntime*, void* opaque) {
    auto* self = static_cast<QuickJsFixtureRuntime*>(opaque);
    if (std::chrono::steady_clock::now() < self->deadline_) {
      return 0;
    }
    self->timed_out_ = true;
    return 1;
  }

  bool IsHeapWithinLimit() override {
    return LEPUS_GetHeapSize(GetRuntime()) < limits_.memory_limit_bytes;
  }

  void ArmExecutionDeadline() override {
    LEPUS_SetMemoryLimit(GetRuntime(), limits_.memory_limit_bytes);
    timed_out_ = false;
    const auto now = std::chrono::steady_clock::now();
    const auto remaining =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::time_point::max() - now);
    deadline_ = limits_.timeout_ms >= remaining.count()
                    ? std::chrono::steady_clock::time_point::max()
                    : now + std::chrono::milliseconds(limits_.timeout_ms);
    LEPUS_SetInterruptHandler(GetRuntime(), Interrupt, this);
  }

  void DisarmExecutionDeadline() override {
    LEPUS_SetInterruptHandler(GetRuntime(), nullptr, nullptr);
  }

  FixtureRuntimeLimits limits_;
  std::chrono::steady_clock::time_point deadline_;
  bool timed_out_ = false;
};

}  // namespace

using runtime::js::Function;
using runtime::js::HostFunctionType;
using runtime::js::JSINativeException;
using runtime::js::PropNameID;
using runtime::js::Runtime;
using runtime::js::String;
using runtime::js::Value;

std::string FixtureValueToJson(Runtime& runtime, const Value& value,
                               const char* fallback) {
  auto json = value.toJsonString(runtime);
  if (!json.has_value() || !json->isString()) {
    return fallback;
  }
  return json->getString(runtime).utf8(runtime);
}

Function CreateFixtureReadAssetFunction(Runtime& runtime,
                                        const std::string& fixture_directory,
                                        size_t max_bytes) {
  return Function::createFromHostFunction(
      runtime, PropNameID::forAscii(runtime, "readAsset"), 1,
      HostFunctionType(
          [fixture_directory, max_bytes](
              Runtime& current_runtime, const Value&, const Value* args,
              size_t count) -> base::expected<Value, JSINativeException> {
            if (count < 1 || !args[0].isString()) {
              return Value::undefined();
            }
            std::string path =
                args[0].getString(current_runtime).utf8(current_runtime);
            std::string content =
                ReadFixtureAsset(fixture_directory, path, max_bytes);
            if (content.empty()) {
              return Value::undefined();
            }
            bool is_json =
                path.size() >= 5 && path.substr(path.size() - 5) == ".json";
            if (is_json) {
              auto value = Value::createFromJsonUtf8(
                  current_runtime,
                  reinterpret_cast<const uint8_t*>(content.data()),
                  content.size());
              if (value.has_value()) {
                return std::move(*value);
              }
            }
            return Value(String::createFromUtf8(current_runtime, content));
          }));
}

FixtureJsRuntime::FixtureJsRuntime(
    std::unique_ptr<runtime::js::Runtime> runtime)
    : runtime_(std::move(runtime)) {
  runtime_->SetEnableJsBindingApiThrowException(true);
}

FixtureJsRuntime::~FixtureJsRuntime() { runtime_->BeforeDestroy(); }

base::expected<runtime::js::Value, runtime::js::JSINativeException>
FixtureJsRuntime::EvaluateScript(std::string source) {
  auto buffer = std::make_shared<runtime::js::StringBuffer>(
      WrapFixtureSource(std::move(source)));
  ScopedFixtureExecution execution(*this);
  // Caller-installed bindings also count toward the engine budget.
  if (!IsHeapWithinLimit()) {
    return base::unexpected(
        BUILD_JSI_NATIVE_EXCEPTION("Fixture runtime memory limit exceeded"));
  }
  return runtime_->evaluateJavaScript(buffer, "fixture.js");
}

void FixtureJsRuntime::BeginExecution() {
  if (execution_depth_++ == 0) {
    ArmExecutionDeadline();
  }
}

void FixtureJsRuntime::EndExecution() {
  if (--execution_depth_ == 0) {
    DisarmExecutionDeadline();
  }
}

ScopedFixtureExecution::ScopedFixtureExecution(FixtureJsRuntime& runtime)
    : runtime_(runtime), scope_(runtime.runtime()) {
  runtime_.BeginExecution();
}

ScopedFixtureExecution::~ScopedFixtureExecution() { runtime_.EndExecution(); }

std::unique_ptr<FixtureJsRuntime> CreateQuickJsFixtureRuntime(
    const FixtureRuntimeLimits& limits) {
  if (!limits.IsValid()) {
    return nullptr;
  }
  auto js_runtime = runtime::js::makeQuickJsRuntime();
  if (!js_runtime) {
    return nullptr;
  }
  runtime::js::StartupData startup_data{};
  auto vm = js_runtime->createVM(&startup_data);
  auto context = js_runtime->createContext(vm);
  js_runtime->InitRuntime(context);
  // Reject budgets that cannot hold even the initialized engine. In particular,
  // do not enter the compiler with a heap already above its allocation limit.
  if (LEPUS_GetHeapSize(static_cast<runtime::js::QuickjsRuntime&>(*js_runtime)
                            .getJSRuntime()) >= limits.memory_limit_bytes) {
    js_runtime->BeforeDestroy();
    return nullptr;
  }
  return std::make_unique<QuickJsFixtureRuntime>(std::move(js_runtime), limits);
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
