// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/ng/media_query/media_feature.h"

#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace css {

namespace {

MediaFeatureType TypeFromU32(uint32_t raw) {
  switch (static_cast<MediaFeatureType>(raw)) {
    case MediaFeatureType::kInvalid:
    case MediaFeatureType::kNumber:
    case MediaFeatureType::kDimension:
    case MediaFeatureType::kRatio:
    case MediaFeatureType::kIdent:
    case MediaFeatureType::kBoolean:
      return static_cast<MediaFeatureType>(raw);
  }
  return MediaFeatureType::kInvalid;
}

MediaFeatureUnit UnitFromU32(uint32_t raw) {
  switch (static_cast<MediaFeatureUnit>(raw)) {
    case MediaFeatureUnit::kUnknown:
    case MediaFeatureUnit::kPixels:
    case MediaFeatureUnit::kEm:
    case MediaFeatureUnit::kRem:
    case MediaFeatureUnit::kPercent:
    case MediaFeatureUnit::kViewportWidth:
    case MediaFeatureUnit::kViewportHeight:
    case MediaFeatureUnit::kViewportMin:
    case MediaFeatureUnit::kViewportMax:
    case MediaFeatureUnit::kDppx:
    case MediaFeatureUnit::kDpi:
    case MediaFeatureUnit::kDpcm:
      return static_cast<MediaFeatureUnit>(raw);
  }
  return MediaFeatureUnit::kUnknown;
}

MediaFeatureOperator OperatorFromU32(uint32_t raw) {
  switch (static_cast<MediaFeatureOperator>(raw)) {
    case MediaFeatureOperator::kNone:
    case MediaFeatureOperator::kEq:
    case MediaFeatureOperator::kLt:
    case MediaFeatureOperator::kLe:
    case MediaFeatureOperator::kGt:
    case MediaFeatureOperator::kGe:
      return static_cast<MediaFeatureOperator>(raw);
  }
  return MediaFeatureOperator::kNone;
}

MediaFeatureId FeatureIdFromU32(uint32_t raw) {
  switch (static_cast<MediaFeatureId>(raw)) {
    case MediaFeatureId::kUnknown:
    case MediaFeatureId::kWidth:
    case MediaFeatureId::kMinWidth:
    case MediaFeatureId::kMaxWidth:
    case MediaFeatureId::kHeight:
    case MediaFeatureId::kMinHeight:
    case MediaFeatureId::kMaxHeight:
    case MediaFeatureId::kDeviceWidth:
    case MediaFeatureId::kMinDeviceWidth:
    case MediaFeatureId::kMaxDeviceWidth:
    case MediaFeatureId::kDeviceHeight:
    case MediaFeatureId::kMinDeviceHeight:
    case MediaFeatureId::kMaxDeviceHeight:
    case MediaFeatureId::kOrientation:
    case MediaFeatureId::kResolution:
    case MediaFeatureId::kMinResolution:
    case MediaFeatureId::kMaxResolution:
    case MediaFeatureId::kAspectRatio:
    case MediaFeatureId::kMinAspectRatio:
    case MediaFeatureId::kMaxAspectRatio:
    case MediaFeatureId::kDeviceAspectRatio:
    case MediaFeatureId::kMinDeviceAspectRatio:
    case MediaFeatureId::kMaxDeviceAspectRatio:
    case MediaFeatureId::kHover:
    case MediaFeatureId::kPointer:
    case MediaFeatureId::kPrefersColorScheme:
    case MediaFeatureId::kColor:
    case MediaFeatureId::kMinColor:
    case MediaFeatureId::kMaxColor:
    case MediaFeatureId::kDevicePixelRatio:
    case MediaFeatureId::kMinDevicePixelRatio:
    case MediaFeatureId::kMaxDevicePixelRatio:
      return static_cast<MediaFeatureId>(raw);
  }
  return MediaFeatureId::kUnknown;
}

}  // namespace

// --- MediaFeatureId resolution ----------------------------------------------

