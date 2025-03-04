// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_EXCEPTION_H_
#define CORE_RUNTIME_VM_LEPUS_EXCEPTION_H_

#include <sstream>
#include <string>

#include "core/base/json/json_util.h"
#include "core/runtime/vm/lepus/token.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace lepus {
#define RANGEERROR ""
#define SYNTAXERROR ""
#define TYPEERROR "Type error"
#define REFERENCEERROR ""
class Exception {
 public:
  Exception() { stream_ = new std::ostringstream(); }
  ~Exception() {
    if (stream_) {
      delete (stream_);
    }
    stream_ = NULL;
  }
  const std::string message() const { return stream_->str(); }

 protected:
  std::ostringstream& stream() { return *stream_; }

 private:
  std::ostringstream* stream_;
};

class EncodeException : public Exception {
 public:
  std::string trim(const std::string& s) {
    std::string result = s;
    if (result.empty()) {
      return result;
    }

    result.erase(0, result.find_first_not_of(" "));
    result.erase(result.find_last_not_of(" ") + 1);
    return result;
  }
  EncodeException(const char* msg) { stream() << msg; }
};

class CompileException : public Exception {
 public:
  std::string trim(const std::string& s) {
    std::string result = s;
    if (result.empty()) {
      return result;
    }

    result.erase(0, result.find_first_not_of(" "));
    result.erase(result.find_last_not_of(" ") + 1);
    return result;
  }

  CompileException(const char* msg, const Token& token, std::string str_line_) {
    stream() << "(line:" << token.line_ << ", column:" << token.column_
             << "):" << msg << " around \"" << trim(str_line_) << "\"";
  }

  CompileException(const char* key, const char* msg, const Token& token,
                   std::string str_line_) {
    stream() << "(line:" << token.line_ << ", column:" << token.column_
             << "):" << key << msg << " around \"" << trim(str_line_) << "\"";
  }

  explicit CompileException(const char* msg) { stream() << msg; }
};
class RuntimeException : public Exception {
 public:
  RuntimeException(const char* msg) { stream() << msg; }
  RuntimeException(const char* tag, const char* msg) { stream() << tag << msg; }
};

class ParseException : public Exception {
 public:
  ParseException(const char* msg, const char* file,
                 const rapidjson::Value& location) {
    msg_ = msg;
    file_ = file;
    location_ = base::ToJson(location);
  }

  ParseException(const char* msg, const char* file) {
    msg_ = msg;
    file_ = file;
  }

  void SetFile(const std::string& file) { file_ = file; }

  std::string msg_;
  std::string file_;
  std::string location_;
};

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_EXCEPTION_H_
