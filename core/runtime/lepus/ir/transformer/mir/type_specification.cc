// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/type_specification.h"

#include <set>

#include "core/runtime/lepus/bindings/renderer.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/StringSwitch.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/lepus_global.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {
static bool IsReadonlyMathMethodName(const std::string& name) {
  // Conservative list: exclude functions that may depend on mutable runtime
  // state or allocate (e.g. random).
  return name == constants::kMathAbs || name == constants::kMathAcos ||
         name == constants::kMathAsin || name == constants::kMathAtan ||
         name == constants::kMathCeil || name == constants::kMathCos ||
         name == constants::kMathExp || name == constants::kMathFloor ||
         name == constants::kMathLog || name == constants::kMathMax ||
         name == constants::kMathMin || name == constants::kMathPow ||
         name == constants::kMathRound || name == constants::kMathSin ||
         name == constants::kMathSqrt || name == constants::kMathTan;
}

static bool IsReadonlyStringStaticMethodName(const std::string& name) {
  // String functions registered in `RegisterStringAPI`.
  return name == constants::kStringIndexOf ||
         name == constants::kStringLength || name == constants::kStringSubstr;
}

static bool IsReadonlyJSONMethodName(const std::string& name) {
  // Both are implemented in C++ and do not mutate existing JS heap objects.
  return name == constants::kJSONStringify || name == constants::kJSONParse;
}

static bool IsReadonlyDateMethodName(const std::string& name) {
  return name == constants::kDateNow;
}

static bool IsReadonlyObjectMethodName(const std::string& name) {
  // `keys` creates a new array but does not mutate input objects.
  // NOTE: `assign`/`freeze` are mutating and must not be treated as read-only.
  return name == constants::kObjectKeys;
}

static bool IsReadonlyArrayPrototypeMethodName(const std::string& name) {
  // Must be consistent with `GetArrayPrototypeAPI` in `array_api.cc`.
  // Keep conservative:
  // - Exclude mutators: push/pop/shift
  // - Exclude callbacks: map/filter/find/findIndex/forEach
  return name == constants::kArrayConcat || name == constants::kArrayJoin ||
         name == constants::kArrayIncludes || name == constants::kArraySlice;
}

static bool IsReadonlyStringPrototypeMethodName(const std::string& name) {
  // Must be consistent with `GetStringPrototypeAPI` in `string_api.cc`.
  // Keep conservative:
  // - Exclude `replace` because it may invoke a user-provided replacer closure.
  return name == constants::kStringSplit || name == constants::kStringTrim ||
         name == constants::kStringCharAt || name == constants::kStringSearch ||
         name == constants::kStringMatch || name == constants::kStringSlice ||
         name == constants::kStringSubstring ||
         name == constants::kStringSubstr;
}

static bool IsReadonlyRegExpPrototypeMethodName(const std::string& name) {
  // `GetRegexPrototypeAPI` in `regexp_api.cc`.
  return name == constants::kRegExpTest;
}

static bool IsReadonlyNumberPrototypeMethodName(const std::string& name) {
  // `GetNumberPrototypeAPI` in `base_api.cc`.
  return name == constants::kNumberToFixed;
}

