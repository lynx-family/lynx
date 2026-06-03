// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_H_

#include <node_api.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/value/base_value.h"
#include "core/base/lynx_export.h"
#include "core/public/layout_node_manager.h"
#include "core/public/layout_node_value.h"
#include "core/renderer/ui_wrapper/common/harmony/prop_bundle_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/event/event_target.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/shadow_node/measure_mode.h"

namespace lynx {
namespace tasm {

class PropBundle;

namespace harmony {
class ParagraphContent;
class LynxContext;

class CustomMeasureFunc {
 public:
  virtual LayoutResult Measure(float width, MeasureMode width_mode,
                               float height, MeasureMode height_mode,
                               bool final_measure) = 0;
  virtual void Align() = 0;
};

class MeasureFuncHarmony : public MeasureFunc {
 public:
  explicit MeasureFuncHarmony(CustomMeasureFunc* custom_measure_func);
  ~MeasureFuncHarmony() override = default;
  LayoutResult Measure(float width, int32_t width_mode, float height,
                       int32_t height_mode, bool final_measure) override;
  void Alignment() override;

 private:
  CustomMeasureFunc* custom_measure_func_;
};

class LYNX_EXPORT ShadowNode : public fml::RefCountedThreadSafeStorage {
 public:
  explicit ShadowNode(const int sign, const std::string& tag)
      : sign_(sign), tag_(tag) {}
  ~ShadowNode() override = default;
  virtual void OnLayoutBefore();
  virtual void UpdateLayout(float left, float top, float width, float height);
  virtual void UpdateProps(PropBundleHarmony* props);
  virtual void AddChild(ShadowNode* child, int index);
  virtual void RemoveChild(ShadowNode* child);
  virtual fml::RefPtr<fml::RefCountedThreadSafeStorage> getExtraBundle() {
    return nullptr;
  }
  virtual void OnPropsUpdate(const std::string& name,
                             const lepus::Value& value);
  void AdoptSlNode();
  void SetParent(ShadowNode* parent) {
    parent_sign_ = parent ? parent->Signature() : -1;
  }
  const std::vector<ShadowNode*>& GetChildren() const { return children_; }
  const std::string& GetIdSelector() const { return id_selector_; }
  float ScaleDensity() const;
  virtual bool IsVirtual() const { return false; }
  const char* Tag() const { return tag_.data(); }
  int Signature() const { return sign_; }
  void AlignLayoutNode(float left, float top) const;
  virtual void AlignTo(float left, float top) const;
  virtual ParagraphContent* AsParagraphContent() { return nullptr; }
  LayoutResult MeasureLayoutNode(float width, MeasureMode width_mode,
                                 float height, MeasureMode height_mode,
                                 bool final_measure) const;
  float ComputedWidth() const;
  float ComputedHeight() const;
  float ComputedMinWidth() const;
  // if max-width is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
  float ComputedMaxWidth() const;
  float ComputedMinHeight() const;
  // if max-height is not set, return LayoutNodeStyle::UNDEFINED_MAX_SIZE
  float ComputedMaxHeight() const;
  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager);
  virtual void MeasureChildrenNode(float width, MeasureMode width_mode,
                                   float height, MeasureMode height_mode,
                                   bool final_measure);
  virtual bool IsPlaceholder() const { return false; };
  virtual bool IsTextShadowNode() const { return false; }

  virtual void OnContextReady() {}

  void SetContext(LynxContext* context) {
    context_ = context;
    OnContextReady();
  }

  bool IsBindEvent() const;

  /**
   *
   * @return A js object to the object
   */
  virtual napi_value GetJSObject() const { return nullptr; }
  virtual bool HasJSObject() const { return false; }
  virtual void MarkDirty() const;
  virtual void RequestLayout() const;
  virtual void Destroy(){};
  virtual void MarkDestroyed() { is_destroyed_ = true; }
  inline bool IsDestroyed() const { return is_destroyed_; }

 protected:
  void SetCustomMeasureFunc(CustomMeasureFunc* measure_func);
  LynxContext* context_{nullptr};
  std::vector<std::string> events_;
  LynxPointerEventsValue pointer_events_{LynxPointerEventsValue::kUnset};
  LynxEventPropStatus event_through_{LynxEventPropStatus::kUndefined};
  LynxEventPropStatus ignore_focus_{LynxEventPropStatus::kUndefined};
  void ReleaseSelf() const override;

 private:
  void SetPointerEvents(const lepus::Value& value);
  void SetIgnoreFocus(const lepus::Value& value);
  void SetEventThrough(const lepus::Value& value);
  void SetIdSelector(const lepus::Value& value);

  const ShadowNode* FindNonVirtualNode() const;
  void SetEvent(const std::vector<lepus::Value>& events);
  using PropSetter = void (ShadowNode::*)(const lepus::Value& value);
  static std::unordered_map<std::string, PropSetter> prop_setters_;
  int32_t sign_;
  std::string tag_;
  std::string id_selector_;
  std::vector<ShadowNode*> children_;
  int32_t parent_sign_{-1};
  CustomMeasureFunc* custom_measure_func_ = nullptr;
  LayoutNodeManager* layout_node_manager_ = nullptr;
  bool is_destroyed_ = false;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_SHADOW_NODE_SHADOW_NODE_H_
