// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_TEXT_LAYOUT_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_TEXT_LAYOUT_ANDROID_H_

#include <jni.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/text_layout_impl.h"
#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace tasm {
class PropArrayAndroid;
class FiberElement;
class TextElement;
class ViewElement;
class ImageElement;

class TextLayoutAndroid : public TextLayoutImpl {
 public:
  TextLayoutAndroid(JNIEnv* env, jobject text_layout);
  ~TextLayoutAndroid() override;

  LayoutResult Measure(Element* element, float width, int width_mode,
                       float height, int height_mode) override;

  void Align(Element* element) override;

  void DispatchLayoutBefore(Element* element) override;

 private:
  void BuildTextPropsBuffer(TextElement* element, std::string& output,
                            size_t& current_length, bool use_utf16,
                            PropArrayAndroid* props, bool* has_inline_view);
  void ProcessChildProps(FiberElement* element, std::string& output,
                         size_t& current_length, bool use_utf16,
                         PropArrayAndroid* props, bool* has_inline_view);
  static void AppendTextProps(TextElement* element, size_t pos_start,
                              size_t pos_end, PropArrayAndroid* props);
  static void AppendViewProps(ViewElement* view_element, size_t start,
                              size_t end, PropArrayAndroid* props);
  static void AppendImageProps(ImageElement* image_element, size_t start,
                               size_t end, PropArrayAndroid* props);
  void MeasureInlineViewRecursively(Element* element,
                                    const starlight::Constraints& constraints,
                                    std::vector<float>& layout_result);
  void AlignChildrenRecursively(
      Element* element,
      const std::unordered_map<int, std::pair<float, float>>& result_map);

  base::android::ScopedWeakGlobalJavaRef<jobject> text_layout_;
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_LAYOUT_ANDROID_TEXT_LAYOUT_ANDROID_H_
