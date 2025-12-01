// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/android/text_layout_android.h"

#include <utility>
#include <vector>

#include "core/base/android/android_jni.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_compact_array_buffer.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "platform/android/lynx_android/src/main/jni/gen/TextLayout_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/TextLayout_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForTextLayout(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace tasm {
TextLayoutAndroid::TextLayoutAndroid(JNIEnv* env, jobject text_layout)
    : text_layout_(env, text_layout) {}

TextLayoutAndroid::~TextLayoutAndroid() = default;

LayoutResult TextLayoutAndroid::Measure(Element* element, float width,
                                        int width_mode, float height,
                                        int height_mode) {
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull()) {
    return LayoutResult{0.f, 0.f, 0.f};
  }

  std::vector<float> layout_result;
  TextElement* text_element = static_cast<TextElement*>(element);
  if (text_element->need_layout_children()) {
    starlight::Constraints constraints;
    constraints[starlight::kHorizontal] = starlight::OneSideConstraint(
        width, static_cast<SLMeasureMode>(width_mode));
    constraints[starlight::kVertical] = starlight::OneSideConstraint(
        height, static_cast<SLMeasureMode>(height_mode));
    MeasureInlineViewRecursively(element, constraints, layout_result);
  }

  jsize result_size = static_cast<jsize>(layout_result.size());
  jfloatArray layout_result_array = env->NewFloatArray(result_size);
  if (layout_result_array == nullptr) {
    return LayoutResult{0.f, 0.f, 0.f};
  }

  env->SetFloatArrayRegion(layout_result_array, 0, layout_result.size(),
                           layout_result.data());

  const lynx::base::android::ScopedLocalJavaRef<jfloatArray> result_array =
      Java_TextLayout_measureText(env, local_ref.Get(), element->impl_id(),
                                  width, width_mode, height, height_mode,
                                  layout_result_array);
  env->DeleteLocalRef(layout_result_array);
  if (!result_array.Get()) {
    return LayoutResult{0.f, 0.f, 0.f};
  }
  jfloat* result = env->GetFloatArrayElements(result_array.Get(), JNI_FALSE);
  float measured_width = result[0];
  float measured_height = result[1];
  float base_line = result[2];

  env->ReleaseFloatArrayElements(result_array.Get(), result, JNI_ABORT);
  return LayoutResult{measured_width, measured_height, base_line};
}

void TextLayoutAndroid::MeasureInlineViewRecursively(
    Element* element, const starlight::Constraints& constraints,
    std::vector<float>& layout_result) {
  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    if (child->is_text()) {
      MeasureInlineViewRecursively(child, constraints, layout_result);
    } else if (child->is_view()) {
      FiberElement* fiber_element = static_cast<FiberElement*>(child);
      FloatSize size =
          fiber_element->slnode()->UpdateMeasureByPlatform(constraints, true);
      layout_result.push_back(fiber_element->impl_id());
      layout_result.push_back(size.width_);
      layout_result.push_back(size.height_);
      layout_result.push_back(size.baseline_);
    }
  }
}

void TextLayoutAndroid::Align(Element* element) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull() || !element->is_text()) {
    return;
  }
  TextElement* text_element = static_cast<TextElement*>(element);
  if (!text_element->need_layout_children()) {
    return;
  }
  std::unordered_map<int, std::pair<float, float>> result_map;

  base::android::ScopedLocalJavaRef<jfloatArray> result =
      Java_TextLayout_align(env, local_ref.Get(), element->impl_id());
  if (!result.Get()) {
    return;
  }
  jsize size = env->GetArrayLength(result.Get());
  if (size <= 0) {
    return;
  }
  jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
  for (auto i = 0; i < size / 3; i++) {
    result_map[arr[i * 3]] = std::make_pair(arr[i * 3 + 1], arr[i * 3 + 2]);
  }
  AlignChildrenRecursively(element, result_map);

  env->ReleaseFloatArrayElements(result.Get(), arr, 0);
}

