// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/prop_bundle_impl.h"

#include <array>
#include <utility>

#include "clay/lynx_adaptor/value_converter.h"
#include "core/renderer/css/css_property_id.h"

namespace lynx {

// Get rid of Lynx symbol exporting
static const char* GetPropertyName(tasm::CSSPropertyID id) {
  static const char* kLynxCSSPropertyNames[] = {
      "",  // start
#define DECLARE_PROPERTY_ID(name, c, value) tasm::kPropertyName##name,
      FOREACH_ALL_PROPERTY(DECLARE_PROPERTY_ID)
#undef DECLARE_PROPERTY_ID
  };
  if (id > tasm::kPropertyStart && id < tasm::kPropertyEnd) {
    return kLynxCSSPropertyNames[id];
  }
  return kLynxCSSPropertyNames[0];  // Empty string
}

PropBundleImpl::~PropBundleImpl() {}

void PropBundleImpl::SetNullProps(const char* key) {
  map_[key] = clay::Value::Null();
}

void PropBundleImpl::SetProps(const char* key, unsigned int value) {
  map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, int value) {
  map_[key] = clay::Value{value};
}

bool PropBundleImpl::Contains(const char* key) const {
  return map_.find(key) != map_.end();
}

void PropBundleImpl::SetProps(const char* key, const char* value) {
  map_[key] = clay::Value(value);
}
void PropBundleImpl::SetProps(const char* key, bool value) {
  map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, double value) {
  map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, const pub::Value& value) {
  map_[key] = ValueConverter::CreateClayValue(value);
}

void PropBundleImpl::SetProps(const pub::Value& value) {
  auto prev_value_vector =
      pub::ScopedCircleChecker::InitVectorIfNecessary(value);
  value.ForeachMap(
      [this, &prev_value_vector](const pub::Value& k, const pub::Value& v) {
        map_[k.str().c_str()] =
            ValueConverter::CreateClayValue(v, prev_value_vector.get(), 1);
      });
}

void PropBundleImpl::SetNullPropsByID(tasm::CSSPropertyID id) {
  auto property_name = GetPropertyName(id);
  SetNullProps(property_name);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, unsigned int value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, int value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, const char* value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, bool value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, double value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id,
                                  const pub::Value& value) {
  auto property_name = GetPropertyName(id);
  SetProps(property_name, value);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, const uint8_t* data,
                                  size_t size) {
  auto property_name = GetPropertyName(id);
  auto array = ValueConverter::CreateClayValue(data, size);
  map_[property_name] = std::move(array);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, const uint32_t* data,
                                  size_t size) {
  auto property_name = GetPropertyName(id);
  auto array = ValueConverter::CreateClayValue(data, size);
  map_[property_name] = std::move(array);
}

void PropBundleImpl::SetEventHandler(const pub::Value& event) {
  auto name = event.GetValueAtIndex(0);
  event_handlers_.emplace_back(name->str().c_str());
}

void PropBundleImpl::ResetEventHandler() { event_handlers_.clear(); }

void PropBundleImpl::SetGestureDetector(const tasm::GestureDetector& detector) {
  // TODO(luochangan.adrian): need to implement set gesture detector in Clay.
}

fml::RefPtr<tasm::PropBundle> PropBundleCreatorClay::CreatePropBundle() {
  return fml::MakeRefCounted<PropBundleImpl>();
}

}  // namespace lynx
