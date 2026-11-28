// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/android/text_layout_android.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/include/string/unicode_decode_utils.h"
#include "core/base/android/android_jni.h"
#include "core/base/android/jni_helper.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/image_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/text_props.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_compact_array_buffer.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "platform/android/lynx_android/src/main/jni/gen/TextLayout_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/TextLayout_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForTextLayout(JNIEnv* env) { return RegisterNativesImpl(env); }
}  // namespace jni
}  // namespace lynx

namespace lynx {
namespace tasm {
namespace {

bool IsInlineTruncation(Element* element) {
  return element->GetTag().IsEqual("inline-truncation");
}

base::String GetRawTextContent(Element* element) {
  auto* data_model = element->data_model();
  if (data_model == nullptr) {
    return base::String();
  }
  BASE_STATIC_STRING_DECL(kTextAttr, "text");
  const auto& attributes = data_model->attributes();
  auto it = attributes.find(kTextAttr);
  if (it == attributes.end() || !it->second.IsString()) {
    return base::String();
  }
  return it->second.String();
}

base::UnicodeDecodeProperty WordBreakToDecodeProperty(
    starlight::WordBreakType word_break) {
  switch (word_break) {
    case starlight::WordBreakType::kBreakAll:
      return base::UnicodeDecodeProperty::kInsertZeroWidthChar;
    case starlight::WordBreakType::kKeepAll:
      return base::UnicodeDecodeProperty::kCjkInsertWordJoiner;
    default:
      return base::UnicodeDecodeProperty::kDefault;
  }
}

base::UnicodeDecodeProperty DecodePropertyForTextElement(TextElement* element) {
  if (element == nullptr) {
    return base::UnicodeDecodeProperty::kDefault;
  }
  const auto& text_attributes =
      element->computed_css_style()->GetTextAttributes();
  if (!text_attributes.has_value()) {
    return base::UnicodeDecodeProperty::kDefault;
  }
  return WordBreakToDecodeProperty(text_attributes->word_break);
}

TextElement* FindParentTextElement(Element* child) {
  for (Element* parent = child->parent(); parent != nullptr;
       parent = parent->parent()) {
    if (parent->is_text()) {
      return static_cast<TextElement*>(parent);
    }
  }
  return nullptr;
}

bool NeedsTruncationFontSpan(Element* token) {
  auto* style = token->computed_css_style();
  const auto& font = style->GetTextAttributes();
  if (!font) return false;
  if (auto* parent = FindParentTextElement(token)) {
    const auto& inherited = parent->computed_css_style()->GetTextAttributes();
    if (inherited && inherited->font_weight == font->font_weight &&
        inherited->font_style == font->font_style) {
      return false;
    }
  }
  const auto& resolved = style->GetResolvedValues();
  return resolved.count(kPropertyIDFontWeight) ||
         resolved.count(kPropertyIDFontStyle);
}

}  // namespace

TextLayoutAndroid::TextLayoutAndroid(JNIEnv* env, jobject text_layout)
    : text_layout_(env, text_layout) {}

TextLayoutAndroid::~TextLayoutAndroid() = default;

void TextLayoutAndroid::Destroy(Element* element) {
  if (element == nullptr) {
    return;
  }
  truncation_event_ranges_.erase(element->impl_id());
  if (prepared_detached_id_ == element->impl_id()) {
    prepared_detached_id_.reset();
  }
  if (element->is_text()) {
    static_cast<TextElement*>(element)->ClearTextLineLayoutInfo();
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (!local_ref.IsNull()) {
    Java_TextLayout_destroyText(env, local_ref.Get(), element->impl_id());
  }
}

LayoutResult TextLayoutAndroid::Measure(Element* element, float width,
                                        int width_mode, float height,
                                        int height_mode) {
  auto result =
      MeasureInternal(element, width, width_mode, height, height_mode);
  if (element && element->is_text()) {
    // Publish geometry only for normal layout, not detached measurement probes.
    auto* text_element = static_cast<TextElement*>(element);
    text_element->UpdateInlineTextGeometry(
        result ? GetInlineTextRegions(text_element)
               : PlatformTextEventTargetRegions{});
  }
  return result.value_or(LayoutResult{});
}

std::optional<LayoutResult> TextLayoutAndroid::MeasureDetached(
    Element* element, float width, int width_mode, float height,
    int height_mode) {
  if (element == nullptr || prepared_detached_id_ != element->impl_id()) {
    return std::nullopt;
  }
  return MeasureInternal(element, width, width_mode, height, height_mode);
}

std::optional<LayoutResult> TextLayoutAndroid::MeasureInternal(
    Element* element, float width, int width_mode, float height,
    int height_mode) {
  TextElement* text_element = static_cast<TextElement*>(element);
  text_element->ClearTextLineLayoutInfo();
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull()) {
    return std::nullopt;
  }

  std::vector<float> layout_result;
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
    base::android::CheckException(env);
    return std::nullopt;
  }

