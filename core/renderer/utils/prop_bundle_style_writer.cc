// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/prop_bundle_style_writer.h"

#include "core/build/gen/lynx_sub_error_code.h"
#include "core/public/prop_bundle.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

// This class is used to push different values from style module. Now, all
// properties are lepus::Value got from ComputedCSSValue. So we use a
// DefaultWriterFunc to handle it. We are going to make it cleaner by
// refactoring values to property specified values (e.g. ColorValue,
// GradientValue, ImageValue, etc.). Once a specified value is ready, we'll add
// corresponding writer here.

void PropBundleStyleWriter::PushStyleToBundle(
    PropBundle* bundle, CSSPropertyID id, starlight::ComputedCSSStyle* style) {
  // Add settings here. Decide whether to use DefaultWriterFunc or
  // SpecificWriter based on the setting, and also perform a performance
  // comparison through the setting.

  static bool kUseSpecificWriter = LynxEnv::GetInstance().GetBoolEnv(
      lynx::tasm::LynxEnv::Key::OPT_PUSH_STYLE_TO_BUNDLE, false);

  if (id > kPropertyStart && id < kPropertyEnd) {
    if (void (*const writer)(PropBundle*, starlight::ComputedCSSStyle*) =
            GetWriter()[id]) {
      if (kUseSpecificWriter) {
        (*writer)(bundle, style);
      } else {
        DefaultWriterFunc(bundle, id, style);
      }
      return;
    }
  }
  LynxWarning(
      false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
      "PropBundleStyleWriter can't find writer function for style id:%d.", id);
}

#define FOREACH_NEED_OPT_PROPERTY(V)     \
  V(BackgroundImage)                     \
  V(BackgroundPosition)                  \
  V(BackgroundSize)                      \
  V(MaskImage)                           \
  V(MaskSize)                            \
  V(MaskPosition)                        \
  V(Filter)                              \
  V(Transform)                           \
  V(TransformOrigin)                     \
  V(Animation)                           \
  V(AnimationName)                       \
  V(AnimationTimingFunction)             \
  V(LayoutAnimationCreateTimingFunction) \
  V(LayoutAnimationDeleteTimingFunction) \
  V(LayoutAnimationUpdateTimingFunction) \
  V(Transition)                          \
  V(EnterTransitionName)                 \
  V(ExitTransitionName)                  \
  V(PauseTransitionName)                 \
  V(ResumeTransitionName)                \
  V(BoxShadow)                           \
  V(TextDecoration)                      \
  V(TextShadow)                          \
  V(VerticalAlign)                       \
  V(BorderRadius)                        \
  V(BorderTopLeftRadius)                 \
  V(BorderTopRightRadius)                \
  V(BorderBottomRightRadius)             \
  V(BorderBottomLeftRadius)              \
  V(Perspective)                         \
  V(TextIndent)                          \
  V(XAutoFontSize)                       \
  V(XAutoFontSizePresetSizes)

#define NAME_TO_LEPUS(name) name##ToLepus
#define WRITE_STYLE_IMPL(name)                                               \
  void PropBundleStyleWriter::Write##name(                                   \
      PropBundle* bundle, starlight::ComputedCSSStyle* style) {              \
    bundle->SetPropsByID(CSSPropertyID::kPropertyID##name,                   \
                         pub::ValueImplLepus(style->NAME_TO_LEPUS(name)())); \
  }
FOREACH_NEED_OPT_PROPERTY(WRITE_STYLE_IMPL)
#undef WRITE_STYLE_IMPL
#undef NAME_TO_LEPUS

// TODO(songshourui.null): the following methods should directly call specific
// get methods of ComputedCSSStyle to obtain the value of CSSPropertyID, pushing
// it directly to PropBundle, instead of calling DefaultWriterFunc. This will
// optimize performance.
void PropBundleStyleWriter::WriteOpacity(PropBundle* bundle,
                                         starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOpacity, style);
}

void PropBundleStyleWriter::WritePosition(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDPosition, style);
}

void PropBundleStyleWriter::WriteOverflow(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOverflow, style);
}

void PropBundleStyleWriter::WriteOverflowX(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOverflowX, style);
}

void PropBundleStyleWriter::WriteOverflowY(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOverflowY, style);
}

void PropBundleStyleWriter::WriteFontSize(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDFontSize, style);
}

void PropBundleStyleWriter::WriteLineHeight(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDLineHeight, style);
}

void PropBundleStyleWriter::WriteLetterSpacing(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDLetterSpacing, style);
}

