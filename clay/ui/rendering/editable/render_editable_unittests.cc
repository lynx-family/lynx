// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/gpu_object.h"
#include "clay/gfx/picture.h"
#include "clay/ui/component/editable/text_editing_controller.h"
#include "clay/ui/compositing/pending_container_layer.h"
#include "clay/ui/compositing/pending_effect_layer.h"
#include "clay/ui/painter/painting_context.h"
#include "clay/ui/rendering/editable/render_editable.h"
#include "clay/ui/rendering/renderer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

fml::RefPtr<fml::TaskRunner> GetCurrentTaskRunner() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  return fml::MessageLoop::GetCurrent().GetTaskRunner();
}

class FakeRendererClient : public RendererClient {
 public:
  void RequestNewFrame() override {}

  RenderPhase GetRenderPhase() const override { return RenderPhase::kIdle; }

  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr picture,
                                             skity::Vec2 size) override {
    return nullptr;
  }

  void RegisterUploadTask(OneShotCallback<>&& task, int image_id) override {}
};

const PendingEffectLayer* FindFirstEffectLayer(PendingLayer* layer) {
  if (!layer) {
    return nullptr;
  }
  if (layer->GetName() == "PendingEffectLayer") {
    return static_cast<const PendingEffectLayer*>(layer);
  }
  for (PendingLayer* child = layer->FirstChild(); child;
       child = child->NextSibling()) {
    if (auto* effect_layer = FindFirstEffectLayer(child)) {
      return effect_layer;
    }
  }
  return nullptr;
}

}  // namespace

TEST(RenderEditableTest, ClipRectUsesPaintOffsetCoordinates) {
  auto unref_queue = fml::MakeRefCounted<GPUUnrefQueue>(GetCurrentTaskRunner());
  FakeRendererClient client;
  Renderer renderer(&client, unref_queue);

  TextEditingController controller;
  RenderEditable render_editable;
  render_editable.SetRenderer(&renderer);
  render_editable.SetTextEditingController(&controller);
  render_editable.SetWidth(120.f);
  render_editable.SetHeight(40.f);
  render_editable.SetMultiline(true);
  render_editable.SetDefaultLineHeight(14.f);
  render_editable.SetRoughTextLineHeight(14.f);
  render_editable.SetCaretWidth(1.f);
  render_editable.SetCaretDisplay(true);

  const FloatPoint paint_offset = FloatPoint(30.f, 20.f);
  const FloatPoint expected_clip_offset =
      paint_offset + render_editable.PaintOffset();
  const FloatRect expected_clip_rect(
      expected_clip_offset.x(), expected_clip_offset.y(),
      render_editable.Width() - render_editable.HorizontalThickness() -
          render_editable.CaretWidth(),
      render_editable.Height() - render_editable.VerticalThickness());

  PendingContainerLayer root_layer;
  PaintingContext context(&root_layer, &render_editable, unref_queue);
  render_editable.Paint(context, paint_offset);

  const PendingEffectLayer* effect_layer = FindFirstEffectLayer(&root_layer);
  ASSERT_NE(effect_layer, nullptr);
  ASSERT_TRUE(effect_layer->GetClipRectForTesting().has_value());
  EXPECT_EQ(effect_layer->GetClipRectForTesting().value(), expected_clip_rect);

  context.StopRecordingIfNeeded();
  unref_queue->Drain();
}

}  // namespace clay
