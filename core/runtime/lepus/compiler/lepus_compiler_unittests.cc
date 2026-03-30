// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "base/include/sorted_for_each.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/lepus/binary_input_stream.h"
#include "core/runtime/lepus/builtin.h"
#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepus/context_binary_writer.h"
#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/js_object.h"
#include "core/runtime/lepus/json_parser.h"
#include "core/runtime/lepus/lepus_date.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using namespace lynx;  // NOLINT

bool HasLepusNullPropAsUndefMarker(const std::string& source) {
  std::string kLepusNullPropAsUndefMarker = "//lepusNullPropAsUndef";
  size_t len = kLepusNullPropAsUndefMarker.size();
  if (source.size() < len) return false;
  return source.compare(0, len, kLepusNullPropAsUndefMarker) == 0;
}

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
static const char* kCFuncSetStyleObject = "__SetStyleObject";
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
static int count = 0;

#pragma clang diagnostic pop

static lepus::Value EmptyFunc(runtime::MTSContext* context, lepus::Value*,
                              int) {
  ++count;
  // std::cout << count << std::endl;
  return lepus::Value();
}

static void Print_Value(const lepus::Value* val, std::ostream& output);

static void Dump_Table(const lepus::Value* val, std::ostream& output) {
  auto table = val->Table();
  output << "{ " << std::endl;
  base::sorted_for_each(
      table->begin(), table->end(),
      [&](const auto& it) {
        output << it.first.str() << " : ";
        Print_Value(&(it.second), output);
        output << "\n";
      },
      [](const auto& left, const auto& right) {
        return left.first.str() < right.first.str();
      });
  output << "} " << std::endl;
}
static void Print_Value(const lepus::Value* val, std::ostream& output) {
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
        Print_Value(&(val->Array()->get(i)), output);
        if (i != (val->Array()->size() - 1)) {
          output << ", ";
        }
      }
      output << "]";
      break;
    case lepus::ValueType::Value_CDate:
      lynx::fml::static_ref_ptr_cast<lepus::CDate>(val->RefCounted())
          ->print(output);
      break;
    case lepus::ValueType::Value_Closure:
    case lepus::ValueType::Value_CFunction:
    case lepus::ValueType::Value_CPointer:
    case lepus::ValueType::Value_RefCounted:
      output << "closure/cfunction/cpointer/refcounted" << std::endl;
      break;
    case lepus::ValueType::Value_RegExp:
      output << "regexp" << std::endl;
      output << "pattern: "
             << lynx::fml::static_ref_ptr_cast<lepus::RegExp>(val->RefCounted())
                    ->get_pattern()
                    .str()
             << std::endl;
      output << "flags: "
             << lynx::fml::static_ref_ptr_cast<lepus::RegExp>(val->RefCounted())
                    ->get_flags()
                    .str()
             << std::endl;
      break;
    case lepus::ValueType::Value_NaN:
      output << "NaN";
      break;
    case lepus::ValueType::Value_JSObject:
      output << "LEPUSObject id="
             << lynx::fml::static_ref_ptr_cast<lepus::LEPUSObject>(
                    val->RefCounted())
                    ->JSIObjectID();
      break;
  }
}

lepus::Value Print(runtime::MTSContext* context, lepus::Value* argv, int argc) {
  long params_count = argc;
  for (long i = 0; i < params_count; i++) {
    lepus::Value v(static_cast<lepus::VMContext*>(context)->GetParam(i));
    std::ostringstream s;
    Print_Value(&v, s);
    LOGE(s.str());
  }
  return lepus::Value();
}

static lepus::Value Assert(runtime::MTSContext* context, lepus::Value* argv,
                           int argc) {
  if (argv->IsTrue()) {
    return lepus::Value();
  } else {
    std::cout << "Test Failed!" << std::endl;
    abort();
  }
}

