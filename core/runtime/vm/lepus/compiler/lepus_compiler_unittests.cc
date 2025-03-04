// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/binary_input_stream.h"
#include "core/runtime/vm/lepus/builtin.h"
#include "core/runtime/vm/lepus/bytecode_generator.h"
#include "core/runtime/vm/lepus/context_binary_writer.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/js_object.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_date.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/quick_context.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using namespace lynx;  // NOLINT

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"

static const char* kCFuncGetLength = "_GetLength";
static const char* kCFuncIndexOf = "_IndexOf";
static const char* kCFuncSetValueToMap = "_SetValueToMap";

static const char* kCFuncAttachPage = "_AttachPage";
static const char* kCFuncAppendChild = "_AppendChild";
static const char* kCFuncCreateVirtualNode = "_CreateVirtualNode";
static const char* kCFuncCreateVirtualComponent = "_CreateVirtualComponent";
static const char* kCFuncCreatePage = "_CreatePage";
static const char* kCFuncSetAttributeTo = "_SetAttributeTo";
static const char* kCFuncSetStaticAttributeTo = "_SetStaticAttributeTo";
static const char* kCFuncSetStyleTo = "_SetStyleTo";
static const char* kCFuncSetStaticStyleTo = "_SetStaticStyleTo";
static const char* kCFuncSetDynamicStyleTo = "_SetDynamicStyleTo";
static const char* kCFuncSetClassTo = "_SetClassTo";
static const char* kCFuncSetStaticClassTo = "_SetStaticClassTo";
static const char* kCFuncSetStaticEventTo = "_SetStaticEventTo";
static const char* kCFuncSetDataSetTo = "_SetDataSetTo";
static const char* kCFuncSetEventTo = "_SetEventTo";
static const char* kCFuncSetId = "_SetId";

static const char* kCFuncCreateVirtualSlot = "_CreateVirtualSlot";
static const char* kCFuncCreateVirtualPlug = "_CreateVirtualPlug";
static const char* kCFuncMarkComponentHasRenderer = "_MarkComponentHasRenderer";

// Relevant to component
static const char* kCFuncSetProp = "_SetProp";
static const char* kCFuncSetData = "_SetData";
static const char* kCFuncProcessComponentData = "_ProcessData";
static const char* kCFuncAddPlugToComponent = "_AddPlugToComponent";
static const char* kCFuncGetComponentData = "_GetComponentData";
static const char* kCFuncGetComponentProps = "_GetComponentProps";

static const char* kCFuncPrint = "print";
static const char* kCFuncAssert = "Assert";
static const char* kCFuncTypeof = "typeof";
static const char* kCFuncSetFlag = "setFlag";
static int count = 0;

#pragma clang diagnostic pop

static lepus::Value EmptyFunc(lepus::Context* context) {
  ++count;
  // std::cout << count << std::endl;
  return lepus::Value();
}

static void Print_Value(lepus::Value* val, std::ostream& output);

static void Dump_Table(lepus::Value* val, std::ostream& output) {
  auto it = val->Table()->begin();
  output << "{ " << std::endl;
  for (; it != val->Table()->end(); it++) {
    output << it->first.str() << " : ";
    Print_Value(&(it->second), output);
    output << "\n";
  }
  output << "} " << std::endl;
}
static void Print_Value(lepus::Value* val, std::ostream& output) {
  switch (val->Type()) {
    case lepus::ValueType::Value_Nil:
      output << "null";
      break;
    case lepus::ValueType::Value_Undefined:
      output << "undefined";
      break;
    case lepus::ValueType::Value_Double:
      output << val->Number();
      break;
    case lepus::ValueType::Value_Int32:
      output << val->Int32();
      break;
    case lepus::ValueType::Value_Int64:
      output << val->Int64();
      break;
    case lepus::ValueType::Value_UInt32:
      output << val->UInt32();
      break;
    case lepus::ValueType::Value_UInt64:
      output << val->UInt64();
      break;
    case lepus::ValueType::Value_Bool:
      output << (val->Bool() ? "true" : "false");
      break;
    case lepus::ValueType::Value_String:
      output << "'" << val->StdString() << "'";
      break;
    case lepus::ValueType::Value_Table:
      Dump_Table(val, output);
      break;
    case lepus::ValueType::Value_Array:
      output << "[";
      for (size_t i = 0; i < val->Array()->size(); i++) {
        Print_Value(const_cast<lepus::Value*>(&(val->Array()->get(i))), output);
        if (i != (val->Array()->size() - 1)) {
          output << ", ";
        }
      }
      output << "]";
      break;
    case lepus::ValueType::Value_CDate:
      val->Date()->print(output);
      break;
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
      output << "closure/cfunction/cpointer/refcounted" << std::endl;
      break;
    case lepus::ValueType::Value_RegExp:
      output << "regexp" << std::endl;
      output << "pattern: " << val->RegExp()->get_pattern().str() << std::endl;
      output << "flags: " << val->RegExp()->get_flags().str() << std::endl;
      break;
    case lepus::ValueType::Value_NaN:
      output << "NaN";
      break;
    case lepus::ValueType::Value_JSObject:
      output << "LEPUSObject id=" << val->LEPUSObject()->JSIObjectID();
      break;
  }
}

