// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_LEPUS_DATE_H_
#define CORE_RUNTIME_VM_LEPUS_LEPUS_DATE_H_
#if !defined(_WIN32)
#include <sys/time.h>

#include <string>
#endif
#include <ctime>
#include <iostream>
#include <sstream>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace lepus {
class Value;

struct tm_extend : public tm {
#ifdef OS_WIN
  long tm_gmtoff; /* Seconds east of UTC */
#endif
};

class CDate : public fml::RefCountedThreadSafeStorage {
 public:
  static Value GetTimeZoneOffset();
  static fml::RefPtr<CDate> Create() {
    return fml::AdoptRef<CDate>(new CDate());
  }
  static std::string FormatToString(Value* date, const std::string& format);
  static size_t CountNum(const std::string& format, size_t index, size_t max);
  static void ParserFormatString(std::string date, const std::string& format,
                                 tm_extend& tm_, int& ms);

  static fml::RefPtr<CDate> ParseNumberToDate(int64_t parse_number);

  static fml::RefPtr<CDate> ParseStringToDate(int params_count,
                                              const std::string& p1,
                                              const std::string& p2);

  static fml::RefPtr<CDate> Create(const tm_extend& date) {
    return fml::AdoptRef<CDate>(new CDate(date));
  }

  static Value LepusNow();
  static fml::RefPtr<CDate> Create(const tm_extend& date, int ms) {
    return fml::AdoptRef<CDate>(new CDate(date, ms));
  }

  static fml::RefPtr<CDate> Create(const tm_extend& date, const int& ms,
                                   const int& t) {
    return fml::AdoptRef<CDate>(new CDate(date, ms, t));
  }

  static fml::RefPtr<CDate> Create(const tm_extend* date) {
    return fml::AdoptRef<CDate>(new CDate(date));
  }

  static fml::RefPtr<CDate> Create(const tm_extend* date, int ms) {
    return fml::AdoptRef<CDate>(new CDate(date, ms));
  }

  void ReleaseSelf() const override { delete this; }

  ~CDate() override = default;

  friend bool operator==(const CDate& left, const CDate& right);
  BASE_EXPORT_FOR_DEVTOOL void print(std::stringstream& ss);
  void print(std::ostream& ss);
  friend bool operator!=(const CDate& left, const CDate& right) {
    return !(left == right);
  }

  const tm_extend& get_date_() const { return date_; }
  int get_language() const { return language; }
  const int& get_ms_() const { return ms_; }
  void SetDate(tm_extend& date, int ms, int languages) {
    date_ = date;
    ms_ = ms;
    language = languages;
  }

  time_t get_time_t_();

  void SetLanguage(int t) { language = t; }

  static int global_language;

 protected:
  CDate() : date_(), ms_(0), language(global_language) { initialize(); }
  CDate(CDate& t) : date_(t.date_), ms_(t.ms_), language(global_language) {
    initialize();
  }

  CDate(const tm_extend& date)
      : date_(date), ms_(0), language(global_language) {
    initialize();
  }
  CDate(const tm_extend& date, int ms)
      : date_(date), ms_(ms), language(global_language) {
    initialize();
  }
  CDate(const tm_extend& date, const int& ms, const int& t)
      : date_(date), ms_(ms), language(t) {
    initialize();
  }
  CDate(const tm_extend* date)
      : date_(*date), ms_(0), language(global_language) {
    initialize();
  }
  CDate(const tm_extend* date, int ms)
      : date_(*date), ms_(ms), language(global_language) {
    initialize();
  }

 private:
  void initialize();
  tm_extend date_;  // year ... min seconds

  int ms_;  // ms
  int language;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_LEPUS_DATE_H_
