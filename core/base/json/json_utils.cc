// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "base/include/log/logging.h"
#include "core/base/json/json_util.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

namespace lynx {
namespace base {

rapidjson::MemoryPoolAllocator<>* global_allocate_ =
    new rapidjson::MemoryPoolAllocator<>();

std::string ToJson(const rapidjson::Value& json) {
  rapidjson::Value msg(rapidjson::kObjectType);
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json.Accept(writer);
  std::string str = buffer.GetString();

  return str;
}

bool IsNumber(const rapidjson::Value& value) { return value.IsNumber(); }

bool IsArray(const rapidjson::Value& value) { return value.IsArray(); }

bool IsNull(const rapidjson::Value& value) { return value.IsNull(); }

const char* TypeName(const rapidjson::Value& value) {
  switch (value.GetType()) {
    case rapidjson::kNullType:
      return "null";
    case rapidjson::kNumberType:
      return "number";
    case rapidjson::kStringType:
      return "string";
    case rapidjson::kTrueType:
    case rapidjson::kFalseType:
      return "bool";
    case rapidjson::kArrayType:
      return "array";
    case rapidjson::kObjectType:
      return "object";
    default:
      return "";
  }
  return "";
}

rapidjson::Document strToJson(const char* json) {
  rapidjson::Document document;

  if (document.Parse(json).HasParseError()) {
    printf(" parse json str error: %s\n", json);
    return document;
  }

  return document;
}
}  // namespace base
}  // namespace lynx