lepus::Value Print(lepus::Context* context) {
  long params_count = context->GetParamsSize();
  for (long i = 0; i < params_count; i++) {
    lepus::Value* v = context->GetParam(i);
    std::ostringstream s;
    Print_Value(v, s);
    LOGE(s.str());
  }
  return lepus::Value();
}
static lepus::Value Assert(lepus::Context* context) {
  lepus::Value* val = context->GetParam(0);
  if (val->IsTrue()) {
    return lepus::Value();
  } else {
    std::cout << "Test Failed!" << std::endl;
    abort();
  }
}
static lepus::Value Typeof(lepus::Context* context) {
  lepus::Value* val = context->GetParam(0);
  switch (val->Type()) {
    case lepus::ValueType::Value_Nil:
      val->SetString("null");
      break;
    case lepus::ValueType::Value_Undefined:
      val->SetString("undefined");
      break;
    case lepus::ValueType::Value_Double:
    case lepus::ValueType::Value_Bool:
    case lepus::ValueType::Value_Int32:
    case lepus::ValueType::Value_Int64:
    case lepus::ValueType::Value_UInt32:
    case lepus::ValueType::Value_UInt64:
      val->SetString("number");
      break;
    case lepus::ValueType::Value_String:
      val->SetString("string");
      break;
    case lepus::ValueType::Value_Table:
      val->SetString("table");
      break;
    case lepus::ValueType::Value_Array:
      val->SetString("object");
      break;
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
      val->SetString("function");
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
      val->SetString("pointer");
      break;
  }
  return *val;
}

static lepus::Value SetFlag(lepus::Context* context) {
  lepus::Value* parm1 = context->GetParam(0);
  if (parm1->String().IsEqual("lepusNullPropAsUndef")) {
    lepus::VMContext::Cast(context)->SetNullPropAsUndef(
        context->GetParam(1)->Bool());
  }
  return lepus::Value();
}

static lepus::Value CheckArgs(lepus::Context* context) {
  lepus::Value* param1 = context->GetParam(0);

  if (!param1->IsString()) {
    return context->ReportFatalError("arg is not string", false,
                                     error::E_MTS_RENDERER_FUNCTION_FATAL);
  }
  return *param1;
}

void RegisterBuiltin(lepus::VMContext* context) {
  lepus::RegisterCFunction(context, kCFuncCreatePage, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncAttachPage, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncCreateVirtualComponent, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncCreateVirtualNode, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncAppendChild, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetClassTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetStyleTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetEventTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetAttributeTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetStaticClassTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetStaticStyleTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetStaticAttributeTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetDataSetTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetStaticEventTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetId, &EmptyFunc);

  lepus::RegisterCFunction(context, kCFuncCreateVirtualSlot, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncCreateVirtualPlug, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncMarkComponentHasRenderer, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetProp, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetData, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncAddPlugToComponent, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncGetComponentData, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncGetComponentProps, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncSetDynamicStyleTo, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncPrint, &Print);
  lepus::RegisterCFunction(context, kCFuncAssert, &Assert);
  lepus::RegisterCFunction(context, kCFuncTypeof, &Typeof);
  lepus::RegisterCFunction(context, kCFuncSetFlag, &SetFlag);
  lepus::RegisterCFunction(context, "TestReportFatal", &CheckArgs);
  // lepus::RegisterCFunction(context, kCFuncSetData, &ProcessComponentData);
}