static void SetCallReadOnlyAttr(IRContext* ir_ctx, FuncOp* func) {
  auto* vm_ctx = ir_ctx->GetVMContext();
  if (!vm_ctx) return;
  auto* global = vm_ctx->global();
  auto* builtin = vm_ctx->builtin();
  auto lepus_func = func->GetLepusFunction();

  // Pre-compute indices
  int math_global_index = global ? global->Search(constants::kGlobalMath) : -1;
  int string_global_index =
      global ? global->Search(constants::kGlobalString) : -1;
  int json_builtin_index =
      builtin ? builtin->Search(constants::kBuiltinJSON) : -1;
  int date_builtin_index =
      builtin ? builtin->Search(constants::kBuiltinDate) : -1;
  int object_builtin_index =
      builtin ? builtin->Search(constants::kBuiltinObject) : -1;

  std::set<uint32_t> readonly_builtin_indices;
  if (builtin) {
    auto add_if_present = [&](const char* name) {
      int idx = builtin->Search(name);
      if (idx >= 0) {
        readonly_builtin_indices.insert(static_cast<uint32_t>(idx));
      }
    };
    add_if_present(constants::kParseFloat);
    add_if_present(constants::kParseInt);
    add_if_present(constants::kIsFinite);
    add_if_present(constants::kIsNaN);
    add_if_present(constants::kEncodeURI);
    add_if_present(constants::kEncodeURIComponent);
    add_if_present(constants::kDecodeURI);
    add_if_present(constants::kDecodeURIComponent);
  }

  auto get_lepus_value_func = [&](Value* v) -> lepus::Value {
    if (auto* literal = llvh::dyn_cast<Literal>(v)) {
      switch (literal->GetKind()) {
        case ValueKind::LiteralInt8Kind:
          return lepus::Value(static_cast<int32_t>(
              llvh::cast<LiteralInt8>(literal)->GetValue()));
        case ValueKind::LiteralInt32Kind:
          return lepus::Value(llvh::cast<LiteralInt32>(literal)->GetValue());
        case ValueKind::LiteralUint8Kind:
          return lepus::Value(static_cast<uint32_t>(
              llvh::cast<LiteralUint8>(literal)->GetValue()));
        case ValueKind::LiteralUint32Kind:
          return lepus::Value(llvh::cast<LiteralUint32>(literal)->GetValue());
        case ValueKind::LiteralFloat64Kind:
          return lepus::Value(llvh::cast<LiteralFloat64>(literal)->GetValue());
        case ValueKind::LiteralBoolKind:
          return lepus::Value(llvh::cast<LiteralBool>(literal)->GetValue());
        default:
          break;
      }
    } else if (auto* load_const_inst = llvh::dyn_cast<LoadConstInst>(v)) {
      Literal* lit = load_const_inst->GetConst();
      if (auto* u32 = llvh::dyn_cast<LiteralUint32>(lit)) {
        if (lepus_func &&
            u32->GetValue() < lepus_func->GetConstValue().size()) {
          return *lepus_func->GetConstValue(u32->GetValue());
        }
      }
    }
    return lepus::Value();
  };

  auto is_readonly_builtin_call = [&](CallInst* call) -> bool {
    Value* callee = call->GetFunction();
    auto* get_builtin = llvh::dyn_cast<GetBuiltinInst>(callee);
    if (!get_builtin) return false;
    uint32_t idx = get_builtin->GetBuiltinIndex()->GetValue();
    return readonly_builtin_indices.count(idx);
  };

  auto is_readonly_global_call = [&](CallInst* call) -> bool {
    Value* callee = call->GetFunction();
    auto* get_table = llvh::dyn_cast<GetTableInst>(callee);
    if (!get_table) return false;

    Value* key_or_obj = get_table->GetObject();
    Value* fn_name_v = get_table->GetProp();
    if (!llvh::dyn_cast<LoadConstInst>(fn_name_v)) return false;
    if (!fn_name_v->GetType() || !fn_name_v->GetType()->IsStringType())
      return false;

    lepus::Value fn_val = get_lepus_value_func(fn_name_v);
    if (!fn_val.IsString()) return false;
    std::string func_name = fn_val.StdString();

    if (auto* get_global = llvh::dyn_cast<GetGlobalInst>(key_or_obj)) {
      auto* global_u32 =
          llvh::dyn_cast<LiteralUint32>(get_global->GetGlobalIndex());
      if (!global_u32) return false;
      int global_index = static_cast<int>(global_u32->GetValue());
      if (global_index == math_global_index)
        return IsReadonlyMathMethodName(func_name);
      if (global_index == string_global_index)
        return IsReadonlyStringStaticMethodName(func_name);
      return false;
    }

    if (auto* get_builtin = llvh::dyn_cast<GetBuiltinInst>(key_or_obj)) {
      int builtin_index =
          static_cast<int>(get_builtin->GetBuiltinIndex()->GetValue());
      if (builtin_index == json_builtin_index)
        return IsReadonlyJSONMethodName(func_name);
      if (builtin_index == date_builtin_index)
        return IsReadonlyDateMethodName(func_name);
      if (builtin_index == object_builtin_index)
        return IsReadonlyObjectMethodName(func_name);
      return false;
    }
    return false;
  };

  auto is_readonly_prototype_call = [&](CallInst* call) -> bool {
    Value* callee = call->GetFunction();
    Value* receiver = nullptr;
    std::string fn_name;
    if (auto* get_table = llvh::dyn_cast<GetTableInst>(callee)) {
      receiver = get_table->GetObject();
      Value* fn_name_v = get_table->GetProp();
      if (!llvh::dyn_cast<LoadConstInst>(fn_name_v)) return false;
      if (!fn_name_v->GetType() || !fn_name_v->GetType()->IsStringType())
        return false;

      lepus::Value fn_val = get_lepus_value_func(fn_name_v);
      if (!fn_val.IsString()) return false;
      fn_name = fn_val.StdString();
    } else if (auto* get_table_const =
                   llvh::dyn_cast<GetTableConstStringKeyInst>(callee)) {
      receiver = get_table_const->GetObject();
      auto* idx_lit =
          llvh::dyn_cast<LiteralUint32>(get_table_const->GetConstIndex());
      if (!idx_lit) return false;
      if (!lepus_func ||
          idx_lit->GetValue() >= lepus_func->GetConstValue().size())
        return false;
      auto* v = lepus_func->GetConstValue(idx_lit->GetValue());
      if (!v || !v->IsString()) return false;
      fn_name = v->StdString();
    } else {
      return false;
    }

    auto* ty = receiver ? receiver->GetType() : nullptr;
    if (!ty) return false;

    if (ty->IsStringType()) {
      return IsReadonlyStringPrototypeMethodName(fn_name);
    }
    if (ty->IsArrayType()) {
      return IsReadonlyArrayPrototypeMethodName(fn_name);
    }
    if (ty->GetTypeKind() == TypeOp::RegExp) {
      return IsReadonlyRegExpPrototypeMethodName(fn_name);
    }
    if (ty->IsNumberType()) {
      return IsReadonlyNumberPrototypeMethodName(fn_name);
    }

    return false;
  };

  for (auto& bb : *func) {
    for (auto& inst : bb) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        if (is_readonly_builtin_call(call_inst) ||
            is_readonly_global_call(call_inst) ||
            is_readonly_prototype_call(call_inst)) {
          call_inst->SetReadonlyCall(true);
        }
      }
    }
  }
}

