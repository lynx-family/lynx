// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_BASE_JSON_JSON_UTIL_H_
#define CORE_BASE_JSON_JSON_UTIL_H_

#include <string>

#include "base/include/base_export.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"
namespace lynx {
namespace base {

extern rapidjson::MemoryPoolAllocator<>* global_allocate_;

//   rapidjson::Value ParseJson(const std::string& json);
BASE_EXPORT_FOR_DEVTOOL std::string ToJson(const rapidjson::Value& json);

const char* TypeName(const rapidjson::Value& value);
bool IsNumber(const rapidjson::Value& value);
bool IsArray(const rapidjson::Value& value);
bool IsNull(const rapidjson::Value& value);
BASE_EXPORT_FOR_DEVTOOL rapidjson::Document strToJson(const char* json);

//  const rapidjson::Value& GetDefault(const rapidjson::Value& json, const
//  std::string& key, rapidjson::Value&& value);
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_JSON_JSON_UTIL_H_
