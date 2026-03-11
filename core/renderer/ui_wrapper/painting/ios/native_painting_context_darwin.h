// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_DARWIN_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_DARWIN_H_

#include <memory>
#include <string>
#include <vector>

#import <Lynx/LynxUIContext.h>
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"

@protocol LUIBodyView;

namespace lynx {
namespace tasm {

class NativePaintingCtxDarwin : public PaintingCtxPlatformImpl, public NativePaintingContext {
 public:
  NativePaintingCtxDarwin(UIView<LUIBodyView> *body_view, void *textra);
  ~NativePaintingCtxDarwin() override = default;

  NativePaintingCtxDarwin(const NativePaintingCtxDarwin &) = delete;
  NativePaintingCtxDarwin &operator=(const NativePaintingCtxDarwin &) = delete;

  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface> &queue) override {
    queue_ = std::static_pointer_cast<shell::DynamicUIOperationQueue>(queue);
  }

  // NativePaintingContextDarwin do not need impl this interface.
  void CreatePaintingNode(int id, const std::string &tag,
                          const fml::RefPtr<PropBundle> &painting_data, bool flatten,
                          bool create_node_async, uint32_t node_index) override {}

  // NativePaintingContextDarwin do not need impl this interface.
  void UpdatePaintingNode(int id, bool tend_to_flatten,
                          const fml::RefPtr<PropBundle> &painting_data) override {}

  // NativePaintingContextDarwin do not need impl this interface.
  void UpdateLayout(int tag, float x, float y, float width, float height, const float *paddings,
                    const float *margins, const float *borders, const float *bounds,
                    const float *sticky, float max_height, uint32_t node_index) override {}

  // NativePaintingContextDarwin do not need impl this interface.
  void SetKeyframes(fml::RefPtr<PropBundle> keyframes_data) override {}

  // NativePaintingContextDarwin do not need impl this interface.
  virtual void HandleValidate(int tag) override{};

  std::unique_ptr<pub::Value> GetTextInfo(const std::string &content,
                                          const pub::Value &info) override;

  std::vector<float> getBoundingClientOrigin(int id) override;

  std::vector<float> getWindowSize(int id) override;

  std::vector<float> GetRectToWindow(int id) override;

  std::vector<float> GetRectToLynxView(int64_t id) override;

  std::vector<float> ScrollBy(int64_t id, float width, float height) override;

  int32_t GetTagInfo(const std::string &tag_name) override;

  bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override;

  bool NeedAnimationProps() override;

  void Invoke(int64_t id, const std::string &method, const pub::Value &params,
              const std::function<void(int32_t, const pub::Value &)> &callback) override;

  void StopExposure(const pub::Value &options) override;
  void ResumeExposure() override;

  void UpdatePlatformExtraBundle(int32_t id, PlatformExtraBundle *bundle) override;

  void FinishTasmOperation(const std::shared_ptr<PipelineOptions> &options) override;

  void FinishLayoutOperation(const std::shared_ptr<PipelineOptions> &options) override;

  void Flush() override;

  NativePaintingContext *CastToNativeCtx() override {
    return static_cast<NativePaintingContext *>(this);
  }

  bool EnableUIOperationQueue() override { return true; }

#pragma region NativePaintingContext

  void CreatePlatformRenderer(int id, PlatformRendererType type,
                              const fml::RefPtr<PropBundle> &init_data) override;
  void CreatePlatformExtendedRenderer(int id, const base::String &tag_name,
                                      const fml::RefPtr<PropBundle> &init_data) override;

  void UpdateDisplayList(int id, DisplayList display_list) override;

  void UpdateTextBundle(int id, intptr_t bundle) override;

  void DestroyTextBundle(int id) override;

  void ReconstructEventTargetTreeRecursively() override;

  void UpdatePlatformEventBundle(int32_t id, PlatformEventBundle bundle) override;

  void CreateImage(int id, base::String src, float width, float height,
                   int32_t event_mask = 0) override;

#pragma endregion  // NativePaintingContext

 private:
  template <typename F>
  void Enqueue(F &&func);

  std::shared_ptr<shell::DynamicUIOperationQueue> queue_;

  std::unique_ptr<PlatformRendererContextDarwin> context_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_DARWIN_H_