static void SetPrototypeCallType(IRContext* ir_ctx, FuncOp* func) {
  OpBuilder* builder = ir_ctx->GetOpBuilder();

  auto get_const_string_by_index = [&](uint32_t idx) -> std::string {
    auto lepus_func = func->GetLepusFunction();
    if (lepus_func && idx < lepus_func->GetConstValue().size()) {
      auto* val = lepus_func->GetConstValue(idx);
      if (val && val->IsString()) return val->StdString();
    }
    return "";
  };

  auto get_const_string = [&](Value* v) -> std::string {
    if (auto* load_const = llvh::dyn_cast<LoadConstInst>(v)) {
      if (auto* lit =
              llvh::dyn_cast<LiteralUint32>(load_const->GetSingleOperand())) {
        return get_const_string_by_index(lit->GetValue());
      }
    }
    return "";
  };

  for (auto& bb : *func) {
    for (auto& inst : bb) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        Value* callee = call_inst->GetFunction();

        // String prototype calls are handled by
        // `SpecifyGetTableForStringProtoType`.
        if (callee && callee->GetType() &&
            callee->GetType()->IsStringProtoAPIType()) {
          continue;
        }

        Value* receiver = nullptr;
        std::string name;
        if (auto* get_table = llvh::dyn_cast<GetTableInst>(callee)) {
          receiver = get_table->GetObject();
          name = get_const_string(get_table->GetProp());
        } else if (auto* get_table_const =
                       llvh::dyn_cast<GetTableConstStringKeyInst>(callee)) {
          receiver = get_table_const->GetObject();
          if (auto* lit = llvh::dyn_cast<LiteralUint32>(
                  get_table_const->GetConstIndex())) {
            name = get_const_string_by_index(lit->GetValue());
          }
        }

        if (!receiver || !receiver->GetType()) continue;
        if (name.empty()) continue;

        TypeOp* type = nullptr;

        // Prefer receiver static type when available. If receiver type is not
        // yet inferred (or is too generic), fall back to method-name-based
        // classification so prototype return types stay stable under IR
        // canonicalization.
        bool is_array_receiver = receiver->GetType()->IsArrayType();
        bool is_number_receiver = receiver->GetType()->IsNumberType();
        bool is_regexp_receiver =
            receiver->GetType()->GetTypeKind() == TypeOp::RegExp;

        // Only apply name-based fallback when the receiver type is unknown.
        if (!is_array_receiver && !is_number_receiver && !is_regexp_receiver &&
            receiver->GetType()->IsAnyType()) {
          if (name == constants::kArrayPush || name == constants::kArrayPop ||
              name == constants::kArrayFindIndex ||
              name == constants::kArrayMap || name == constants::kArrayFilter ||
              name == constants::kArrayConcat ||
              name == constants::kArraySlice || name == constants::kArrayJoin ||
              name == constants::kArrayIncludes) {
            is_array_receiver = true;
          } else if (name == constants::kNumberToFixed) {
            is_number_receiver = true;
          } else if (name == constants::kRegExpTest) {
            is_regexp_receiver = true;
          }
        }

        TypeOp::TypeKind receiver_kind_for_name =
            receiver->GetType()->GetTypeKind();
        if (is_array_receiver) {
          receiver_kind_for_name = TypeOp::Array;
        } else if (is_number_receiver) {
          receiver_kind_for_name = TypeOp::Number;
        } else if (is_regexp_receiver) {
          receiver_kind_for_name = TypeOp::RegExp;
        }

        if (is_array_receiver) {
          // string prototype: push, pop, shift, map, filter, concat, join,
          // findIndex, find, includes, slice, foreach
          if (name == constants::kArrayPush || name == constants::kArrayPop) {
            type = TypeOp::CreateUint64(builder);
          } else if (name == constants::kArrayFindIndex) {
            type = TypeOp::CreateInt64(builder);
          } else if (name == constants::kArrayMap ||
                     name == constants::kArrayFilter ||
                     name == constants::kArrayConcat ||
                     name == constants::kArraySlice) {
            type = TypeOp::CreateArray(builder);
          } else if (name == constants::kArrayJoin) {
            type = TypeOp::CreateString(builder);
          } else if (name == constants::kArrayIncludes) {
            type = TypeOp::CreateBoolean(builder);
          }
        } else if (is_number_receiver) {
          // number prototype: toFixed
          if (name == constants::kNumberToFixed) {
            type = TypeOp::CreateString(builder);
          }
        } else if (is_regexp_receiver) {
          // regexp prototype: test
          if (name == constants::kRegExpTest) {
            type = TypeOp::CreateBoolean(builder);
          }
        }

        if (type) {
          call_inst->SetType(type);
          call_inst->SetBuiltinFuncName(
              std::string(TypeOp::RetKindStr(receiver_kind_for_name)) + "." +
              name);
        }
      }
    }
  }
}

