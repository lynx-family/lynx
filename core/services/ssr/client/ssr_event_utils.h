// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_CLIENT_SSR_EVENT_UTILS_H_
#define CORE_SERVICES_SSR_CLIENT_SSR_EVENT_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace ssr {

lepus::Value FormatEventArgsForIOS(const std::string& method_name,
                                   const lepus::Value& args);

lepus::Value FormatEventArgsForAndroid(const std::string& method_name,
                                       const lepus::Value& args);

lepus::Value ReplacePlaceholdersForString(
    const lepus::Value& value, const lepus::Value& dict,
    std::vector<base::String>* placeholder_keys = nullptr,
    bool for_ssr_script = false);

}  // namespace ssr
}  // namespace lynx

#endif  // CORE_SERVICES_SSR_CLIENT_SSR_EVENT_UTILS_H_
