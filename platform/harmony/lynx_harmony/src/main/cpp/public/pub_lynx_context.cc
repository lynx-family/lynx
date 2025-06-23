// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_lynx_context.h"

#include "base/include/fml/message_loop_impl.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_shadow_node_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/public/pub_ui_owner.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {
namespace harmony {

PubLynxContext::PubLynxContext(
    PubUIOwner* ui_owner, PubShadowNodeOwner* node_owner,
    const std::shared_ptr<PubLynxContextDelegate>& delegate,
    const std::shared_ptr<pub::LynxResourceLoader>& resource_loader) {
  context_ =
      std::make_shared<LynxContext>(node_owner->NodeOwner(), ui_owner->Owner());
  context_->SetContextDelegate(delegate);
  ui_owner->SetContext(this);
  node_owner->SetContext(this);
  context_->SetTapSlop("50px");
  context_->SetEnableTextOverflow(true);
  // The ui_task_runner is created separately here. MessageLoopTaskQueues
  // maintains its own ui_task_runner to prevent the same ui_task_runner from
  // being used in external MessageLoopTaskQueues
  fml::RefPtr<fml::TaskRunner> ui_task_runner =
      lynx::fml::MakeRefCounted<lynx::fml::TaskRunner>(
          lynx::fml::MessageLoop::EnsureInitializedForCurrentThread()
              .GetLoopImpl());
  context_->OnLynxCreate(nullptr, nullptr, nullptr, resource_loader,
                         ui_task_runner, ui_task_runner);
}

const std::shared_ptr<LynxContext>& PubLynxContext::Context() const {
  return context_;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