  env->SetFloatArrayRegion(layout_result_array, 0, layout_result.size(),
                           layout_result.data());
  if (env->ExceptionCheck()) {
    base::android::CheckException(env);
    env->DeleteLocalRef(layout_result_array);
    return std::nullopt;
  }

  const lynx::base::android::ScopedLocalJavaRef<jfloatArray> result_array =
      Java_TextLayout_measureText(env, local_ref.Get(), element->impl_id(),
                                  width, width_mode, height, height_mode,
                                  layout_result_array);
  const bool measure_failed =
      base::android::HasJNIException() || env->ExceptionCheck();
  env->DeleteLocalRef(layout_result_array);
  if (measure_failed || !result_array.Get()) {
    text_element->ClearTextLineLayoutInfo();
    return std::nullopt;
  }

  jsize array_length = env->GetArrayLength(result_array.Get());
  if (array_length < 3) {
    text_element->ClearTextLineLayoutInfo();
    return std::nullopt;
  }

  jfloat* result = env->GetFloatArrayElements(result_array.Get(), JNI_FALSE);
  if (result == nullptr) {
    base::android::CheckException(env);
    text_element->ClearTextLineLayoutInfo();
    return std::nullopt;
  }
  float measured_width = result[0];
  float measured_height = result[1];
  float base_line = result[2];

  // Parse line layout information if available
  // Format: [width, height, baseline, lineCount, line1_start, line1_end,
  // line1_ellipsisCount, ..., eventWidth, eventHeight]. Event dimensions retain
  // the platform event's density-independent units and text-content size.
  if (array_length > 3 && text_element) {
    int line_count = static_cast<int>(result[3]);
    if (line_count > 0 &&
        array_length >= static_cast<jsize>(4 + line_count * 3)) {
      // Allocate array for line layout info
      TextLineInfoArray line_infos(new TextLineInfo[line_count]);

      for (int i = 0; i < line_count; i++) {
        line_infos[i].start = static_cast<int>(result[4 + i * 3]);
        line_infos[i].end = static_cast<int>(result[4 + i * 3 + 1]);
        line_infos[i].ellipsis_count = static_cast<int>(result[4 + i * 3 + 2]);
      }

      // Store line layout info in TextElement
      text_element->SetTextLineLayoutInfo(std::move(line_infos), line_count);
      const jsize event_size_offset = 4 + line_count * 3;
      if (text_element->HasLayoutEvent() &&
          array_length >= event_size_offset + 2) {
        text_element->SetTextLayoutEventSize(FloatSize(
            result[event_size_offset], result[event_size_offset + 1]));
      }
      auto truncation = truncation_event_ranges_.find(element->impl_id());
      if (truncation != truncation_event_ranges_.end() &&
          array_length >= event_size_offset + 3) {
        const auto& info = truncation->second;
        const auto cut = static_cast<int32_t>(result[event_size_offset + 2]);
        std::vector<PlatformTextEventTargetRange> ranges;
        for (auto range : info.ranges) {
          if (range.start >= info.body_length) {
            if (cut < 0) continue;
            range.start += cut - info.body_length;
            range.end += cut - info.body_length;
          } else {
            range.end = std::min(range.end, cut < 0 ? info.body_length : cut);
          }
          if (range.start < range.end) ranges.push_back(range);
        }
        element->element_manager()
            ->painting_context()
            ->impl()
            ->CastToNativeCtx()
            ->UpdateTextEventTargetRanges(element->impl_id(),
                                          std::move(ranges));
      }
    } else {
      text_element->ClearTextLineLayoutInfo();
    }
  } else {
    text_element->ClearTextLineLayoutInfo();
  }

