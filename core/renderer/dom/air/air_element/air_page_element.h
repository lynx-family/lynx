// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_PAGE_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_PAGE_ELEMENT_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/page_proxy.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/compile_options.h"

namespace lynx {
namespace tasm {

class AirComponentElement;
class AirForElement;

class AirPageElement : public AirElement {
 public:
  constexpr const static char kDefaultPageTag[] = "page";

  AirPageElement(ElementManager* manager, uint32_t lepus_id, int32_t id = -1)
      : AirElement(kAirPage, manager, BASE_STATIC_STRING(kDefaultPageTag),
                   lepus_id, id) {
    manager->SetRootOnLayout(impl_id());
    manager->catalyzer()->set_air_root(this);
    manager->SetAirRoot(this);
  }

  bool UpdatePageData(const lepus::Value& table,
                      const UpdatePageOption& update_page_option,
                      PipelineOptions& pipeline_options);

  void SetContext(lepus::Context* context) { context_ = context; }

  void SetRadon(bool is_radon) { is_radon_ = is_radon; }
  bool IsRadon() const { return is_radon_; }

  bool RefreshWithGlobalProps(const lynx::lepus::Value& table,
                              bool should_render);

  void DeriveFromMould(ComponentMould* data);

  bool is_page() const override { return true; }
  void FlushRecursively() override;
  /**
   * save stack of for_element
   */
  void PushForElement(AirForElement* for_element) {
    for_stack_.push_back(for_element);
  };
  void PopForElement() { for_stack_.pop_back(); };

  AirForElement* GetCurrentForElement() {
    return for_stack_.size() ? for_stack_.back() : nullptr;
  };

  /**
   * stack for component
   */
  void PushComponentElement(AirComponentElement* component) {
    component_stack_.push_back(component);
  }
  void PopComponentElement() { component_stack_.pop_back(); }

  AirComponentElement* GetCurrentComponentElement() {
    return component_stack_.size() ? component_stack_.back() : nullptr;
  }

  lepus::Value GetData() override;
  lepus::Value GetPageDataByKey(const std::vector<std::string>& keys);
  lepus::Value GetProperties() override { return lepus::Value(); }

  uint64_t GetKeyForCreatedElement(uint32_t lepus_id);

  // Trigger Component LifeCycle Event.
  void FireComponentLifeCycleEvent(const std::string& name, int component_id);
  AirElement* GetNextElementForAsyncThread();
  void AppendLastElement();
  void CalcStyleAsync();
  void UpdateFirstScreenListState();
  void InitFirstScreenList(size_t size);

  void SetParsedStyles(const AirCompStylesMap& parsed_styles) override;

  void AddFontFaces(const AirCompStylesMap& parsed_styles, std::string path);
  void SetFontFaces();
  void RecordFirstScreenElement(AirElement* element) {
    last_element_ = element;
  }

 private:
  std::unordered_map<std::string, bool> component_fontfaces_map_{};
  lepus::Context* context_;
  std::vector<AirForElement*> for_stack_;
  std::vector<AirComponentElement*> component_stack_;
  std::unique_ptr<lepus::Value> current_page_data_;
  bool is_radon_{false};
  std::vector<AirElement*> first_screen_list_;
  std::atomic_long ui_thread_cursor_{-1};
  std::atomic_long async_thread_cursor_{-1};
  std::mutex first_screen_list_mutex_;
  AirElement* last_element_{nullptr};

  lepus::Value init_data_{};
  lepus::Value data_{};

  CSSFontFaceRuleMap font_faces_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_PAGE_ELEMENT_H_
