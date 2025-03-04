// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_RUNTIME_STANDALONE_HELPER_H_
#define CORE_SHELL_RUNTIME_STANDALONE_HELPER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/lynx_actor.h"
#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/public/lynx_resource_loader.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/runtime/bindings/jsi/modules/lynx_module_manager.h"
#include "core/services/timing_handler/timing_handler.h"
#include "core/shared_data/white_board_runtime_delegate.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace shell {

struct InitRuntimeStandaloneResult {
  std::shared_ptr<LynxActor<runtime::LynxRuntime>> runtime_actor_;
  // will be bind to LynxShell when LynxBackgroundRuntime is attached to
  // LynxView
  std::shared_ptr<LynxActor<tasm::timing::TimingHandler>> timing_actor_;
  // will be released by LynxBackgroundRuntime if not attached to LynxView
  std::shared_ptr<LynxActor<NativeFacade>> native_runtime_facade_;
  std::shared_ptr<tasm::WhiteBoardRuntimeDelegate> white_board_delegate_;
};

InitRuntimeStandaloneResult InitRuntimeStandalone(
    const std::string& group_name, const std::string& group_id,
    std::unique_ptr<NativeFacade> native_facade_runtime,
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& runtime_observer,
    const std::shared_ptr<lynx::pub::LynxResourceLoader>& resource_loader,
    const std::shared_ptr<lynx::piper::LynxModuleManager>& module_manager,
    const std::shared_ptr<tasm::PropBundleCreator>& prop_bundle_creator,
    const std::shared_ptr<tasm::WhiteBoard>& white_board,
    const std::function<
        void(const std::shared_ptr<LynxActor<runtime::LynxRuntime>>&,
             const std::shared_ptr<LynxActor<NativeFacade>>&)>&
        on_runtime_actor_created,
    std::vector<std::string> preload_js_paths, bool enable_js_group_thread,
    bool force_reload_js_core, bool force_use_light_weight_js_engine = false,
    bool pending_js_task = false, bool enable_user_bytecode = false,
    const std::string& bytecode_source_url = "");

void TriggerDestroyRuntime(
    const std::shared_ptr<LynxActor<runtime::LynxRuntime>>& runtime_actor,
    std::string js_group_thread_name);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_RUNTIME_STANDALONE_HELPER_H_
