// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_DEVTOOL_WRAPPER_DEVTOOL_POOL_H_
#define CORE_DEVTOOL_WRAPPER_DEVTOOL_POOL_H_

#include <memory>
#include <vector>

#include "core/inspector/observer/inspector_lepus_observer.h"
#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace devtool {

// Motivation
// - The normal devtool initialization path happens during LynxTemplateRender
//   initialization, which typically assumes a LynxView exists.
// - TemplateBundle allows pre-creating (and optionally pre-executing)
//   MTSRuntime before any LynxView is created. To debug those early runtimes,
//   we need a way to create the Lepus inspector objects earlier.
//
// Design overview
//   (A) Platform layer (Android/ObjC)
//       - Provides LynxDevToolPool as a LIFO container of *temporary* view-less
//         LynxDevtool instances for pre-executed runtimes.
//   (B) Core / native layer (C++)
//       - Uses DevToolPool to ask platform to create/pop the temporary devtool,
//         and to synchronously obtain a lepus::InspectorLepusObserver.
//
// - Gating: the devtool/inspector initialization is only performed for
//   pre-executed runtimes that satisfy (devtool_pool exists + LepusNG +
//   enable_mts_pre_execute). Other pre-created runtimes will skip all
//   inspector/devtool work.
//
// Typical flow (simplified)
// 1) TemplateBundle creates LynxDevToolPool.
// 2) Native bundle stores the shared DevToolPool and wires it into
//    MTSRuntimePool.
// 3) When MTSRuntimePool creates a new runtime and the gating
//    conditions are met:
//    - devtool_pool_->CreateDevTool();
//    - observer = devtool_pool_->OnMTSRuntimeCreated();
//    - runtime->InitInspector(observer, ...);
//      runtime->SetDebugInfoURL(...);
//      runtime->DeSerialize(...);
//      runtime->TryExecute();
//
//    Terminology used below:
//    - devtool2 / observer2 / debugger2: created for the pre-executed runtime
//    (no LynxView).
//    - devtool1 / observer1 / debugger1: the view/render side devtool created
//    later during
//      LynxTemplateRender initialization.
// 4) Later, when TemplateEntry reuses a pre-executed runtime from the local
//    pool:
//    - We keep using observer2/debugger2 created for the pre-executed runtime.
//    - Concretely, we call observer2->TakeOver(observer1) so
//    observer2/debugger2 can take over the necessary platform facade / mediator
//    / console state from
//      observer1/debugger1.
//    - Then TemplateAssembler/Entry replaces its stored observer with
//    observer2.
//    - After takeover, DevToolPool::PopDevTool() is called to dispose the
//    matching temp devtool2.
class DevToolPool : public std::enable_shared_from_this<DevToolPool> {
 public:
  DevToolPool() = default;
  virtual ~DevToolPool() = default;

  virtual void CreateDevTool() = 0;
  virtual void PopDevTool() = 0;

  virtual std::shared_ptr<lepus::InspectorLepusObserver> OnMTSRuntimeCreated() {
    if (!lepus_observers_.empty()) {
      auto observer = lepus_observers_.back();
      lepus_observers_.pop_back();
      return observer;
    }
    return nullptr;
  }

  void AddLepusObserver(
      const std::shared_ptr<lepus::InspectorLepusObserver>& observer) {
    lepus_observers_.emplace_back(observer);
  }

 protected:
  std::vector<std::shared_ptr<lepus::InspectorLepusObserver>> lepus_observers_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // CORE_DEVTOOL_WRAPPER_DEVTOOL_POOL_H_
