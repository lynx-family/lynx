// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/keyframes_data.h"

#include <tuple>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/component/keywords.h"

namespace clay {

namespace {

// Keep same as "core/style/transform_raw_data.h"
struct TransformRawData {
  static constexpr int INDEX_FUNC = 0;
  static constexpr int INDEX_TRANSLATE_0 = 1;
  static constexpr int INDEX_TRANSLATE_0_UNIT = 2;
  static constexpr int INDEX_TRANSLATE_1 = 3;
  static constexpr int INDEX_TRANSLATE_1_UNIT = 4;
  static constexpr int INDEX_TRANSLATE_2 = 5;
  static constexpr int INDEX_TRANSLATE_2_UNIT = 6;
};

std::vector<BoxShadowValue> ConvertToBoxShadowValue(const Value& value) {
  std::vector<BoxShadowValue> values;
  const auto& shadows = value.GetArray();
  for (auto& shadow : shadows) {
    auto& shadow_ary = shadow.GetArray();
    BoxShadowValue value;
    value.h_offset = shadow_ary[0].GetFloat();
    value.v_offset = shadow_ary[1].GetFloat();
    value.blur = shadow_ary[2].GetFloat();
    value.spread = shadow_ary[3].GetFloat();
    value.option = shadow_ary[4].GetDouble();
    value.color = shadow_ary[5].GetDouble();
    values.push_back(std::move(value));
  }
  return values;
}

std::vector<FilterValue> ConvertToFilterValue(const Value& value) {
  std::vector<FilterValue> values;
  std::vector<std::tuple<int, double>> filter;
  const auto& ary = value.GetArray();
  for (auto& item : ary) {
    const auto& ary1 = item.GetArray();
    if (ary1.size() != 2) {
      continue;
    }
    FilterValue value;
    value.type = ary1[0].GetUint();
    value.value = ary1[1].GetDouble();
    values.push_back(value);
  }
  return values;
}

std::unique_ptr<ClayTransform> ConvertToClayTransform(const Value& value) {
  auto transform = std::make_unique<ClayTransform>();
  transform->size = 0;
  transform->op = nullptr;
  if (!value.IsArray()) {
    return transform;
  }

  const auto& items = value.GetArray();
  transform->size = static_cast<int>(items.size());
  transform->op = new ClayTransformOP[items.size()];
  for (size_t i = 0; i < items.size(); ++i) {
    const auto& item = items[i];
    FML_DCHECK(item.IsArray() && item.GetArray().size() == 7u);
    const auto& arr = item.GetArray();
    transform->op[i].type = static_cast<ClayTransformType>(
        arr[TransformRawData::INDEX_FUNC].GetInt());
    transform->op[i].value[0] =
        arr[TransformRawData::INDEX_TRANSLATE_0].GetDouble();
    transform->op[i].value[1] =
        arr[TransformRawData::INDEX_TRANSLATE_1].GetDouble();
    transform->op[i].value[2] =
        arr[TransformRawData::INDEX_TRANSLATE_2].GetDouble();
    transform->op[i].unit[0] = static_cast<ClayPlatformLengthUnit>(
        arr[TransformRawData::INDEX_TRANSLATE_0_UNIT].GetInt());
    transform->op[i].unit[1] = static_cast<ClayPlatformLengthUnit>(
        arr[TransformRawData::INDEX_TRANSLATE_1_UNIT].GetInt());
    transform->op[i].unit[2] = static_cast<ClayPlatformLengthUnit>(
        arr[TransformRawData::INDEX_TRANSLATE_2_UNIT].GetInt());
  }
  return transform;
}

}  // namespace

KeyframesData::KeyframesData(const Value& prop_keyframes_value) {
  if (!prop_keyframes_value.IsMap()) {
    this->size = 0;
    this->keyframe_rules = nullptr;
  } else {
    const auto& rules_map = prop_keyframes_value.GetMap();
    rules_ = std::make_unique<ClayKeyframesRule[]>(rules_map.size());
    this->size = static_cast<int>(rules_map.size());
    this->keyframe_rules = rules_.get();
    int i = -1;
    for (auto& rule : rules_map) {
      rules_[++i].name = rule.first.c_str();
      if (!rule.second.IsMap()) {
        rules_[i].size = 0;
        rules_[i].keyframes = nullptr;
      } else {
        const auto& keyframes_map = rule.second.GetMap();
        keyframes_.push_back(
            std::make_unique<ClayKeyframe[]>(keyframes_map.size()));
        rules_[i].size = static_cast<int>(keyframes_map.size());
        rules_[i].keyframes = keyframes_.back().get();
        int j = -1;
        for (auto& keyframe : keyframes_map) {
          keyframes_.back()[++j].percentage = std::stof(keyframe.first);
          if (!keyframe.second.IsMap()) {
            keyframes_.back()[j].size = 0;
            keyframes_.back()[j].properties = nullptr;
          } else {
            const auto& properties_map = keyframe.second.GetMap();
            properties_.push_back(
                std::make_unique<ClayAnimationPropertyValue[]>(
                    properties_map.size()));
            keyframes_.back()[j].size = static_cast<int>(properties_map.size());
            keyframes_.back()[j].properties = properties_.back().get();
            int k = -1;
            for (auto& prop : properties_map) {
              ++k;
              Value prop_value;
              auto kw = GetKeywordID(prop.first);
              if (kw == KeywordID::kOpacity) {
                properties_.back()[k].type =
                    ClayAnimationPropertyType::kOpacity;
                prop_value = Value(static_cast<float>(prop.second.GetDouble()));
              } else if (kw == KeywordID::kBackgroundColor) {
                properties_.back()[k].type =
                    ClayAnimationPropertyType::kBackgroundColor;
                prop_value = Value(prop.second.GetUint());
              } else if (kw == KeywordID::kTransform) {
                properties_.back()[k].type =
                    ClayAnimationPropertyType::kTransform;
                transforms_.push_back(ConvertToClayTransform(prop.second));
                ClayPointer ptr{ClayPointer::kClayPointerTypeTransform,
                                transforms_.back().get()};
                prop_value = Value{ptr};
              } else if (kw == KeywordID::kColor) {
                properties_.back()[k].type = ClayAnimationPropertyType::kColor;
                prop_value = Value(prop.second.GetUint());
              } else if (kw == KeywordID::kFilter) {
                properties_.back()[k].type = ClayAnimationPropertyType::kFilter;
                filters_.push_back(std::make_unique<std::vector<FilterValue>>(
                    ConvertToFilterValue(prop.second)));
                ClayPointer ptr{ClayPointer::kClayPointerTypeFilter,
                                filters_.back().get()};
                prop_value = Value{ptr};
              } else if (kw == KeywordID::kBoxShadow) {
                properties_.back()[k].type =
                    ClayAnimationPropertyType::kBoxShadow;
                shadows_.push_back(
                    std::make_unique<std::vector<BoxShadowValue>>(
                        ConvertToBoxShadowValue(prop.second)));
                ClayPointer ptr{ClayPointer::kClayPointerTypeBoxShadow,
                                shadows_.back().get()};
                prop_value = Value{ptr};
              } else {
                FML_DLOG(ERROR)
                    << "SetKeyframes doesn't support property: " << prop.first;
                properties_.back()[k].type = ClayAnimationPropertyType::kNone;
                prop_value = Value::Null();
              }
              properties_.back()[k].value = std::move(prop_value);
            }
          }
        }
      }
    }
  }
}

KeyframesData::~KeyframesData() {
  for (auto& item : transforms_) {
    if (item && item->op) {
      delete[] item->op;
    }
  }
}

}  // namespace clay
