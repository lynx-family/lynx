// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/css_property.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/style/length.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/common/value_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"

namespace clay {
namespace utils = attribute_utils;

static std::vector<Length> GetBorderRadius(const clay::Value::Array& array) {
  std::vector<Length> values;
  if (array.size() % 2 != 0) {
    FML_DCHECK(false);
    return values;
  }

  for (size_t i = 0; i < array.size() / 2; i++) {
    values.emplace_back(ParseLengthValue(array[i * 2], array[i * 2 + 1]));
  }
  return values;
}

static void AppendMaskPositions(std::vector<MaskPosition>& positions,
                                const clay::Value::Array& array) {
  FML_DCHECK(array.size() % 2 == 0);
  if (array.size() % 2 != 0) {
    return;
  }
  for (size_t i = 0; i < array.size(); i += 2) {
    positions.emplace_back(
        utils::GetDouble(array[i]),
        static_cast<ClayPlatformLengthUnit>(utils::GetInt(array[i + 1])));
  }
}

static void SetMaskPosition(BaseView* view, const clay::Value::Array& array) {
  std::vector<MaskPosition> positions;
  if (!array.empty() && array.front().IsArray()) {
    for (const auto& layer : array) {
      AppendMaskPositions(positions, utils::GetArray(layer));
    }
  } else {
    AppendMaskPositions(positions, array);
  }
  view->SetMaskPosition(positions);
}

static void AppendMaskSizes(std::vector<MaskSize>& sizes,
                            const clay::Value::Array& array) {
  FML_DCHECK(array.size() % 2 == 0);
  if (array.size() % 2 != 0) {
    return;
  }
  for (size_t i = 0; i < array.size(); i += 2) {
    sizes.emplace_back(
        utils::GetDouble(array[i]),
        static_cast<ClayPlatformLengthUnit>(utils::GetInt(array[i + 1])));
  }
}

static void SetMaskSize(BaseView* view, const clay::Value::Array& array) {
  std::vector<MaskSize> sizes;
  if (!array.empty() && array.front().IsArray()) {
    for (const auto& layer : array) {
      AppendMaskSizes(sizes, utils::GetArray(layer));
    }
  } else {
    AppendMaskSizes(sizes, array);
  }
  view->SetMaskSize(sizes);
}

static void SetMaskRepeat(BaseView* view, const clay::Value::Array& array) {
  std::vector<ClayMaskRepeatType> repeats;
  if (!array.empty() && array.front().IsArray()) {
    for (const auto& layer : array) {
      for (const auto& item : utils::GetArray(layer)) {
        repeats.emplace_back(
            static_cast<ClayMaskRepeatType>(utils::GetInt(item)));
      }
    }
  } else {
    for (const auto& item : array) {
      repeats.emplace_back(
          static_cast<ClayMaskRepeatType>(utils::GetInt(item)));
    }
  }
  view->SetMaskRepeat(std::move(repeats));
}

static void SetMaskOrigin(BaseView* view, const clay::Value::Array& array) {
  std::vector<ClayMaskOriginType> origins;
  for (const auto& item : array) {
    origins.emplace_back(static_cast<ClayMaskOriginType>(utils::GetInt(item)));
  }
  view->SetMaskOrigin(std::move(origins));
}

static void SetMaskClip(BaseView* view, const clay::Value::Array& array) {
  std::vector<ClayMaskClipType> clips;
  for (const auto& item : array) {
    clips.emplace_back(static_cast<ClayMaskClipType>(utils::GetInt(item)));
  }
  view->SetMaskClip(std::move(clips));
}

static bool IsMaskImageNone(const clay::Value::Array& array) {
  return array.size() == 1 &&
         static_cast<ClayMaskImageType>(utils::GetUint(array[0])) ==
             ClayMaskImageType::kNone;
}

bool CSSProperty::SetAttribute(BaseView* view, KeywordID property_id,
                               const clay::Value& value) {
  switch (property_id) {
    case KeywordID::kOpacity: {
      view->SetOpacity(utils::GetDouble(value, 1.0f));
    } break;
    case KeywordID::kBackgroundColor: {
      view->SetBackgroundColor(Color(utils::GetUint(value, 0)));
    } break;
    case KeywordID::kBackgroundImage: {
      view->SetBackgroundImage(utils::GetArray(value));
    } break;
    case KeywordID::kBackgroundClip: {
      view->SetBackgroundClip(utils::GetArray(value));
    } break;
    case KeywordID::kBackgroundOrigin: {
      view->SetBackgroundOrigin(utils::GetArray(value));
    } break;

    case KeywordID::kBackgroundPosition: {
      const auto& array = utils::GetArray(value);
      FML_DCHECK(array.size() % 2 == 0);
      if (array.size() % 2 == 0) {
        std::vector<BackgroundPosition> positions;
        for (size_t i = 0; i < array.size(); i += 2) {
          // For keyword values, Lynx is responsible for converting them to
          // {value, kPercent} format. For example, 'top' would be converted to
          // {0.f, kPercent}
          double value = utils::GetDouble(array[i]);
          ClayPlatformLengthUnit type =
              static_cast<ClayPlatformLengthUnit>(utils::GetInt(array[i + 1]));
          positions.emplace_back(value, type);
        }
        view->SetBackgroundPosition(std::move(positions));
      }
    } break;

    case KeywordID::kBackgroundRepeat: {
      view->SetBackgroundRepeat(utils::GetArray(value));
    } break;

    case KeywordID::kBackgroundSize: {
      const auto& array = utils::GetArray(value);
      FML_DCHECK(array.size() % 2 == 0);
      if (array.size() % 2 == 0) {
        std::vector<BackgroundSize> sizes;
        for (size_t i = 0; i < array.size(); i += 2) {
          double value = utils::GetDouble(array[i]);
          ClayPlatformLengthUnit type =
              static_cast<ClayPlatformLengthUnit>(utils::GetInt(array[i + 1]));
          sizes.emplace_back(value, type);
        }
        view->SetBackgroundSize(std::move(sizes));
      }
    } break;
    case KeywordID::kMask: {
      const auto& array = utils::GetArray(value);
      if (array.size() >= 7) {
        const auto& mask_images = utils::GetArray(array[1]);
        if (IsMaskImageNone(mask_images)) {
          view->ClearMask();
          break;
        }
        view->SetMaskImage(mask_images);
        SetMaskPosition(view, utils::GetArray(array[2]));
        SetMaskSize(view, utils::GetArray(array[3]));
        SetMaskRepeat(view, utils::GetArray(array[4]));
        SetMaskOrigin(view, utils::GetArray(array[5]));
        SetMaskClip(view, utils::GetArray(array[6]));
        if (array.size() >= 8) {
          view->SetMaskComposite(utils::GetArray(array[7]));
        }
      }
    } break;
    case KeywordID::kMaskImage: {
      view->SetMaskImage(utils::GetArray(value));
    } break;
    case KeywordID::kMaskPosition: {
      SetMaskPosition(view, utils::GetArray(value));
    } break;
    case KeywordID::kMaskOrigin: {
      view->SetMaskOrigin(utils::GetArray(value));
    } break;
    case KeywordID::kMaskRepeat: {
      view->SetMaskRepeat(utils::GetArray(value));
    } break;
    case KeywordID::kMaskSize: {
      SetMaskSize(view, utils::GetArray(value));
    } break;
    case KeywordID::kMaskClip: {
      view->SetMaskClip(utils::GetArray(value));
    } break;
    case KeywordID::kMaskComposite: {
      view->SetMaskComposite(utils::GetArray(value));
    } break;
    case KeywordID::kBorderColor: {
      view->SetBorderColor({Side::kAll}, {utils::GetUint(value)});
    } break;
    case KeywordID::kBorderTopColor: {
      view->SetBorderColor({Side::kTop}, {utils::GetUint(value)});
    } break;
    case KeywordID::kBorderRightColor: {
      view->SetBorderColor({Side::kRight}, {utils::GetUint(value)});
    } break;
    case KeywordID::kBorderBottomColor: {
      view->SetBorderColor({Side::kBottom}, {utils::GetUint(value)});
    } break;
    case KeywordID::kBorderLeftColor: {
      view->SetBorderColor({Side::kLeft}, {utils::GetUint(value)});
    } break;
    case KeywordID::kBorderStyle: {
      view->SetBorderStyle(
          {Side::kAll}, {static_cast<BorderStyleType>(utils::GetInt(value))});
    } break;
    case KeywordID::kBorderTopStyle: {
      view->SetBorderStyle(
          {Side::kTop}, {static_cast<BorderStyleType>(utils::GetInt(value))});
    } break;
    case KeywordID::kBorderRightStyle: {
      view->SetBorderStyle(
          {Side::kRight}, {static_cast<BorderStyleType>(utils::GetInt(value))});
    } break;
    case KeywordID::kBorderBottomStyle: {
      view->SetBorderStyle(
          {Side::kBottom},
          {static_cast<BorderStyleType>(utils::GetInt(value))});
    } break;
    case KeywordID::kBorderLeftStyle: {
      view->SetBorderStyle(
          {Side::kLeft}, {static_cast<BorderStyleType>(utils::GetInt(value))});
    } break;
    case KeywordID::kBorderWidth: {
      float width = utils::GetDouble(value);
      view->SetBorderWidth({Side::kAll}, {width});
    } break;
    case KeywordID::kBorderTopWidth: {
      float width = utils::GetDouble(value);
      view->SetBorderWidth({Side::kTop}, {width});
    } break;
    case KeywordID::kBorderRightWidth: {
      float width = utils::GetDouble(value);
      view->SetBorderWidth({Side::kRight}, {width});
    } break;
    case KeywordID::kBorderBottomWidth: {
      float width = utils::GetDouble(value);
      view->SetBorderWidth({Side::kBottom}, {width});
    } break;
    case KeywordID::kBorderLeftWidth: {
      float width = utils::GetDouble(value);
      view->SetBorderWidth({Side::kLeft}, {width});
    } break;
    case KeywordID::kBorderRadius: {
      view->SetBorderRadius(4, GetBorderRadius(utils::GetArray(value)));
    } break;
    case KeywordID::kBorderTopLeftRadius: {
      view->SetBorderRadius(0, GetBorderRadius(utils::GetArray(value)));
    } break;
    case KeywordID::kBorderTopRightRadius: {
      view->SetBorderRadius(1, GetBorderRadius(utils::GetArray(value)));
    } break;
    case KeywordID::kBorderBottomRightRadius: {
      view->SetBorderRadius(2, GetBorderRadius(utils::GetArray(value)));
    } break;
    case KeywordID::kBorderBottomLeftRadius: {
      view->SetBorderRadius(3, GetBorderRadius(utils::GetArray(value)));
    } break;

    case KeywordID::kTransform: {
      auto transform_raws = ParseTransformRawValues(value);
      view->SetTransform(std::move(transform_raws));
    } break;

    case KeywordID::kTransformOrigin: {
      const auto& array = utils::GetArray(value);
      FML_DCHECK(array.size() == 0 || array.size() == 2 || array.size() == 4);
      if (array.size() == 0 || array.size() == 2 || array.size() == 4) {
        std::vector<Length> transform_origin_values;
        if (array.size() >= 2) {
          transform_origin_values.emplace_back(
              ParseLengthValue(array[0], array[1]));
          if (array.size() == 4) {
            transform_origin_values.emplace_back(
                ParseLengthValue(array[2], array[3]));
          }
        }
        view->SetTransformOrigin(std::move(transform_origin_values));
      }
    } break;

    case KeywordID::kPerspective: {
      const auto& array = utils::GetArray(value);
      if (array.size() >= 2) {
        view->SetPerspective(array);
      }
    } break;

    case KeywordID::kBoxShadow: {
      const auto& array = utils::GetArray(value);
      std::vector<Shadow> shadows(array.size());
      for (size_t i = 0; i < array.size(); i++) {
        const auto& arr = utils::GetArray(array[i]);
        shadows[i].offset_x = utils::GetDouble(arr[0]);
        shadows[i].offset_y = utils::GetDouble(arr[1]);
        shadows[i].blur_radius = utils::GetDouble(arr[2]);
        shadows[i].spread_radius = utils::GetDouble(arr[3]);
        auto option = static_cast<ClayShadowOption>(utils::GetInt(arr[4]));
        shadows[i].inset = (option == ClayShadowOption::kInset);
        shadows[i].color = Color(utils::GetUint(arr[5]));
      }
      view->SetShadows(std::move(shadows));
    } break;

    case KeywordID::kOutlineColor: {
      view->SetOutlineColor(utils::GetUint(value, 0xff000000));
    } break;
    case KeywordID::kOutlineStyle: {
      view->SetOutlineStyle(static_cast<BorderStyleType>(utils::GetInt(value)));
    } break;
    case KeywordID::kOutlineWidth: {
      view->SetOutlineWidth(utils::GetInt(value));
    } break;
    case KeywordID::kAnimation: {
      view->SetAnimation(utils::GetArray(value));
    } break;
    case KeywordID::kTransition: {
      view->SetTransition(utils::GetArray(value));
    } break;
    case KeywordID::kOverflow: {
      view->SetOverflowWithMask(
          OVERFLOW_XY,
          utils::GetInt(value, static_cast<int>(ClayOverflowType::kVisible)));
    } break;
    case KeywordID::kOverflowX: {
      view->SetOverflowWithMask(
          OVERFLOW_X,
          utils::GetInt(value, static_cast<int>(ClayOverflowType::kVisible)));
    } break;
    case KeywordID::kOverflowY: {
      view->SetOverflowWithMask(
          OVERFLOW_Y,
          utils::GetInt(value, static_cast<int>(ClayOverflowType::kVisible)));
    } break;
    case KeywordID::kVisibility: {
      view->SetVisible(
          utils::GetInt(value,
                        static_cast<int>(ClayVisibilityType::kVisible)) !=
          static_cast<int>(ClayVisibilityType::kHidden));
    } break;
    case KeywordID::kCursor: {
      view->SetCursor(utils::GetArray(value));
    } break;
    case KeywordID::kFilter: {
      if (value.IsArray()) {
        view->SetFilter(utils::GetArray(value));
      } else {
        // value.type maybe clay::Value::kNone and in this case we clear the
        // filter status.
        view->ClearFilter();
      }
    } break;
    case KeywordID::kImageRendering: {
      view->SetImageRendering(
          static_cast<ClayImageRendering>(utils::GetInt(value)));
      break;
    }
    case KeywordID::kClipPath: {
      if (value.IsArray()) {
        view->SetClipOffsetPath(utils::GetArray(value), true);
      } else {
        view->ClearClipPath();
      }
      break;
    }
    case KeywordID::kOffsetPath: {
      if (value.IsArray()) {
        view->SetClipOffsetPath(utils::GetArray(value), false);
      } else {
        view->ClearOffsetPath();
      }
      break;
    }
    case KeywordID::kOffsetRotate: {
      view->SetOffsetRotate(utils::GetDouble(value));
      break;
    }
    case KeywordID::kOffsetDistance: {
      view->SetOffsetDistance(utils::GetDouble(value));
      break;
    }
#ifdef ENABLE_ACCESSIBILITY
    case KeywordID::kAccessibilityElement: {
      view->SetAccessibilityElement(utils::GetBool(value, false));
      break;
    }
    case KeywordID::kAccessibilityLabel: {
      if (value.IsString()) {
        view->SetAccessibilityLabel(utils::GetCString(value));
      }
      break;
    }
    case KeywordID::kAccessibilityElements: {
      if (value.IsString()) {
        view->SetAccessibilityElements(utils::GetCString(value));
      }
      break;
    }
#endif
    case KeywordID::kDirection: {
      view->SetDirection(utils::GetInt(value));
      break;
    }
    default: {
      return false;
    }
  }
  return true;
}

}  // namespace clay
