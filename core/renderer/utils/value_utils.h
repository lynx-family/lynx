// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_VALUE_UTILS_H_
#define CORE_RENDERER_UTILS_VALUE_UTILS_H_
#include <functional>
#include <string>
#include <utility>

#include "base/include/compiler_specific.h"
#include "core/public/pipeline_option.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"

namespace lynx {
namespace tasm {

// shadow equal check for table value
bool CheckTableValueNotEqual(const lepus::Value& target_item_value,
                             const lepus::Value& update_item_value);
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
bool CheckTableDeepUpdated(const lepus::Value& target,
                           const lepus::Value& update, bool first_layer);
#endif
// shadow equal for table
bool CheckTableShadowUpdated(const lepus::Value& target,
                             const lepus::Value& update);

void ForEachLepusValue(const lepus::Value& value,
                       lepus::LepusValueIterator func);

std::string GetTimingFlag(const lepus_value& table);

lepus::Value ConvertJSValueToLepusValue(const lepus::Value& value);

// TODO(kechenglong): impl ToLepusValue in PipelineOptions.
lepus::Value PipelineOptionsToLepusValue(
    const PipelineOptions& pipeline_options);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_VALUE_UTILS_H_
