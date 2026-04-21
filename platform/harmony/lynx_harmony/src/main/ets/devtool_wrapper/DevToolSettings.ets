// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';

export class DevToolSettings {
  public static readonly SP_KEY_ENABLE_DEVTOOL: string = 'enable_devtool';
  public static readonly SP_KEY_ENABLE_LOGBOX: string = 'enable_logbox';
  public static readonly SP_KEY_ENABLE_DEBUG_MODE: string = 'enable_debug_mode';
  public static readonly SP_KEY_ENABLE_LAUNCH_RECORD: string = 'enable_launch_record';
  public static readonly SP_KEY_ENABLE_QUICKJS_DEBUG: string = 'enable_quickjs_debug';
  public static readonly SP_KEY_ENABLE_QUICKJS_CACHE: string = 'enable_quickjs_cache';
  public static readonly SP_KEY_ENABLE_DOM_TREE: string = 'enable_dom_tree';
  public static readonly SP_KEY_ENABLE_LONG_PRESS_MENU: string = 'enable_long_press_menu';
  public static readonly SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT: string = 'enable_preview_screen_shot';
  public static readonly SP_KEY_ENABLE_PIXEL_COPY: string = 'enable_pixel_copy';
  public static readonly SP_KEY_ENABLE_PERF_MONITOR_DEBUG: string = 'enable_perf_monitor_debug';
  public static readonly SP_KEY_ENABLE_IGNORE_ERROR_CSS: string = 'error_code_css';

  private static previewScreenshotEnabled: boolean = true;
  private static perfMonitorDebugEnabled: boolean = true;

  public static initialize(): void {
    DevToolSettings.syncToNative(
      DevToolSettings.SP_KEY_ENABLE_DEVTOOL, DevToolSettings.isDevToolEnabled());
    DevToolSettings.syncToNative(
      DevToolSettings.SP_KEY_ENABLE_LOGBOX, DevToolSettings.isLogBoxEnabled());
    DevToolSettings.syncToNative(
      DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG, DevToolSettings.isQuickjsDebugEnabled());
    DevToolSettings.syncToNative(
      DevToolSettings.SP_KEY_ENABLE_DOM_TREE, DevToolSettings.isDOMTreeEnabled());
    DevToolSettings.syncToNative(
      DevToolSettings.SP_KEY_ENABLE_QUICKJS_CACHE, DevToolSettings.isQuickjsCacheEnabled());
  }

  /**
   * Persistence: true
   * Sync to native: true
   * Default: false
   */
  public static isDevToolEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, false);
  }

  public static setDevToolEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, enabled);
    DevToolSettings.syncToNative(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: true
   * Default: true
   */
  public static isLogBoxEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LOGBOX, true);
  }

  public static setLogBoxEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LOGBOX, enabled);
    DevToolSettings.syncToNative(DevToolSettings.SP_KEY_ENABLE_LOGBOX, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: false
   * Default: false
   */
  public static isDebugModeEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DEBUG_MODE, false);
  }

  public static setDebugModeEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DEBUG_MODE, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: false
   * Default: false
   */
  public static isLaunchRecordEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LAUNCH_RECORD, false);
  }

  public static setLaunchRecordEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LAUNCH_RECORD, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: true
   * Default: true
   */
  public static isQuickjsDebugEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG, true);
  }

  public static setQuickjsDebugEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
    DevToolSettings.syncToNative(DevToolSettings.SP_KEY_ENABLE_QUICKJS_DEBUG, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: true
   * Default: true
   */
  public static isQuickjsCacheEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_QUICKJS_CACHE, true);
  }

  public static setQuickjsCacheEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
    DevToolSettings.syncToNative(DevToolSettings.SP_KEY_ENABLE_QUICKJS_CACHE, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: true
   * Default: true
   */
  public static isDOMTreeEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DOM_TREE, true);
  }

  public static setDOMTreeEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_DOM_TREE, enabled);
    DevToolSettings.syncToNative(DevToolSettings.SP_KEY_ENABLE_DOM_TREE, enabled);
  }

  /**
   * Persistence: true
   * Sync to native: false
   * Default: true
   */
  public static isLongPressMenuEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU, true);
  }

  public static setLongPressMenuEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU, enabled);
  }

  /**
   * Persistence: false
   * Sync to native: false
   * Default: true
   */
  public static isPreviewScreenshotEnabled(): boolean {
    return DevToolSettings.previewScreenshotEnabled;
  }

  public static setPreviewScreenshotEnabled(enabled: boolean): void {
    DevToolSettings.previewScreenshotEnabled = enabled;
  }

  /**
   * Persistence: true
   * Sync to native: false
   * Default: false
   */
  public static isPixelCopyEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_PIXEL_COPY, false);
  }

  public static setPixelCopyEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_PIXEL_COPY, enabled);
  }

  /**
   * Persistence: false
   * Sync to native: false
   * Default: true
   */
  public static isPerfMonitorDebugEnabled(): boolean {
    return DevToolSettings.perfMonitorDebugEnabled;
  }

  public static setPerfMonitorDebugEnabled(enabled: boolean): void {
    DevToolSettings.perfMonitorDebugEnabled = enabled;
  }

  /**
   * Persistence: true
   * Sync to native: false
   * Default: false
   */
  public static isCSSErrorIgnored(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_IGNORE_ERROR_CSS, false);
  }

  public static setCSSErrorIgnored(ignored: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_IGNORE_ERROR_CSS, ignored);
  }

  private static setPersistedBoolean(key: string, value: boolean): void {
    lynx.DevToolSettingsHarmony.setPersistedBoolean(key, value);
  }

  private static getPersistedBoolean(key: string, defaultValue: boolean): boolean {
    return lynx.DevToolSettingsHarmony.getPersistedBoolean(key, defaultValue);
  }

  private static syncToNative(key: string, value: boolean): void {
    lynx.DevToolSettingsHarmony.syncBooleanToNative(key, value);
  }
}
