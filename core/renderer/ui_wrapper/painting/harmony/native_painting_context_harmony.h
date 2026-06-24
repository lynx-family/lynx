// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/public/painting_ctx_platform_impl.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_owner.h"

namespace lynx {
namespace tasm {

class TextLayoutManagerHarmony;

namespace harmony {
class ShadowNodeOwner;
}  // namespace harmony

class NativePaintingCtxHarmony : public PaintingCtxPlatformImpl,
                                 public NativePaintingContext {
 public:
  NativePaintingCtxHarmony(harmony::UIOwner* ui_owner,
                           harmony::ShadowNodeOwner* node_owner);
  ~NativePaintingCtxHarmony() override;

  NativePaintingContext* CastToNativeCtx() override {
    return static_cast<NativePaintingContext*>(this);
  }

  void CreatePaintingNode(int id, const std::string& tag,
                          const fml::RefPtr<PropBundle>& painting_data,
                          bool flatten, bool create_node_async,
                          uint32_t node_index) override {}
  void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const fml::RefPtr<PropBundle>& painting_data) override {}
  void UpdateLayout(int tag, float x, float y, float width, float height,
                    const float* paddings, const float* margins,
                    const float* borders, const float* bounds,
                    const float* sticky, float max_height, uint32_t node_index,
                    bool display_none) override {}
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override {}
  void Flush() override;
  void HandleValidate(int tag) override {}

  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override;
  std::vector<float> getBoundingClientOrigin(int id) override { return {}; }
  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info) override {
    return nullptr;
  }
  void StopExposure(const pub::Value& options) override {}
  void ResumeExposure() override {}
  std::vector<float> getWindowSize(int id) override { return {}; }
  std::vector<float> GetRectToWindow(int id) override { return {}; }
  std::vector<float> GetRectToLynxView(int64_t id) override { return {}; }
  std::vector<float> ScrollBy(int64_t id, float width, float height) override {
    return {};
  }
  void Invoke(int64_t id, const std::string& method, const pub::Value& params,
              const std::function<void(int32_t code, const pub::Value& data)>&
                  callback) override {}
  void EnqueueInvoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>& callback)
      override {}
  int32_t GetTagInfo(const std::string& tag_name) override;
  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override {
    return false;
  }
  void UpdatePlatformExtraBundle(int32_t id,
                                 PlatformExtraBundle* bundle) override;
  bool NeedAnimationProps() override { return false; }
  void InvokeUIMethod(int32_t id, const std::string& method,
                      fml::RefPtr<tasm::PropBundle> value,
                      int32_t callback_id) override {}
  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface>& queue) override;
  bool DefaultOverflowAlwaysVisible() override { return true; }
  bool EnableUIOperationQueue() override { return true; }

  void OnFirstScreen() override;

  void CreatePlatformRenderer(int id, PlatformRendererType type,
                              const fml::RefPtr<PropBundle>& init_data,
                              const PlatformRendererInitConfig& init_config =
                                  PlatformRendererInitConfig()) override;
  void CreatePlatformExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config =
          PlatformRendererInitConfig()) override;
  void UpdateDisplayList(int id, DisplayList display_list) override;
  fml::RefPtr<PaintImage> CreateImage(
      int id, base::String src, const ImagePaintInfo& paint_info, float width,
      float height, int32_t event_mask = 0,
      bool disable_default_resize = false) override;
  void UpdateTextBundle(int id, intptr_t bundle) override;
  void DestroyTextBundle(int id) override;
  void ReconstructEventTargetTreeRecursively() override;
  void UpdatePlatformEventBundle(int id, PlatformEventBundle bundle) override;

 private:
  void Enqueue(shell::UIOperation&& op);

  std::unique_ptr<harmony::UIOwner> ui_owner_;
  std::unique_ptr<TextLayoutManagerHarmony> text_layout_manager_;
  std::shared_ptr<harmony::LynxRendererContext> renderer_context_;
  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;
  bool has_first_screen_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_NATIVE_PAINTING_CONTEXT_HARMONY_H_
