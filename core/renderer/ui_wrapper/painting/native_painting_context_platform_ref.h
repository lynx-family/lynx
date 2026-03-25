// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_

#include <memory>
#include <string>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/public/painting_ctx_platform_impl.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/dom/fragment/event/platform_event_emitter.h"
#include "core/renderer/dom/fragment/event/platform_event_handler.h"
#include "core/renderer/dom/fragment/event/platform_event_target_helper.h"
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"

namespace lynx {

namespace shell {
class LynxEngine;
}

namespace event {
class Event;
}

namespace tasm {

class DisplayList;
class PlatformEventTargetExposure;

class NativePaintingCtxPlatformRef : public PaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformRef(
      std::unique_ptr<PlatformRendererFactory> view_factory);
  ~NativePaintingCtxPlatformRef() override = default;

  void Destroy();

  void CreatePlatformRenderer(int id, PlatformRendererType type,
                              const fml::RefPtr<PropBundle> &init_data);
  void CreatePlatformExtendedRenderer(int id, const base::String &tag_name,
                                      const fml::RefPtr<PropBundle> &init_data);
  void UpdateDisplayList(int id, DisplayList &&display_list);

  void RemovePaintingNode(int parent, int child, int index,
                          bool is_move) override;
  void DestroyPaintingNode(int parent, int child, int index) override;
  void UpdateAttributes(int id, const fml::RefPtr<PropBundle> &attributes,
                        bool tend_to_flatten);

  // Set the engine actor for the painting context ref.
  void SetLynxEngineActorForPlatformContextRef(
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
  // Update the platform event bundle of the target element.
  void UpdatePlatformEventBundle(int32_t id, PlatformEventBundle bundle);
  // Get the platform event bundle of the target element.
  const PlatformEventBundle *GetPlatformEventBundle(int32_t id) const;
  // Reconstruct the event target tree recursively.
  fml::RefPtr<PlatformEventTarget> ReconstructEventTargetTreeRecursively();
  // did_reconstruct is set to true if the event target tree is reconstructed.
  fml::RefPtr<PlatformEventTarget> ReconstructEventTargetTreeRecursively(
      bool *did_reconstruct);
  // Add the target element to the exposure target map.
  void AddPlatformEventTargetToExposure(
      const fml::RefPtr<PlatformEventTarget> &target,
      const lepus::Value &option);
  // Remove the target element from the exposure target map.
  void RemovePlatformEventTargetFromExposure(
      const fml::RefPtr<PlatformEventTarget> &target,
      const lepus::Value &option);
  // Clear the exposure target map.
  void ClearExposureTargetMap();
  // Invoke the method of the ui element.
  void InvokeUIMethod(
      int32_t id, const std::string &method, const lepus::Value &params,
      base::MoveOnlyClosure<void, int32_t, const pub::Value &> callback);

  // Get the location of the root view on the screen.
  virtual void GetRootViewLocationOnScreen(float location[2]) {}

  // Get the size of the screen.
  virtual void GetScreenSize(float size[2]) {}

  bool IsNativePaintingCtxPlatformRef() override { return true; }

 protected:
  void RebuildSubLayers(const fml::RefPtr<PlatformRenderer> &renderer,
                        const base::InlineVector<int, 16> &new_children);

  std::unique_ptr<PlatformRendererFactory> view_factory_;
  base::InlineOrderedFlatMap<int32_t, fml::RefPtr<PlatformRenderer>, 64>
      renderers_;
  std::shared_ptr<shell::LynxActor<shell::LynxEngine>> engine_actor_{nullptr};
  std::unique_ptr<PlatformEventHandler> event_handler_ =
      std::make_unique<PlatformEventHandler>(this);
  std::unique_ptr<PlatformEventEmitter> event_emitter_ =
      std::make_unique<PlatformEventEmitter>(this);
  std::unique_ptr<PlatformEventTargetHelper> event_target_helper_ =
      std::make_unique<PlatformEventTargetHelper>(this);
  std::shared_ptr<PlatformEventTargetExposure> event_target_exposure_;
  base::InlineOrderedFlatMap<int32_t, PlatformEventBundle, 64>
      platform_event_bundles_;
  bool need_reconstruct_event_target_tree_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_NATIVE_PAINTING_CONTEXT_PLATFORM_REF_H_