class TestLepus : public lynx::lepus::ContextBinaryWriter {
 public:
  TestLepus() : lynx::lepus::ContextBinaryWriter(new lynx::lepus::VMContext) {}
  ~TestLepus() { delete context_; }

  static const char* input;
  void Run(const char* source) {
    std::string lepus_resource = source;
    if (lepus_resource == "") {
      lepus_resource = lepus::readFile(TestLepus::input);
    }
    context_->Initialize();
    RegisterBuiltin(lynx::lepus::VMContext::Cast(context_));
    lynx::lepus::VMContext::Cast(context_)->SetClosureFix(true);
    auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(
        context_, lepus_resource, "2.6");

    if (!error.empty()) {
      LOGE("error: compile  failed:" << error << "\n");
      return;
    } else {
      if (strcmp(source, "") == 0) {
        std::cout << "Compile successed!!!" << std::endl;
      }
    }

    {
      long long start = std::chrono::duration_cast<std::chrono::microseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();
      context_->Execute();
      long long end = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
      if (strcmp(source, "") == 0) {
        std::cout << "init time: " << (end - start) / 1000.f << std::endl;
      }
    }
  }

 private:
};

const char* TestLepus::input = "";

#define RenderInfo(...)

#define RenderWarning(expression, ...)

#define RenderFatal(expression, ...)

#define RENDERER_FUNCTION(name)                                       \
  static LEPUSValue name(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv)
#define RUNTIME_FUNCTION_DATA(name)                                   \
  static LEPUSValue name(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv, int magic,  \
                         LEPUSValue* func_data)
#define CONVERT_ARG(name, index)               \
  lepus::Value __##name##__(ctx, argv[index]); \
  lepus::Value* name = &__##name##__;
