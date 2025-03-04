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
  if (id > kPropertyStart && id < kPropertyEnd) {
    if (void (*const writer)(PropBundle*, CSSPropertyID,
                             starlight::ComputedCSSStyle*) = kWriter[id]) {
      (*writer)(bundle, id, style);
      return;
    }
  }
  LynxWarning(
      false, error::E_CSS_COMPUTED_CSS_VALUE_UNKNOWN_SETTER,
      "PropBundleStyleWriter can't find writer function for style id:%d.", id);
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