void TextLayoutAndroid::AlignChildrenRecursively(
    Element* element,
    const std::unordered_map<int, std::pair<float, float>>& result_map) {
  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    if (child->is_text()) {
      AlignChildrenRecursively(child, result_map);
    } else if (child->is_view()) {
      FiberElement* fiber_element = static_cast<FiberElement*>(child);
      auto it = result_map.find(fiber_element->impl_id());
      if (it != result_map.end()) {
        fiber_element->slnode()->AlignmentByPlatform(it->second.first,
                                                     it->second.second);
      }
    }
  }
}

void TextLayoutAndroid::DispatchLayoutBefore(Element* element) {
  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull() || !element->is_text()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto props = std::make_unique<PropArrayAndroid>();
  TextElement* text_element = static_cast<TextElement*>(element);
  std::string output_str;
  size_t current_length = 0;
  bool use_utf16 =
      text_element->is_inline_element() || text_element->has_inline_child();
  bool has_inline_view = false;
  BuildTextPropsBuffer(text_element, output_str, current_length, use_utf16,
                       props.get(), &has_inline_view);
  text_element->set_need_layout_children(has_inline_view);

  props->AddProp(kPropTextString);
  props->AddProp(output_str.c_str());

  std::optional<base::android::CompactArrayBuffer> text_ab = std::nullopt;

  if (props.get()->GetArrayBuffer()) {
    text_ab = props.get()->GetArrayBuffer();
  }
  auto j_text_props = text_ab ? base::android::JReadableCompactArrayBuffer::
                                    CreateReadableCompactArrayBuffer(*text_ab)
                              : std::nullopt;

  if (!j_text_props.has_value()) {
    return;
  }
  Java_TextLayout_dispatchLayoutBefore(env, local_ref.Get(), element->impl_id(),
                                       j_text_props->Get());
}

void TextLayoutAndroid::BuildTextPropsBuffer(
    TextElement* element, std::string& output, size_t& current_length,
    bool use_utf16, PropArrayAndroid* props, bool* has_inline_view) {
  size_t start = current_length;
  base::String& content = element->content();
  if (!content.empty()) {
    output += content.str();
    current_length +=
        use_utf16 ? element->content_utf16_length() : content.length();
  }

  auto* child = element->first_render_child();
  while (child) {
    if (static_cast<FiberElement*>(child)->is_raw_text()) {
      auto* raw_text_child = static_cast<RawTextElement*>(child);
      const auto& raw_content = raw_text_child->content();
      if (!raw_content.empty()) {
        output += raw_content.str();
        current_length += use_utf16 ? raw_text_child->content_utf16_length()
                                    : raw_content.length();
      }
    } else if (child->is_text()) {
      // inline text
      BuildTextPropsBuffer(static_cast<TextElement*>(child), output,
                           current_length, use_utf16, props, has_inline_view);
    } else if (child->is_image()) {
      // inline image
      output += kInlinePlaceHolder;
      current_length += 1;  // placeholder's length is 1
      AppendImageProps(static_cast<ImageElement*>(child), current_length - 1,
                       current_length, props);
    } else if (child->is_view()) {
      // inline view
      output += kInlinePlaceHolder;
      current_length += 1;  // placeholder's length is 1
      AppendViewProps(static_cast<ViewElement*>(child), current_length - 1,
                      current_length, props);
      *has_inline_view = true;
    }
    child = child->next_render_sibling();
  }

  auto end = current_length;
  if (end > start) {
    AppendTextProps(element, start, end, props);
  }
}

