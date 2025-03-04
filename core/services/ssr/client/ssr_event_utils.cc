// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/ssr/client/ssr_event_utils.h"

#include "base/include/string/string_utils.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/services/ssr/ssr_type_info.h"

namespace lynx {
namespace ssr {
namespace {
// The parameter placeholder_key is like 'a' , 'a.b' , 'a.b.1' , which
// represents the key's path in dict. So we split placeholder_key to an array
// and loop its items in dict to search for its value.
lepus::Value GetPlaceholderValue(const base::String& placeholder_key,
                                 const lepus::Value& dict) {
  std::vector<std::string> path_vec;
  base::SplitString(placeholder_key.string_view(), '.', path_vec);
  lepus::Value result = dict;
  std::for_each(path_vec.begin(), path_vec.end(), [&result](const auto& path) {
    result = result.IsArray() ? result.GetProperty(std::stoi(path))
                              : result.GetProperty(path);
  });
  return result;
}
}  // namespace

/* Format args for SSR sever event.
 origin format:
 {tasmEntryName: __Card__, callbackId: 0, fromPiper: true, methodDetail:
 {method:openSchema, module:LynxTestModule, param:[arg1, arg2, ...]}}

 processed format:
 while method_name == "call":
 [true, 0, {method:openSchema, module:LynxTestModule, data}], data means
 elements in arg2 which is a dictionary.

 while method_name != "call":
 [true, 0, arg1, arg2, ... , {method:openSchema, module:LynxTestModule}]
 */
lepus::Value FormatEventArgsForIOS(const std::string& method_name,
                                   const lepus::Value& args) {
  if (!args.IsTable() || args.GetLength() <= 0) {
    return args;
  }
  auto from_piper =
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleFromPiper));
  if (from_piper.IsNil() || !from_piper.Bool()) {
    return args;
  }
  auto processed_args = lepus::CArray::Create();
  processed_args->emplace_back(true);
  auto callback_id =
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleCallbackId));
  processed_args->push_back(callback_id.IsNil() ? lepus::Value(0)
                                                : callback_id);
  auto method_detail_map =
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleMethodDetail));
  if (!method_detail_map.IsTable()) {
    return args;
  }
  auto detail_map = lepus::Dictionary::Create();
  auto params_array =
      method_detail_map.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleParam));
  // process call method
  if (method_name == ssr::kLepusModuleCallMethod &&
      params_array.GetLength() > 2) {
    auto raw_data = params_array.GetProperty(1);
    tasm::ForEachLepusValue(raw_data, [&detail_map](const lepus::Value& key,
                                                    const lepus::Value& arg) {
      detail_map->SetValue(key.String(), arg);
    });
  } else {
    // process normal method
    tasm::ForEachLepusValue(
        params_array,
        [&processed_args](const lepus::Value& key, const lepus::Value& arg) {
          processed_args->push_back(arg);
        });
  }
  detail_map->SetValue(BASE_STATIC_STRING(ssr::kLepusModuleMethod),
                       method_detail_map.GetProperty(
                           BASE_STATIC_STRING(ssr::kLepusModuleMethod)));
  detail_map->SetValue(BASE_STATIC_STRING(ssr::kLepusModuleModule),
                       method_detail_map.GetProperty(
                           BASE_STATIC_STRING(ssr::kLepusModuleModule)));
  processed_args->emplace_back(std::move(detail_map));
  return lepus::Value(std::move(processed_args));
}

/* Format args for SSR sever event.
 origin format:
 {tasmEntryName: __Card__, callbackId: 0, fromPiper: true, methodDetail :
 {method:openSchema,module:LynxTestModule,param:[arg1, arg2, ...]}}

 processed format:
 while method_name == "call":
 {tasmEntryName: __Card__, callbackId: 0, fromPiper: true, methodDetail :
 {method:openSchema, module:LynxTestModule, data}}, data means elements in arg2
 which is a dictionary.

 while method_name != "call":
 {tasmEntryName: __Card__, callbackId: 0,
 fromPiper: true, methodDetail : {module:LynxTestModule, param:[arg1, arg2,
 ...]}}
 */
