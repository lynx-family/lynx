// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_LYNX_ACTOR_SPECIALIZATION_H_
#define CORE_SHELL_LYNX_ACTOR_SPECIALIZATION_H_

#include <string>
#include <utility>

#include "base/include/lynx_actor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/shell/lynx_engine.h"
#include "core/shell/native_facade.h"

namespace lynx {
namespace shell {

class BTSRuntime;

template <typename T>
inline constexpr bool kIsLynxActor = std::is_same_v<T, shell::BTSRuntime> ||
                                     std::is_same_v<T, shell::LynxEngine> ||
                                     std::is_same_v<T, shell::NativeFacade> ||
                                     std::is_same_v<T, tasm::LayoutContext>;

template <typename T>
inline constexpr const char* kActorTag = "";

template <>
inline constexpr const char* kActorTag<shell::NativeFacade> = "NativeFacade";

template <>
inline constexpr const char* kActorTag<shell::LynxEngine> = "LynxEngine";

template <>
inline constexpr const char* kActorTag<shell::BTSRuntime> = "BTSRuntime";

template <>
inline constexpr const char* kActorTag<tasm::LayoutContext> = "LayoutContext";

template <typename C, typename T>
class LynxActorMixinBase {
 public:
  void BeforeInvoked() {
    int32_t instance_id =
        static_cast<std::add_pointer_t<C>>(this)->GetInstanceId();
    TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY,
                      std::string(kTag)
                          .append("::Invoke::")
                          .append(std::to_string(instance_id)),
                      [instance_id](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations(
                            INSTANCE_ID, std::to_string(instance_id));
                      });
    tasm::report::FeatureCounter::Instance()->UpdateAndBackupCurrentInstanceId(
        instance_id);
  }

  void AfterInvoked() {
    {
      TRACE_EVENT(
          LYNX_TRACE_CATEGORY, nullptr,
          [instance_id =
               static_cast<std::add_pointer_t<C>>(this)->GetInstanceId()](
              lynx::perfetto::EventContext ctx) {
            ctx.event()->set_name(std::string(kTag).append("::AfterInvoked"));
            ctx.event()->add_debug_annotations(INSTANCE_ID,
                                               std::to_string(instance_id));
          });
      auto* impl = static_cast<std::add_pointer_t<C>>(this)->Impl();
      if (impl != nullptr) {
        ConsumeImplIfNeeded(impl);
        tasm::report::EventTracker::Flush(
            static_cast<std::add_pointer_t<C>>(this)->GetInstanceId());
      }
      // In order to ensure that the feature is counted, this method must be
      // called last.
      tasm::report::FeatureCounter::Instance()->RestoreCurrentInstanceId();
    }

    TRACE_EVENT_END(LYNX_TRACE_CATEGORY);
  }
  void ConsumeImplIfNeeded(T* impl) {}

 private:
  static inline constexpr const char* kTag = kActorTag<T>;
};

template <typename C, typename T>
class LynxActorMixin<
    C, T,
    typename std::enable_if_t<kIsLynxActor<T> &&
                              !std::is_same_v<T, shell::BTSRuntime>>>
    : public LynxActorMixinBase<C, T> {};

template <typename C>
class LynxActorMixin<C, shell::BTSRuntime, void>
    : public LynxActorMixinBase<C, shell::BTSRuntime> {
 public:
  void BeforeInvoked() {
    LynxActorMixinBase<C, shell::BTSRuntime>::BeforeInvoked();
    int32_t instance_id =
        static_cast<std::add_pointer_t<C>>(this)->GetInstanceId();
    auto& runner = static_cast<std::add_pointer_t<C>>(this)->GetRunner();
    runner->PushCurrentAllocSlot(runner->GetInstanceMemorySlot(instance_id));
  }

  void AfterInvoked() {
    auto& runner = static_cast<std::add_pointer_t<C>>(this)->GetRunner();
    runner->PopCurrentAllocSlot();
    LynxActorMixinBase<C, shell::BTSRuntime>::AfterInvoked();
  }
};

template <>
inline void
LynxActorMixinBase<LynxActor<LynxEngine>, LynxEngine>::ConsumeImplIfNeeded(
    LynxEngine* impl) {
  impl->Flush();
}

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_LYNX_ACTOR_SPECIALIZATION_H_
