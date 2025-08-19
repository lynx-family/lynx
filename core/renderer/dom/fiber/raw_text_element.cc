// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/raw_text_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/text_props.h"

namespace lynx {
namespace tasm {

RawTextElement::RawTextElement(ElementManager* manager)
    : FiberElement(manager, BASE_STATIC_STRING(kRawTextTag)) {}

void RawTextElement::SetText(const lepus::Value& text) {
  if (!EnableLayoutInElementMode()) {
    SetAttribute(BASE_STATIC_STRING(kTextAttr), text);
  } else {
    if (text.IsString()) {
      content_ = text.String();
    } else if (text.IsNumber()) {
      std::stringstream stream;
      stream << text.Number();
      content_ = stream.str();
    }
    content_utf16_length_ =
        GetUtf16SizeFromUtf8(content_.c_str(), content_.length());
    MarkLayoutDirty();
  }
}

void RawTextElement::SetAttributeInternal(const base::String& key,
                                          const lepus::Value& value) {
  if (EnableLayoutInElementMode()) {
    // TODO(songshourui.null): we may need other attributes here.
    if (key.IsEqual(kTextAttr)) {
      content_ = value.String();
      return;
    }
  }
  FiberElement::SetAttributeInternal(key, value);
}

ParallelFlushReturn RawTextElement::PrepareForCreateOrUpdate() {
  bool need_update = ConsumeAllAttributes();

  if (need_update && !IsNewlyCreated()) {
    // If text attributes change, we need to force a requestLayout to ensure
    // that Layout is triggered in FlushElementTree.
    RequestLayout();
  }

  PerformElementContainerCreateOrUpdate(need_update);

  // reset all dirty bits, some bits may never be processed
  ResetAllDirtyBits();

  UpdateLayoutNodeByBundle();

  ResetPropBundle();

  if (ShouldProcessParallelTasks()) {
    return CreateParallelTaskHandler();
  }

  return []() {};
}

}  // namespace tasm
}  // namespace lynx
