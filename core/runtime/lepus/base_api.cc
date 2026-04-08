// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/lepus/base_api.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#include "base/include/value/table.h"
#include "core/runtime/common/lynx_console_helper.h"
#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {

static std::string GetPrintStr(VMContext* context) {
  long params_count = context->GetParamsSize();
  std::ostringstream s;
  s << "[main-thread.js] ";
  for (long i = 0; i < params_count; i++) {
    Value v(*context->GetParam(i));
    v.PrintValue(s);
    if (i < params_count - 1) {
      s << " ";
    }
  }
  return s.str();
}

static RestrictedValue Console_Log(VMContext* context) {
  std::string msg = GetPrintStr(context);
#ifdef LEPUS_PC
  LOGE(msg);
#endif
  context->OnBTSConsoleEvent(runtime::ConsoleLog, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Warn(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleWarn, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Error(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleError, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Info(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleInfo, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Debug(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleDebug, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Report(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleReport, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Alog(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleAlog, msg);
  return RestrictedValue();
}

static RestrictedValue Assert(VMContext* context) {
  UNUSED_LOG_VARIABLE auto* condition = context->GetParam(1);
  auto* msg = context->GetParam(2);
  std::string s = "Assertion failed:" + msg->StdString();
  assert(condition->IsTrue() && s.c_str());
  return RestrictedValue();
}

static RestrictedValue Console_Count(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleCount, msg);
  return RestrictedValue();
}

static RestrictedValue Console_CountReset(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleCountReset, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Group(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleGroup, msg);
  return RestrictedValue();
}

static RestrictedValue Console_GroupCollapsed(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleGroupCollapsed, msg);
  return RestrictedValue();
}

static RestrictedValue Console_GroupEnd(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleGroupEnd, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Time(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleTime, msg);
  return RestrictedValue();
}

static RestrictedValue Console_TimeLog(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleTimeLog, msg);
  return RestrictedValue();
}

static RestrictedValue Console_TimeEnd(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleTimeEnd, msg);
  return RestrictedValue();
}

static RestrictedValue Console_Table(VMContext* context) {
  std::string msg = GetPrintStr(context);
  context->OnBTSConsoleEvent(runtime::ConsoleTable, msg);
  return RestrictedValue();
}

void RegisterBaseAPI(VMContext* ctx) {
#if 1
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, runtime::ConsoleLog, &Console_Log);
  RegisterTableFunction(ctx, table, runtime::ConsoleWarn, &Console_Warn);
  RegisterTableFunction(ctx, table, runtime::ConsoleError, &Console_Error);
  RegisterTableFunction(ctx, table, runtime::ConsoleInfo, &Console_Info);
  RegisterTableFunction(ctx, table, runtime::ConsoleDebug, &Console_Debug);
  RegisterTableFunction(ctx, table, runtime::ConsoleReport, &Console_Report);
  RegisterTableFunction(ctx, table, runtime::ConsoleAlog, &Console_Alog);
  RegisterTableFunction(ctx, table, runtime::ConsoleAssert, &Assert);
  RegisterTableFunction(ctx, table, runtime::ConsoleCount, &Console_Count);
  RegisterTableFunction(ctx, table, runtime::ConsoleCountReset,
                        &Console_CountReset);
  RegisterTableFunction(ctx, table, runtime::ConsoleGroup, &Console_Group);
  RegisterTableFunction(ctx, table, runtime::ConsoleGroupCollapsed,
                        &Console_GroupCollapsed);
  RegisterTableFunction(ctx, table, runtime::ConsoleGroupEnd,
                        &Console_GroupEnd);
  RegisterTableFunction(ctx, table, runtime::ConsoleTime, &Console_Time);
  RegisterTableFunction(ctx, table, runtime::ConsoleTimeLog, &Console_TimeLog);
  RegisterTableFunction(ctx, table, runtime::ConsoleTimeEnd, &Console_TimeEnd);
  RegisterTableFunction(ctx, table, runtime::ConsoleTable, &Console_Table);
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

static RestrictedValue toFixed(VMContext* context) {
  long params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2);
  RestrictedValue n;               // for precision
  auto* v = context->GetParam(1);  // for value
  if (params_count == 1) {
    n = RestrictedValue(0);
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
  return RestrictedValue(os.str());
}

const RestrictedValue& GetNumberPrototypeAPI(const base::String& key) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::NumberPrototype,
                                   {
                                       {"toFixed", &toFixed},
                                   });

  return apis.GetFunction(key);
}

}  // namespace lepus
}  // namespace lynx
