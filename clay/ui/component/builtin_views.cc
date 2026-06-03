// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/component/builtin_views.h"

#include "clay/ui/component/component.h"

#ifndef ENABLE_CLAY_LITE
#include "clay/ui/component/editable/input_ng_view.h"
#include "clay/ui/component/editable/input_view.h"
#include "clay/ui/component/editable/textarea_ng_view.h"
#include "clay/ui/component/editable/textarea_view.h"
#include "clay/ui/shadow/editable_shadow_node.h"
#endif  // ENABLE_CLAY_LITE

#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
#include "clay/ui/component/list/list_wrapper.h"
#endif  // LYNX_ENABLE_CLAY_NATIVE_LIST

#include "clay/ui/component/image_view.h"
#include "clay/ui/component/list/list_container/list_container_wrapper.h"
#include "clay/ui/component/list/list_item_view.h"
#include "clay/ui/component/scroll_wrapper.h"
#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/component/view_registry.h"
#include "clay/ui/shadow/image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"

#if (defined(OS_MAC) || defined(OS_WIN))
#include "clay/ui/component/title_bar_view.h"
#endif

namespace clay {

void keepBuiltinElements() {}

REGISTER_CLAY_ELEMENT("view", View, void);
REGISTER_CLAY_ELEMENT("image", ImageView, ImageShadowNode);
REGISTER_CLAY_ELEMENT("text", TextView, TextShadowNode);
REGISTER_CLAY_ELEMENT("x-text", TextView, TextShadowNode);
REGISTER_CLAY_ELEMENT("raw-text", RawTextView, RawTextShadowNode);
REGISTER_CLAY_ELEMENT("inline-text", InlineTextView, InlineTextShadowNode);
REGISTER_CLAY_ELEMENT("x-inline-text", InlineTextView, InlineTextShadowNode);
REGISTER_CLAY_ELEMENT("inline-image", InlineImageView, InlineImageShadowNode);
REGISTER_CLAY_ELEMENT("x-inline-image", InlineImageView, InlineImageShadowNode);
REGISTER_CLAY_ELEMENT("inline-view", View, void);
REGISTER_CLAY_ELEMENT("inline-truncation", View, InlineTruncationShadowNode);
REGISTER_CLAY_ELEMENT("x-inline-truncation", View, InlineTruncationShadowNode);
REGISTER_CLAY_ELEMENT("scroll-view", ScrollWrapper, void);
REGISTER_CLAY_ELEMENT("x-scroll-view", ScrollWrapper, void);
REGISTER_CLAY_ELEMENT("component", Component, void);
REGISTER_CLAY_ELEMENT("list-item", ListItemView, void);
#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
REGISTER_CLAY_ELEMENT("list", ListWrapper, void);
#else
REGISTER_CLAY_ELEMENT("list", ListContainerWrapper, void);
#endif
REGISTER_CLAY_ELEMENT("list-container", ListContainerWrapper, void);

#ifndef ENABLE_CLAY_LITE
REGISTER_CLAY_ELEMENT("x-input-ng", InputNGView, EditableShadowNode);
REGISTER_CLAY_ELEMENT("x-textarea-ng", TextAreaNGView, EditableShadowNode);
REGISTER_CLAY_ELEMENT("x-textarea", TextAreaView, EditableShadowNode);
REGISTER_CLAY_ELEMENT("textarea", TextAreaView, EditableShadowNode);
REGISTER_CLAY_ELEMENT("input", InputView, EditableShadowNode);
REGISTER_CLAY_ELEMENT("x-input", InputView, EditableShadowNode);
#endif  // ENABLE_CLAY_LITE

#if (defined(OS_MAC) || defined(OS_WIN))
REGISTER_CLAY_ELEMENT("title-bar-view", TitleBarView, void);
#endif
}  // namespace clay
