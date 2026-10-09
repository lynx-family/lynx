// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_module_fixture_replay.h"

#include <cmath>
#include <utility>
#include <vector>

#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace runtime {
namespace js {

base::expected<Value, JSINativeException> ModuleFixtureReplay::invokeMethod(
    const MethodMetadata& method, Runtime* rt, const Value* args,
    size_t count) {
  // Fixture replay has no recorded moduleData JSON; dispatch straight to the
  // handler instead of going through the V1 fetch-data / JSON-matching path.
  return invokeMethodKernel(method, rt, args, count);
}

std::string ModuleFixtureReplay::SerializeArgs(
    Runtime* rt, const Value* args, size_t count,
    std::vector<Function>& out_callbacks,
    std::vector<size_t>& out_callback_positions) {
  rapidjson::Document doc(rapidjson::kArrayType);
  auto& alloc = doc.GetAllocator();
  auto json = rt->global().getPropertyAsObject(*rt, "JSON");
  if (!json) return {};
  auto stringify = json->getPropertyAsFunction(*rt, "stringify");
  if (!stringify) return {};
  // Preserve recorder sentinel strings for undefined and NaN values.
  auto replacer = Function::createFromHostFunction(
      *rt, PropNameID::forAscii(*rt, "fixtureArgument"), 2,
      [](Runtime& runtime, const Value&, const Value* values,
         size_t count) -> base::expected<Value, JSINativeException> {
        if (count < 2) return Value::undefined();
        if (values[1].isUndefined())
          return String::createFromUtf8(runtime, "undefined");
        if (values[1].isNumber() && std::isnan(values[1].getNumber()))
          return String::createFromUtf8(runtime, "NaN");
        return Value(runtime, values[1]);
      });

  for (size_t i = 0; i < count; i++) {
    const Value* arg = &args[i];
    // Function arguments cannot cross runtimes. Their original positions are
    // retained so FixtureContext can install function placeholders before the
    // generated matcher evaluates `typeof actual === "function"`.
    if (arg->isObject() && arg->getObject(*rt).isFunction(*rt)) {
      out_callbacks.push_back(arg->getObject(*rt).getFunction(*rt));
      out_callback_positions.push_back(i);
      doc.PushBack(rapidjson::Value(rapidjson::kNullType), alloc);
      continue;
    }
    auto json_opt = stringify->call(*rt, *arg, replacer);
    if (json_opt.has_value() && json_opt->isString()) {
      std::string js = json_opt->getString(*rt).utf8(*rt);
      rapidjson::Document elem(&alloc);
      elem.Parse(js.c_str());
      if (!elem.HasParseError()) {
        doc.PushBack(rapidjson::Value(elem, alloc), alloc);
        continue;
      }
    }
    doc.PushBack(rapidjson::Value(rapidjson::kNullType), alloc);
  }

  rapidjson::StringBuffer sb;
  rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
  doc.Accept(writer);
  return sb.GetString();
}

Value ModuleFixtureReplay::invokeMethodKernel(const MethodMetadata& method,
                                              Runtime* rt, const Value* args,
                                              size_t count) {
  auto context = fixture_context_.lock();
  if (!context || !lifetime_) {
    return Value::undefined();
  }

  std::vector<Function> callback_functions;
  std::vector<size_t> callback_positions;
  std::string args_json =
      SerializeArgs(rt, args, count, callback_functions, callback_positions);

  tasm::replay::FixtureDispatchResult result =
      context->Dispatch(name_, method.name, args_json, callback_positions);

  if (!result.handled) {
    LOGE("Fixture dispatch failed: " << name_ << "." << method.name);
    return Value::undefined();
  }

  // Replay the callbacks the handler invoked, in order, on the testbench
  // thread to preserve JSON replay's asynchronous callback semantics.
  for (const auto& cb_call : result.callbacks) {
    if (cb_call.index >= callback_functions.size()) {
      continue;
    }
    // Each invocation consumes a registration, so clone the function to allow
    // a handler to invoke the same callback more than once.
    Function callback =
        callback_functions[cb_call.index].getFunction(*rt);  // clones a ref.
    InvokeJsbCallbackJson(std::move(callback), cb_call.value_json,
                          cb_call.delay_ms, lifetime_);
  }

  // Convert the handler's return value (if any) into a JSI value.
  if (result.return_value_json.empty() ||
      result.return_value_json == "undefined") {
    return Value::undefined();
  }
  auto value = Value::createFromJsonUtf8(
      *rt, reinterpret_cast<const uint8_t*>(result.return_value_json.data()),
      result.return_value_json.size());
  return value ? std::move(*value) : Value::undefined();
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
