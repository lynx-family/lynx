// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <string>
#include <vector>

#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "quickjs/include/quickjs.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#ifdef OS_IOS
#include "trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace base {

class LepusStackMethods : public ::testing::Test {
 protected:
  LepusStackMethods() = default;
  ~LepusStackMethods() override = default;
};

TEST_F(LepusStackMethods, TestDefaultStackSize) {
  lepus::QuickContext qctx;
  std::string src = "function entry(){let a=1;let b=1;return a+b;}";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
  qctx.Execute();
  LEPUSValue res = qctx.GetAndCall("entry", nullptr, 0);
  LEPUSValue expected_res = LEPUS_MKVAL(LEPUS_TAG_INT, 2);
  EXPECT_EQ(LEPUS_VALUE_GET_TAG(res), LEPUS_VALUE_GET_TAG(expected_res));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(res), LEPUS_VALUE_GET_INT(expected_res));
}

TEST_F(LepusStackMethods, TestSetNormalStackSize) {
  lepus::QuickContext qctx;
  qctx.SetStackSize(1024 * 1024);
  std::string src = "function sayHello(){let a=1;let b=1;return a+b;}";
  lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
  qctx.Execute();
  LEPUSValue res = qctx.GetAndCall("sayHello", nullptr, 0);
  LEPUSValue expected_res = LEPUS_MKVAL(LEPUS_TAG_INT, 2);
  EXPECT_EQ(LEPUS_VALUE_GET_TAG(res), LEPUS_VALUE_GET_TAG(expected_res));
  EXPECT_EQ(LEPUS_VALUE_GET_INT(res), LEPUS_VALUE_GET_INT(expected_res));
}

TEST_F(LepusStackMethods, TestStackOverflow) {
  lepus::QuickContext qctx;
  if (LEPUS_IsGCMode(qctx.context())) {
    std::string src = "function sayHello(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    qctx.SetStackSize(1);
    LEPUSAtom name_atom =
        LEPUS_NewAtom(qctx.context(), std::string("sayHello").c_str());
    LEPUSValue caller = LEPUS_GetGlobalVar(qctx.context(), name_atom, 0);
    LEPUSValue global = LEPUS_GetGlobalObject(qctx.context());
    std::vector<LEPUSValue> args;
    LEPUS_Call(qctx.context(), caller, global, static_cast<int>(args.size()),
               const_cast<LEPUSValue*>(args.data()));
    LEPUSValue error = LEPUS_GetException(qctx.context());
    HandleScope func_scope(qctx.context(), &error, HANDLE_TYPE_LEPUS_VALUE);
    LEPUSValue message = LEPUS_GetPropertyStr(qctx.context(), error, "message");
    const char* message_str = LEPUS_ToCString(qctx.context(), message);
    (void)message_str;
#ifdef OS_IOS
    EXPECT_EQ(std::string(message_str), "stack overflow");
#endif
  } else {
    std::string src = "function sayHello(){let a=1;let b=1;return a+b;}";
    lepus::BytecodeGenerator::GenerateBytecode(&qctx, src, "2.0");
    qctx.Execute();
    qctx.SetStackSize(1);
    LEPUSAtom name_atom =
        LEPUS_NewAtom(qctx.context(), std::string("sayHello").c_str());
    LEPUSValue caller = LEPUS_GetGlobalVar(qctx.context(), name_atom, 0);
    LEPUSValue global = LEPUS_GetGlobalObject(qctx.context());
    std::vector<LEPUSValue> args;
    LEPUS_Call(qctx.context(), caller, global, static_cast<int>(args.size()),
               const_cast<LEPUSValue*>(args.data()));
    LEPUSValue error = LEPUS_GetException(qctx.context());
    LEPUSValue message = LEPUS_GetPropertyStr(qctx.context(), error, "message");
    const char* message_str = LEPUS_ToCString(qctx.context(), message);
#ifdef OS_IOS
    EXPECT_EQ(std::string(message_str), "stack overflow");
#endif
    LEPUS_FreeValue(qctx.context(), caller);
    LEPUS_FreeValue(qctx.context(), global);
    LEPUS_FreeValue(qctx.context(), error);
    LEPUS_FreeValue(qctx.context(), message);
    LEPUS_FreeCString(qctx.context(), message_str);
  }
}

}  // namespace base
}  // namespace lynx
