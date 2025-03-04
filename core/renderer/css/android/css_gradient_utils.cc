// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/android/css_gradient_utils.h"

#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/jni_helper.h"
#include "core/build/gen/GradientUtils_jni.h"
#include "core/renderer/css/css_style_utils.h"
#include "core/renderer/css/css_utils.h"
#include "core/renderer/css/measure_context.h"
#include "core/renderer/tasm/react/android/mapbuffer/map_buffer_builder.h"
#include "core/renderer/tasm/react/android/mapbuffer/readable_map_buffer.h"
#include "core/renderer/ui_wrapper/common/android/prop_bundle_android.h"
#include "core/value_wrapper/value_impl_lepus.h"

jfloatArray GetRadialRadius(JNIEnv* env, jclass jcaller, jint shape,
                            jint shapeSize, jfloat cx, jfloat cy, jfloat sx,
                            jfloat sy) {
  auto radius = lynx::tasm::GetRadialGradientRadius(
      static_cast<lynx::starlight::RadialGradientShapeType>(shape),
      static_cast<lynx::starlight::RadialGradientSizeType>(shapeSize), cx, cy,
      sx, sy);
  auto arr = env->NewFloatArray(2);
  jfloat ret[] = {radius.first, radius.second};
  env->SetFloatArrayRegion(arr, 0, 2, ret);
  return arr;
}

jobject GetGradientArray(JNIEnv* env, jclass jcaller, jstring gradientDef,
                         jfloat screen_width, jfloat layouts_unit_per_px,
                         jfloat physical_pixels_per_layout_unit,
                         jfloat root_node_font_size, jfloat cur_node_font_size,
                         jfloat font_scale, jfloat viewport_width,
                         jfloat viewport_height) {
  std::string gradient_def =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, gradientDef);
  auto gradient_data =
      lynx::starlight::CSSStyleUtils::GetGradientArrayFromString(
          gradient_def.c_str(), gradient_def.length(),
          lynx::tasm::CssMeasureContext(
              screen_width, layouts_unit_per_px,
              physical_pixels_per_layout_unit, root_node_font_size,
              cur_node_font_size, lynx::starlight::LayoutUnit(viewport_width),
              lynx::starlight::LayoutUnit(viewport_height)),
          lynx::tasm::CSSParserConfigs());

  if (!gradient_data.IsArray()) {
    return nullptr;
  }
  lynx::base::android::MapBufferBuilder builder{};
  lynx::pub::ValueImplLepus value{std::move(gradient_data)};
  lynx::tasm::PropBundleAndroid::AssembleMapBuffer(builder, 0, value);
  auto map_buffer =
      std::make_unique<lynx::base::android::MapBuffer>(builder.build());
  auto map_buffer_jobject =
      lynx::base::android::JReadableMapBuffer::CreateReadableMapBuffer(
          *map_buffer);
  return env->NewLocalRef(map_buffer_jobject.Get());  // NOLINT
}

namespace lynx {
namespace tasm {

bool CSSGradientUtils::RegisterJNI(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace tasm
}  // namespace lynx
