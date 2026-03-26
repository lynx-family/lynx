// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/bytecode_generator.h"

#include <memory>

#include "core/parser/input_stream.h"
// NOTE(OSS): RTS runtime headers are non-open-source. This file only needs
// lepus/quickjs compilation paths, so avoid including RTS headers.
#include "core/runtime/lepus/code_generator.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/pass_manager/pipeline.h"
#include "core/runtime/lepus/ir/value.h"
#include "core/runtime/lepus/parser.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace lepus {

std::string BytecodeGenerator::GenerateBytecode(runtime::MTSContext* context,
                                                const std::string& source,
                                                const std::string& sdk_version,
                                                const std::string& file_name,
                                                const char* ir_dump_path) {
  if (!context) {
    return "Compile error: the context is nullptr.";
  }
  context->SetSdkVersion(sdk_version);
  if (context->Type() == runtime::ContextType::VMContextType) {
    return GenerateBytecodeForVMContext(static_cast<VMContext*>(context),
                                        source, sdk_version, ir_dump_path);
  }
  return GenerateBytecodeForQuickContext(static_cast<QuickContext*>(context),
                                         source, sdk_version, file_name);
}

std::string BytecodeGenerator::GenerateBytecodeForVMContext(
    VMContext* context, const std::string& source,
    const std::string& sdk_version, const char* ir_dump_path) {
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

  auto root_func = context->GetRootFunction();
  if (!root_func) return std::string();
  root_func->SetSource(source);

  if (context->GetOptBytecode()) {
    // optimize lepus bytecode.
    // std::cout << "opt lepus bytecode..." << std::endl;
    root_func->SetTopLevelFunction(true);
    auto ir_context = std::make_unique<ir::IRContext>(context);
    ir_context->Init(root_func, context);
    ir::RunO1OptimizationPasses(*ir_context->GetMainMod(), ir_dump_path);
  }

  return std::string();
}

std::string BytecodeGenerator::GenerateBytecodeForRTSContext(
    const std::string& source, const std::string& sdk_version) {
  // Decode base64 source to binary
  std::string decoded = source;
  lynx_modp_b64_decode(decoded);
  if (decoded.empty()) {
    return "Compile error: failed to decode base64 source.";
  }

  return decoded;
}

std::string BytecodeGenerator::GenerateBytecodeForQuickContext(
    QuickContext* context, const std::string& source,
    const std::string& sdk_version, const std::string& file_name) {
  if (context->GetDebuginfoOutside()) {
    SetLynxTargetSdkVersion(context->context(), sdk_version.c_str());
  }

  int eval_flags;
  LEPUSValue obj;

  context->SetDebugSourceCode(source);

  eval_flags = LEPUS_EVAL_FLAG_COMPILE_ONLY | LEPUS_EVAL_TYPE_GLOBAL;
  obj = LEPUS_Eval(context->context(), source.data(), source.length(),
                   file_name.c_str(), eval_flags);
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