void TextLayoutAndroid::AppendTextProps(TextElement* element, size_t pos_start,
                                        size_t pos_end,
                                        PropArrayAndroid* props) {
  TextProps* text_props = element->text_props();
  CSSIDBitset& property_bits = element->property_bits();
  if (!text_props && !property_bits.HasAny()) {
    return;
  }
  // only inline text need the pass the range，   kPropRangeStart should be
  // the first key
  if (element->is_inline_element()) {
    props->AddProp(kPropInlineStart);
    props->AddProp(static_cast<int>(pos_start));
  }

  // styles
  auto& text_attributes = element->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value()) {
    for (CSSPropertyID id : property_bits) {
      switch (id) {
        case kPropertyIDFontSize:
          props->AddProp(kTextPropFontSize);
          props->AddProp(
              static_cast<float>(element->computed_css_style()->GetFontSize()));
          break;

        case kPropertyIDColor: {
          if (text_attributes->text_gradient.has_value()) {
            auto& gradient_value = text_attributes->text_gradient.value();
            if (gradient_value.IsArray()) {
              const auto& gradient_array = *gradient_value.Array();
              if (gradient_array.size() >= 2) {
                lepus::Value type_value = gradient_array.get(0);
                lepus::Value params_array = gradient_array.get(1);
                const auto& params_vector = *params_array.Array();
                if (type_value.IsNumber() && params_array.IsArray()) {
                  auto type = static_cast<starlight::BackgroundImageType>(
                      type_value.Number());
                  if (type == starlight::BackgroundImageType::kLinearGradient) {
                    // linearGradientData:[angle_float, colors_int_array,
                    // positions_float_array,side_or_corner_int]
                    if (params_vector.size() >= 4) {
                      // key
                      props->AddProp(
                          static_cast<int>(kPropColorLinearGradient));
                      // angle
                      props->AddProp(
                          static_cast<float>(params_vector.get(0).Number()));

                      const auto& color_array = *params_vector.get(1).Array();
                      // color_count
                      props->AddProp(static_cast<int>(color_array.size()));
                      // colors
                      for (size_t i = 0; i < color_array.size(); ++i) {
                        props->AddProp(
                            static_cast<int>(color_array.get(i).UInt32()));
                      }
                      const auto& position_array =
                          *params_vector.get(2).Array();
                      // position_count
                      props->AddProp(static_cast<int>(position_array.size()));
                      // positions
                      for (size_t i = 0; i < position_array.size(); ++i) {
                        props->AddProp(
                            static_cast<float>(position_array.get(i).Number()));
                      }
                      // side_or_corner
                      props->AddProp(
                          static_cast<int>(params_vector.get(3).Number()));
                    }
                  } else if (type ==
                             starlight::BackgroundImageType::kRadialGradient) {
                    text_attributes->ProcessRadialGradientIfNeeded(
                        element->computed_css_style()->GetMeasureContext(),
                        element->element_manager()->GetCSSParserConfigs());
                    // radialGradientData:[shape_array, color_int_array,
                    // position_int_array] shape_array:[shape_int,
                    // shape_size_int, center_x, center_x_pattern_int, center_y,
                    // center_y_pattern_int, {size_x_pattern, size_x_value,
                    // size_y_pattern, size_y_value}]

                    // key
                    props->AddProp(static_cast<int>(kPropColorRadialGradient));

                    const auto& shape_array = *params_vector.get(0).Array();
                    // shape array(flat)
                    // shape
                    props->AddProp(
                        static_cast<int>(shape_array.get(0).UInt32()));
                    // shape_size
                    uint32_t shape_size = shape_array.get(1).UInt32();
                    props->AddProp(static_cast<int>(shape_size));
                    // x-position pattern
                    props->AddProp(
                        static_cast<int>(shape_array.get(2).UInt32()));
                    // x-position value
                    props->AddProp(shape_array.get(3).Number());
                    // y-position pattern
                    props->AddProp(
                        static_cast<int>(shape_array.get(4).UInt32()));
                    // y-position value
                    props->AddProp(shape_array.get(5).Number());

                    if (shape_size ==
                            static_cast<uint32_t>(
                                starlight::RadialGradientSizeType::kLength) &&
                        shape_array.size() == 14) {
                      // we put correct size data form index:10,11,12,13, the
                      // {index:6,7,8,9} is useless data} size_x_value
                      props->AddProp(shape_array.get(10).Number());
                      // size_x_pattern
                      props->AddProp(
                          static_cast<int>(shape_array.get(11).UInt32()));
                      // size_y_value
                      props->AddProp(shape_array.get(12).Number());
                      // size_y_pattern
                      props->AddProp(
                          static_cast<int>(shape_array.get(13).UInt32()));
                    }

                    const auto& color_array = *params_vector.get(1).Array();
                    // color_count
                    props->AddProp(static_cast<int>(color_array.size()));
                    // colors
                    for (size_t i = 0; i < color_array.size(); ++i) {
                      props->AddProp(
                          static_cast<int>(color_array.get(i).UInt32()));
                    }
                    const auto& position_array = *params_vector.get(2).Array();
                    // position_count
                    props->AddProp(static_cast<int>(position_array.size()));
                    // positions
                    for (size_t i = 0; i < position_array.size(); ++i) {
                      props->AddProp(
                          static_cast<float>(position_array.get(i).Number()));
                    }

                  } else if (type ==
                             starlight::BackgroundImageType::kConicGradient) {
                    // TODO(linxs): tbd

                  } else {
                    props->AddProp(kTextPropColor);
                    props->AddProp(static_cast<int>(
                        text_attributes->color.has_value()
                            ? text_attributes->color.value()
                            : starlight::DefaultColor::DEFAULT_TEXT_COLOR));
                    break;
                  }
                }
              }
            }
          } else {
            props->AddProp(kTextPropColor);
            props->AddProp(static_cast<int>(
                text_attributes->color.has_value()
                    ? text_attributes->color.value()
                    : starlight::DefaultColor::DEFAULT_TEXT_COLOR));
          }
          break;
        }

        case kPropertyIDWhiteSpace:
          props->AddProp(kTextPropWhiteSpace);
          props->AddProp(static_cast<int>(text_attributes->white_space));
          break;

        case kPropertyIDTextOverflow:
          props->AddProp(kTextPropTextOverflow);
          props->AddProp(static_cast<int>(text_attributes->text_overflow));
          break;

        case kPropertyIDFontWeight:
          props->AddProp(kTextPropFontWeight);
          props->AddProp(static_cast<int>(text_attributes->font_weight));
          break;
        case kPropertyIDFontStyle:
          props->AddProp(kTextPropFontStyle);
          props->AddProp(static_cast<int>(text_attributes->font_style));
          break;

        case kPropertyIDFontFamily:
          props->AddProp(kTextPropFontFamily);
          props->AddProp(text_attributes->font_family.c_str());
          break;

        case kPropertyIDLineHeight:
          props->AddProp(kTextPropLineHeight);
          props->AddProp(text_attributes->computed_line_height);
          break;

        case kPropertyIDLetterSpacing:
          props->AddProp(kTextPropLetterSpacing);
          props->AddProp(text_attributes->letter_spacing);
          break;

        case kPropertyIDTextAlign:
          props->AddProp(kTextPropTextAlign);
          props->AddProp(static_cast<int>(text_attributes->text_align));
          break;

        case kPropertyIDVerticalAlign:
          props->AddProp(kTextPropVerticalAlign);
          props->AddProp(static_cast<int>(text_attributes->vertical_align));
          props->AddProp(text_attributes->vertical_align_length);
          break;

        case kPropertyIDTextDecoration: {
          props->AddProp(kTextPropTextDecoration);
          int flags = 0;
          if (text_attributes->underline_decoration) {
            flags |=
                static_cast<int>(starlight::TextDecorationType::kUnderLine);
          }
          if (text_attributes->line_through_decoration) {
            flags |=
                static_cast<int>(starlight::TextDecorationType::kLineThrough);
          }
          // type
          props->AddProp(flags);
          // style
          props->AddProp(
              static_cast<int>(text_attributes->text_decoration_style));
          // color
          props->AddProp(static_cast<int>(
              text_attributes->text_decoration_color.has_value()
                  ? *text_attributes->text_decoration_color
                  : starlight::DefaultColor::DEFAULT_TEXT_COLOR));
        } break;

        default:
          break;
      }
    }
  }
  // attributes
  // text_maxline
  if (text_props) {
    if (text_props->text_max_line) {
      props->AddProp(kTextPropTextMaxLine);
      props->AddProp(*text_props->text_max_line);
    }
  }

  // only inline text need the pass the range, kPropRangeEnd should be the
  // first key
  if (element->is_inline_element()) {
    props->AddProp(kPropInlineEnd);
    props->AddProp(static_cast<int>(pos_end));
  }
}
void TextLayoutAndroid::AppendViewProps(ViewElement* view_element, size_t start,
                                        size_t end, PropArrayAndroid* props) {
  // range start
  props->AddProp(kPropInlineStart);
  props->AddProp(static_cast<int>(start));

  props->AddProp(kPropInlineViewSign);
  props->AddProp(view_element->impl_id());

  const auto& text_attributes =
      view_element->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value() &&
      text_attributes->vertical_align !=
          starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN) {
    props->AddProp(kTextPropVerticalAlign);
    props->AddProp(static_cast<int>(text_attributes->vertical_align));
    props->AddProp(text_attributes->vertical_align_length);
  }
  // range end
  props->AddProp(kPropInlineEnd);
  props->AddProp(static_cast<int>(end));
}