MediaFeatureId ResolveMediaFeatureId(const std::string& name) {
  static const std::unordered_map<std::string, MediaFeatureId> kMap = {
      {"width", MediaFeatureId::kWidth},
      {"min-width", MediaFeatureId::kMinWidth},
      {"max-width", MediaFeatureId::kMaxWidth},
      {"height", MediaFeatureId::kHeight},
      {"min-height", MediaFeatureId::kMinHeight},
      {"max-height", MediaFeatureId::kMaxHeight},
      {"device-width", MediaFeatureId::kDeviceWidth},
      {"min-device-width", MediaFeatureId::kMinDeviceWidth},
      {"max-device-width", MediaFeatureId::kMaxDeviceWidth},
      {"device-height", MediaFeatureId::kDeviceHeight},
      {"min-device-height", MediaFeatureId::kMinDeviceHeight},
      {"max-device-height", MediaFeatureId::kMaxDeviceHeight},
      {"orientation", MediaFeatureId::kOrientation},
      {"resolution", MediaFeatureId::kResolution},
      {"min-resolution", MediaFeatureId::kMinResolution},
      {"max-resolution", MediaFeatureId::kMaxResolution},
      {"aspect-ratio", MediaFeatureId::kAspectRatio},
      {"min-aspect-ratio", MediaFeatureId::kMinAspectRatio},
      {"max-aspect-ratio", MediaFeatureId::kMaxAspectRatio},
      {"device-aspect-ratio", MediaFeatureId::kDeviceAspectRatio},
      {"min-device-aspect-ratio", MediaFeatureId::kMinDeviceAspectRatio},
      {"max-device-aspect-ratio", MediaFeatureId::kMaxDeviceAspectRatio},
      {"hover", MediaFeatureId::kHover},
      {"pointer", MediaFeatureId::kPointer},
      {"prefers-color-scheme", MediaFeatureId::kPrefersColorScheme},
      {"color", MediaFeatureId::kColor},
      {"min-color", MediaFeatureId::kMinColor},
      {"max-color", MediaFeatureId::kMaxColor},
      {"device-pixel-ratio", MediaFeatureId::kDevicePixelRatio},
      {"min-device-pixel-ratio", MediaFeatureId::kMinDevicePixelRatio},
      {"max-device-pixel-ratio", MediaFeatureId::kMaxDevicePixelRatio},
  };
  auto it = kMap.find(name);
  return it != kMap.end() ? it->second : MediaFeatureId::kUnknown;
}

// --- MediaFeatureValue serialization ----------------------------------------

lepus_value MediaFeatureValue::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back((static_cast<uint32_t>(type_) << 16) |
                    static_cast<uint32_t>(unit_));
  arr->emplace_back(numeric_);
  arr->emplace_back(denominator_);
  arr->emplace_back(text_);
  return lepus_value(std::move(arr));
}

// static
MediaFeatureValue MediaFeatureValue::FromLepus(const lepus_value& value) {
  MediaFeatureValue v;
  if (!value.IsArray()) return v;
  const auto& arr = value.Array();
  if (arr->size() < 4) return v;
  uint32_t packed = arr->get(0).UInt32();
  v.type_ = TypeFromU32(packed >> 16);
  v.unit_ = UnitFromU32(packed & 0xFFFF);
  v.numeric_ = arr->get(1).Number();
  v.denominator_ = arr->get(2).Number();
  v.text_ = arr->get(3).StdString();
  return v;
}

// --- MediaFeature serialization ---------------------------------------------

lepus_value MediaFeature::ToLepus() const {
  auto arr = lepus::CArray::Create();
  arr->emplace_back(static_cast<uint32_t>(id_));
  arr->emplace_back(name_);
  arr->emplace_back(static_cast<uint32_t>(left_op_));
  arr->emplace_back(left_value_.ToLepus());
  const bool has_right = HasRightBound();
  arr->emplace_back(has_right);
  if (has_right) {
    arr->emplace_back(static_cast<uint32_t>(right_op_));
    arr->emplace_back(right_value_.ToLepus());
  }
  return lepus_value(std::move(arr));
}

// static
MediaFeature MediaFeature::FromLepus(const lepus_value& value) {
  MediaFeature feature;
  if (!value.IsArray()) return feature;
  const auto& arr = value.Array();
  if (arr->size() < 5) return feature;
  feature.id_ = FeatureIdFromU32(arr->get(0).UInt32());
  feature.name_ = arr->get(1).StdString();
  feature.left_op_ = OperatorFromU32(arr->get(2).UInt32());
  feature.left_value_ = MediaFeatureValue::FromLepus(arr->get(3));
  const bool has_right = arr->get(4).Bool();
  if (has_right) {
    if (arr->size() < 7) {
      return MediaFeature();
    }
    feature.right_op_ = OperatorFromU32(arr->get(5).UInt32());
    feature.right_value_ = MediaFeatureValue::FromLepus(arr->get(6));
  }
  return feature;
}

}  // namespace css
}  // namespace lynx
