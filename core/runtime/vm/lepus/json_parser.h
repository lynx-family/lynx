// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_VM_LEPUS_JSON_PARSER_H_
#define CORE_RUNTIME_VM_LEPUS_JSON_PARSER_H_

#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

#include "base/include/base_export.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/vm_context.h"
#include "third_party/rapidjson/document.h"
#include "third_party/rapidjson/error/en.h"
#include "third_party/rapidjson/reader.h"
#include "third_party/rapidjson/stringbuffer.h"
#include "third_party/rapidjson/writer.h"

typedef rapidjson::Value rapid_value;

namespace lynx {
namespace lepus {

using LepusValueSet = std::set<const lepus_value*>;

std::string readFile(const char* file);

std::string writeFile(const uint8_t* content, const char* file, int len,
                      size_t& pos);
lepus_value jsonValueTolepusValue(const rapid_value& rapValue);
lepus_value jsonValueTolepusValue(const char* json);
std::string lepusValueToJSONString(const lepus_value& value,
                                   bool in_order = false);
BASE_EXPORT_FOR_DEVTOOL std::string lepusValueToString(
    const lepus_value& value);
std::string lepusValueMapToJSONString(
    const std::unordered_map<base::String, lepus::Value>& map);

// If want the output keys to be ordered, the ordered must be true. Otherwise,
// ordered can be false.
void lepusValueToJSONString(std::stringstream& ss, const lepus_value& value,
                            bool ordered,
                            const std::shared_ptr<LepusValueSet>& all_set =
                                std::make_shared<LepusValueSet>());

void qjsValueToJSONString(std::stringstream& ss, const lepus::Value& value,
                          bool ordered);

void qjsArrayToJSONString(std::stringstream& ss, const lepus::Value& value,
                          bool ordered);

void qjsObjectToJSONString(std::stringstream& ss, const lepus::Value& value,
                           bool ordered);

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_JSON_PARSER_H_