void PropBundleStyleWriter::WriteLineSpacing(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDLineSpacing, style);
}

void PropBundleStyleWriter::WriteColor(PropBundle* bundle,
                                       starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDColor, style);
}

void PropBundleStyleWriter::WriteBackground(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBackground, style);
}

void PropBundleStyleWriter::WriteBackgroundClip(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBackgroundClip, style);
}

void PropBundleStyleWriter::WriteBackgroundColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBackgroundColor, style);
}

void PropBundleStyleWriter::WriteBackgroundOrigin(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBackgroundOrigin, style);
}

void PropBundleStyleWriter::WriteBackgroundRepeat(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBackgroundRepeat, style);
}

void PropBundleStyleWriter::WriteMaskOrigin(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDMaskOrigin, style);
}

void PropBundleStyleWriter::WriteMaskClip(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDMaskClip, style);
}

void PropBundleStyleWriter::WriteMaskRepeat(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDMaskRepeat, style);
}

void PropBundleStyleWriter::WriteBorderLeftColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderLeftColor, style);
}

void PropBundleStyleWriter::WriteBorderRightColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderRightColor, style);
}

void PropBundleStyleWriter::WriteBorderTopColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderTopColor, style);
}

void PropBundleStyleWriter::WriteBorderBottomColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderBottomColor, style);
}

void PropBundleStyleWriter::WriteBorderLeftWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderLeftColor, style);
}

void PropBundleStyleWriter::WriteBorderRightWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderRightWidth, style);
}

void PropBundleStyleWriter::WriteBorderTopWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderTopWidth, style);
}

void PropBundleStyleWriter::WriteBorderBottomWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderBottomWidth, style);
}

void PropBundleStyleWriter::WriteAnimationDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationDuration, style);
}

void PropBundleStyleWriter::WriteAnimationDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationDelay, style);
}

void PropBundleStyleWriter::WriteAnimationIterationCount(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationIterationCount,
                    style);
}

void PropBundleStyleWriter::WriteAnimationDirection(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationDirection,
                    style);
}

void PropBundleStyleWriter::WriteAnimationFillMode(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationFillMode, style);
}

void PropBundleStyleWriter::WriteAnimationPlayState(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDAnimationPlayState,
                    style);
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationCreateDuration, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationCreateDelay, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationCreateProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationCreateProperty, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationDeleteDuration, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationDeleteDelay, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationDeleteProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationDeleteProperty, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationUpdateDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationUpdateDuration, style);
}

void PropBundleStyleWriter::WriteLayoutAnimationUpdateDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(
      bundle, CSSPropertyID::kPropertyIDLayoutAnimationUpdateDelay, style);
}

void PropBundleStyleWriter::WriteTransitionProperty(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTransitionProperty,
                    style);
}

void PropBundleStyleWriter::WriteTransitionDuration(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTransitionDuration,
                    style);
}

void PropBundleStyleWriter::WriteTransitionDelay(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTransitionDelay, style);
}

void PropBundleStyleWriter::WriteTransitionTimingFunction(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTransitionTimingFunction,
                    style);
}

void PropBundleStyleWriter::WriteVisibility(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDVisibility, style);
}

void PropBundleStyleWriter::WriteBorderLeftStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderLeftStyle, style);
}

void PropBundleStyleWriter::WriteBorderRightStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderRightStyle, style);
}

void PropBundleStyleWriter::WriteBorderTopStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderTopStyle, style);
}

void PropBundleStyleWriter::WriteBorderBottomStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderBottomStyle, style);
}

void PropBundleStyleWriter::WriteOutlineColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOutlineColor, style);
}

void PropBundleStyleWriter::WriteOutlineStyle(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOutlineStyle, style);
}

void PropBundleStyleWriter::WriteOutlineWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDOutlineWidth, style);
}

void PropBundleStyleWriter::WriteBorderColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDBorderColor, style);
}

void PropBundleStyleWriter::WriteFontFamily(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDFontFamily, style);
}

void PropBundleStyleWriter::WriteCaretColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDCaretColor, style);
}

void PropBundleStyleWriter::WriteDirection(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDDirection, style);
}

void PropBundleStyleWriter::WriteWhiteSpace(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDWhiteSpace, style);
}

void PropBundleStyleWriter::WriteFontWeight(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDFontWeight, style);
}

void PropBundleStyleWriter::WriteWordBreak(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDWordBreak, style);
}

