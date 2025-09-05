// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export class LynxEnvKey {
  // Keys for devtool.
  static readonly KEY_LYNX_DEBUG_ENABLED: string = 'lynx_debug_enabled';
  static readonly KEY_DEVTOOL_COMPONENT_ATTACH: string = 'devtool_component_attach';
  static readonly SP_KEY_ENABLE_DEVTOOL: string = 'enable_devtool';
  static readonly SP_KEY_ENABLE_LOGBOX: string = 'enable_logbox';
  /** @deprecated */
  static readonly SP_KEY_SHOW_DEVTOOL_BADGE: string = 'show_devtool_badge';
  static readonly SP_KEY_ENABLE_QUICKJS_CACHE: string = 'enable_quickjs_cache';
  static readonly SP_KEY_ENABLE_DOM_TREE: string = 'enable_dom_tree';
  static readonly SP_KEY_ENABLE_LONG_PRESS_MENU: string = 'enable_long_press_menu';
  static readonly SP_KEY_IGNORE_ERROR_TYPES: string = 'ignore_error_types';
  static readonly SP_KEY_ENABLE_IGNORE_ERROR_CSS: string = 'error_code_css';
  static readonly SP_KEY_ENABLE_PIXEL_COPY: string = 'enable_pixel_copy';
  static readonly SP_KEY_DEVTOOL_CONNECTED: string = 'devtool_connected';
  static readonly SP_KEY_ENABLE_QUICKJS_DEBUG: string = 'enable_quickjs_debug';
  static readonly SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT: string = 'enable_preview_screen_shot';
  static readonly SP_KEY_ACTIVATED_CDP_DOMAINS: string = 'activated_cdp_domains';
  static readonly SP_KEY_ENABLE_CDP_DOMAIN_DOM: string = 'enable_cdp_domain_dom';
  static readonly SP_KEY_ENABLE_CDP_DOMAIN_CSS: string = 'enable_cdp_domain_css';
  static readonly SP_KEY_ENABLE_CDP_DOMAIN_PAGE: string = 'enable_cdp_domain_page';
}
