// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_COMPONENT_ELEMENT_H_
#define CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_COMPONENT_ELEMENT_H_

#include <memory>
#include <vector>

#include "core/renderer/dom/air/air_element/air_element.h"
#include "core/template_bundle/template_codec/moulds.h"

namespace lynx {
namespace tasm {

class AirComponentElement : public AirElement {
 public:
  AirComponentElement(ElementManager* manager, int tid, uint32_t lepus_id,
                      int32_t id, lepus::Context* context);
  void DeriveFromMould(ComponentMould* mould);

  bool is_component() const override { return true; }

  void SetName(const base::String& name) { name_ = name; }
  void SetPath(const base::String& path) { path_ = path; }

  void SetProperty(const base::String& key, const lepus::Value& value);
  void SetProperties(const lepus::Value& value);
  void SetData(const lepus::Value& data);
  void SetData(const base::String& key, const lepus::Value& value);
  lepus::Value GetProperties() override { return properties_; };
  lepus::Value GetData() override { return data_; };

  void CreateComponentInLepus();
  bool UpdateComponentInLepus(const lepus::Value& data);

  uint32_t NonVirtualNodeCountInParent() override;

  void OnElementRemoved() override;

  void SetParsedStyles(const AirCompStylesMap& parsed_styles) override;

 private:
  lepus::Value data_{};
  lepus::Value properties_{};
  base::String name_;
  base::String path_;
  int32_t tid_;
  lepus::Context* context_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_AIR_ELEMENT_AIR_COMPONENT_ELEMENT_H_
