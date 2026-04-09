// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/common/native_prop_bundle.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/events/gesture.h"

namespace lynx {
namespace tasm {

NativePropBundle::NativePropBundle() = default;

NativePropBundle::NativePropBundle(
    const PropMap& props,
    const base::flex_optional<std::vector<lepus::Value>>& event)
    : props_(props), event_handler_(event) {}

NativePropBundle::~NativePropBundle() = default;

void NativePropBundle::SetNullProps(const char* key) {
  auto result = props_.emplace(key, lepus::Value());
  if (!result.second) {
    result.first->second = lepus::Value();
  }
}

// XXX(renzhongyue): values can be stored in a better and simpler struct instead
// of lepus::Value. Now values are unwrapped in Element::PushToBundle and
// wrapped again here.
void NativePropBundle::SetProps(const char* key, unsigned int value) {
  auto result = props_.emplace(key, lepus::Value(value));
  if (!result.second) {
    result.first->second = lepus::Value(value);
  }
}

void NativePropBundle::SetProps(const char* key, int value) {
  auto result = props_.emplace(key, lepus::Value(value));
  if (!result.second) {
    result.first->second = lepus::Value(value);
  }
}

bool NativePropBundle::Contains(const char* key) const {
  return props_.find(key) != props_.end();
}

void NativePropBundle::SetProps(const char* key, const char* value) {
  auto result = props_.emplace(std::string(key), value);
  if (!result.second) {
    result.first->second = lepus::Value(value);
  }
}

void NativePropBundle::SetProps(const char* key, bool value) {
  auto result = props_.emplace(key, lepus::Value(value));
  if (!result.second) {
    result.first->second = lepus::Value(value);
  }
}

void NativePropBundle::SetProps(const char* key, double value) {
  auto result = props_.emplace(key, lepus::Value(value));
  if (!result.second) {
    result.first->second = lepus::Value(value);
  }
}

void NativePropBundle::SetProps(const char* key, const pub::Value& value) {
  props_[key] = pub::ValueUtils::ConvertValueToLepusValue(value);
}

void NativePropBundle::SetEventHandler(const pub::Value& event) {
  if (!event_handler_) {
    event_handler_ = std::vector<lepus::Value>();
  }
  event_handler_->emplace_back(
      pub::ValueUtils::ConvertValueToLepusValue(event));
}

void NativePropBundle::SetGestureDetector(const GestureDetector& detector) {
  if (!gesture_detector_map_) {
    gesture_detector_map_ = GestureMap();
  }
  gesture_detector_map_->emplace(
      detector.gesture_id(),
      std::make_shared<GestureDetectorImpl>(
          static_cast<const GestureDetectorImpl&>(detector)));
}

void NativePropBundle::ResetEventHandler() {}

void NativePropBundle::SetPropsByID(CSSPropertyID id, const uint8_t* data,
                                    size_t size) {
  auto array = lepus::Value(lepus::CArray::Create());
  for (size_t i = 0; i < size; ++i) {
    array.SetProperty(static_cast<uint32_t>(i), lepus::Value(data[i]));
  }
  auto key = CSSProperty::GetPropertyNameCStr(id);
  props_[key] = lepus::Value(std::move(array));
}

void NativePropBundle::SetPropsByID(CSSPropertyID id, const uint32_t* data,
                                    size_t size) {
  auto array = lepus::Value(lepus::CArray::Create());
  for (size_t i = 0; i < size; ++i) {
    array.SetProperty(static_cast<uint32_t>(i), lepus::Value(data[i]));
  }
  auto key = CSSProperty::GetPropertyNameCStr(id);
  props_[key] = lepus::Value(std::move(array));
}

fml::RefPtr<PropBundle> NativePropBundle::ShallowCopy() {
  auto prop = fml::MakeRefCounted<NativePropBundle>(props_, event_handler_);
  return prop;
}

void NativePropBundle::SetProps(const pub::Value& value) {
  auto prev_value_vector =
      pub::ScopedCircleChecker::InitVectorIfNecessary(value);
  value.ForeachMap(
      [this, &prev_value_vector](const pub::Value& k, const pub::Value& v) {
        props_[k.str().c_str()] = pub::ValueUtils::ConvertValueToLepusValue(
            v, prev_value_vector.get(), 0);
      });
}

}  // namespace tasm

}  // namespace lynx