static void SetBuiltinCallType(IRContext* ir_ctx, FuncOp* func) {
  auto* vm_ctx = ir_ctx->GetVMContext();
  if (!vm_ctx) return;
  auto* global = vm_ctx->global();
  auto* builtin = vm_ctx->builtin();

  OpBuilder* builder = ir_ctx->GetOpBuilder();

  auto get_const_string = [&](Value* v) -> std::string {
    if (auto* load_const = llvh::dyn_cast<LoadConstInst>(v)) {
      if (auto* lit =
              llvh::dyn_cast<LiteralUint32>(load_const->GetSingleOperand())) {
        auto lepus_func = func->GetLepusFunction();
        if (lepus_func &&
            lit->GetValue() < lepus_func->GetConstValue().size()) {
          auto* val = lepus_func->GetConstValue(lit->GetValue());
          if (val->IsString()) return val->StdString();
        }
      }
    }
    return "";
  };

  auto is_global_func = [&](int idx, const char* name) {
    return global && idx == global->Search(name);
  };

  auto is_builtin_func = [&](int idx, const char* name) {
    return builtin && idx == builtin->Search(name);
  };

  for (auto& bb : *func) {
    for (auto& inst : bb) {
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        auto* callee = call_inst->GetFunction();
        if (auto* get_global = llvh::dyn_cast<GetGlobalInst>(callee)) {
          auto index_lit =
              llvh::dyn_cast<LiteralUint32>(get_global->GetGlobalIndex());
          if (index_lit) {
            int idx = static_cast<int>(index_lit->GetValue());
            if (is_global_func(idx, constants::kGetElementUniqueID)) {
              call_inst->SetType(TypeOp::CreateInt64(builder));
              call_inst->SetBuiltinFuncName(constants::kGetElementUniqueID);
            } else if (is_global_func(idx, constants::kCreateElement)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreateElement);
            } else if (is_global_func(idx, constants::kCreateView)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreateView);
            } else if (is_global_func(idx, constants::kCreateImage)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreateImage);
            } else if (is_global_func(idx, constants::kCreateText)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreateText);
            } else if (is_global_func(idx, constants::kCreatePage)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreatePage);
            } else if (is_global_func(idx, constants::kCreateComponent)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(constants::kCreateComponent);
            } else if (is_global_func(idx, constants::kGetDiffData)) {
              call_inst->SetType(TypeOp::CreateTable(builder));
              call_inst->SetBuiltinFuncName(constants::kGetDiffData);
            } else if (is_global_func(idx, constants::kGetSystemInfo)) {
              call_inst->SetType(TypeOp::CreateTable(builder));
              call_inst->SetBuiltinFuncName(constants::kGetSystemInfo);
            } else if (is_global_func(idx, constants::kGetTextInfo)) {
              call_inst->SetType(TypeOp::CreateTable(builder));
              call_inst->SetBuiltinFuncName(constants::kGetTextInfo);
            } else if (is_global_func(idx, constants::kIsArray)) {
              call_inst->SetType(TypeOp::CreateBoolean(builder));
              call_inst->SetBuiltinFuncName(constants::kIsArray);
            } else if (is_global_func(idx, tasm::kCFunctionSetStyleObject)) {
              call_inst->SetType(TypeOp::CreateAnyType(builder));
              call_inst->SetBuiltinFuncName(tasm::kCFunctionSetStyleObject);
            }
          }
        } else if (auto* get_builtin = llvh::dyn_cast<GetBuiltinInst>(callee)) {
          // builtin function: parseInt, parseFloat, isNaN, encodeURIComponent,
          // decodeURIComponent
          auto index_lit = get_builtin->GetBuiltinIndex();
          if (index_lit) {
            int idx = static_cast<int>(index_lit->GetValue());
            if (is_builtin_func(idx, constants::kParseInt)) {
              call_inst->SetType(TypeOp::CreateNumber(builder));
              call_inst->SetBuiltinFuncName(constants::kParseInt);
            } else if (is_builtin_func(idx, constants::kParseFloat)) {
              call_inst->SetType(TypeOp::CreateNumber(builder));
              call_inst->SetBuiltinFuncName(constants::kParseFloat);
            } else if (is_builtin_func(idx, constants::kIsNaN)) {
              call_inst->SetType(TypeOp::CreateBoolean(builder));
              call_inst->SetBuiltinFuncName(constants::kIsNaN);
            } else if (is_builtin_func(idx, constants::kEncodeURIComponent)) {
              call_inst->SetType(TypeOp::CreateString(builder));
              call_inst->SetBuiltinFuncName(constants::kEncodeURIComponent);
            } else if (is_builtin_func(idx, constants::kDecodeURIComponent)) {
              call_inst->SetType(TypeOp::CreateString(builder));
              call_inst->SetBuiltinFuncName(constants::kDecodeURIComponent);
            }
          }
        } else if (auto* get_table = llvh::dyn_cast<GetTableInst>(callee)) {
          // global function: GetGlobalInst
          Value* obj = get_table->GetObject();
          Value* prop = get_table->GetProp();
          std::string name = get_const_string(prop);
          if (name.empty()) continue;

          if (auto* get_global = llvh::dyn_cast<GetGlobalInst>(obj)) {
            if (auto* idx_lit = llvh::dyn_cast<LiteralUint32>(
                    get_global->GetGlobalIndex())) {
              int idx = static_cast<int>(idx_lit->GetValue());
              if (is_global_func(idx, constants::kGlobalMath)) {
                // Math global function: sin, abs, acos, atan, asin,  ceil, cos,
                // exp, floor, log, max, min, pow, random, round, sqrt, tan
                call_inst->SetType(TypeOp::CreateNumber(builder));
                call_inst->SetBuiltinFuncName(
                    std::string(constants::kGlobalMath) + "." + name);
              } else if (is_global_func(idx, constants::kGlobalString)) {
                // String static methods: indexOf, length, substr
                if (name == constants::kStringIndexOf) {
                  call_inst->SetType(TypeOp::CreateNumber(builder));
                  call_inst->SetBuiltinFuncName(
                      std::string(constants::kGlobalString) + "." + name);
                } else if (name == constants::kStringLength) {
                  call_inst->SetType(TypeOp::CreateNumber(builder));
                  call_inst->SetBuiltinFuncName(
                      std::string(constants::kGlobalString) + "." + name);
                } else if (name == constants::kStringSubstr) {
                  call_inst->SetType(TypeOp::CreateString(builder));
                  call_inst->SetBuiltinFuncName(
                      std::string(constants::kGlobalString) + "." + name);
                }
              }
            }
          } else if (auto* get_builtin = llvh::dyn_cast<GetBuiltinInst>(obj)) {
            int idx =
                static_cast<int>(get_builtin->GetBuiltinIndex()->GetValue());
            if (is_builtin_func(idx, constants::kBuiltinJSON)) {
              if (name == constants::kJSONStringify) {
                call_inst->SetType(TypeOp::CreateString(builder));
              } else if (name == constants::kJSONParse) {
                call_inst->SetType(TypeOp::CreateAnyType(builder));
              }
              call_inst->SetBuiltinFuncName(
                  std::string(constants::kBuiltinJSON) + "." + name);
            } else if (is_builtin_func(idx, constants::kBuiltinDate)) {
              if (name == constants::kDateNow) {
                call_inst->SetType(TypeOp::CreateNumber(builder));
                call_inst->SetBuiltinFuncName(
                    std::string(constants::kBuiltinDate) + "." + name);
              }
            } else if (is_builtin_func(idx, constants::kBuiltinObject)) {
              // object builtin method: assign, freeze, keys
              if (name == constants::kObjectKeys) {
                call_inst->SetType(TypeOp::CreateArray(builder));
              } else if (name == constants::kObjectFreeze) {
                call_inst->SetType(TypeOp::CreateTable(builder));
              } else if (name == constants::kObjectAssign) {
                call_inst->SetType(TypeOp::CreateAnyType(builder));
              }
              call_inst->SetBuiltinFuncName(
                  std::string(constants::kBuiltinObject) + "." + name);
            }
          }
        }
      }
    }
  }
}

