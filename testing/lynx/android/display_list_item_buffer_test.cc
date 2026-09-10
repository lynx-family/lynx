// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/memory/ref_ptr.h"
#include "core/renderer/dom/fragment/display_list_builder.h"
#include "core/renderer/starlight/style/borders_data.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"
#include "testing/lynx/android/gen/DisplayListItemBufferTest_jni.h"

namespace {

lynx::tasm::DisplayList CreateDisplayListFixture() {
  lynx::tasm::DisplayListBuilder builder;
  builder.Reserve(13);

  lynx::starlight::BordersData border;
  border.color_top = 0xFF010203;
  border.color_right = 0xFF111213;
  border.color_bottom = 0xFF212223;
  border.color_left = 0xFF313233;
  border.style_top = static_cast<lynx::starlight::BorderStyleType>(1);
  border.style_right = static_cast<lynx::starlight::BorderStyleType>(2);
  border.style_bottom = static_cast<lynx::starlight::BorderStyleType>(3);
  border.style_left = static_cast<lynx::starlight::BorderStyleType>(4);

  lynx::tasm::RoundedRectangle clip_rect;
  clip_rect.SetX(21.25f);
  clip_rect.SetY(22.5f);
  clip_rect.SetWidth(23.75f);
  clip_rect.SetHeight(24.5f);
  clip_rect.SetRadiusXTopLeft(1.f);
  clip_rect.SetRadiusYTopLeft(2.f);
  clip_rect.SetRadiusXTopRight(3.f);
  clip_rect.SetRadiusYTopRight(4.f);
  clip_rect.SetRadiusXBottomRight(5.f);
  clip_rect.SetRadiusYBottomRight(6.f);
  clip_rect.SetRadiusXBottomLeft(7.f);
  clip_rect.SetRadiusYBottomLeft(8.f);

  lynx::tasm::RoundedRectangle record_box;
  record_box.SetX(31.25f);
  record_box.SetY(32.5f);
  record_box.SetWidth(33.75f);
  record_box.SetHeight(34.5f);
  record_box.SetRadiusXTopLeft(9.f);
  record_box.SetRadiusYTopLeft(10.f);
  record_box.SetRadiusXTopRight(11.f);
  record_box.SetRadiusYTopRight(12.f);
  record_box.SetRadiusXBottomRight(13.f);
  record_box.SetRadiusYBottomRight(14.f);
  record_box.SetRadiusXBottomLeft(15.f);
  record_box.SetRadiusYBottomLeft(16.f);

  lynx::base::Vector<uint32_t> colors;
  colors.push_back(0xFF414243);
  colors.push_back(0xFF515253);
  lynx::base::Vector<float> stops;
  stops.push_back(0.25f);
  stops.push_back(0.75f);

  int32_t record_box_index = -1;
  builder.Begin(101, PlatformRendererType::kView, 1.25f, 2.5f, 300.75f, 400.5f)
      .Fill(0xFFA1B2C3, 7)
      .DrawView(202, 3.25f, -4.5f)
      .DrawImage(fml::MakeRefCounted<lynx::tasm::PaintImage>(303), 8)
      .DrawText(404, 9)
      .BackgroundImage(fml::MakeRefCounted<lynx::tasm::PaintImage>(505), 10, 11,
                       1, 2)
      .Border(12, 13, border)
      .ClipRect(clip_rect)
      .RecordBoxModel(record_box, record_box_index)
      .LinearGradient(123.5f, colors, stops, 14, 15, 1, 0)
      .BoxShadow(16, 17, 0xCC616263, 18.5f,
                 lynx::tasm::DisplayListBuilder::BoxShadowClipMode::kInset)
      .RadialGradient(19.5f, 20.5f, 21.5f, 22.5f, colors, stops, 18, 19, 0, 1)
      .End();

  return builder.Build();
}

const lynx::tasm::DisplayList& GetDisplayListFixture() {
  static lynx::tasm::DisplayList fixture = CreateDisplayListFixture();
  return fixture;
}

}  // namespace

extern "C" JNIEXPORT bool JNICALL
Java_com_lynx_tasm_behavior_render_DisplayListItemBufferTest_registerJNI(
    JNIEnv* env, jclass /*jclazz*/) {
  return RegisterNativesImpl(env);
}

jobject CreateDisplayListItemsBuffer(JNIEnv* env, jobject /*jcaller*/) {
  const lynx::tasm::DisplayList& display_list = GetDisplayListFixture();
  const uint8_t* data = display_list.GetContentItemsData();
  return env->NewDirectByteBuffer(
      const_cast<uint8_t*>(data),
      static_cast<jlong>(display_list.GetContentItemsByteSize()));
}

jobject CreateDisplayListDataBuffer(JNIEnv* env, jobject /*jcaller*/) {
  const lynx::tasm::DisplayList& display_list = GetDisplayListFixture();
  const uint8_t* data = display_list.GetContentData();
  return env->NewDirectByteBuffer(
      const_cast<uint8_t*>(data),
      static_cast<jlong>(display_list.GetContentDataSize()));
}

jint GetDisplayListItemSize(JNIEnv* /*env*/, jobject /*jcaller*/) {
  return static_cast<jint>(sizeof(lynx::tasm::DisplayListItem));
}

jint GetDisplayListItemCount(JNIEnv* /*env*/, jobject /*jcaller*/) {
  return static_cast<jint>(GetDisplayListFixture().GetContentItemsSize());
}
