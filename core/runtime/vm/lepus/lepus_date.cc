// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/lepus_date.h"

#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/lepus_date_api.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/tt_tm.h"

#if defined(OS_WIN)
#include <time.h>

#include <iomanip>
#include <sstream>
#endif

namespace lynx {
namespace lepus {
static tm_extend LocalToUTC(tm_extend local) {
  tm_extend forTran = local;
  time_t temp = timegm(&forTran);
  temp -= local.tm_gmtoff;
  tm_extend UTC;
  memset(&UTC, 0, sizeof(UTC));
  gmtime_r(&temp, &UTC);
  return UTC;
}
Value CDate::GetTimeZoneOffset() {
  tm_extend tm_local;
  time_t time_utc;
  time(&time_utc);
  memset(&tm_local, 0, sizeof(tm_local));
  localtime_r(&time_utc, &tm_local);  // return local tm
  int64_t offset = (-tm_local.tm_gmtoff) / 60;
  return Value(offset);
}
void CDate::print(std::ostream& ss) {
  char buf[25];
  tm_extend UTCtime = LocalToUTC(date_);
  strftime(buf, 25, "%Y-%m-%dT%H:%M:%S.", &UTCtime);
  ss << "d {\n";
  ss << "  '$L': '" << DateContent()[language] << "'," << std::endl;
  ss << "  '$d': " << buf << std::to_string(ms_) << "Z," << std::endl;
  ss << "  '$y': " << date_.tm_year + 1900 << ",\n";
  ss << "  '$M': " << date_.tm_mon << ",\n";
  ss << "  '$D': " << date_.tm_mday << ",\n";
  ss << "  '$W': " << date_.tm_wday << ",\n";
  ss << "  '$H': " << date_.tm_hour << ",\n";
  ss << "  '$m': " << date_.tm_min << ",\n";
  ss << "  '$s': " << date_.tm_sec << ",\n";
  ss << "  '$ms': " << ms_ << " }" << std::endl;
}

Value CDate::LepusNow() {
  time_t now = time(nullptr);
  timeval ms;
  if (gettimeofday(&ms, nullptr) == -1 || now == -1) {
    return Value();
  }
  tm_extend tm_now;
  memset(&tm_now, 0, sizeof(tm_now));
  localtime_r(&now, &tm_now);
  GetTimeZone(tm_now);
  return Value(CDate::Create(tm_now, ms.tv_usec / 1000));
}

void CDate::print(
    std::stringstream& ss) {  // to json string using iso8601 format
  char buf[25];
  tm_extend UTCtime = LocalToUTC(date_);
  strftime(buf, 25, "%Y-%m-%dT%H:%M:%S.", &UTCtime);
  ss << buf << ms_ << "Z" << std::endl;
}

bool operator==(const CDate& left, const CDate& right) {
  tm_extend left_date, right_date;
  left_date = left.date_;
  right_date = right.date_;
  return &left == &right ||
         (left_date.tm_sec == right_date.tm_sec &&
          left_date.tm_min == right_date.tm_min &&
          left_date.tm_hour == right_date.tm_hour &&
          left_date.tm_mday == right_date.tm_mday &&
          left_date.tm_mon == right_date.tm_mon &&
          left_date.tm_year == right_date.tm_year &&
          left_date.tm_wday == right_date.tm_wday &&
          left_date.tm_yday == right_date.tm_yday && left.ms_ == right.ms_);
}

void CDate::ParserFormatString(std::string date, const std::string& format,
                               tm_extend& tm_, int& ms) {
  size_t i = 0, length = date.length();
  bool local_flag = true;
  // only support for English content
  while (i < length) {
    switch (format[i]) {
      case 'Y': {
        size_t count_Y = CountNum(format, i, 4);
        if (count_Y == 3) {
          tm_.tm_year = std::stoi(date.substr(i, 4)) - 1900;
        } else if (count_Y == 1) {
          tm_.tm_year = std::stoi(date.substr(i, 2)) + 100;
        }
        i = i + count_Y + 1;
        break;
      }
      case 'M': {
        size_t count_M = CountNum(format, i, 4);
        if (count_M == 0) {
        } else if (count_M == 1) {
          tm_.tm_mon = std::stoi(date.substr(i, 2)) - 1;
        } else if (count_M == 2) {
        } else if (count_M == 3) {
        }
        i = i + count_M + 1;
        break;
      }
      case 'D': {
        size_t count_D = CountNum(format, i, 2);
        if (count_D == 0) {
        } else if (count_D == 1) {
          tm_.tm_mday = std::stoi(date.substr(i, 2));
        }
        i = i + count_D + 1;
        break;
      }
      case 'H': {
        size_t count_H = CountNum(format, i, 2);
        if (count_H == 1) {
          tm_.tm_hour = std::stoi(date.substr(i, 2));
        }
        i = i + count_H + 1;
        break;
      }
      case 'h': {
        size_t count_h = CountNum(format, i, 2);
        if (count_h == 1) {
          tm_.tm_hour = std::stoi(date.substr(i, 2));
        }
        i = count_h + i + 1;
        break;
      }
      case 'm': {
        size_t count_m = CountNum(format, i, 2);
        if (count_m == 1) {
          tm_.tm_min = std::stoi(date.substr(i, 2));
        }
        i = i + count_m + 1;
        break;
      }
      case 's': {
        size_t count_s = CountNum(format, i, 2);
        if (count_s == 1) {
          tm_.tm_sec = std::stoi(date.substr(i, 2));
        }
        i = i + count_s + 1;
        break;
      }
      case 'S': {
        size_t count_S = CountNum(format, i, 3);
        ms = std::stoi(date.substr(i, count_S + 1));
        i = i + count_S + 1;
        break;
      }
      case 'Z': {
        size_t count_Z = CountNum(format, i, 2);
        if (date[i] == '-') {
          i++;
          tm_.tm_gmtoff = -1;
        } else {
          tm_.tm_gmtoff = 1;
          if (date[i] == '+') i++;
        }
        tm_.tm_gmtoff *= (std::stoi(date.substr(i, 2)) * 60 +
                          std::stoi(date.substr(i + 3 - count_Z, 2)));
        tm_.tm_gmtoff *= 60;
        i = i + 5 - count_Z;
        local_flag = false;
        break;
      }
      default:
        i++;
        break;
    }
  }
  if (local_flag) {
    tm_extend tm_local;
    time_t time_utc;
    time(&time_utc);
    memset(&tm_local, 0, sizeof(tm_local));
    localtime_r(&time_utc, &tm_local);
    GetTimeZone(tm_local);
    tm_.tm_gmtoff = tm_local.tm_gmtoff;
    tm_.tm_isdst = tm_local.tm_isdst;
  }
}

std::string CDate::FormatToString(Value* date, const std::string& format) {
  const tm_extend& time = date->Date()->get_date_();
  int ms = date->Date()->get_ms_();
  std::string destination;
  size_t i = 0, length = format.length();
  constexpr size_t kBufferSize = 32;
  char ss[kBufferSize];

  // only support for English content
  while (i < length) {
    switch (format[i]) {
      case '[': {
        for (++i; i < length; i++) {
          if (format[i] != ']')
            destination.push_back(format[i]);
          else {
            i++;
            break;
          }
        }
        break;
      }
      case 'Y': {
        size_t count_Y = CountNum(format, i, 4);
        if (count_Y == 3) {
          destination += std::to_string(time.tm_year + 1900);
        } else if (count_Y == 2) {
          int32_t utcHour = static_cast<int32_t>(time.tm_gmtoff / 3600);
          snprintf(ss, kBufferSize, "%02d", utcHour);
          destination += ss;
          destination += "00";
        } else if (count_Y == 1) {
          destination += std::to_string(((time.tm_year + 1900) % 1000) % 100);
        } else {
          destination += 'Y';
        }
        i = i + count_Y + 1;
        break;
      }
      case 'M': {
        size_t count_M = CountNum(format, i, 4);
        if (count_M == 0) {
          destination += std::to_string(time.tm_mon + 1);
        } else if (count_M == 1) {
          snprintf(ss, kBufferSize, "%02d", time.tm_mon + 1);
          destination += ss;
        } else if (count_M == 2) {
          destination += std::to_string(time.tm_mon + 1);
        } else if (count_M == 3) {
          destination += std::to_string(time.tm_mon + 1);
        }
        i = i + count_M + 1;
        break;
      }
      case 'D': {
        size_t count_D = CountNum(format, i, 2);
        if (count_D == 0) {
          destination += std::to_string(time.tm_mday);
        } else if (count_D == 1) {
          snprintf(ss, kBufferSize, "%02d", time.tm_mday);
          destination += ss;
        }
        i = i + count_D + 1;
        break;
      }
      case 'd': {
        size_t count_d = CountNum(format, i, 4);
        i = i + count_d + 1;
        destination += std::to_string(time.tm_wday == 0 ? 7 : time.tm_wday);
        break;
      }
      case 'H': {
        size_t count_H = CountNum(format, i, 2);
        if (count_H == 0) {
          destination += std::to_string(time.tm_hour);
        } else if (count_H == 1) {
          snprintf(ss, kBufferSize, "%02d", time.tm_hour);
          destination += ss;
        }
        i = i + count_H + 1;
        break;
      }
      case 'h': {
        size_t count_h = CountNum(format, i, 2);
        if (count_h == 0) {
          destination +=
              std::to_string((time.tm_hour % 12 == 0) ? 12 : time.tm_hour % 12);
        } else if (count_h == 1) {
          snprintf(ss, kBufferSize, "%02d",
                   ((time.tm_hour % 12 == 0) ? 12 : time.tm_hour % 12));
          destination += ss;
        }
        i = count_h + i + 1;
        break;
      }
      case 'm': {
        size_t count_m = CountNum(format, i, 2);
        if (count_m == 0) {
          destination += std::to_string(time.tm_min);
        } else if (count_m == 1) {
          snprintf(ss, kBufferSize, "%02d", time.tm_min);
          destination += ss;
        }
        i = i + count_m + 1;
        break;
      }
      case 's': {
        size_t count_s = CountNum(format, i, 2);
        if (count_s == 0) {
          destination += std::to_string(time.tm_sec);
        } else if (count_s == 1) {
          snprintf(ss, kBufferSize, "%02d", time.tm_sec);
          destination += ss;
        }
        i = i + count_s + 1;
        break;
      }
      case 'S': {
        size_t countS = CountNum(format, i, 3);
        if (countS == 2) {
          snprintf(ss, kBufferSize, "%03d", ms);
          destination += ss;
        }
        i = i + countS + 1;
        break;
      }
      case 'Z': {
        constexpr const char *fmt1 = "%+.2d:%02d", *fmt2 = "%+.2d%02d";
        size_t count_Z = CountNum(format, i, 2);
        auto time_zone_offset = time.tm_gmtoff / 60;
        auto utcHour = time_zone_offset / 60;
        auto utcMin = std::abs(time_zone_offset % 60);
        snprintf(ss, kBufferSize, count_Z == 0 ? fmt1 : fmt2,
                 static_cast<int32_t>(utcHour), static_cast<int32_t>(utcMin));
        destination += ss;
        i = i + count_Z + 1;
        break;
      }
      default:
        destination += format[i];
        i++;
        break;
    }
  }
  return destination;
}
fml::RefPtr<CDate> CDate::ParseNumberToDate(int64_t parse_number) {
  time_t seconds = static_cast<time_t>(parse_number / 1000);
  int ms = parse_number % 1000;
  tm_extend times;
  memset(&times, 0, sizeof(times));
  localtime_r(&seconds, static_cast<tm*>(&times));
  GetTimeZone(times);
  return CDate::Create(times, ms);
}
void CDate::initialize() {
  tm_extend temp = date_;
  timegm(&temp);
  date_.tm_wday = temp.tm_wday;
  date_.tm_yday = temp.tm_yday;
}

size_t CDate::CountNum(const std::string& format, size_t index, size_t max) {
  size_t i = index + 1, length = format.length(), count_num = 0;
  char c = format[index];
  for (; i < index + max && i < length; i++) {
    if (format[i] == c) {
      count_num++;
    } else {
      return count_num;
    }
  }
  return count_num;
}

time_t CDate::get_time_t_() {
  tm_extend temp = date_;
  time_t time = mktime(&temp);
  time += temp.tm_gmtoff;
  time -= date_.tm_gmtoff;
  return time;
}
#if defined(OS_WIN)
// copied from
// https://stackoverflow.com/questions/321849/strptime-equivalent-on-windows/321940
char* strptime(const char* s, const char* f, struct tm* tm) {
  // Isn't the C++ standard lib nice? std::get_time is defined such that its
  // format parameters are the exact same as strptime. Of course, we have to
  // create a string stream first, and imbue it with the current C locale, and
  // we also have to make sure we return the right things if it fails, or
  // if it succeeds, but this is still far simpler an implementation than any
  // of the versions in any of the C standard libraries.
  std::istringstream input(s);
  input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
  input >> std::get_time(tm, f);
  if (input.fail()) {
    return nullptr;
  }
  return const_cast<char*>(s + input.tellg());
}
#endif

fml::RefPtr<CDate> CDate::ParseStringToDate(int params_count,
                                            const std::string& date,
                                            const std::string& format) {
  tm_extend tm_{};
  int ms = 0;
  memset(&tm_, 0, sizeof(tm_));
  if (params_count == 1) {  // ISO8601 format "YYYY-MM-DDTHH-mm-ss.SSS+0800"
    std::string buf = date;
    strptime(buf.c_str(), "%Y-%m-%dT%H:%M:%S.", &tm_);
    ms = std::stoi(buf.substr(20, 3));
    tm_.tm_isdst = -1;
    if (buf[23] == 'Z' || buf[23] == 'z') {
      tm_.tm_gmtoff = 0;
    } else if (buf[23] == '+') {
      tm_.tm_gmtoff =
          (std::stoi(buf.substr(24, 2)) * 60 + std::stoi(buf.substr(26, 2))) *
          60;
    } else if (buf[23] == '-') {
      tm_.tm_gmtoff =
          (std::stoi(buf.substr(24, 2)) * 60 + std::stoi(buf.substr(26, 2))) *
          -60;
    } else {
      tm_extend tm_local{};
      memset(&tm_local, 0, sizeof(tm_local));
      time_t time_utc;
      time(&time_utc);
      localtime_r(&time_utc, static_cast<tm*>(&tm_local));
      GetTimeZone(tm_local);
      tm_.tm_gmtoff = tm_local.tm_gmtoff;
      tm_.tm_isdst = tm_local.tm_isdst;
    }
  } else {
    ms = 0;
    ParserFormatString(date, format, tm_, ms);
  }
  return CDate::Create(tm_, ms);
}
}  // namespace lepus
}  // namespace lynx
