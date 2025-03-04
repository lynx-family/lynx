// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/air/air_element/air_component_element.h"

#include <memory>
#include <string>
#include <utility>

#include "base/include/string/string_utils.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {

AirComponentElement::AirComponentElement(ElementManager *manager, int tid,
                                         uint32_t lepus_id, int32_t id,
                                         lepus::Context *context)
    : AirElement(kAirComponent, manager, BASE_STATIC_STRING(kAirComponentTag),
                 lepus_id, id),
      tid_(tid),
      context_(context) {}

void AirComponentElement::DeriveFromMould(ComponentMould *mould) {
  if (mould != nullptr) {
    properties_ = lepus::Value::Clone(mould->properties(),
                                      context_ && context_->IsLepusNGContext());
    data_ = mould->data();
  }
  if (!data_.IsObject()) {
    data_ = lepus::Value::CreateObject(context_);
  }
  if (!properties_.IsObject()) {
    properties_ = lepus::Value::CreateObject(context_);
  }
}

void AirComponentElement::SetProperties(const lepus::Value &value) {
  if (value.IsObject()) {
    properties_ = value;
  }
}

void AirComponentElement::SetProperty(const base::String &key,
                                      const lepus::Value &value) {
  const lepus::Value &v = properties_.GetProperty(key);
  if (CheckTableValueNotEqual(v, value)) {
    properties_.SetProperty(key, value);
  }
}

void AirComponentElement::SetData(const lepus::Value &data) {
  if (data.IsObject() && UpdateComponentInLepus(data)) {
    PipelineOptions options;
    element_manager()->OnPatchFinishInnerForAir(options);
  }
}

void AirComponentElement::SetData(const base::String &key,
                                  const lepus::Value &value) {
  const lepus::Value &v = data_.GetProperty(key);
  if (CheckTableValueNotEqual(v, value)) {
    data_.SetProperty(key, value);
  }
}

void AirComponentElement::CreateComponentInLepus() {
  if (!data_.IsObject()) {
    data_ = lepus::Value(lepus::Dictionary::Create());
  }
  lepus::Value p1(AirLepusRef::Create(
      element_manager()->air_node_manager()->Get(impl_id())));
  context_->Call("$createComponent" + std::to_string(tid_), p1, data_,
                 properties_);
}

bool AirComponentElement::UpdateComponentInLepus(const lepus::Value &data) {
  auto updated_keys = lepus::CArray::Create();
  tasm::ForEachLepusValue(
      data,
      [this, updated_keys](const lepus::Value &key, const lepus::Value &value) {
        auto key_str = key.String();
        lepus::Value ret = data_.GetProperty(key_str);
        if (!ret.IsEmpty()) {
          if (CheckTableShadowUpdated(ret, value) ||
              value.GetLength() != ret.GetLength()) {
            updated_keys->push_back(key);
            data_.SetProperty(key_str, value);
          }
        } else {
          updated_keys->push_back(key);
          data_.SetProperty(key_str, value);
        }
      });
  // In unittest, context_ will be nullptr(lepus function named $updateComponent
  // is undefined also).
  const bool updated_not_empty = updated_keys->size() != 0;
  if (context_ && updated_not_empty) {
    lepus::Value p1(AirLepusRef::Create(
        element_manager()->air_node_manager()->Get(impl_id())));
    BASE_STATIC_STRING_DECL(kUpdateComponent, "$updateComponent");
    lepus::Value ret = context_->Call(kUpdateComponent, p1, data_, properties_,
                                      lepus::Value(path_),
                                      lepus::Value(std::move(updated_keys)));
    // In some cases, some elements may fail to execute the flush operation due
    // to exceptions in the execution of the lepus code. As a result, layout and
    // other UI operations are not safe.
    if (!(ret.IsBool() && ret.Bool())) {
      return false;
    }
  }
  return updated_not_empty;
}

uint32_t AirComponentElement::NonVirtualNodeCountInParent() {
  uint32_t sum = 0;
  for (auto child : air_children_) {
    sum += child->NonVirtualNodeCountInParent();
  }
  return sum;
}

void AirComponentElement::OnElementRemoved() {
  element_manager()->AirRoot()->FireComponentLifeCycleEvent(kComponentDetached,
                                                            impl_id());
}

void AirComponentElement::SetParsedStyles(
    const AirCompStylesMap &parsed_styles) {
  AirElement::SetParsedStyles(parsed_styles);
  element_manager()->AirRoot()->AddFontFaces(parsed_styles, path_.str());
}

}  // namespace tasm
}  // namespace lynx
