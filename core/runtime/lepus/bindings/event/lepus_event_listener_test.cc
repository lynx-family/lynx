// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bindings/event/lepus_event_listener_test.h"

#include <cstring>
#include <memory>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/table.h"
#include "core/runtime/common/bindings/event/message_event.h"
#include "core/runtime/lepus/bindings/event/lepus_event_listener.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace test {

TEST_F(LepusClosureEventListenerTest, MaterializesMessageDataInLepusNG) {
  auto context = runtime::MTSRuntime::CreateContext(
      runtime::ContextType::LepusNGContextType);
  ASSERT_NE(context, nullptr);
  context->Initialize();

  constexpr char kListenerSource[] = R"(
    var messageDataResult = "not called";
    function listener(event) {
      messageDataResult = [
        Reflect.ownKeys(event.data).sort().join(","),
        Object(event.data) === event.data,
        event.data instanceof Object,
        Object.getPrototypeOf(event.data) === Object.prototype,
        Reflect.ownKeys(event.data.nested).join(","),
        Object(event.data.nested) === event.data.nested,
        event.data.nested instanceof Array
      ].join("|");
    }
    listener;
  )";
  lepus::Value closure;
  ASSERT_TRUE(context->EvalBuf(kListenerSource, std::strlen(kListenerSource),
                               closure, "context-proxy-test.js"));
  ASSERT_TRUE(closure.IsCallable());

  auto nested = lepus::CArray::Create();
  nested->emplace_back("value");
  auto message = lepus::Dictionary::Create();
  message->SetValue("answer", 42);
  message->SetValue("nested", lepus::Value(std::move(nested)));
  auto event = fml::MakeRefCounted<runtime::MessageEvent>(
      runtime::ContextProxy::Type::kJSContext,
      runtime::ContextProxy::Type::kCoreContext,
      std::make_unique<pub::ValueImplLepus>(lepus::Value(std::move(message))));

  LepusClosureEventListener listener(context.get(), std::move(closure));
  listener.Invoke(std::move(event));

  lepus::Value result;
  ASSERT_TRUE(context->GetTopLevelVariableByName(
      base::String("messageDataResult"), &result));
  EXPECT_EQ("answer,nested|true|true|true|0,length|true|true",
            result.StdString());
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
