// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_UI_REFRESH_H_
#define PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_UI_REFRESH_H_

#include <string>
#include <unordered_map>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_view.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIRefresh;

class UIRefreshHeader : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIRefreshHeader(context, sign, tag);
  }
  UIRefresh* refresh_view_{nullptr};

 protected:
  UIRefreshHeader(LynxContext* context, int sign, const std::string& tag);
  ~UIRefreshHeader() override;
  void FrameDidChanged() override;
  bool DefaultOverflowValue() override { return false; }
};

typedef enum {
  LYNX_UI_REFRESH_STATE_IDLE = 0,
  LYNX_UI_REFRESH_STATE_OVER_DRAG_RELEASE = 1,
  LYNX_UI_REFRESH_STATE_REFRESHING = 2,
} LYNX_UI_REFRESH_STATE;

class UIRefresh : public UIView {
 public:
  static UIBase* Make(LynxContext* context, int sign, const std::string& tag) {
    return new UIRefresh(context, sign, tag);
  }
  bool IsScrollable() override;
  float ScrollY() override;
  void UpdateRefreshViewHeight();

 protected:
  UIRefresh(LynxContext* context, int sign, const std::string& tag);
  ~UIRefresh() override;
  void InsertNode(UIBase* child, int index) override;
  void RemoveNode(UIBase* child) override;
  void OnPropUpdate(const std::string& name, const lepus::Value& value) override;
  void InvokeMethod(const std::string& method, const lepus::Value& args,
                    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) override;
  void OnNodeEvent(ArkUI_NodeEvent* event) override;
  bool DefaultOverflowValue() override { return false; }

 public:
  bool enable_refresh_{true};
  UIBase* refresh_header_{nullptr};
  ArkUI_NodeHandle refresh_container_{nullptr};

 private:
  void FinishRefresh(const lepus::Value& args,
                     base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void AutoStartRefresh(const lepus::Value& args,
                        base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  void OnRefreshStateChanged(LYNX_UI_REFRESH_STATE state) const;
  void OnRefreshOffsetChanged() const;
  void OnHeaderReleased() const;
  using UIMethod = void (UIRefresh::*)(
      const lepus::Value& args, base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback);
  static std::unordered_map<std::string, UIMethod> ui_refresh_method_map_;
  int32_t action_last_loop_{UI_TOUCH_EVENT_ACTION_CANCEL};
  int refresh_state_{0};
  float refresh_offset_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_XELEMENT_REFRESH_UI_REFRESH_H_
