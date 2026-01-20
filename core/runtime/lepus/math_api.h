// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_MATH_API_H_
#define CORE_RUNTIME_LEPUS_MATH_API_H_

#include <math.h>
#include <time.h>

#include <cmath>
#include <utility>

namespace lynx {
namespace lepus {

RestrictedValue Sin(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(sin(arg->Number()));
}

RestrictedValue Acos(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(acos(arg->Number()));
}

RestrictedValue Asin(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(asin(arg->Number()));
}

RestrictedValue Abs(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(fabs(arg->Number()));
}

RestrictedValue Atan(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(atan(arg->Number()));
}

RestrictedValue Ceil(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(ceil(arg->Number()));
}

RestrictedValue Cos(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(cos(arg->Number()));
}

RestrictedValue Exp(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(exp(arg->Number()));
}

RestrictedValue Floor(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(floor(arg->Number()));
}

RestrictedValue Log(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(log(arg->Number()));
}

RestrictedValue Max(VMContext* context) {
  auto* arg1 = context->GetParam(0);
  auto* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(fmax(arg1->Number(), arg2->Number()));
}

RestrictedValue Min(VMContext* context) {
  auto* arg1 = context->GetParam(0);
  auto* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(fmin(arg1->Number(), arg2->Number()));
}

RestrictedValue Pow(VMContext* context) {
  auto* arg1 = context->GetParam(0);
  auto* arg2 = context->GetParam(1);
  if (!arg1->IsNumber() || !arg2->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(pow(arg1->Number(), arg2->Number()));
}
RestrictedValue Random(VMContext* context) {
  static bool seeded = false;
  if (!seeded) {
    seeded = true;
    srand(static_cast<unsigned int>(time(NULL)));
  }
  return RestrictedValue(static_cast<float>(rand()) /
                         static_cast<float>(RAND_MAX));
}

RestrictedValue Round(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue((int32_t)round(arg->Number()));
}

RestrictedValue Sqrt(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(sqrt(arg->Number()));
}

RestrictedValue Tan(VMContext* context) {
  auto* arg = context->GetParam(0);
  if (!arg->IsNumber()) {
    return RestrictedValue();
  }
  return RestrictedValue(tan(arg->Number()));
}

void RegisterMathAPI(Context* ctx) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::Math,
                                   {
                                       {"sin", &Sin},
                                       {"abs", &Abs},
                                       {"acos", &Acos},
                                       {"atan", &Atan},
                                       {"asin", &Asin},
                                       {"ceil", &Ceil},
                                       {"cos", &Cos},
                                       {"exp", &Exp},
                                       {"floor", &Floor},
                                       {"log", &Log},
                                       {"max", &Max},
                                       {"min", &Min},
                                       {"pow", &Pow},
                                       {"random", &Random},
                                       {"round", &Round},
                                       {"sqrt", &Sqrt},
                                       {"tan", &Tan},
                                   });
  RegisterFunctionTable(ctx, "Math", &apis);
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_MATH_API_H_
