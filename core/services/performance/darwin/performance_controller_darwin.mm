// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/darwin/performance_controller_darwin.h"
#import <Lynx/LynxPerformanceEntryConverter.h>
#import "LynxPerformanceController+Native.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"

namespace lynx {
namespace tasm {
namespace performance {

void PerformanceControllerDarwin::OnPerformanceEvent(
    const std::unique_ptr<lynx::pub::Value> entry) {
  lepus::Value lepus_entry = pub::ValueUtils::ConvertValueToLepusValue(*entry);
  NSDictionary* entry_dict = lynx::tasm::convertLepusValueToNSObject(lepus_entry);
  LynxPerformanceEntry* pef_entry = [LynxPerformanceEntryConverter makePerformanceEntry:entry_dict];
  [platform_performance_controller_ onPerformanceEvent:pef_entry];
}

void PerformanceControllerDarwin::SetActor(
    const std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>&
        actor) {
  [platform_performance_controller_ setNativeActor:actor];
}

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
