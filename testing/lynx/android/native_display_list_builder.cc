// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <optional>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/style/borders_data.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"
#include "testing/lynx/android/gen/NativeDisplayListBuilder_jni.h"

namespace {

class NativeDisplayListBuilderState {
 public:
  lynx::tasm::DisplayListBuilder builder;
  std::optional<lynx::tasm::DisplayList> display_list;
};

NativeDisplayListBuilderState* State(jlong native_ptr) {
  return reinterpret_cast<NativeDisplayListBuilderState*>(native_ptr);
}

lynx::tasm::RoundedRectangle MakeRoundedRectangle(JNIEnv* env, jfloat x,
                                                  jfloat y, jfloat width,
                                                  jfloat height,
                                                  jfloatArray radii) {
  lynx::tasm::RoundedRectangle rectangle;
  rectangle.SetX(x);
  rectangle.SetY(y);
  rectangle.SetWidth(width);
  rectangle.SetHeight(height);

  if (radii == nullptr || env->GetArrayLength(radii) != 8) {
    return rectangle;
  }

  jfloat values[8];
  env->GetFloatArrayRegion(radii, 0, 8, values);
  rectangle.SetRadiusXTopLeft(values[0]);
  rectangle.SetRadiusYTopLeft(values[1]);
  rectangle.SetRadiusXTopRight(values[2]);
  rectangle.SetRadiusYTopRight(values[3]);
  rectangle.SetRadiusXBottomRight(values[4]);
  rectangle.SetRadiusYBottomRight(values[5]);
  rectangle.SetRadiusXBottomLeft(values[6]);
  rectangle.SetRadiusYBottomLeft(values[7]);
  return rectangle;
}

}  // namespace

extern "C" JNIEXPORT bool JNICALL
Java_com_lynx_tasm_behavior_render_NativeDisplayListBuilder_registerJNI(
    JNIEnv* env, jclass /*jclazz*/) {
  return RegisterNativesImpl(env);
}

jlong Create(JNIEnv* /*env*/, jclass /*jcaller*/) {
  return reinterpret_cast<jlong>(new NativeDisplayListBuilderState());
}

void Destroy(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr) {
  delete State(native_ptr);
}

void Begin(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint id,
           jint type, jfloat x, jfloat y, jfloat width, jfloat height) {
  State(native_ptr)
      ->builder.Begin(id, static_cast<PlatformRendererType>(type), x, y, width,
                      height);
}

void End(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr) {
  State(native_ptr)->builder.End();
}

void Fill(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint color,
          jint clip_index) {
  State(native_ptr)->builder.Fill(static_cast<uint32_t>(color), clip_index);
}

void DrawView(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
              jint view_id, jfloat offset_x, jfloat offset_y) {
  State(native_ptr)->builder.DrawView(view_id, offset_x, offset_y);
}

void Text(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint text_id,
          jint box_index) {
  State(native_ptr)->builder.DrawText(text_id, box_index);
}

void Image(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint image_id,
           jint box_index) {
  State(native_ptr)
      ->builder.DrawImage(fml::MakeRefCounted<lynx::tasm::PaintImage>(image_id),
                          box_index);
}

void BackgroundImage(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
                     jint image_id, jint tiling_index, jint clip_index,
                     jint repeat_x, jint repeat_y) {
  State(native_ptr)
      ->builder.BackgroundImage(
          fml::MakeRefCounted<lynx::tasm::PaintImage>(image_id), tiling_index,
          clip_index, repeat_x, repeat_y);
}