  env->ReleaseFloatArrayElements(result_array.Get(), result, JNI_ABORT);
  return LayoutResult{measured_width, measured_height, base_line};
}

PlatformTextEventTargetRegions TextLayoutAndroid::GetInlineTextRegions(
    TextElement* paragraph) {
  PlatformTextEventTargetRegions regions;
  const auto& ranges = paragraph->GetInlineTextRanges();
  if (ranges.empty()) {
    return regions;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull()) {
    return regions;
  }
  std::vector<int32_t> range_data;
  for (const auto& range : ranges) {
    range_data.insert(range_data.end(), {range.sign, range.start, range.end});
  }
  auto input = base::android::JNIHelper::ConvertToJNIIntArray(env, range_data);
  auto result = Java_TextLayout_getInlineTextRegions(
      env, local_ref.Get(), paragraph->impl_id(), input.Get());
  if (result.IsNull()) {
    return regions;
  }
  const jsize size = env->GetArrayLength(result.Get());
  std::vector<float> data(size);
  env->GetFloatArrayRegion(result.Get(), 0, size, data.data());
  for (jsize i = 0; i + 4 < size; i += 5) {
    int32_t sign;
    std::memcpy(&sign, &data[i], sizeof(sign));
    regions.push_back(PlatformTextEventTargetRegion{
        sign, data[i + 1], data[i + 2], data[i + 3], data[i + 4]});
  }
  return regions;
}

void TextLayoutAndroid::MeasureInlineViewRecursively(
    Element* element, const starlight::Constraints& constraints,
    std::vector<float>& layout_result) {
  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    if (child->is_text() || child->is_wrapper() || IsInlineTruncation(child)) {
      MeasureInlineViewRecursively(child, constraints, layout_result);
    } else if (child->is_view()) {
      FloatSize size =
          child->slnode()->UpdateMeasureByPlatform(constraints, true);
      layout_result.push_back(child->impl_id());
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
  if (size > 0) {
    jfloat* arr = env->GetFloatArrayElements(result.Get(), nullptr);
    if (arr == nullptr) return;
    for (auto i = 0; i < size / 3; i++) {
      result_map[arr[i * 3]] = std::make_pair(arr[i * 3 + 1], arr[i * 3 + 2]);
    }
    env->ReleaseFloatArrayElements(result.Get(), arr, JNI_ABORT);
  }
  AlignChildrenRecursively(element, result_map);
}

void TextLayoutAndroid::AlignChildrenRecursively(
    Element* element,
    const std::unordered_map<int, std::pair<float, float>>& result_map) {
  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    if (child->is_text() || child->is_wrapper() || IsInlineTruncation(child)) {
      AlignChildrenRecursively(child, result_map);
    } else if (child->is_view()) {
      auto it = result_map.find(child->impl_id());
      if (it != result_map.end()) {
        child->slnode()->AlignmentByPlatform(it->second.first,
                                             it->second.second);
      } else {
        child->slnode()->LayoutDisplayNone();
      }
    }
  }
}

struct TextLayoutAndroid::TextBuffer {
  void Append(const base::String& content, TextElement* parent) {
    if (content.empty()) return;
    auto decoded = base::UnicodeDecodeUtils::Decode(
        std::string_view(content.c_str(), content.length()),
        DecodePropertyForTextElement(parent));
    text += decoded;
    length += use_utf16
                  ? GetUtf16SizeFromUtf8(decoded.c_str(), decoded.length())
                  : decoded.length();
  }

