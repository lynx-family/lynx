// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/base_api.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#include "base/include/value/table.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static std::string GetPrintStr(VMContext* context) {
  long params_count = context->GetParamsSize();
  std::ostringstream s;
  s << "[main-thread.js] ";
  for (long i = 0; i < params_count; i++) {
    Value* v = context->GetParam(i);
    v->PrintValue(s);
    if (i < params_count - 1) {
      s << " ";
    }
  }
  return s.str();
}

static Value Console_Log(VMContext* context) {
  std::string msg = GetPrintStr(context);
#ifdef LEPUS_PC
  LOGE(msg);
#endif
  context->OnBTSConsoleEvent("log", msg);
  return Value();
}

static Value Console_Warn(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("warn", msg);
  return Value();
}

static Value Console_Error(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("error", msg);
  return Value();
}

static Value Console_Info(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("info", msg);
  return Value();
}

static Value Console_Debug(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("debug", msg);
  return Value();
}

static Value Console_Report(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("report", msg);
  return Value();
}

static Value Console_Alog(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("alog", msg);
  return Value();
}

static Value Assert(VMContext* context) {
  UNUSED_LOG_VARIABLE Value* condition = context->GetParam(1);
  Value* msg = context->GetParam(2);
  std::string s = "Assertion failed:" + msg->StdString();
  assert(condition->IsTrue() && s.c_str());
  return Value();
}

static Value Console_Count(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("count", msg);
  return Value();
}

static Value Console_CountReset(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("countReset", msg);
  return Value();
}

static Value Console_Group(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("group", msg);
  return Value();
}

static Value Console_GroupCollapsed(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("groupCollapsed", msg);
  return Value();
}

static Value Console_GroupEnd(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("groupEnd", msg);
  return Value();
}

static Value Console_Time(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("time", msg);
  return Value();
}

static Value Console_TimeLog(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("timeLog", msg);
  return Value();
}

static Value Console_TimeEnd(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("timeEnd", msg);
  return Value();
}

static Value Console_Table(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent("table", msg);
  return Value();
}

void RegisterBaseAPI(Context* ctx) {
#if 1
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "log", &Console_Log);
  RegisterTableFunction(ctx, table, "warn", &Console_Warn);
  RegisterTableFunction(ctx, table, "error", &Console_Error);
  RegisterTableFunction(ctx, table, "info", &Console_Info);
  RegisterTableFunction(ctx, table, "debug", &Console_Debug);
  RegisterTableFunction(ctx, table, "report", &Console_Report);
  RegisterTableFunction(ctx, table, "alog", &Console_Alog);
  RegisterTableFunction(ctx, table, "assert", &Assert);
  RegisterTableFunction(ctx, table, "count", &Console_Count);
  RegisterTableFunction(ctx, table, "countReset", &Console_CountReset);
  RegisterTableFunction(ctx, table, "group", &Console_Group);
  RegisterTableFunction(ctx, table, "groupCollapsed", &Console_GroupCollapsed);
  RegisterTableFunction(ctx, table, "groupEnd", &Console_GroupEnd);
  RegisterTableFunction(ctx, table, "time", &Console_Time);
  RegisterTableFunction(ctx, table, "timeLog", &Console_TimeLog);
  RegisterTableFunction(ctx, table, "timeEnd", &Console_TimeEnd);
  RegisterTableFunction(ctx, table, "table", &Console_Table);
  RegisterFunctionTable(ctx, "console", std::move(table));
#else
  // Not using BuiltinFunctionTable for Console apis because user
  // may redirect the api to thier own methods.
  static BuiltinFunctionTable apis(
      BuiltinFunctionTable::Console,
      {
          {"log", &Console_Log},
          {"warn", &Console_Warn},
          {"error", &Console_Error},
          {"info", &Console_Info},
          {"debug", &Console_Debug},
          {"report", &Console_Report},
          {"alog", &Console_Alog},
          {"assert", &Assert},
          {"count", &Console_Count},
          {"countReset", &Console_CountReset},
          {"group", &Console_Group},
          {"groupCollapsed", &Console_GroupCollapsed},
          {"groupEnd", &Console_GroupEnd},
          {"time", &Console_Time},
          {"timeLog", &Console_TimeLog},
          {"timeEnd", &Console_TimeEnd},
          {"table", &Console_Table},
      });
  RegisterFunctionTable(ctx, "console", &apis);
#endif
}

static Value toFixed(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2);
  Value n;                          // for precision
  Value* v = context->GetParam(1);  // for value
  if (params_count == 1) {
    n = Value(0);
    v = context->GetParam(0);
  } else {
    n = *context->GetParam(0);
    v = context->GetParam(1);
  }
  DCHECK(n.IsNumber());
  DCHECK(v->IsNumber());
  std::stringstream os;

  os << std::setiosflags(std::ios::fixed)
     << std::setprecision(static_cast<int>(n.Number())) << v->Number();
  return Value(os.str());
}

const Value& GetNumberPrototypeAPI(const base::String& key) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::NumberPrototype,
                                   {
                                       {"toFixed", &toFixed},
                                   });

  return apis.GetFunction(key);
}

}  // namespace lepus
}  // namespace lynx
