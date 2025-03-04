// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/vm/lepus/bytecode_generator.h"

#include <memory>

#include "core/parser/input_stream.h"
#include "core/runtime/vm/lepus/code_generator.h"
#include "core/runtime/vm/lepus/parser.h"

namespace lynx {
namespace lepus {

std::string BytecodeGenerator::GenerateBytecode(
    Context* context, const std::string& source,
    const std::string& sdk_version) {
  if (!context) {
    return "Compile error: the context is nullptr.";
  }
  context->SetSdkVersion(sdk_version);
  if (context->IsVMContext()) {
    return GenerateBytecodeForVMContext(static_cast<VMContext*>(context),
                                        source, sdk_version);
  }
  return GenerateBytecodeForQuickContext(static_cast<QuickContext*>(context),
                                         source, sdk_version);
}

std::string BytecodeGenerator::GenerateBytecodeForVMContext(
    VMContext* context, const std::string& source,
    const std::string& sdk_version) {
  parser::InputStream input;
  input.Write(source);
  Scanner scanner(&input);
  scanner.SetSdkVersion(sdk_version);
  Parser parser(&scanner);
  parser.SetSdkVersion(sdk_version);
  SemanticAnalysis semantic_analysis;
  semantic_analysis.SetInput(&scanner);
  semantic_analysis.SetSdkVersion(sdk_version);
  semantic_analysis.SetClosureFix(context->GetClosureFix());

  CodeGenerator code_generator(context, &semantic_analysis);
  std::unique_ptr<ASTree> root;
  root.reset(parser.Parse());
  root->Accept(&semantic_analysis, nullptr);
  // ASTDump ast_dump;
  // root->Accept(&ast_dump, nullptr);
  root->Accept(&code_generator, nullptr);

  if (context->GetRootFunction()) {
    context->GetRootFunction()->SetSource(source);
  }

  return std::string();
}

std::string BytecodeGenerator::GenerateBytecodeForQuickContext(
    QuickContext* context, const std::string& source,
    const std::string& sdk_version) {
  if (context->GetDebuginfoOutside()) {
    SetLynxTargetSdkVersion(context->context(), sdk_version.c_str());
  }

  int eval_flags;
  LEPUSValue obj;

  context->SetDebugSourceCode(source);

  eval_flags = LEPUS_EVAL_FLAG_COMPILE_ONLY | LEPUS_EVAL_TYPE_GLOBAL;
  obj = LEPUS_Eval(context->context(), source.data(), source.length(), "",
                   eval_flags);
  if (LEPUS_IsException(obj)) {
    std::string exception_message =
        "Compile error: " + context->GetExceptionMessage();
    LOGE(exception_message);
    return exception_message;
  }

  context->SetTopLevelFunction(obj);
  return std::string();
}

}  // namespace lepus
}  // namespace lynx