static lepus::Value Typeof(runtime::MTSContext* context, lepus::Value* val,
                           int argc) {
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

static lepus::Value CheckArgs(runtime::MTSContext* context,
                              lepus::Value* param1, int argc) {
  if (!param1->IsString()) {
    return context->ReportFatalError("arg is not string", false,
                                     error::E_MTS_RENDERER_FUNCTION_FATAL);
  }
  return *param1;
}

void RegisterBuiltinTest(lepus::VMContext* context) {
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
  lepus::RegisterCFunction(context, kCFuncSetStyleObject, &EmptyFunc);
  lepus::RegisterCFunction(context, kCFuncPrint, &Print);
  lepus::RegisterCFunction(context, kCFuncAssert, &Assert);
  lepus::RegisterCFunction(context, kCFuncTypeof, &Typeof);
  lepus::RegisterCFunction(context, "TestReportFatal", &CheckArgs);
  // lepus::RegisterCFunction(context, kCFuncSetData, &ProcessComponentData);
}

class TestLepusContextHolder {
 protected:
  explicit TestLepusContextHolder(lynx::runtime::ContextType type)
      : owned_context_(new lynx::runtime::MTSRuntime(
            type, false, 0, lynx::tasm::PageOptions())) {}

  lynx::runtime::MTSRuntime* context() const { return owned_context_.get(); }

 private:
  std::unique_ptr<lynx::runtime::MTSRuntime> owned_context_;
};

class TestLepus : private TestLepusContextHolder,
                  public lynx::lepus::ContextBinaryWriter {
 public:
  TestLepus()
      : TestLepusContextHolder(lynx::runtime::ContextType::VMContextType),
        lynx::lepus::ContextBinaryWriter(context()->GetMTSContext()) {}

  static const char* input;
  void Run(const char* source, bool opt_bytecode,
           const char* ir_dump_path = nullptr) {
    std::string lepus_resource = source;
    if (lepus_resource == "") {
      lepus_resource = lepus::readFile(TestLepus::input);
    }
    context()->Initialize();
    RegisterBuiltinTest(lynx::runtime::MTSRuntime::ToVMContext(context()));
    lynx::runtime::MTSRuntime::ToVMContext(context())->SetClosureFix(true);
    lynx::runtime::MTSRuntime::ToVMContext(context())->SetOptBytecode(
        opt_bytecode);
    lynx::runtime::MTSRuntime::ToVMContext(context())->SetNullPropAsUndef(
        HasLepusNullPropAsUndefMarker(lepus_resource));
    auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(
        context()->GetMTSContext(), lepus_resource, "2.6", "", ir_dump_path);

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
      context()->Execute(nullptr);
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

#define PRIM_CFUNCTION(name)                                          \
  static LEPUSValue name(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv)
#define RUNTIME_FUNCTION_DATA(name)                                   \
  static LEPUSValue name(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv, int magic,  \
                         LEPUSValue* func_data)
#define CONVERT_ARG(name, index)                                   \
  lepus::Value __##name##__ = MK_JS_LEPUS_VALUE(ctx, argv[index]); \
  lepus::Value* name = &__##name##__;
#define CONVERT_ARG_AND_CHECK(name, index, Type, FunName)          \
  lepus::Value __##name##__ = MK_JS_LEPUS_VALUE(ctx, argv[index]); \
  lepus::Value* name = &__##name##__;                              \
  RenderFatal(name->Is##Type(),                                    \
              #FunName " params " #index " type should use " #Type)
#define CHECK_ARGC_EQ(name, count) \
  RenderFatal(argc == count, #name " params size should == " #count);
#define CHECK_ARGC_GE(name, count) \
  RenderFatal(argc >= count, #name " params size should == " #count);
#define UNDEFINED LEPUS_UNDEFINED
#define RETURN(v) return lepus::LEPUSValueHelper::ToJsValue(ctx, v)
#define RETURN_UNDEFINED() return LEPUS_UNDEFINED
#define ARGC() (argc)
#define LEPUS_CONTEXT() (lepus::QuickContext::GetFromJsContext(ctx))
#define CONVERT(v) (v->ToLepusValue())
#define CREATE_FUNCTION(NAME)                                         \
  static LEPUSValue NAME(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv) {           \
    return LEPUS_UNDEFINED;                                           \
  }

PRIM_CFUNCTION(Console_Log) {
  lepus::Value value = MK_JS_LEPUS_VALUE(ctx, argv[0]);
  std::cout << value.ToString() << std::endl;
  return LEPUS_UNDEFINED;
}

PRIM_CFUNCTION(Test_val) {
  lepus::Value v = MK_JS_LEPUS_VALUE(ctx, argv[0]);
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

PRIM_CFUNCTION(Test_eq) {
  lepus::Value left = MK_JS_LEPUS_VALUE(ctx, argv[0]);
  lepus::Value right = MK_JS_LEPUS_VALUE(ctx, argv[1]);

  std::cout << left.IsEqual(right) << std::endl;
}

PRIM_CFUNCTION(Test_valueEq) {
  lepus::Value v = MK_JS_LEPUS_VALUE(ctx, argv[0]);
  std::cout << v.IsEqual(v.ToLepusValue()) << std::endl;
}

PRIM_CFUNCTION(Test_set) {
  lepus::Value obj = MK_JS_LEPUS_VALUE(ctx, argv[0]);
  lepus::Value key = MK_JS_LEPUS_VALUE(ctx, argv[1]);
  lepus::Value val = MK_JS_LEPUS_VALUE(ctx, argv[2]);

  if (key.IsString()) {
    obj.SetProperty(key.String(), val);
  } else if (key.IsNumber()) {
    obj.SetProperty((int)key.Number(), val);
  }
}

PRIM_CFUNCTION(Test_Contains) {
  lepus::Value obj = MK_JS_LEPUS_VALUE(ctx, argv[0]);
  lepus::Value key = MK_JS_LEPUS_VALUE(ctx, argv[1]);

  base::String s_key = key.String();
  std::cout << "contains " << s_key.c_str() << " : " << obj.Contains(s_key)
            << std::endl;
}

PRIM_CFUNCTION(UpdateComponentInfo) {
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

PRIM_CFUNCTION(TestArgcNG) {
  CONVERT_ARG(arg1, 0);
  if (!arg1->IsString()) {
    return lepus::LEPUSValueHelper::ToJsValue(
        ctx,
        lepus::QuickContext::GetFromJsContext(ctx)->ReportFatalError(
            "arg is not string", false, error::E_MTS_RENDERER_FUNCTION_FATAL));
  }
  return lepus::LEPUSValueHelper::ToJsValue(ctx, *arg1);
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

class TestLepusNGContextHolder {
 protected:
  explicit TestLepusNGContextHolder(lynx::runtime::ContextType type)
      : owned_context_(new lynx::runtime::MTSRuntime(
            type, false, 0, lynx::tasm::PageOptions())) {}

  lynx::runtime::MTSRuntime* context() const { return owned_context_.get(); }

 private:
  std::unique_ptr<lynx::runtime::MTSRuntime> owned_context_;
};

class TestLepusNG : private TestLepusNGContextHolder,
                    public lynx::lepus::ContextBinaryWriter {
 public:
  TestLepusNG()
      : TestLepusNGContextHolder(
            lynx::runtime::ContextType::LepusNGContextType),
        lynx::lepus::ContextBinaryWriter(context()->GetMTSContext()) {}
  void Run(const char* input, const char* source) {
    std::string lepus_resource = source;
    if (strcmp(source, "") == 0) {
      lepus_resource = lepus::readFile(input);
    }

    lepus::QuickContext* ctx =
        lynx::runtime::MTSRuntime::ToQuickContext(context());
    RegisterQuickBuiltins(ctx);
    ctx->Initialize();
    auto error = lynx::lepus::BytecodeGenerator::GenerateBytecode(
        context()->GetMTSContext(), lepus_resource, "");
    context()->Execute(nullptr);
  }
};

std::vector<uint8_t> ReadBinary(std::string full_path) {
  std::ifstream instream(full_path, std::ios::in | std::ios::binary);
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)),
                            std::istreambuf_iterator<char>());
  return data;
}

static bool g_dump_ir = false;

void CustomInit(int argc, char** argv) {
  // Keep historical behavior: if `argv[1]` provides an input file path, only
  // compile that single file.
  if (argc > 1 && argv[1] != nullptr && argv[1][0] != '-') {
    TestLepus::input = argv[1];
  }

  // Only dump IR when explicitly requested.
  for (int i = 1; i < argc; i++) {
    const char* arg = argv[i];
    if (arg == nullptr) {
      continue;
    }
    if (strcmp(arg, "--dump-ir") == 0 || strcmp(arg, "--dump-ir=1") == 0 ||
        strcmp(arg, "--dump-ir=true") == 0) {
      g_dump_ir = true;
    }
  }
}

TEST(lepus, compile) {
  TestLepus t;
  t.Run("", false);
}

TEST(lepus, compile_opt_lepus_bytecode) {
  // if use --dump-ir, dump ir passes in folder
  // `lynx/core/runtime/lepus/compiler/unit_test/ir_dumps`
  const auto unit_test_dir =
      std::filesystem::path("lynx/core/runtime/lepus/compiler/unit_test");
  const auto dump_root = unit_test_dir / "ir_dumps";

  // If `argv[1]` provides an input file path, only compile that single file.
  if (TestLepus::input != nullptr && strcmp(TestLepus::input, "") != 0) {
    const std::filesystem::path input_path(TestLepus::input);
    TestLepus t;
    if (g_dump_ir) {
      std::filesystem::create_directories(dump_root);
      const std::string dump_path = (dump_root / input_path.stem()).string();
      t.Run("", true, dump_path.c_str());
    } else {
      t.Run("", true, nullptr);
    }
    return;
  }
}

static bool CompileAndExecuteAndReadResult(const std::string& src,
                                           bool opt_bytecode,
                                           bool null_prop_as_undef,
                                           bool strict_check,
                                           lepus::Value* out_result) {
  auto rt =
      runtime::MTSRuntime::CreateContext(runtime::ContextType::VMContextType);
  if (!rt) return false;
  rt->Initialize();

  auto* vm = runtime::MTSRuntime::ToVMContext(rt.get());
  if (!vm) return false;
  RegisterBuiltinTest(vm);
  vm->SetClosureFix(true);
  vm->SetOptBytecode(opt_bytecode);
  vm->SetNullPropAsUndef(null_prop_as_undef);
  vm->SetEnableStrictCheck(strict_check);

  auto err = lepus::BytecodeGenerator::GenerateBytecode(
      rt->GetMTSContext(), src, "2.6", "", nullptr);
  if (!err.empty()) return false;
  if (!rt->Execute(nullptr)) return false;

  if (!out_result) return true;
  lepus::Value v;
  if (!vm->GetTopLevelVariableByName(base::String("result"), &v)) {
    return false;
  }
  *out_result = v;
  return true;
}

TEST(lepus, opt_bytecode_strict_optional_chain_behavior_consistent) {
  // This test protects the semantic contract: enabling IR/O1 bytecode
  // optimization must not change runtime behavior.
  //
  // In strict-check mode, property access on nullish receivers throws.
  // Optional chaining lowering relies on a guard to avoid the throw.
  const std::string src = R"(
var x = null;
var result = x?.a;
)";

  constexpr bool kNullPropAsUndef = false;  // majority pages
  constexpr bool kStrict = true;

  lepus::Value res_no_ir;
  lepus::Value res_ir;
  ASSERT_TRUE(CompileAndExecuteAndReadResult(
      src, /*opt_bytecode=*/false, kNullPropAsUndef, kStrict, &res_no_ir));
  ASSERT_TRUE(CompileAndExecuteAndReadResult(
      src, /*opt_bytecode=*/true, kNullPropAsUndef, kStrict, &res_ir));

  EXPECT_EQ(res_no_ir.Type(), res_ir.Type());
  EXPECT_TRUE(res_no_ir.IsNil());
}
