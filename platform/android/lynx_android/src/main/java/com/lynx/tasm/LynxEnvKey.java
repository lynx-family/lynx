// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

public enum LynxEnvKey {
  FORCE_DISABLE_QUICKJS_CACHE("ANDROID_DISABLE_QUICKJS_CODE_CACHE"),
  ENABLE_VSYNC_ALIGNED_FLUSH("enable_vsync_aligned_flush_local"),
  ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC("ENABLE_GLOBAL_FEATURE_SWITCH_STATISTIC"),
  ENABLE_FEATURE_COUNTER("ENABLE_FEATURE_COUNTER"),
  ENABLE_JSB_TIMING("enable_jsb_timing"),
  ENABLE_LONG_TASK_TIMING("enable_long_task_timing"),
  ENABLE_CHECK_ACCESS_FROM_NON_UI_THREAD("enable_check_access_from_non_ui_thread"),
  VSYNC_ALIGNED_FLUSH_EXP_KEY("enable_vsync_aligned_flush"),
  USE_NEW_IMAGE("useNewImage"),
  ENABLE_PIPER_MONITOR("enablePiperMonitor"),
  ENABLE_FLUENCY_TRACE("ENABLE_FLUENCY_TRACE"),
  DISABLE_POST_PROCESSOR("disable_post_processor"),
  ENABLE_IMAGE_MEMORY_REPORT("enable_image_memory_report"),
  ENABLE_COMPONENT_STATISTIC_REPORT("enable_component_statistic_report"),
  ENABLE_TRANSFORM_FOR_POSITION_CALCULATION("enable_transform_for_position_calculation"),
  DEVTOOL_COMPONENT_ATTACH("devtool_component_attach"),
  ENABLE_GENERIC_LYNX_RESOURCE_FETCHER_FONT_KEY("enable_generic_lynx_resource_fetcher_font"),
  ENABLE_VSYNC_ALIGNED_MESSAGE_LOOP_GLOBAL("enable_vsync_aligned_message_loop_global"),
  ENABLE_LAYOUT_ON_BACKGROUND_THREAD("enable_layout_on_background_thread"),
  ENABLE_REPORT_CREATE_ASYNC_TAG("enable_report_create_async_tag"),
  ENABLE_SVG_ASYNC("enable_svg_async"),
  ENABLE_IMAGE_EVENT_REPORT("enable_image_event_report"),

  ENABLE_IMAGE_ASYNC_REDIRECT("enable_image_async_redirect"),

  ENABLE_IMAGE_ASYNC_REDIRECT_ON_CREATE("enable_image_async_redirect_create"),
  ENABLE_IMAGE_ASYNC_REQUEST("enable_image_async_request"),
  ENABLE_GENERIC_RESOURCE_FETCHER("enable_generic_resource_fetcher"),
  ENABLE_TEXT_BORING_LAYOUT("enable_text_boring_layout"),
  ENABLE_LIFECYCLE_TIME_REPORT("enable_lifecycle_time_report");

  private final String description;

  LynxEnvKey(String description) {
    this.description = description;
  }

  public String getDescription() {
    return description;
  }

  public static final String SP_KEY_ENABLE_DEBUG_MODE = "enable_debug_mode";
  public static final String SP_KEY_ENABLE_LAUNCH_RECORD = "enable_launch_record";
  public static final String SP_KEY_FORCE_DISABLE_QUICKJS_CACHE =
      FORCE_DISABLE_QUICKJS_CACHE.getDescription();
  public static final String SP_KEY_DISABLE_LEPUSNG_OPTIMIZE = "DISABLE_LEPUSNG_OPTIMIZE";
  public static final String SP_KEY_ENABLE_VSYNC_ALIGNED_FLUSH =
      ENABLE_VSYNC_ALIGNED_FLUSH.getDescription();

  // Keys for devtool.
  public static final String KEY_LYNX_DEBUG_ENABLED = "lynx_debug_enabled";
  public static final String KEY_DEVTOOL_COMPONENT_ATTACH =
      DEVTOOL_COMPONENT_ATTACH.getDescription();
  public static final String SP_KEY_ENABLE_DEVTOOL = "enable_devtool";
  public static final String SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW =
      "enable_devtool_for_debuggable_view";
  public static final String SP_KEY_ENABLE_LOGBOX = "enable_logbox";
  public static final String SP_KEY_ENABLE_HIGHLIGHT_TOUCH = "enable_highlight_touch";
  @Deprecated public static final String SP_KEY_SHOW_DEVTOOL_BADGE = "show_devtool_badge";
  public static final String SP_KEY_ENABLE_QUICKJS_CACHE = "enable_quickjs_cache";
  public static final String SP_KEY_ENABLE_V8 = "enable_v8";
  public static final String SP_KEY_ENABLE_DOM_TREE = "enable_dom_tree";
  public static final String SP_KEY_ENABLE_LONG_PRESS_MENU = "enable_long_press_menu";
  public static final String SP_KEY_IGNORE_ERROR_TYPES = "ignore_error_types";
  public static final String SP_KEY_ENABLE_IGNORE_ERROR_CSS = "error_code_css";
  public static final String SP_KEY_ENABLE_PIXEL_COPY = "enable_pixel_copy";
  public static final String SP_KEY_DEVTOOL_CONNECTED = "devtool_connected";
  public static final String SP_KEY_ENABLE_QUICKJS_DEBUG = "enable_quickjs_debug";
  public static final String SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT = "enable_preview_screen_shot";
  public static final String SP_KEY_ACTIVATED_CDP_DOMAINS = "activated_cdp_domains";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_DOM = "enable_cdp_domain_dom";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_CSS = "enable_cdp_domain_css";
  public static final String SP_KEY_ENABLE_CDP_DOMAIN_PAGE = "enable_cdp_domain_page";
}
