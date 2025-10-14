// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_NATIVE_VIEW_SERVICE_H_
#define CLAY_UI_PLATFORM_NATIVE_VIEW_SERVICE_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/common/service/service.h"
#include "clay/ui/common/measure_constraint.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"

namespace clay {

class NativeView;
class SharedImageSink;

class NativeViewPlugin : public ActorObject<Owner::kPlatform> {
 public:
  explicit NativeViewPlugin(int id) : id_(id) {}
  virtual ~NativeViewPlugin() = default;

  virtual std::string GetName() const = 0;

  // lifecycle should only called once
  virtual bool OnCreate(std::string tag) = 0;
  virtual void OnAttach() = 0;
  virtual void OnDetach() = 0;
  virtual void OnDestroy() = 0;
  virtual void OnTouchEvent(const PointerEvent& point_event,
                            const FloatPoint& transformed_postion) = 0;
  virtual void OnFocusChanged(bool focused, bool is_leaf) {}
  virtual void LayoutChanged(float left, float top, float width,
                             float height) = 0;
  virtual MeasureResult Measure(const MeasureConstraint& constraint) = 0;

  virtual void UpdatePaddings(float padding_left, float padding_top,
                              float padding_right, float padding_bottom) {}

  // Implement this function if we need to apply custom attributes on platform
  // view
  virtual void UpdatePlatformAttributes(const clay::Value::Map& attrs,
                                        const clay::Value::Array& events) {}
  // Implement this function if we need to handle custom method on platform
  // view
  virtual void InvokePlatformMethod(const std::string& method,
                                    const clay::Value::Map& args,
                                    const LynxUIMethodCallback& callback) {}

  virtual bool SupportHybridComposition() { return false; }

  virtual bool SupportScrolling() { return false; }
  virtual void ResignFirstResponder() {}

  virtual fml::RefPtr<SharedImageSink> GetSharedImageSink() = 0;

 protected:
  int id_;
};

class NativeViewService : public Service<NativeViewService, Owner::kPlatform> {
 public:
  static std::shared_ptr<NativeViewService> Create();

  virtual std::unique_ptr<NativeViewPlugin> CreateNativeViewPlugin(
      int id, NativeView* view_ptr) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_NATIVE_VIEW_SERVICE_H_