void Border(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr, jint out_index,
            jint inner_index, jintArray colors, jintArray styles) {
  jint color_values[4];
  jint style_values[4];
  env->GetIntArrayRegion(colors, 0, 4, color_values);
  env->GetIntArrayRegion(styles, 0, 4, style_values);

  lynx::starlight::BordersData border;
  border.color_top = static_cast<uint32_t>(color_values[0]);
  border.color_right = static_cast<uint32_t>(color_values[1]);
  border.color_bottom = static_cast<uint32_t>(color_values[2]);
  border.color_left = static_cast<uint32_t>(color_values[3]);
  border.style_top =
      static_cast<lynx::starlight::BorderStyleType>(style_values[0]);
  border.style_right =
      static_cast<lynx::starlight::BorderStyleType>(style_values[1]);
  border.style_bottom =
      static_cast<lynx::starlight::BorderStyleType>(style_values[2]);
  border.style_left =
      static_cast<lynx::starlight::BorderStyleType>(style_values[3]);
  State(native_ptr)->builder.Border(out_index, inner_index, border);
}

void ClipRect(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr, jfloat x,
              jfloat y, jfloat width, jfloat height, jfloatArray radii) {
  State(native_ptr)
      ->builder.ClipRect(MakeRoundedRectangle(env, x, y, width, height, radii));
}

void RecordBox(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr, jfloat x,
               jfloat y, jfloat width, jfloat height, jfloatArray radii) {
  int32_t index = -1;
  State(native_ptr)
      ->builder.RecordBoxModel(
          MakeRoundedRectangle(env, x, y, width, height, radii), index);
}

void LinearGradient(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr,
                    jintArray colors, jfloatArray stops, jint tiling_index,
                    jint clip_index, jint repeat_x, jint repeat_y,
                    jfloat angle) {
  lynx::base::Vector<uint32_t> color_values;
  if (colors != nullptr) {
    const jsize color_count = env->GetArrayLength(colors);
    color_values.reserve(color_count);
    jint* elements = env->GetIntArrayElements(colors, nullptr);
    for (jsize i = 0; i < color_count; ++i) {
      color_values.push_back(static_cast<uint32_t>(elements[i]));
    }
    env->ReleaseIntArrayElements(colors, elements, JNI_ABORT);
  }

  lynx::base::Vector<float> stop_values;
  if (stops != nullptr) {
    const jsize stop_count = env->GetArrayLength(stops);
    stop_values.reserve(stop_count);
    jfloat* elements = env->GetFloatArrayElements(stops, nullptr);
    for (jsize i = 0; i < stop_count; ++i) {
      stop_values.push_back(elements[i]);
    }
    env->ReleaseFloatArrayElements(stops, elements, JNI_ABORT);
  }

  State(native_ptr)
      ->builder.LinearGradient(angle, color_values, stop_values, tiling_index,
                               clip_index, repeat_x, repeat_y);
}

void BoxShadow(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
               jint shadow_box_index, jint clip_box_index, jint color,
               jfloat blur_radius, jint clip_mode) {
  State(native_ptr)
      ->builder.BoxShadow(
          shadow_box_index, clip_box_index, static_cast<uint32_t>(color),
          blur_radius,
          static_cast<lynx::tasm::DisplayListBuilder::BoxShadowClipMode>(
              clip_mode));
}

void Build(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr) {
  NativeDisplayListBuilderState* state = State(native_ptr);
  state->display_list.emplace(state->builder.Build());
}

jobject GetItemsBuffer(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr) {
  const lynx::tasm::DisplayList& display_list =
      State(native_ptr)->display_list.value();
  const uint8_t* data = display_list.GetContentItemsData();
  static uint8_t empty_buffer;
  return env->NewDirectByteBuffer(
      const_cast<uint8_t*>(data == nullptr ? &empty_buffer : data),
      static_cast<jlong>(display_list.GetContentItemsByteSize()));
}

jobject GetDataBuffer(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr) {
  const lynx::tasm::DisplayList& display_list =
      State(native_ptr)->display_list.value();
  const uint8_t* data = display_list.GetContentData();
  if (data == nullptr) {
    return nullptr;
  }
  return env->NewDirectByteBuffer(
      const_cast<uint8_t*>(data),
      static_cast<jlong>(display_list.GetContentDataSize()));
}
