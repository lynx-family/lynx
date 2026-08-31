// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_FRAME_VIEW_H_
#define CLAY_UI_COMPONENT_FRAME_VIEW_H_

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "clay/flow/frame_surface_registry.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/base_view.h"

namespace lynx {
namespace tasm {
class LynxTemplateBundle;
class TemplateData;
}  // namespace tasm
}  // namespace lynx

namespace clay {

class FrameChildSurfaceHost;
class KeyEvent;
class LayerTree;
struct PointerEvent;

class FrameView : public WithTypeInfo<FrameView, BaseView> {
 public:
  FrameView(int id, PageView* page_view);
  ~FrameView() override;

  void SetAttribute(const char* attr, const clay::Value& value) override;
  void OnLayoutUpdated() override;
  void HandleEvent(const PointerEvent& event) override;
  bool OnKeyEvent(const KeyEvent* event) override;

  void OnReceiveAppBundle(
      const std::shared_ptr<lynx::tasm::LynxTemplateBundle>& bundle);
  void MarkPropsUpdated();
  bool SubmitChildLayerTree(
      std::shared_ptr<LayerTree> layer_tree,
      std::optional<skity::Rect> damage_rect = std::nullopt);
  void SetIntrinsicContentSize(float width, float height);
  void SetEventForwarder(std::function<bool(const PointerEvent&)> pointer,
                         std::function<bool(const KeyEvent*)> key);
  void ClearEventForwarder();
  void DestroyChildContent();

  const FloatRect& content_frame() const { return content_frame_; }
  FloatSize child_viewport_size() const;
  bool auto_width() const { return auto_width_; }
  bool auto_height() const { return auto_height_; }
  const std::string& url() const { return url_; }
  int embedded_mode() const { return embedded_mode_; }
  const std::optional<bool>& enable_multi_async_thread() const {
    return enable_multi_async_thread_;
  }
  bool IsPointerEventForwardingEnabled() const {
    return !!pointer_event_forwarder_;
  }
  bool HasPendingBundleReady() const;
  bool HasPendingMetadataUpdate() const;
  void CommitPendingBundleAttached();
  void CommitPendingMetadataUpdated();
  const std::optional<FrameSurfaceId>& current_surface_id() const {
    return current_surface_id_;
  }
  const std::shared_ptr<lynx::tasm::LynxTemplateBundle>& pending_bundle()
      const {
    return pending_bundle_;
  }
  const std::shared_ptr<lynx::tasm::TemplateData>& pending_data() const {
    return pending_data_;
  }
  const std::shared_ptr<lynx::tasm::TemplateData>& pending_global_props()
      const {
    return pending_global_props_;
  }

 private:
  void ForwardMouseRegionEvent(const PointerEvent& event);
  void ApplyIntrinsicContentSize(bool force_layout = false);
  void ResetIntrinsicContentSize();
  std::optional<float> ToPresetLength(const clay::Value& value) const;

  static std::optional<float> ToNumber(const clay::Value& value);
  static std::optional<bool> ToBool(const clay::Value& value);
  static std::shared_ptr<lynx::tasm::TemplateData> ToTemplateData(
      const clay::Value& value);

  bool CanLoadPendingBundle() const;

  std::string url_;
  bool is_url_changed_ = false;
  bool is_props_updated_ = false;
  bool has_attached_bundle_ = false;

  bool auto_width_ = false;
  bool auto_height_ = false;
  std::optional<float> preset_width_;
  std::optional<float> preset_height_;
  int embedded_mode_ = 0;
  std::optional<bool> enable_multi_async_thread_;

  FloatRect content_frame_;
  bool content_frame_initialized_ = false;
  std::optional<FloatSize> intrinsic_content_size_;
  std::optional<FrameSurfaceId> current_surface_id_;
  std::shared_ptr<lynx::tasm::LynxTemplateBundle> pending_bundle_;
  std::shared_ptr<lynx::tasm::TemplateData> pending_data_;
  std::shared_ptr<lynx::tasm::TemplateData> pending_global_props_;
  std::unique_ptr<FrameChildSurfaceHost> child_surface_host_;
  std::function<bool(const PointerEvent&)> pointer_event_forwarder_;
  std::function<bool(const KeyEvent*)> key_event_forwarder_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_FRAME_VIEW_H_