const std::string& TypeSpecification::GetConstString(
    LoadConstInst* load_const_inst) {
  auto* lit = llvh::cast<LiteralUint32>(load_const_inst->GetSingleOperand());
  auto const_id = lit->GetValue();

  lynx::lepus::Value* method_str =
      func_->GetLepusFunction()->GetConstValue(const_id);
  if (LEPUS_UNLIKELY(!method_str || !method_str->IsString())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: TypeSpecification expected const value to be string");
  }
  return method_str->StdString();
}

void TypeSpecification::Reset(FuncOp* func) {
  func_ = func;
  to_removed_.clear();
}

void TypeSpecification::SpecifyGetTableForStringProtoType() {
  OpBuilder* builder = ir_ctx_->GetOpBuilder();
  bool changed = false;

  auto get_const_string_by_index = [&](uint32_t idx) -> std::string {
    auto lepus_func = func_->GetLepusFunction();
    if (!lepus_func || idx >= lepus_func->GetConstValue().size()) return "";
    auto* v = lepus_func->GetConstValue(idx);
    if (!v || !v->IsString()) return "";
    return v->StdString();
  };

  for (auto& bb : *func_) {
    for (auto& inst : bb) {
      // find GetTableInst with string op
      if (auto* get_table_inst = llvh::dyn_cast<GetTableInst>(&inst)) {
        auto* object = get_table_inst->GetObject();
        auto* prop = get_table_inst->GetProp();
        // for getTable with object & prop as string, it means we call
        // string proto api.
        if (object->GetType()->IsStringType() &&
            prop->GetType()->IsStringType()) {
          get_table_inst->SetType(TypeOp::CreateStringProtoAPI(builder));

          // if the get_table is getStringLength, use new inst
          if (auto* load_const = llvh::dyn_cast<LoadConstInst>(prop)) {
            if (LEPUS_UNLIKELY(!load_const->GetType()->IsStringType())) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: TypeSpecification expected string proto "
                  "property to have string type");
            }
            auto str = GetConstString(load_const);
            // string.length, replace with GetStringLengthInst
            if (str == constants::kStringLength) {
              builder->SetInsertionPointAfter(get_table_inst);
              auto* get_length = builder->Create<GetStringLengthInst>(
                  get_table_inst->GetLocation(), get_table_inst->GetObject());
              get_table_inst->ReplaceAllUsesWith(get_length);
              to_removed_.push_back(get_table_inst);
            }
          }

          changed = true;
        }
      } else if (auto* get_table_const =
                     llvh::dyn_cast<GetTableConstStringKeyInst>(&inst)) {
        auto* object = get_table_const->GetObject();
        if (!object || !object->GetType() || !object->GetType()->IsStringType())
          continue;
        auto* idx_lit =
            llvh::dyn_cast<LiteralUint32>(get_table_const->GetConstIndex());
        if (!idx_lit) continue;

        const std::string name = get_const_string_by_index(idx_lit->GetValue());
        if (name.empty()) continue;

        get_table_const->SetType(TypeOp::CreateStringProtoAPI(builder));

        if (name == constants::kStringLength) {
          builder->SetInsertionPointAfter(get_table_const);
          auto* get_length = builder->Create<GetStringLengthInst>(
              get_table_const->GetLocation(), get_table_const->GetObject());
          get_table_const->ReplaceAllUsesWith(get_length);
          to_removed_.push_back(get_table_const);
        }

        changed = true;
      }
    }
  }

  if (!changed) return;

  llvh::for_each(to_removed_,
                 [&](Operation* inst) { inst->EraseFromParent(); });

  for (auto& bb : *func_) {
    for (auto& inst : bb) {
      // find CallInst with string proto api op
      if (auto* call_inst = llvh::dyn_cast<CallInst>(&inst)) {
        auto* call_func = call_inst->GetFunction();
        if (call_func->GetType()->IsStringProtoAPIType()) {
          std::string str;
          if (auto* get_table_inst = llvh::dyn_cast<GetTableInst>(call_func)) {
            auto prop = get_table_inst->GetProp();
            if (LEPUS_UNLIKELY(!llvh::isa<LoadConstInst>(prop))) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: TypeSpecification expected string proto "
                  "property to be LoadConstInst");
            }
            if (LEPUS_UNLIKELY(!llvh::cast<LoadConstInst>(prop)
                                    ->GetType()
                                    ->IsStringType())) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: TypeSpecification expected string proto "
                  "property LoadConstInst to have string type");
            }
            auto load_const = llvh::cast<LoadConstInst>(prop);
            str = GetConstString(load_const);
          } else if (auto* get_table_const =
                         llvh::dyn_cast<GetTableConstStringKeyInst>(
                             call_func)) {
            auto* idx_lit =
                llvh::dyn_cast<LiteralUint32>(get_table_const->GetConstIndex());
            if (LEPUS_UNLIKELY(!idx_lit)) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: TypeSpecification expected string proto "
                  "const key to be LiteralUint32");
            }
            str = get_const_string_by_index(idx_lit->GetValue());
            if (LEPUS_UNLIKELY(str.empty())) {
              throw ::lynx::lepus::CompileException(
                  "Lepus IR error: TypeSpecification expected const value to "
                  "be string");
            }
          } else {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: TypeSpecification expected string proto call "
                "function to be GetTableInst/GetTableConstStringKeyInst");
          }

          TypeOp* type =
              llvh::StringSwitch<TypeOp*>(str)
                  .Case(constants::kStringLength, TypeOp::CreateInt64(builder))
                  .Case(constants::kStringSplit, TypeOp::CreateArray(builder))
                  .Case(constants::kStringTrim, TypeOp::CreateString(builder))
                  .Case(constants::kStringCharAt, TypeOp::CreateString(builder))
                  .Case(constants::kStringMatch, TypeOp::CreateArray(builder))
                  .Case(constants::kStringSearch, TypeOp::CreateInt64(builder))
                  .Case(constants::kStringReplace,
                        TypeOp::CreateString(builder))
                  .Case(constants::kStringSlice, TypeOp::CreateString(builder))
                  .Case(constants::kStringSubstring,
                        TypeOp::CreateString(builder))
                  .Default(nullptr);

          if (type) {
            call_inst->SetType(type);
          } else {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: TypeSpecification encountered unsupported "
                "String proto API name");
          }
        }
      }
    }
  }
}