#define CONVERT_ARG_AND_CHECK(name, index, Type, FunName) \
  lepus::Value __##name##__(ctx, argv[index]);            \
  lepus::Value* name = &__##name##__;                     \
  RenderFatal(name->Is##Type(),                           \
              #FunName " params " #index " type should use " #Type)
#define CHECK_ARGC_EQ(name, count) \
  RenderFatal(argc == count, #name " params size should == " #count);
#define CHECK_ARGC_GE(name, count) \
  RenderFatal(argc >= count, #name " params size should == " #count);
#define UNDEFINED LEPUS_UNDEFINED
#define RETURN(v) return (v).ToJSValue(ctx)
#define RETURN_UNDEFINED() return LEPUS_UNDEFINED
#define ARGC() (argc)
#define LEPUS_CONTEXT() (lepus::QuickContext::GetFromJsContext(ctx))
#define CONVERT(v) (v->ToLepusValue())
#define CREATE_FUNCTION(NAME)                                         \
  static LEPUSValue NAME(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv) {           \
    return LEPUS_UNDEFINED;                                           \
  }

RENDERER_FUNCTION(Console_Log) {
  lepus::Value value(ctx, argv[0]);
  std::cout << value.ToString() << std::endl;
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Test_val) {
  lepus::Value v(ctx, argv[0]);
  if (v.IsInt64()) {
    std::cout << v.Int64() << std::endl;
  } else if (v.IsString()) {
    std::cout << v.StdString() << std::endl;
  } else if (v.IsNumber()) {
    std::cout << v.IsNumber() << std::endl;
  } else {
    std::cout << "unknow" << std::endl;
  }
}

RENDERER_FUNCTION(Test_eq) {
  lepus::Value left(ctx, argv[0]);
  lepus::Value right(ctx, argv[1]);

  std::cout << left.IsEqual(right) << std::endl;
}

RENDERER_FUNCTION(Test_valueEq) {
  lepus::Value v(ctx, argv[0]);
  std::cout << v.IsEqual(v.ToLepusValue()) << std::endl;
}

RENDERER_FUNCTION(Test_set) {
  lepus::Value obj(ctx, argv[0]);
  lepus::Value key(ctx, argv[1]);
  lepus::Value val(ctx, argv[2]);

  if (key.IsString()) {
    obj.SetProperty(key.String(), val);
  } else if (key.IsNumber()) {
    obj.SetProperty((int)key.Number(), val);
  }
}

RENDERER_FUNCTION(Test_Contains) {
  lepus::Value obj(ctx, argv[0]);
  lepus::Value key(ctx, argv[1]);

  base::String s_key = key.String();
  std::cout << "contains " << s_key.c_str() << " : " << obj.Contains(s_key)
            << std::endl;
}

RENDERER_FUNCTION(UpdateComponentInfo) {
  CHECK_ARGC_EQ(UpdateComponentInfo, 4);
  CONVERT_ARG_AND_CHECK(arg0, 0, CPointer, UpdateComponentInfo);
  CONVERT_ARG_AND_CHECK(arg1, 1, base::String, UpdateComponentInfo);
  // CONVERT_ARG_AND_CHECK(arg2, 2, Array, UpdateComponentInfo);
  CONVERT_ARG(arg2, 2);
  CONVERT_ARG_AND_CHECK(arg3, 3, base::String, UpdateComponentInfo);
  lepus::Value slot1 = CONVERT(arg2);
  lepus::Value slot2 = CONVERT(arg3);
  RenderFatal(slot1.IsArray(), "UpdateComponentInfo: arg2 should be array");
  RETURN_UNDEFINED();
}

RENDERER_FUNCTION(TestArgcNG) {
  CONVERT_ARG(arg1, 0);
  if (!arg1->IsString()) {
    return lepus::QuickContext::GetFromJsContext(ctx)
        ->ReportFatalError("arg is not string", false,
                           error::E_MTS_RENDERER_FUNCTION_FATAL)
        .ToJSValue(ctx);
  }
  return arg1->ToJSValue(ctx);
}

void RegisterQuickCFun(lepus::QuickContext* ctx, LEPUSValue obj,
                       const char* name, int argc, LEPUSCFunction* func) {
  LEPUSContext* lepus_ctx = ctx->context();
  LEPUSValue cf = LEPUS_NewCFunction(lepus_ctx, func, name, argc);
  HandleScope func_scope(lepus_ctx, &cf, HANDLE_TYPE_LEPUS_VALUE);
  lepus::LEPUSValueHelper::SetProperty(lepus_ctx, obj, name, cf);
}

void RegisterQuickBuiltins(lepus::QuickContext* ctx) {
  LEPUSValue obj = LEPUS_NewObject(ctx->context());
  HandleScope func_scope(ctx->context(), &obj, HANDLE_TYPE_LEPUS_VALUE);
  RegisterQuickCFun(ctx, obj, "log", 1, Console_Log);

  ctx->RegisterGlobalProperty("console", obj);
  ctx->RegisterGlobalFunction("Test", Test_val, 1);
  ctx->RegisterGlobalFunction("Equal", Test_eq, 2);
  ctx->RegisterGlobalFunction("Set", Test_set, 3);
  ctx->RegisterGlobalFunction("Contains", Test_Contains, 2);
  ctx->RegisterGlobalFunction("UpdateComponentInfo", UpdateComponentInfo, 4);
  ctx->RegisterGlobalFunction("ValueEq", Test_valueEq, 2);
  ctx->RegisterGlobalFunction("TestReportFatal", TestArgcNG);
}

class TestLepusNG : public lynx::lepus::ContextBinaryWriter {
 public:
  TestLepusNG()
      : lynx::lepus::ContextBinaryWriter(new lynx::lepus::QuickContext) {}
  void Run(const char* input, const char* source) {
    std::string lepus_resource = source;
    if (strcmp(source, "") == 0) {
      lepus_resource = lepus::readFile(input);
    }

    lepus::QuickContext ctx;
    RegisterQuickBuiltins(&ctx);
    ctx.Initialize();
    auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(
        &ctx, lepus_resource, "");
    ctx.Execute();
  }
};

std::vector<uint8_t> ReadBinary(std::string full_path) {
  std::ifstream instream(full_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)),
                            std::istreambuf_iterator<char>());
  return data;
}

void CustomInit(int argc, char** argv) {
  if (argc > 1) {
    TestLepus::input = argv[1];
  }
}

TEST(lepus, compile) {
  TestLepus t;
  t.Run("");
}
