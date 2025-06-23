// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_TEXT_EVENT_TARGET_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_TEXT_EVENT_TARGET_H_
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {

class TextEventTarget : public std::enable_shared_from_this<TextEventTarget>,
                        public EventTarget {
 public:
  explicit TextEventTarget(int32_t sign)
      : TextEventTarget(0, 0, sign, LynxEventPropStatus::kUndefined,
                        LynxEventPropStatus::kUndefined) {}
  TextEventTarget(size_t start, size_t end, int32_t sign,
                  LynxEventPropStatus event_through,
                  LynxEventPropStatus ignore_focus)
      : event_through_(event_through),
        ignore_focus_(ignore_focus),
        start_(start),
        end_(end),
        sign_(sign) {}

  void AddChild(const std::shared_ptr<TextEventTarget>& child);

  void SetStart(size_t start) { start_ = start; }
  size_t Start() const { return start_; }
  void SetEnd(size_t end) { end_ = end; }
  size_t End() const { return end_; }
  std::list<std::shared_ptr<TextEventTarget>> Children() const {
    return children_;
  }

  EventTarget* ParentTarget() override;
  void GetPointInTarget(float res[2], EventTarget* parent_target,
                        float point[2]) override{};
  bool BlockNativeEvent() override;
  bool EventThrough() override;
  bool IgnoreFocus() override;
  ConsumeSlideDirection ConsumeSlideEvent() override;
  EventTarget* HitTest(float point[2]) override;
  bool ShouldHitTest() override;
  bool ContainsPoint(float point[2]) override;
  bool IsOnResponseChain() override;
  bool IsInterceptGesture() override { return false; }
  void SetParent(EventTarget* parent);
  void AddRect(float left, float top, float right, float bottom);

  int Sign() const override { return sign_; };
  bool HasUI() override { return false; };
  std::weak_ptr<EventTarget> WeakTarget() override { return weak_from_this(); };
  void OnPseudoStatusChanged(PseudoStatus pre_status,
                             PseudoStatus current_status) override{};
  PseudoStatus GetPseudoStatus() override { return PseudoStatus::kNone; };
  bool TouchPseudoPropagation() override { return false; };

 private:
  std::list<std::shared_ptr<TextEventTarget>> children_;
  EventTarget* parent_;
  LynxEventPropStatus event_through_{LynxEventPropStatus::kUndefined};
  LynxEventPropStatus ignore_focus_{LynxEventPropStatus::kUndefined};

  struct Rect {
    float left{0.f};
    float top{0.f};
    float right{0.f};
    float bottom{0.f};

    Rect(float l, float t, float r, float b)
        : left(l), top(t), right(r), bottom(b) {}
  };
  std::vector<Rect> text_rects_;
  size_t start_;
  size_t end_;
  const int32_t sign_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_TEXT_TEXT_EVENT_TARGET_H_
