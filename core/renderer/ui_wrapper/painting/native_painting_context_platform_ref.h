// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_

#include <memory>

#include "base/include/value/base_string.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/renderer/dom/fragment/event/platform_event_emitter.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"

constexpr const static int32_t kRootId = 10;

namespace lynx {

namespace shell {
class LynxEngine;
}

namespace event {
class Event;
}

namespace tasm {

class DisplayList;

class NativePaintingCtxPlatformRef : public PaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformRef(
      std::unique_ptr<PlatformRendererFactory> view_factory);
  ~NativePaintingCtxPlatformRef() override = default;

  void CreatePlatformRenderer(int id, PlatformRendererType type);
  void CreatePlatformExtendedRenderer(int id, const base::String &tag_name);
  void UpdateDisplayList(int id, DisplayList &&display_list);

  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;

  void SetLynxEngineActorForPlatformRendererContext(
      std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor);
  // The event data from the platform layer is forwarded to PlatformEventHandler
  // for subsequent event processing.
  bool DispatchPlatformInputEvent(int int_event_data[],
                                  float float_event_data[]);
  // The current state of PlatformEventHandler is obtained to determine the
  // gesture handling at the platform layer.
  int GetPlatformEventHandlerState();
  // Send event to the target element.
  void SendEvent(int32_t target_id, fml::RefPtr<event::Event> event);
  // Update the pseudo status of the target element.
  void UpdatePseudoStatusStatus(int32_t target_id, uint32_t pre_status,
                                uint32_t current_status);
  // Get PlatformEventEmitter instance.
  PlatformEventEmitter *GetEventEmitter();
  // Get PlatformEventTargetHelper instance.
  PlatformEventTargetHelper *GetEventTargetHelper();

 protected:
  void RebuildSubLayers(const fml::RefPtr<PlatformRenderer> &renderer,
                        const base::InlineVector<int, 16> &new_children);

  std::unique_ptr<PlatformRendererFactory> view_factory_;
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformRenderer>, 64>
      renderers_;
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_{nullptr};
  std::unique_ptr<PlatformEventEmitter> event_emitter_ =
      std::make_unique<PlatformEventEmitter>(this);
  std::unique_ptr<PlatformEventTargetHelper> event_target_helper_ =
      std::make_unique<PlatformEventTargetHelper>();
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_