  bool use_utf16;
  std::string text;
  size_t length = 0;
  bool has_inline_view = false;
  Element* truncation = nullptr;
  PropArrayAndroid props;
};

void TextLayoutAndroid::DispatchLayoutBefore(Element* element) {
  const bool is_detached_root =
      !element->parent() && !element->render_parent() && element->IsDetached();
  if (is_detached_root) {
    prepared_detached_id_.reset();
  }
  base::android::ScopedLocalJavaRef<jobject> local_ref(text_layout_);
  if (local_ref.IsNull() || !element->is_text()) {
    return;
  }
  JNIEnv* env = base::android::AttachCurrentThread();
  auto* text_element = static_cast<TextElement*>(element);
  TextBuffer buffer{};
  buffer.use_utf16 =
      text_element->is_inline_element() || text_element->has_inline_child();
  building_event_target_ranges_.clear();
  AppendTextContent(element, buffer);
  const auto body_length = buffer.length;
  if (auto* token = buffer.truncation) {
    AppendTextContent(token, buffer);
    if (buffer.length > body_length) {
      AppendTextProps(token, body_length, buffer.length, &buffer.props);
      for (auto* parent = token->parent(); parent && parent != element;
           parent = parent->parent()) {
        if (parent->is_text())
          AppendTextProps(parent, body_length, buffer.length, &buffer.props);
      }
      buffer.props.AddProp(kPropInlineTruncation);
      buffer.props.AddProp(static_cast<int>(body_length));
    }
  }
  if (buffer.length > 0)
    AppendTextProps(element, 0, buffer.length, &buffer.props);
  text_element->set_need_layout_children(buffer.has_inline_view);

  text_element->SetInlineTextRanges(building_event_target_ranges_);
  truncation_event_ranges_.erase(element->impl_id());
  if (element->EnableFragmentLayerRender() && !is_detached_root) {
    if (buffer.length > body_length && !building_event_target_ranges_.empty()) {
      truncation_event_ranges_.emplace(
          element->impl_id(),
          TruncationEventRanges{static_cast<int32_t>(body_length),
                                std::move(building_event_target_ranges_)});
    } else if (auto* manager = element->element_manager();
               manager && manager->painting_context()) {
      manager->painting_context()
          ->impl()
          ->CastToNativeCtx()
          ->UpdateTextEventTargetRanges(
              element->impl_id(), std::move(building_event_target_ranges_));
    }
  }

  buffer.props.AddProp(kPropTextString);
  buffer.props.AddProp(buffer.text.c_str());
  auto text_ab = buffer.props.GetArrayBuffer();
  auto j_text_props = text_ab ? base::android::JReadableCompactArrayBuffer::
                                    CreateReadableCompactArrayBuffer(*text_ab)
                              : std::nullopt;

  if (!j_text_props.has_value() || !j_text_props->Get()) {
    return;
  }
  Java_TextLayout_dispatchLayoutBefore(env, local_ref.Get(), element->impl_id(),
                                       j_text_props->Get());
  if (is_detached_root && !base::android::HasJNIException() &&
      !env->ExceptionCheck()) {
    prepared_detached_id_ = element->impl_id();
  }
}

void TextLayoutAndroid::AppendTextContent(Element* element,
                                          TextBuffer& buffer) {
  if (element->is_text()) {
    auto* text = static_cast<TextElement*>(element);
    buffer.Append(text->content(), text);
  }
  for (auto* child = element->first_render_child(); child;
       child = child->next_render_sibling()) {
    const auto start = buffer.length;
    if (IsInlineTruncation(child)) {
      buffer.truncation = child;
    } else if (child->is_raw_text()) {
      buffer.Append(static_cast<RawTextElement*>(child)->content(),
                    FindParentTextElement(child));
    } else if (child->GetTag().IsEqual(RawTextElement::kRawTextTag)) {
      buffer.Append(GetRawTextContent(child), FindParentTextElement(child));
    } else if (child->is_text() || child->is_wrapper()) {
      AppendTextContent(child, buffer);
      if (child->is_text() && buffer.length > start) {
        AppendTextProps(child, start, buffer.length, &buffer.props);
      }
    } else if (child->is_image() || child->is_view()) {
      buffer.text += kInlinePlaceHolder;
      ++buffer.length;
      if (child->is_image()) {
        AppendImageProps(static_cast<ImageElement*>(child), start,
                         buffer.length, &buffer.props);
      } else {
        AppendViewProps(child, start, buffer.length, &buffer.props);
        buffer.has_inline_view = true;
      }
    }
  }
}

void TextLayoutAndroid::AppendTextProps(Element* element, size_t pos_start,
                                        size_t pos_end,
                                        PropArrayAndroid* props) {
  auto* text_element =
      element->is_text() ? static_cast<TextElement*>(element) : nullptr;
  TextProps* text_props = text_element ? text_element->text_props() : nullptr;
  auto inline_end = kPropInlineEnd;
  CSSIDBitset token_properties;
  if (!text_element) {
    token_properties.Set(kPropertyIDFontSize);
    for (const auto& style : element->GetParsedStylesMap()) {
      if (style.first != kPropertyIDFontWeight &&
          style.first != kPropertyIDFontStyle) {
        token_properties.Set(style.first);
      }
    }
    if (NeedsTruncationFontSpan(element)) {
      // Android applies weight and italic together, including an explicit reset
      // to normal.
      token_properties.Set(kPropertyIDFontWeight);
      token_properties.Set(kPropertyIDFontStyle);
      inline_end = kPropInlineEndWithFont;
    }
  }
  const CSSIDBitset& property_bits =
      text_element ? text_element->property_bits() : token_properties;
  const bool is_inline = !text_element || element->is_inline_element();
  const bool has_tap_event = is_inline && element->HasEventListener("tap");
  if (!text_props && !property_bits.HasAny() && !has_tap_event) {
    return;
  }
  // only inline text need the pass the range，   kPropRangeStart should be
  // the first key
  if (is_inline) {
    props->AddProp(kPropInlineStart);
    props->AddProp(static_cast<int>(pos_start));
    if (has_tap_event) {
      props->AddProp(kPropInlineEventTarget);
      props->AddProp(element->impl_id());
      // LayoutInElement uses these ranges for geometry queries, including when
      // fragment rendering is disabled.
      if (element->EnableLayoutInElementMode()) {
        building_event_target_ranges_.push_back(
            {element->impl_id(), static_cast<int32_t>(pos_start),
             static_cast<int32_t>(pos_end)});
      }
    }
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

        case kPropertyIDFontVariationSettings: {
          props->AddProp(kTextPropFontVariationSettings);
          const auto& axes = text_attributes->font_variation_settings;
          const size_t axis_count = axes ? axes->size() / 2 : 0;
          props->AddProp(static_cast<int>(axis_count));
          for (size_t i = 0; i < axis_count; ++i) {
            props->AddProp(axes->get(i * 2).String().c_str());
            props->AddProp(axes->get(i * 2 + 1).Number());
          }
          break;
        }

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

        case kPropertyIDXAutoFontSize:
          props->AddProp(kTextPropAutoFontSize);
          props->AddProp(text_attributes->is_auto_font_size);
          props->AddProp(text_attributes->auto_font_size_min_size);
          props->AddProp(text_attributes->auto_font_size_max_size);
          props->AddProp(text_attributes->auto_font_size_step_granularity);
          break;

        case kPropertyIDXAutoFontSizePresetSizes:
          props->AddProp(kTextPropAutoFontSizePresetSizes);
          if (text_attributes->auto_font_size_preset_sizes) {
            const auto& preset_sizes =
                *text_attributes->auto_font_size_preset_sizes;
            props->AddProp(static_cast<int>(preset_sizes.size()));
            for (float size : preset_sizes) {
              props->AddProp(size);
            }
          } else {
            props->AddProp(0);
          }
          break;

        case kPropertyIDXAutoFontSizeLineRanges:
          props->AddProp(kTextPropAutoFontSizeLineRanges);
          if (text_attributes->auto_font_size_line_ranges) {
            const auto& line_ranges =
                *text_attributes->auto_font_size_line_ranges;
            props->AddProp(static_cast<int>(line_ranges.size()));
            for (const auto& range : line_ranges) {
              props->AddProp(range.start_line);
              props->AddProp(range.end_line);
              props->AddProp(range.min_size);
              props->AddProp(range.max_size);
            }
          } else {
            props->AddProp(0);
          }
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
  if (is_inline) {
    props->AddProp(inline_end);
    props->AddProp(static_cast<int>(pos_end));
  }
}
void TextLayoutAndroid::AppendViewProps(Element* element, size_t start,
                                        size_t end, PropArrayAndroid* props) {
  // range start
  props->AddProp(kPropInlineStart);
  props->AddProp(static_cast<int>(start));

  props->AddProp(kPropInlineViewSign);
  props->AddProp(element->impl_id());

  const auto& text_attributes =
      element->computed_css_style()->GetTextAttributes();
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

  if (image_element->HasEventListener("tap")) {
    props->AddProp(kPropInlineEventTarget);
    props->AddProp(image_element->impl_id());
    if (image_element->EnableFragmentLayerRender()) {
      building_event_target_ranges_.push_back({image_element->impl_id(),
                                               static_cast<int32_t>(start),
                                               static_cast<int32_t>(end)});
    }
  }

  // src
  props->AddProp(kPropImageSrc);
  props->AddProp(image_element->src().c_str());

  props->AddProp(kPropImageLoopCount);
  props->AddProp(image_element->paint_info().loop_count);

  props->AddProp(kPropImageMode);
  props->AddProp(static_cast<int>(image_element->paint_info().mode));

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
