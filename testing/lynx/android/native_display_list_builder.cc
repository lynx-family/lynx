// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <optional>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/style/borders_data.h"
#include "core/renderer/ui_wrapper/painting/android/platform_renderer_android.h"
#include "core/renderer/ui_wrapper/painting/public/paint_image.h"
#include "testing/lynx/android/gen/NativeDisplayListBuilder_jni.h"

namespace {

class NativeDisplayListBuilderState {
 public:
  lynx::tasm::DisplayListBuilder builder;
  std::optional<lynx::tasm::DisplayList> display_list;
  std::vector<fml::RefPtr<lynx::gfx::Path>> paths;
  fml::RefPtr<lynx::tasm::PlatformRendererAndroid> renderer;
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

void CreateRenderer(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
                    jlong context_ptr, jint id) {
  auto* context =
      reinterpret_cast<lynx::tasm::PlatformRendererContext*>(context_ptr);
  State(native_ptr)->renderer =
      fml::MakeRefCounted<lynx::tasm::PlatformRendererAndroid>(
          context, id, PlatformRendererType::kPage, nullptr);
}

void Submit(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
            jlong renderer_owner_ptr) {
  auto* state = State(native_ptr);
  State(renderer_owner_ptr)
      ->renderer->UpdateDisplayList(std::move(state->display_list.value()));
  state->display_list.reset();
}

void Fill(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint color,
          jint clip_index) {
  State(native_ptr)->builder.Fill(static_cast<uint32_t>(color), clip_index);
}

jint AddPath(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr, jfloat width,
             jfloat height, jfloatArray radii, jintArray commands,
             jfloatArray points) {
  fml::RefPtr<lynx::gfx::Path> path;
  if (commands != nullptr && points != nullptr) {
    const auto count = env->GetArrayLength(commands);
    const auto point_values = env->GetArrayLength(points);
    if (point_values % 2 != 0) return -1;
    std::vector<jint> verbs(count);
    std::vector<jfloat> coords(point_values);
    env->GetIntArrayRegion(commands, 0, count, verbs.data());
    env->GetFloatArrayRegion(points, 0, point_values, coords.data());
    std::vector<lynx::gfx::PathVerb> path_verbs;
    std::vector<lynx::gfx::PathPoint> path_points;
    for (jint verb : verbs)
      path_verbs.push_back(static_cast<lynx::gfx::PathVerb>(verb));
    for (jsize i = 0; i < point_values; i += 2)
      path_points.push_back({coords[i], coords[i + 1]});
    path = lynx::gfx::Path::MakeCommands(std::move(path_verbs),
                                         std::move(path_points));
  } else if (radii != nullptr && env->GetArrayLength(radii) == 8) {
    float values[8];
    env->GetFloatArrayRegion(radii, 0, 8, values);
    path = lynx::gfx::Path::MakeRoundedRect(width, height, values);
  } else {
    path = lynx::gfx::Path::MakeRect(width, height);
  }
  if (!path) return -1;
  auto& paths = State(native_ptr)->paths;
  paths.push_back(std::move(path));
  return static_cast<jint>(paths.size() - 1);
}

jint CopyPath(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr,
              jlong source_ptr, jint index) {
  auto& source = State(source_ptr)->paths;
  if (index < 0 || static_cast<size_t>(index) >= source.size()) return -1;
  auto& paths = State(native_ptr)->paths;
  paths.push_back(source[index]);
  return static_cast<jint>(paths.size() - 1);
}

void PathOp(JNIEnv* /*env*/, jclass /*jcaller*/, jlong native_ptr, jint index,
            jint color, jint rule, jfloat x, jfloat y, jboolean clip) {
  auto* state = State(native_ptr);
  auto path = index >= 0 && static_cast<size_t>(index) < state->paths.size()
                  ? state->paths[index]
                  : fml::RefPtr<lynx::gfx::Path>();
  if (clip) {
    state->builder.ClipPath(std::move(path),
                            static_cast<lynx::gfx::PathFillRule>(rule), x, y);
  } else {
    state->builder.FillPath(std::move(path), static_cast<uint32_t>(color),
                            static_cast<lynx::gfx::PathFillRule>(rule), x, y);
  }
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
                     jint repeat_x, jint repeat_y, jboolean auto_width,
                     jboolean auto_height, jfloat position_x,
                     jfloat position_y) {
  State(native_ptr)
      ->builder.BackgroundImage(
          fml::MakeRefCounted<lynx::tasm::PaintImage>(image_id), tiling_index,
          clip_index, repeat_x, repeat_y, auto_width, auto_height, position_x,
          position_y);
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

void RadialGradient(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr,
                    jintArray colors, jfloatArray stops, jint tiling_index,
                    jint clip_index, jint repeat_x, jint repeat_y,
                    jfloat center_x, jfloat center_y, jfloat radius_x,
                    jfloat radius_y) {
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
      ->builder.RadialGradient(center_x, center_y, radius_x, radius_y,
                               color_values, stop_values, tiling_index,
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

jobject GetPathKeysBuffer(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr) {
  const auto& list = State(native_ptr)->display_list.value();
  return list.GetPathCount() == 0
             ? nullptr
             : env->NewDirectByteBuffer(
                   const_cast<uint64_t*>(list.GetPathKeysData()),
                   list.GetPathCount() * sizeof(uint64_t));
}

jobject GetPathBuffer(JNIEnv* env, jclass /*jcaller*/, jlong native_ptr,
                      jint index, jlong key, jint part) {
  if (index < 0) return nullptr;
  const auto* path = State(native_ptr)->display_list->GetPath(index);
  if (!path || path->GetKey() != static_cast<uint64_t>(key)) return nullptr;
  if (part == 0)
    return env->NewDirectByteBuffer(
        const_cast<lynx::gfx::PathHeader*>(&path->GetHeader()),
        sizeof(lynx::gfx::PathHeader));
  if (part == 1 && !path->GetCommands().empty())
    return env->NewDirectByteBuffer(
        const_cast<lynx::gfx::PathVerb*>(path->GetCommands().data()),
        path->GetCommands().size() * sizeof(lynx::gfx::PathVerb));
  if (part == 2 && !path->GetPoints().empty())
    return env->NewDirectByteBuffer(
        const_cast<lynx::gfx::PathPoint*>(path->GetPoints().data()),
        path->GetPoints().size() * sizeof(lynx::gfx::PathPoint));
  return nullptr;
}