void PropBundleStyleWriter::WriteFontStyle(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDFontStyle, style);
}

void PropBundleStyleWriter::WriteTextAlign(PropBundle* bundle,
                                           starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextAlign, style);
}

void PropBundleStyleWriter::WriteTextOverflow(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextOverflow, style);
}

void PropBundleStyleWriter::WriteTextDecorationColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextDecorationColor,
                    style);
}

void PropBundleStyleWriter::WriteZIndex(PropBundle* bundle,
                                        starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDZIndex, style);
}

void PropBundleStyleWriter::WriteImageRendering(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDImageRendering, style);
}

void PropBundleStyleWriter::WriteListMainAxisGap(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDListMainAxisGap, style);
}

void PropBundleStyleWriter::WriteListCrossAxisGap(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDListCrossAxisGap, style);
}

void PropBundleStyleWriter::WriteCursor(PropBundle* bundle,
                                        starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDCursor, style);
}

void PropBundleStyleWriter::WriteClipPath(PropBundle* bundle,
                                          starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDClipPath, style);
}

void PropBundleStyleWriter::WriteOffsetPath(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  auto path = style->GetOffsetPath();
  if (path.get()) {
    bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetPath,
                         pub::ValueImplLepus(lepus::Value(path)));
  } else {
    bundle->SetNullPropsByID(CSSPropertyID::kPropertyIDOffsetPath);
  }
}

void PropBundleStyleWriter::WriteOffsetDistance(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetDistance,
                       style->GetOffsetDistance());
}

void PropBundleStyleWriter::WriteOffsetRotate(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  bundle->SetPropsByID(CSSPropertyID::kPropertyIDOffsetRotate,
                       style->GetOffsetRotate());
}

void PropBundleStyleWriter::WriteTextStroke(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextStroke, style);
}

void PropBundleStyleWriter::WriteTextStrokeWidth(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextStrokeWidth, style);
}

void PropBundleStyleWriter::WriteTextStrokeColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDTextStrokeColor, style);
}

void PropBundleStyleWriter::WriteHyphens(PropBundle* bundle,
                                         starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDHyphens, style);
}

void PropBundleStyleWriter::WriteXAppRegion(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDXAppRegion, style);
}

void PropBundleStyleWriter::WriteXHandleSize(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDXHandleSize, style);
}

void PropBundleStyleWriter::WriteXHandleColor(
    PropBundle* bundle, starlight::ComputedCSSStyle* style) {
  DefaultWriterFunc(bundle, CSSPropertyID::kPropertyIDXHandleColor, style);
}

const std::array<PropBundleStyleWriter::WriterFunc, kPropertyEnd>&
PropBundleStyleWriter::GetWriter() {
  static constexpr std::array<WriterFunc, kPropertyEnd> kSpecificWriter = [] {
    std::array<WriterFunc, kPropertyEnd> writer = {nullptr};
#define SET_STYLE_WRITER(name) writer[kPropertyID##name] = &Write##name;
    FOREACH_PLATFORM_PROPERTY(SET_STYLE_WRITER);
#undef SET_STYLE_WRITER
    return writer;
  }();

  return kSpecificWriter;
}

void PropBundleStyleWriter::DefaultWriterFunc(
    PropBundle* bundle, CSSPropertyID id, starlight::ComputedCSSStyle* style) {
  switch (const lepus_value style_value = style->GetValue(id);
          style_value.Type()) {
    case lepus::Value_Int32:
    case lepus::Value_Int64:
      bundle->SetPropsByID(id, static_cast<int>(style_value.Number()));
      break;
    case lepus::Value_UInt32:
    case lepus::Value_UInt64:
      bundle->SetPropsByID(id, static_cast<unsigned int>(style_value.Number()));
      break;
    case lepus::Value_Double:
      bundle->SetPropsByID(id, style_value.Number());
      break;
    case lepus::Value_Bool:
      bundle->SetPropsByID(id, style_value.Bool());
      break;
    case lepus::Value_String:
      bundle->SetPropsByID(id, style_value.CString());
      break;
    case lepus::Value_Array:
    case lepus::Value_Table:
      bundle->SetPropsByID(id, pub::ValueImplLepus(style_value));
      break;
    case lepus::Value_Nil:
      bundle->SetNullPropsByID(id);
      break;
    default:
      LynxWarning(false, error::E_CSS, "ResolveStyleValue");
      break;
  }
}
}  // namespace tasm
}  // namespace lynx