void TextLayoutAndroid::AppendImageProps(ImageElement* image_element,
                                         size_t start, size_t end,
                                         PropArrayAndroid* props) {
  // inline range start
  props->AddProp(kPropInlineStart);
  props->AddProp(static_cast<int>(start));

  // src
  props->AddProp(kPropImageSrc);
  props->AddProp(image_element->src());

  // mode
  // TBD

  // size
  props->AddProp(kPropRectSize);
  float width = starlight::NLengthToFakeLayoutUnit(
                    image_element->slnode()->GetCSSStyle()->GetWidth())
                    .ClampIndefiniteToZero()
                    .ToFloat();
  float height = starlight::NLengthToFakeLayoutUnit(
                     image_element->slnode()->GetCSSStyle()->GetHeight())
                     .ClampIndefiniteToZero()
                     .ToFloat();
  props->AddProp(static_cast<int>(width));
  props->AddProp(static_cast<int>(height));

  // margin
  int margin_left = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginLeft())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_top = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginTop())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_right = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginRight())
          .ClampIndefiniteToZero()
          .ToFloat());
  int margin_bottom = static_cast<int>(
      starlight::NLengthToFakeLayoutUnit(
          image_element->slnode()->GetCSSStyle()->GetMarginBottom())
          .ClampIndefiniteToZero()
          .ToFloat());
  if (margin_left | margin_top | margin_right | margin_bottom) {
    props->AddProp(kPropMargin);
    props->AddProp(margin_left);
    props->AddProp(margin_top);
    props->AddProp(margin_right);
    props->AddProp(margin_bottom);
  }

  auto computed_css_style = image_element->computed_css_style();
  if (computed_css_style->HasBorderRadius()) {
    props->AddProp(kPropBorderRadius);
    // only support: left,top,right,bottom border-radius for such mode
    const auto& top_left = computed_css_style->GetSimpleBorderTopLeftRadius();
    const auto& top_right = computed_css_style->GetSimpleBorderTopLeftRadius();
    const auto& bottom_left =
        computed_css_style->GetSimpleBorderTopLeftRadius();
    const auto& bottom_right =
        computed_css_style->GetSimpleBorderTopLeftRadius();
    const auto resolve_length = [](PropArrayAndroid* props,
                                   const starlight::NLength& length) {
      if (length.NumericLength().ContainsPercentage()) {
        props->AddProp(static_cast<float>(
            length.NumericLength().GetPercentagePart() / 100.f));
        props->AddProp(
            static_cast<int>(starlight::PlatformLengthUnit::PERCENTAGE));
      } else if (length.NumericLength().ContainsFixedValue()) {
        props->AddProp(
            static_cast<float>(length.NumericLength().GetFixedPart()));
        props->AddProp(static_cast<int>(starlight::PlatformLengthUnit::NUMBER));
      } else {
        // fallback: emit zero number
        props->AddProp(0.f);
        props->AddProp(static_cast<int>(starlight::PlatformLengthUnit::NUMBER));
      }
    };
    resolve_length(props, top_left);
    resolve_length(props, top_right);
    resolve_length(props, bottom_left);
    resolve_length(props, bottom_right);
  }

  const auto& text_attributes =
      image_element->computed_css_style()->GetTextAttributes();
  if (text_attributes.has_value() &&
      text_attributes->vertical_align !=
          starlight::DefaultComputedStyle::DEFAULT_VERTICAL_ALIGN) {
    props->AddProp(kTextPropVerticalAlign);
    props->AddProp(static_cast<int>(text_attributes->vertical_align));
    props->AddProp(text_attributes->vertical_align_length);
  }

  // inline range end
  props->AddProp(kPropInlineEnd);
  props->AddProp(static_cast<int>(end));
}

}  // namespace tasm
}  // namespace lynx