bool TypeSpecification::RunOnFunction(FuncOp* func) {
  Reset(func);

  SpecifyGetTableForStringProtoType();

  // set type for callInst
  SetBuiltinCallType(ir_ctx_, func);
  SetPrototypeCallType(ir_ctx_, func);

  // set readonly attribute for callInst
  SetCallReadOnlyAttr(ir_ctx_, func);

  // Propagate types
  PropagateTypes();

  return true;
}

void TypeSpecification::PropagateTypes() {
  bool changed = true;
  uint32_t iter = 0;
  while (changed && iter++ < constants::kTypePropagationIter) {
    changed = false;
    for (auto& bb : *func_) {
      for (auto& inst : bb) {
        if (auto* unary_inst = llvh::dyn_cast<UnaryOperatorInst>(&inst)) {
          auto old_type = unary_inst->GetType();
          unary_inst->SelectType(unary_inst->GetSingleOperand()->GetType(),
                                 unary_inst->GetKind());
          if (old_type->GetKind() != unary_inst->GetType()->GetKind()) {
            changed = true;
          }
        } else if (auto* binary_inst =
                       llvh::dyn_cast<BinaryOperatorInst>(&inst)) {
          auto old_type = binary_inst->GetType();
          binary_inst->SelectType(binary_inst->GetLeftHandSide()->GetType(),
                                  binary_inst->GetRightHandSide()->GetType(),
                                  binary_inst->GetKind());
          if (old_type->GetKind() != binary_inst->GetType()->GetKind()) {
            changed = true;
          }
        } else if (auto* phi_inst = llvh::dyn_cast<PhiInst>(&inst)) {
          auto old_type = phi_inst->GetType();
          phi_inst->RecalculateResultType();
          if (old_type->GetKind() != phi_inst->GetType()->GetKind()) {
            changed = true;
          }
        }
      }
    }
  }
}

Pass* CreateTypeSpecificationPass(IRContext* ir_ctx) {
  return new TypeSpecification(ir_ctx);
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
