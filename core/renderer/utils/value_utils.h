// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_VALUE_UTILS_H_
#define CORE_RENDERER_UTILS_VALUE_UTILS_H_
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/base/lynx_export.h"
#include "core/public/pipeline_option.h"

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

LYNX_EXPORT_FOR_DEVTOOL void ForEachLepusValue(const lepus::Value& value,
                                               lepus::LepusValueIterator func);

std::string GetTimingFlag(const lepus_value& table);

lepus::Value ConvertJSValueToLepusValue(const lepus::Value& value);

// TODO(kechenglong): impl ToLepusValue in PipelineOptions.
lepus::Value PipelineOptionsToLepusValue(
    const std::shared_ptr<PipelineOptions>& pipeline_options);

std::string ConcatenateTableKeys(const lepus::Value& value);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_VALUE_UTILS_H_
