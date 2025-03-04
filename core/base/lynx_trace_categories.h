// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_LYNX_TRACE_CATEGORIES_H_
#define CORE_BASE_LYNX_TRACE_CATEGORIES_H_

static constexpr const char* const LYNX_TRACE_CATEGORY = "lynx";
static constexpr const char* const LYNX_TRACE_CATEGORY_VITALS = "vitals";
static constexpr const char* const LYNX_TRACE_CATEGORY_JAVASCRIPT =
    "javascript";
static constexpr const char* const LYNX_TRACE_CATEGORY_SCREENSHOTS =
    "disabled-by-default-devtools.screenshot";
static constexpr const char* const LYNX_TRACE_CATEGORY_FPS =
    "disabled-by-default-devtools.timeline.frame";
static constexpr const char* const LYNX_TRACE_CATEGORY_DEVTOOL_TIMELINE =
    "disabled-by-default-devtools.timeline";
static constexpr const char* const LYNX_TRACE_CATEGORY_JSB = "jsb";
static constexpr const char* const LYNX_TRACE_CATEGORY_ATRACE = "system";
#endif  // CORE_BASE_LYNX_TRACE_CATEGORIES_H_
