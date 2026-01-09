// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_NATIVE_VIEW_H_
#define CLAY_UI_COMPONENT_NATIVE_VIEW_H_

#include <memory>
#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/public/value.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/view_tree_observer.h"
#include "clay/ui/platform/native_view_service.h"

namespace clay {

class NativeView : public WithTypeInfo<NativeView, BaseView>,
                   public OnPaintingListener {
 public:
  NativeView(int id, std::string tag, PageView* page_view);
  ~NativeView() override = default;

  void OnPainting() override;
  void OnAttachToTree() override;
  void OnDetachFromTree() override;
  void SetPaddings(float padding_left, float padding_top, float padding_right,
                   float padding_bottom) override;
  void SendMotionEvent(const PointerEvent& point_event,
                       const FloatPoint& transformed_postion);
  void SetAttribute(const char* attr, const clay::Value& value) override;
  void DidUpdateAttributes() override;
  void HandleEvent(const PointerEvent& event) override;

  // Invoke platform view's platform method
  void InvokePlatformMethod(const std::string& method_name,
                            clay::Value::Map args,
                            const LynxUIMethodCallback& callback);
  FloatRect bounds() const { return bounds_; }
  bool IsNativeViewAvailable() const { return is_available_; }
  bool IsScrollEnabled() const { return is_scroll_enabled_; }

  MeasureResult Measure(const MeasureConstraint& constraint);
  void MarkLayoutDirty();

  void MarkAsEditing();
  void ResignFirstResponder();

 private:
  void OnDestroy() override;
  void FocusHasChanged(bool focused, bool is_leaf) override;

  void ApplyUpdateChanged();

  Puppet<Owner::kUI, std::unique_ptr<NativeViewPlugin>> native_view_plugin_;
  // Cache all attributes which need to be updated
  // then apply these to platform view if needed.
  clay::Value::Map staging_attrs_;

  std::optional<int64_t> tex_id_;
  FloatRect bounds_;
  float device_pixel_ratio_ = 1.0;
  bool is_scroll_enabled_;
  bool is_editing_ = false;
  bool is_available_ = false;
};
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_NATIVE_VIEW_H_
