// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/lepus_date_api.h"

#include <map>
#include <utility>

#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/lepus_date.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {

int CDate::global_language = 1;  // default english

static const std::map<std::string, int>& DateGlobalization() {
  static const std::map<std::string, int> date_globalization = {{"zh-cn", 0},
                                                                {"en", 1}};
  return date_globalization;
}

const std::vector<std::string>& DateContent() {
  static const std::vector<std::string> date_content = {"zh-cn", "en"};
  return date_content;
}

static RestrictedValue ParseStringToDate(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2);
  auto* parsed = context->GetParam(0);
  DCHECK(parsed->IsNumber() || parsed->IsString());
  if (parsed->IsNumber()) {
    DCHECK(params_count == 1);
    int64_t parseNumber;
    if (parsed->IsInt64()) {
      parseNumber = parsed->Int64();
    } else if (parsed->IsInt32()) {
      parseNumber = parsed->Int32();
    } else {
      parseNumber = parsed->Number();
    }
    return RestrictedValue(CDate::ParseNumberToDate(parseNumber));
  } else if (parsed->IsString()) {
    const auto& date = context->GetParam(0)->StdString();
    std::string format;
    if (params_count == 1) {  // ISO8601 format "YYYY-MM-DDTHH-mm-ss.SSS+0800"
    } else {
      format = context->GetParam(1)->StdString();
    }
    return RestrictedValue(
        CDate::ParseStringToDate(static_cast<int>(params_count), date, format));
  } else {
    return RestrictedValue();
  }
}

static std::string FormatDateToString(RestrictedValue* date,
                                      const std::string& format) {
  return CDate::FormatToString(date, format);
}

static RestrictedValue LepusNow(VMContext* context) {
  return CDate::LepusNow();
}

static RestrictedValue LepusLocal(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 0);
  if (params_count == 0) {
    return RestrictedValue(DateContent()[CDate::global_language]);
  }
  const auto& setLanguage = context->GetParam(0)->StdString();
  auto it = DateGlobalization().find(setLanguage);
  if (it != DateGlobalization().end()) {
    CDate::global_language = it->second;
  }
  return RestrictedValue();
}

static RestrictedValue Locale(VMContext* context) {
  auto params_count = context->GetParamsSize();
  DCHECK(params_count == 1 || params_count == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count - 1)->RefCounted());
  if (params_count == 1) {
    return RestrictedValue(DateContent()[date->get_language()]);
  }
  const auto& setLanguage = context->GetParam(0)->StdString();
  auto it = DateGlobalization().find(setLanguage);
  if (it != DateGlobalization().end()) {
    return RestrictedValue(
        CDate::Create(date->get_date_(), date->get_ms_(), it->second));
  } else {
    return RestrictedValue();
  }
}

static RestrictedValue Unix(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1);
  auto date =
      fml::static_ref_ptr_cast<CDate>(context->GetParam(0)->RefCounted());
  time_t time1 = date->get_time_t_();
  int64_t time1_v = static_cast<int64_t>(time1);
  int64_t ret = static_cast<int64_t>(time1_v * 1000 + date->get_ms_());
  if (ret == -1) return RestrictedValue();
  return RestrictedValue((int64_t(ret)));
}

static RestrictedValue Format(VMContext* context) {
  auto params_count = context->GetParamsSize();
  if (params_count == 1) {
    char buf[64];
    const tm_extend t =
        fml::static_ref_ptr_cast<CDate>(context->GetParam(0)->RefCounted())
            ->get_date_();
    strftime(buf, 64, "%Y-%m-%dT%H:%M:%S", &t);
    return RestrictedValue(buf);
  }
  if (params_count != 2) {
    return RestrictedValue();
  }
  RestrictedValue* date = nullptr;
  const std::string* format;
  if (context->GetParam(0)->IsCDate()) {
    date = context->GetParam(0);
    format = &context->GetParam(1)->StdString();
  } else if (context->GetParam(0)->IsString()) {
    date = context->GetParam(1);
    format = &context->GetParam(0)->StdString();
  } else {
    return RestrictedValue();
  }
  return RestrictedValue(FormatDateToString(date, *format));
}

static RestrictedValue Year(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int year = date->get_date_().tm_year + 1900;
  return RestrictedValue(static_cast<uint32_t>(year));
}

static RestrictedValue Month(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int month = date->get_date_().tm_mon;
  return RestrictedValue(static_cast<uint32_t>(month));
}

static RestrictedValue Date(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int dat = date->get_date_().tm_mday;
  return RestrictedValue(static_cast<uint32_t>(dat));
}

static RestrictedValue Day(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int day = date->get_date_().tm_wday;
  return RestrictedValue(static_cast<uint32_t>(day));
}

static RestrictedValue Hour(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int Hour = date->get_date_().tm_hour;
  return RestrictedValue(static_cast<uint32_t>(Hour));
}

static RestrictedValue Minute(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int Min = date->get_date_().tm_min;
  return RestrictedValue(static_cast<uint32_t>(Min));
}

static RestrictedValue Sec(VMContext* context) {
  auto params_count_ = context->GetParamsSize();
  DCHECK(params_count_ == 1 || params_count_ == 2);
  auto date = fml::static_ref_ptr_cast<CDate>(
      context->GetParam(params_count_ - 1)->RefCounted());
  int second = date->get_date_().tm_sec;
  return RestrictedValue(static_cast<uint32_t>(second));
}

static RestrictedValue GetTimeZoneOffset(VMContext* context) {
  // return UTC - local / min
  return CDate::GetTimeZoneOffset();
}

void RegisterLepusDateAPI(Context* ctx) {
  static BuiltinFunctionTable apis(
      BuiltinFunctionTable::LepusDate,
      {
          {"now", &LepusNow},
          {"parse", &ParseStringToDate},
          {"locale", &LepusLocal},
          {"format", &Format},
          {"getTimezoneOffset", &GetTimeZoneOffset},
      });
  RegisterBuiltinFunctionTable(ctx, "LepusDate", &apis);
}

const RestrictedValue& GetDatePrototypeAPI(const base::String& key) {
  static BuiltinFunctionTable apis(
      BuiltinFunctionTable::DatePrototype,
      {
          {"format", &Format},
          {"unix", &Unix},
          {"year", &Year},
          {"month", &Month},
          {"date", &Date},
          {"day", &Day},
          {"hour", &Hour},
          {"minute", &Minute},
          {"second", &Sec},
          {"locale", &Locale},
          {"format", &Format},
          {"getTimezoneOffset", &GetTimeZoneOffset},
      });

  return apis.GetFunction(key);
}

}  // namespace lepus
}  // namespace lynx