lepus::Value FormatEventArgsForAndroid(const std::string& method_name,
                                       const lepus::Value& args) {
  if (!args.IsTable() || args.GetLength() <= 0) {
    return args;
  }
  auto from_piper =
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleFromPiper));
  if (from_piper.IsNil() || !from_piper.Bool()) {
    return args;
  }

  auto processed_args = lepus::Dictionary::Create();
  processed_args->SetValue(BASE_STATIC_STRING(ssr::kLepusModuleFromPiper),
                           true);
  processed_args->SetValue(
      BASE_STATIC_STRING(ssr::kLepusModuleTasmEntryName),
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleTasmEntryName)));
  processed_args->SetValue(
      BASE_STATIC_STRING(ssr::kLepusModuleCallbackId),
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleCallbackId)));
  auto method_detail_map_new = lepus::Dictionary::Create();
  auto method_detail_map =
      args.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleMethodDetail));
  method_detail_map_new->SetValue(
      BASE_STATIC_STRING(ssr::kLepusModuleModule),
      method_detail_map.GetProperty(
          BASE_STATIC_STRING(ssr::kLepusModuleModule)));
  auto params_array =
      method_detail_map.GetProperty(BASE_STATIC_STRING(ssr::kLepusModuleParam));
  // process "call" method
  if (method_name == ssr::kLepusModuleCallMethod &&
      params_array.GetLength() > 2) {
    method_detail_map_new->SetValue(
        BASE_STATIC_STRING(ssr::kLepusModuleMethod),
        method_detail_map.GetProperty(
            BASE_STATIC_STRING(ssr::kLepusModuleMethod)));
    auto raw_data = params_array.GetProperty(1);
    tasm::ForEachLepusValue(
        raw_data, [&method_detail_map_new](const lepus::Value& key,
                                           const lepus::Value& arg) {
          method_detail_map_new->SetValue(key.String(), arg);
        });
  } else {
    // process normal method
    method_detail_map_new->SetValue(BASE_STATIC_STRING(ssr::kLepusModuleParam),
                                    params_array);
  }
  processed_args->SetValue(BASE_STATIC_STRING(ssr::kLepusModuleMethodDetail),
                           std::move(method_detail_map_new));
  return lepus::Value(std::move(processed_args));
}

// Will replace the magic words works as a placeholder for SSR in place if
// possible. And will return the replaced value anyways even if the value cannot
// be replaced in place.
// The name of placeholder variable will be enclosed by $_ph{ }ph_# pattern.
// Find all the placeholder variable name and replace it with injected data from
// client.
lepus::Value ReplacePlaceholdersForString(
    const lepus::Value& value, const lepus::Value& dict,
    std::vector<base::String>* placeholder_keys, bool for_ssr_script) {
  // Calling value.String() may return a local shared pointer, hold that to
  // avoid deconstruct.
  const auto& str = value.StdString();

  auto prefix_start = str.find(kSSRPlaceHolderMagicPrefix);
  auto suffix_start = str.find(kSSRPlaceHolderMagicSuffix, prefix_start);
  if (prefix_start == 0 &&
      suffix_start + kSSRPlaceHolderMagicSuffixLength == str.size()) {
    auto placeholder_key = str.substr(
        kSSRPlaceHolderMagicPrefixLength,
        suffix_start - prefix_start - kSSRPlaceHolderMagicPrefixLength);
    if (placeholder_keys != nullptr) {
      placeholder_keys->emplace_back(base::String(placeholder_key));
    }
    return GetPlaceholderValue(placeholder_key, dict);
  }

  // Placeholders injected inside a string.
  // Now only string injected data is supported if the placeholder is put within
  // a string.
  if (prefix_start != std::string::npos && suffix_start != std::string::npos) {
    std::string result;
    size_t last_suffix_end = 0;
    // A $_ph{ }ph_# enclosed placeholder variable is found in string.
    while (prefix_start != std::string::npos &&
           suffix_start != std::string::npos) {
      // Append the substring before the placeholder variable to result string.
      result =
          result + str.substr(last_suffix_end, prefix_start - last_suffix_end);
      // Append the replaced value to string.
      auto placeholder_key = str.substr(
          kSSRPlaceHolderMagicPrefixLength + prefix_start,
          suffix_start - prefix_start - kSSRPlaceHolderMagicPrefixLength);
      lepus::Value dict_value = GetPlaceholderValue(placeholder_key, dict);
      if (!for_ssr_script && dict_value.IsString()) {
        // lepusValueToString will add escape characters, causing the string
        // value to be converted form xxx to \"xxx\"; replacing placeholders in
        // node's value does not require escape characters, so take the value
        // directly here.
        result += dict_value.StdString();
      } else {
        result += lepusValueToString(dict_value);
      }

      if (placeholder_keys != nullptr) {
        placeholder_keys->emplace_back(base::String(placeholder_key));
      }
      // Find next enclosed placeholder variable.
      prefix_start = str.find(kSSRPlaceHolderMagicPrefix,
                              suffix_start + kSSRPlaceHolderMagicSuffixLength);
      last_suffix_end = suffix_start + kSSRPlaceHolderMagicSuffixLength;
      suffix_start = str.find(kSSRPlaceHolderMagicSuffix, prefix_start);
    }
    // Append the rest of string,
    // if there is some string after all the placeholder variables.
    if (last_suffix_end < str.length()) {
      result = result + str.substr(last_suffix_end);
    }
    return lepus::Value(std::move(result));
  } else {
    return value;
  }
}

}  // namespace ssr
}  // namespace lynx
