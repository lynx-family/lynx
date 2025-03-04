// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_EMPTY_PAINTING_CONTEXT_IMPLEMENTATION_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_EMPTY_PAINTING_CONTEXT_IMPLEMENTATION_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/public/platform_extra_bundle.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"
#include "core/services/timing_handler/timing.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

class PaintingContextPlatformImpl : public PaintingCtxPlatformImpl {
 public:
  PaintingContextPlatformImpl() {
    platform_ref_ = std::make_shared<PaintingCtxPlatformRef>();
  }
  virtual ~PaintingContextPlatformImpl() {}
  virtual void CreatePaintingNode(
      int id, const std::string& tag,
      const std::shared_ptr<PropBundle>& painting_data, bool flatten,
      bool create_node_async, uint32_t node_index) override {}
  virtual void InsertPaintingNode(int parent, int child, int index) override {}
  virtual void RemovePaintingNode(int parent, int child, int index,
                                  bool is_move) override {}
  virtual void DestroyPaintingNode(int parent, int child, int index) override {}
  virtual void UpdatePaintingNode(
      int id, bool tend_to_flatten,
      const std::shared_ptr<PropBundle>& painting_data) override {}
  virtual void UpdateLayout(int tag, float x, float y, float width,
                            float height, const float* paddings,
                            const float* margins, const float* borders,
                            const float* bounds, const float* sticky,
                            float max_height,
                            uint32_t node_index = 0) override {}
  virtual void SetKeyframes(
      std::unique_ptr<PropBundle> keyframes_data) override {}
  virtual void Flush() override {}
  virtual void HandleValidate(int tag) override {}
  virtual void FinishTasmOperation(const PipelineOptions& options) override {}
  virtual void FinishLayoutOperation(const PipelineOptions& options) override {}

  virtual std::vector<float> getBoundingClientOrigin(int id) override {
    return floats_;
  }
  virtual std::vector<float> getWindowSize(int id) override { return floats_; }
  virtual std::vector<float> GetRectToWindow(int id) override {
    return floats_;
  }
  virtual std::vector<float> GetRectToLynxView(int64_t id) override {
    return floats_;
  }
  virtual std::vector<float> ScrollBy(int64_t id, float width,
                                      float height) override {
    return floats_;
  }

  virtual void ConsumeGesture(int64_t id, int32_t gesture_id,
                              const pub::Value& params) override {}

  virtual void Invoke(
      int64_t id, const std::string& method, const pub::Value& params,
      const std::function<void(int32_t code, const pub::Value& data)>& callback)
      override {}
  virtual int32_t GetTagInfo(const std::string& tag_name) override { return 0; }
  virtual bool IsFlatten(base::MoveOnlyClosure<bool, bool> func) override {
    return false;
  }

  virtual std::unique_ptr<pub::Value> GetTextInfo(
      const std::string& content, const pub::Value& info) override {
    lepus::Value lepus_result;
    return std::make_unique<PubLepusValue>(lepus_result);
  }

  virtual bool NeedAnimationProps() override { return false; }

 private:
  std::vector<float> floats_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_EMPTY_PAINTING_CONTEXT_IMPLEMENTATION_H_
