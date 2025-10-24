// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_RENDER_DELEGATE_H_
#define CLAY_UI_RENDER_DELEGATE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/task_runners.h"
#include "clay/flow/frame_timings.h"
#include "clay/gfx/paint_image.h"
#ifdef ENABLE_ACCESSIBILITY
#include "clay/ui/semantics/semantics_update_node.h"
#endif

namespace clay {

class BaseView;
class DrawableImage;
class LayerTree;
class ResourceLoaderIntercept;
class ShadowNode;

class RenderDelegate {
 public:
  virtual ~RenderDelegate() {}

  virtual void ScheduleFrame() = 0;
  virtual void ForceBeginFrame() = 0;
  virtual void OnFirstMeaningfulLayout() = 0;
  virtual void ScheduleLayout() {}
  virtual bool Raster(
      std::unique_ptr<clay::LayerTree> layer_tree,
      std::unique_ptr<clay::FrameTimingsRecorder> recorder = nullptr,
      bool force = false) = 0;
  virtual void ShowSoftInput(int type, int action) = 0;
  virtual void HideSoftInput() = 0;
  virtual std::string ShouldInterceptUrl(const std::string& origin_url,
                                         bool should_decode) = 0;
  virtual std::shared_ptr<ResourceLoaderIntercept>
  GetResourceLoaderIntercept() {
    return nullptr;
  }

  virtual void MakeRasterSnapshot(
      std::unique_ptr<LayerTree> layer_tree,
      std::function<void(fml::RefPtr<PaintImage>)> callback) = 0;
  virtual fml::RefPtr<PaintImage> MakeRasterSnapshot(
      GrPicturePtr picture, skity::Vec2 picture_size) = 0;

  virtual void DumpInfoToDevtoolEnabled(bool enabled) {}
  virtual void SetClipboardData(const std::u16string& data) = 0;
  virtual std::u16string GetClipboardData() = 0;

#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  // Text input related functions Begin.
  virtual void SetTextInputClient(int client_id, const char* input_action,
                                  const char* input_type) = 0;
  virtual void ClearTextInputClient() = 0;
  virtual void SetEditableTransform(const float transform_matrix[16]) = 0;
  virtual void SetEditingState(uint64_t selection_base,
                               uint64_t composing_extent,
                               const std::string& selection_affinity,
                               const std::string& text,
                               bool selection_directional,
                               uint64_t selection_extent,
                               uint64_t composing_base) = 0;
  virtual void SetCaretRect(float x, float y, float width, float height) = 0;
  virtual void setMarkedTextRect(float x, float y, float width,
                                 float height) = 0;
  virtual void ShowTextInput() = 0;
  virtual void HideTextInput() = 0;
  // Text input related functions End.
  virtual void WindowMove() = 0;
  virtual void ActivateSystemCursor(int type, const std::string& path) = 0;
#endif

  virtual void FilterInputAsync(
      const std::string& input, const std::string& pattern,
      std::function<void(const std::string&)> callback) = 0;
  virtual void ReportTiming(
      const std::unordered_map<std::string, int64_t>& timing,
      const std::string& flag) = 0;

  virtual void ClearTextCache() {}
  virtual BaseView* FindViewById(int view_id) = 0;
  virtual ShadowNode* FindShadowNodeById(int node_id) = 0;
  virtual void UpdateRootSize(int32_t width, int32_t height) {}
#ifdef ENABLE_ACCESSIBILITY
  virtual void UpdateSemantics(const SemanticsUpdateNodes& update_nodes) {}
#endif

  virtual void RegisterDrawableImage(
      std::shared_ptr<DrawableImage> drawable_image) = 0;
  virtual void UnregisterDrawableImage(int64_t id) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_RENDER_DELEGATE_H_
