// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_

#include <arkui/native_gesture.h>
#include <native_drawing/drawing_canvas.h>
#include <node_api.h>

#include <algorithm>
#include <string>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/geometry/rect.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIText : public UIBase {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIText(context, sign, tag);
  }
  UIText(LynxContext* context, int sign, const std::string& tag);
  ~UIText() override;
  void Update(ParagraphHarmony* paragraph);
  void UpdateExtraData(
      const fml::RefPtr<fml::RefCountedThreadSafeStorage>& extra_data) override;
  void OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;
  void OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) override;
  void Render(OH_Drawing_Canvas* canvas) const;
  void OnNodeReady() override;
  void FrameDidChanged() override;
  EventTarget* HitTest(float point[2]) override;
  const std::string& GetAccessibilityLabel() const override;
  bool HasOverlappingRendering() override { return HasBackground(); }

  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&>
                        callback) override;

 private:
  void OnPropUpdate(const std::string& name,
                    const lepus::Value& value) override;
  void UpdateInlineImageFrame();

  // Selection methods.
  void GetTextBoundingRect(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void SetTextSelection(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void GetSelectedText(
      const lepus::Value& args,
      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  // Gestures.
  void InitSelectionGestures();
  void DisposeSelectionGestures();
  void UpdateSelectionGestureState();
  static void OnLongPress(ArkUI_GestureEvent* event, void* extra);
  static void OnPan(ArkUI_GestureEvent* event, void* extra);
  static void OnTap(ArkUI_GestureEvent* event, void* extra);
  static ArkUI_GestureInterruptResult OnGestureInterrupt(
      ArkUI_GestureInterruptInfo* info);

  // Handle dot nodes.
  static void HandleNodeCustomEventReceiver(ArkUI_NodeCustomEvent* event);
  static void OnHandlePan(ArkUI_GestureEvent* event, void* extra);
  static ArkUI_GestureInterruptResult OnHandleGestureInterrupt(
      ArkUI_GestureInterruptInfo* info);

  struct HandleDrawExtra {
    UIText* owner{nullptr};
    int32_t type{0};
  };

  struct HandlePanExtra {
    UIText* owner{nullptr};
    bool is_start{false};
  };

  bool HasEvent(const std::string& name) const {
    return std::find(events_.begin(), events_.end(), name) != events_.end();
  }

  int32_t GetTextChar16Count() const;
  bool HasValidSelection() const;
  void ClearSelection();
  void UpdateSelectionRange(int32_t start, int32_t end);
  void UpdateSelectStartEnd();
  void SendSelectionChangeEvent() const;
  void DrawSelectionHighlight(OH_Drawing_Canvas* canvas) const;
  void DrawSelectionHandles(OH_Drawing_Canvas* canvas) const;

  float GetXForEvent(const ArkUI_UIInputEvent* event) const;
  float GetYForEvent(const ArkUI_UIInputEvent* event) const;
  int32_t GetOffsetForPosition(float x, float y) const;
  void HandleLongPress(ArkUI_GestureEventActionType action);
  void HandlePan(ArkUI_GestureEventActionType action, float x, float y);
  void PerformEndSelection(float x, float y);
  void AdjustStartPosition(float x, float y);
  void AdjustEndPosition(float x, float y);
  bool ShouldInterceptPan() const;

  std::vector<base::geometry::FloatRect> GetTextBoundingBoxes(
      int32_t start, int32_t end) const;
  fml::RefPtr<lepus::Dictionary> GetTextBoundingRectFromBoxes(
      const std::vector<base::geometry::FloatRect>& boxes,
      const lepus::Value& args, float* rect) const;
  fml::RefPtr<lepus::Dictionary> GetMapFromRect(
      float* rect, const base::geometry::FloatRect& box) const;
  std::vector<std::vector<float>> GetHandlesInfo() const;
  fml::RefPtr<lepus::Dictionary> GetHandleMap(const std::vector<float>& handle,
                                              float* rect) const;

  void EnsureHandleNodesCreated();
  void EnsureHandleNodesAttached();
  void UpdateHandleNodes();
  void HideHandleNodes();
  void DrawHandleNode(OH_Drawing_Canvas* canvas, int32_t type) const;
  void HandlePanOnHandleNode(bool is_start, ArkUI_GestureEvent* event);

  fml::RefPtr<ParagraphHarmony> paragraph_{nullptr};
  float translate_left_offset_{0.f};
  float translate_top_offset_{0.f};

  // Selection props.
  bool enable_text_selection_{false};
  bool enable_custom_context_menu_{false};
  bool enable_custom_text_selection_{false};

  // Selection state.
  bool is_in_selection_{false};
  bool track_long_press_{false};
  bool is_forward_{true};
  int32_t select_start_{-1};
  int32_t select_end_{-1};
  int32_t last_select_start_{-1};
  int32_t last_select_end_{-1};
  float long_press_x_{0.f};
  float long_press_y_{0.f};
  float start_x_{0.f};
  float start_y_{0.f};
  float end_x_{0.f};
  float end_y_{0.f};
  bool is_adjust_start_{false};
  bool is_adjust_end_{false};
  bool long_press_fired_{false};
  bool show_start_handle_{true};
  bool show_end_handle_{true};

  ArkUI_GestureRecognizer* long_press_gesture_{nullptr};
  ArkUI_GestureRecognizer* pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* tap_gesture_{nullptr};
  bool selection_gestures_inited_{false};

  // Selection painting.
  uint32_t selection_bg_color_argb_{0x141B7DF0};
  uint32_t handle_color_argb_{0xFF1B7DF0};
  float stem_width_vp_{2.f};
  float handle_dot_size_vp_{10.f};

  // Handle nodes.
  ArkUI_NodeHandle start_handle_dot_node_{nullptr};
  ArkUI_NodeHandle end_handle_dot_node_{nullptr};
  ArkUI_GestureRecognizer* start_handle_pan_gesture_{nullptr};
  ArkUI_GestureRecognizer* end_handle_pan_gesture_{nullptr};
  HandleDrawExtra start_dot_draw_extra_{};
  HandleDrawExtra end_dot_draw_extra_{};
  HandlePanExtra start_pan_extra_{};
  HandlePanExtra end_pan_extra_{};
  bool handle_nodes_attached_{false};
  ArkUI_NodeHandle handle_parent_{nullptr};
  ArkUI_NodeHandle handle_parent_clip_node_{nullptr};
  int32_t handle_parent_clip_value_{0};
  bool handle_parent_clip_overridden_{false};
  float start_dot_left_vp_{0.f};
  float start_dot_top_vp_{0.f};
  float end_dot_left_vp_{0.f};
  float end_dot_top_vp_{0.f};
  float dot_node_size_vp_{0.f};
  float dot_radius_px_{0.f};
  float handle_pan_accept_x_{0.f};
  float handle_pan_accept_y_{0.f};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UI_TEXT_H_
