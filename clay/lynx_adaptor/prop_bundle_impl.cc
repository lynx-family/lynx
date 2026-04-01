// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/prop_bundle_impl.h"

#include <stdint.h>

#include <utility>

#include "clay/lynx_adaptor/value_converter.h"
#include "core/renderer/css/css_property_id.h"

namespace lynx {

namespace {

clay::Value ShallowCloneValue(const clay::Value& value) {
  if (value.IsNone()) {
    return clay::Value();
  }
  if (value.IsNull()) {
    return clay::Value::Null();
  }
  switch (value.type()) {
    case clay::Value::kBool:
      return clay::Value(value.GetBool());
    case clay::Value::kInt:
      return clay::Value(value.GetInt());
    case clay::Value::kUInt:
      return clay::Value(value.GetUint());
    case clay::Value::kLong:
      return clay::Value(value.GetLong());
    case clay::Value::kFloat:
      return clay::Value(value.GetFloat());
    case clay::Value::kDouble:
      return clay::Value(value.GetDouble());
    case clay::Value::kString:
      return clay::Value(value.GetString());
    case clay::Value::kPointer:
      return clay::Value(value.value<ClayPointer>());
    case clay::Value::kArray:
      return clay::Value(value.value<std::shared_ptr<clay::Value::Array>>());
    case clay::Value::kArrayBuffer:
      return clay::Value(
          value.value<std::shared_ptr<clay::Value::ArrayBuffer>>());
    case clay::Value::kMap:
      return clay::Value(value.value<std::shared_ptr<clay::Value::Map>>());
    default:
      return clay::Value();
  }
}

clay::Value::Map ShallowCloneMap(const clay::Value::Map& map) {
  clay::Value::Map result;
  result.reserve(map.size());
  for (const auto& [key, value] : map) {
    result.emplace(key, ShallowCloneValue(value));
  }
  return result;
}

}  // namespace

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

PropBundleImpl::SharedData::SharedData(
    const clay::Value::Map& props,
    const std::vector<std::string>& event_handlers)
    : map_(ShallowCloneMap(props)), event_handlers_(event_handlers) {}

PropBundleImpl::SharedData::SharedData(const SharedData& other)
    : map_(ShallowCloneMap(other.map_)),
      event_handlers_(other.event_handlers_),
      gesture_detector_map_(other.gesture_detector_map_) {}

PropBundleImpl::PropBundleImpl() : data_(std::make_shared<SharedData>()) {}

PropBundleImpl::PropBundleImpl(std::shared_ptr<SharedData> data)
    : data_(std::move(data)) {
  if (!data_) {
    data_ = std::make_shared<SharedData>();
  }
}

PropBundleImpl::~PropBundleImpl() = default;

PropBundleImpl::PropBundleImpl(const clay::Value::Map& props,
                               const std::vector<std::string>& event_handlers)
    : data_(std::make_shared<SharedData>(props, event_handlers)) {}

PropBundleImpl::SharedData& PropBundleImpl::UniqueData() {
  if (!data_) {
    data_ = std::make_shared<SharedData>();
    return *data_;
  }

  if (!data_.unique()) {
    data_ = std::make_shared<SharedData>(*data_);
  }
  return *data_;
}

void PropBundleImpl::SetNullProps(const char* key) {
  UniqueData().map_[key] = clay::Value::Null();
}

void PropBundleImpl::SetProps(const char* key, unsigned int value) {
  UniqueData().map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, int value) {
  UniqueData().map_[key] = clay::Value{value};
}

bool PropBundleImpl::Contains(const char* key) const {
  return data_->map_.find(key) != data_->map_.end();
}

void PropBundleImpl::SetProps(const char* key, const char* value) {
  UniqueData().map_[key] = clay::Value(value);
}
void PropBundleImpl::SetProps(const char* key, bool value) {
  UniqueData().map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, double value) {
  UniqueData().map_[key] = clay::Value{value};
}

void PropBundleImpl::SetProps(const char* key, const pub::Value& value) {
  UniqueData().map_[key] = ValueConverter::CreateClayValue(value);
}

void PropBundleImpl::SetProps(const pub::Value& value) {
  auto prev_value_vector =
      pub::ScopedCircleChecker::InitVectorIfNecessary(value);
  auto& map = UniqueData().map_;
  value.ForeachMap(
      [&map, &prev_value_vector](const pub::Value& k, const pub::Value& v) {
        map[k.str().c_str()] =
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
  UniqueData().map_[property_name] = std::move(array);
}

void PropBundleImpl::SetPropsByID(tasm::CSSPropertyID id, const uint32_t* data,
                                  size_t size) {
  auto property_name = GetPropertyName(id);
  auto array = ValueConverter::CreateClayValue(data, size);
  UniqueData().map_[property_name] = std::move(array);
}

void PropBundleImpl::SetEventHandler(const pub::Value& event) {
  auto name = event.GetValueAtIndex(0);
  UniqueData().event_handlers_.emplace_back(name->str().c_str());
}

void PropBundleImpl::ResetEventHandler() {
  UniqueData().event_handlers_.clear();
}

void PropBundleImpl::SetGestureDetector(const tasm::GestureDetector& detector) {
  auto& data = UniqueData();
  if (!data.gesture_detector_map_) {
    data.gesture_detector_map_ = clay::GestureMap();
  }
  data.gesture_detector_map_->emplace(
      detector.gesture_id(),
      std::make_shared<clay::GestureDetector>(
          detector.gesture_id(),
          static_cast<clay::GestureHandlerType>(detector.gesture_type()),
          detector.gesture_callback_names(), detector.relation_map(),
          ValueConverter::CreateClayValue(detector.gesture_config())));
}

fml::RefPtr<tasm::PropBundle> PropBundleCreatorClay::CreatePropBundle() {
  return fml::MakeRefCounted<PropBundleImpl>();
}

}  // namespace lynx
