// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_MATH_API_H_
#define CORE_RUNTIME_VM_LEPUS_MATH_API_H_

#include <math.h>
#include <time.h>

#include <utility>

namespace lynx {
namespace lepus {

Value Sin(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(sin(arg->Number()));
}
Value Acos(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(acos(arg->Number()));
}

Value Asin(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(asin(arg->Number()));
}

Value Abs(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(fabs(arg->Number()));
}

Value Atan(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(atan(arg->Number()));
}
Value Ceil(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(ceil(arg->Number()));
}
Value Cos(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(cos(arg->Number()));
}
Value Exp(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(exp(arg->Number()));
}
Value Floor(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(floor(arg->Number()));
}
Value Log(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(log(arg->Number()));
}
Value Max(VMContext* context) {
  Value* arg1 = context->GetParam(0);
  Value* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return Value();
  }
  return Value(fmax(arg1->Number(), arg2->Number()));
}
Value Min(VMContext* context) {
  Value* arg1 = context->GetParam(0);
  Value* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return Value();
  }
  return Value(fmin(arg1->Number(), arg2->Number()));
}
Value Pow(VMContext* context) {
  Value* arg1 = context->GetParam(0);
  Value* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return Value();
  }
  return Value(pow(arg1->Number(), arg2->Number()));
}
Value Random(VMContext* context) {
  static bool seeded = false;
  if (!seeded) {
    seeded = true;
    srand(static_cast<unsigned int>(time(NULL)));
  }
  return Value(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}
Value Round(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value((int32_t)round(arg->Number()));
}
Value Sqrt(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(sqrt(arg->Number()));
}
Value Tan(VMContext* context) {
  Value* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return Value();
  }
  return Value(tan(arg->Number()));
}

void RegisterMathAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "sin", &Sin);
  RegisterTableFunction(ctx, table, "abs", &Abs);
  RegisterTableFunction(ctx, table, "acos", &Acos);
  RegisterTableFunction(ctx, table, "atan", &Atan);
  RegisterTableFunction(ctx, table, "asin", &Asin);
  RegisterTableFunction(ctx, table, "ceil", &Ceil);
  RegisterTableFunction(ctx, table, "cos", &Cos);
  RegisterTableFunction(ctx, table, "exp", &Exp);
  RegisterTableFunction(ctx, table, "floor", &Floor);
  RegisterTableFunction(ctx, table, "log", &Log);
  RegisterTableFunction(ctx, table, "max", &Max);
  RegisterTableFunction(ctx, table, "min", &Min);
  RegisterTableFunction(ctx, table, "pow", &Pow);
  RegisterTableFunction(ctx, table, "random", &Random);
  RegisterTableFunction(ctx, table, "round", &Round);
  RegisterTableFunction(ctx, table, "sqrt", &Sqrt);
  RegisterTableFunction(ctx, table, "tan", &Tan);
  RegisterFunctionTable(ctx, "Math", std::move(table));
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_VM_LEPUS_MATH_API_H_
