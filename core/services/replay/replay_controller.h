// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_REPLAY_CONTROLLER_H_
#define CORE_SERVICES_REPLAY_REPLAY_CONTROLLER_H_

#include <map>
#include <memory>
#include <string>

#include "core/base/lynx_export.h"
#include "core/inspector/observer/inspector_common_observer.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {

namespace lepus {
class Value;
}  // namespace lepus

namespace tasm {
namespace replay {

class ReplayController {
 public:
  LYNX_EXPORT_FOR_DEVTOOL static bool Enable();
  LYNX_EXPORT_FOR_DEVTOOL static void StartTest();
  LYNX_EXPORT_FOR_DEVTOOL static void EndTest(const std::string& file_path);
  LYNX_EXPORT_FOR_DEVTOOL static void SendFileByAgent(const std::string& type,
                                                      const std::string& file);
  LYNX_EXPORT_FOR_DEVTOOL static std::string GetLayoutTree(SLNode* slnode);
  LYNX_EXPORT_FOR_DEVTOOL static void SetDevToolObserver(
      const std::shared_ptr<lynx::tasm::InspectorCommonObserver>& observer);
  static std::string ConvertEventInfo(const lepus::Value& info);
};
}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_REPLAY_CONTROLLER_H_
