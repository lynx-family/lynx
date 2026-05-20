// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_SCROLL_COORDINATOR_UI_SCROLL_COORDINATOR_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_SCROLL_COORDINATOR_UI_SCROLL_COORDINATOR_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {
static constexpr ArkUI_NodeEventType kFoldViewNodeEventTypes[] = {
    NODE_SCROLL_EVENT_ON_SCROLL,
    NODE_SCROLL_EVENT_ON_WILL_SCROLL,
};

class UIScrollCoordinator : public UIView {
 public:
  void UpdateFoldViewLayout();
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScrollCoordinator(context, sign, tag);
  }
  float GetHeaderHeight() const;
  bool IsScrollable() override;
  float ScrollY() override;

  bool CanConsumeGesture(float deltaX, float deltaY) override;

  bool IsAtBorder(bool isStart) override;

  int8_t GetScrollContainerDirection() override { return GestureConstants::DIRECTION_VERTICAL; }

  std::vector<float> GestureScrollBy(float delta_x, float delta_y) override;

  std::vector<float> ScrollBy(float delta_x, float delta_y) override;

 protected:
  UIScrollCoordinator(LynxContext* context, int sign, const std::string& tag);
  virtual bool IsToolbarTag(const std::string& tag) const;
  virtual bool IsHeaderTag(const std::string& tag) const;
  virtual bool IsSlotTag(const std::string& tag) const;
  virtual std::string RequiredChildrenErrorMessage() const;
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  void InsertNode(UIBase* child, int index) override;
  void RemoveNode(UIBase* child) override;
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  void OnNodeReady() override;
  EventTarget* HitTest(float point[2]) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) override;
  ~UIScrollCoordinator() override;
  bool DefaultOverflowValue() override { return false; }

 private:
  bool header_over_slot_{false};
  float fold_distance_{0.0};
  float granularity_{0.01};
  float pre_offset_{0.0};
  int32_t nested_scroll_forward_ = static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
  int32_t nested_scroll_backward_ = static_cast<int32_t>(ARKUI_SCROLL_NESTED_MODE_SELF_ONLY);
  using UIMethod = void (UIScrollCoordinator::*)(
      const lepus::Value& args, base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> foldview_ui_method_map_;
  ArkUI_NodeHandle foldview_{nullptr};
  ArkUI_NodeHandle column_{nullptr};
  UIBase* toolbar_{nullptr};
  UIBase* header_{nullptr};
  UIBase* slot_{nullptr};

  void SetFoldExpanded(const lepus::Value& args,
                       base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);

  void ScrollByMethod(const lepus::Value& args,
                      base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
};

class UIScrollCoordinatorToolBar : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScrollCoordinatorToolBar(context, sign, tag);
  }
  UIScrollCoordinatorToolBar(LynxContext* context, int sign, const std::string& tag);
  void GetOriginRect(float origin_rect[4]) override;
  float OffsetYForCalcPosition() override;
  UIScrollCoordinator* fold_view_{nullptr};
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  ~UIScrollCoordinatorToolBar() override;

 protected:
  bool DefaultOverflowValue() override { return false; }
};

class UIScrollCoordinatorHeader : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScrollCoordinatorHeader(context, sign, tag);
  }
  void GetOriginRect(float origin_rect[4]) override;
  UIScrollCoordinatorHeader(LynxContext* context, int sign, const std::string& tag);
  UIScrollCoordinator* fold_view_{nullptr};
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  ~UIScrollCoordinatorHeader() override;

 protected:
  bool DefaultOverflowValue() override { return false; }
};

class UIScrollCoordinatorSlot : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScrollCoordinatorSlot(context, sign, tag);
  }
  UIScrollCoordinatorSlot(LynxContext* context, int sign, const std::string& tag);
  void GetOriginRect(float origin_rect[4]) override;
  UIScrollCoordinator* fold_view_{nullptr};
  void UpdateLayout(float left, float top, float width, float height, const float* paddings,
                    const float* margins, const float* sticky, float max_height,
                    uint32_t node_index) override;
  ~UIScrollCoordinatorSlot() override;

 protected:
  bool DefaultOverflowValue() override { return false; }
};

class UIScrollCoordinatorSlotDrag : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIScrollCoordinatorSlotDrag(context, sign, tag);
  }
  UIScrollCoordinatorSlotDrag(LynxContext* context, int sign, const std::string& tag);

 protected:
  bool DefaultOverflowValue() override { return false; }
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_SCROLL_COORDINATOR_UI_SCROLL_COORDINATOR_H_
