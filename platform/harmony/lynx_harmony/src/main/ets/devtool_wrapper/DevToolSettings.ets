// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { LLog } from '../base/LLog';

const TAG = 'DevToolSettings';

export class DevToolSettings {
  public static readonly SP_KEY_ENABLE_DEVTOOL: string = 'enable_devtool';
  public static readonly SP_KEY_ENABLE_LOGBOX: string = 'enable_logbox';
  public static readonly SP_KEY_ENABLE_DEBUG_MODE: string = 'enable_debug_mode';
  public static readonly SP_KEY_ENABLE_LAUNCH_RECORD: string = 'enable_launch_record';
  public static readonly SP_KEY_ENABLE_QUICKJS_DEBUG: string = 'enable_quickjs_debug';
  public static readonly SP_KEY_ENABLE_QUICKJS_CACHE: string = 'enable_quickjs_cache';
  public static readonly SP_KEY_ENABLE_DOM_TREE: string = 'enable_dom_tree';
  public static readonly SP_KEY_ENABLE_LONG_PRESS_MENU: string = 'enable_long_press_menu';
  public static readonly SP_KEY_ENABLE_HIGHLIGHT_TOUCH: string = 'enable_highlight_touch';
  public static readonly SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT: string = 'enable_preview_screen_shot';
  public static readonly SP_KEY_ENABLE_PIXEL_COPY: string = 'enable_pixel_copy';
  public static readonly SP_KEY_ENABLE_PERF_MONITOR_DEBUG: string = 'enable_perf_monitor_debug';
  public static readonly SP_KEY_ENABLE_IGNORE_ERROR_CSS: string = 'error_code_css';

  // CDP domain keys use the format "enable_cdp_domain_{domain_name}".
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_CSS: string = 'enable_cdp_domain_css';
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_DEBUGGER: string = 'enable_cdp_domain_debugger';
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_DOM: string = 'enable_cdp_domain_dom';
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_OVERLAY: string = 'enable_cdp_domain_overlay';
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_PAGE: string = 'enable_cdp_domain_page';
  public static readonly SP_KEY_ENABLE_CDP_DOMAIN_RUNTIME: string = 'enable_cdp_domain_runtime';

  private static readonly SP_KEY_ACTIVATED_CDP_DOMAINS: string = 'activated_cdp_domains';
  private static readonly CDP_DOMAIN_KEY_PREFIX: string = 'enable_cdp_domain_';

  private static highlightTouchEnabled: boolean = false;
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
    DevToolSettings.syncEnabledCDPDomainsToNative(DevToolSettings.getEnabledCDPDomains());
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
   * Default: false
   */
  public static isLongPressMenuEnabled(): boolean {
    return DevToolSettings.getPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU, false);
  }

  public static setLongPressMenuEnabled(enabled: boolean): void {
    DevToolSettings.setPersistedBoolean(DevToolSettings.SP_KEY_ENABLE_LONG_PRESS_MENU, enabled);
  }

  /**
   * Persistence: false
   * Sync to native: false
   * Default: false
   */
  public static isHighlightTouchEnabled(): boolean {
    return DevToolSettings.highlightTouchEnabled;
  }

  public static setHighlightTouchEnabled(enabled: boolean): void {
    DevToolSettings.highlightTouchEnabled = enabled;
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

  /**
   * Persistence: true
   * Sync to native: true
   * Default: empty set
   */
  public static getEnabledCDPDomains(): Set<string> {
    const raw = lynx.DevToolSettingsHarmony.getPersistedString(DevToolSettings.SP_KEY_ACTIVATED_CDP_DOMAINS, '');
    if (raw.length === 0) {
      return new Set<string>();
    }
    try {
      return new Set<string>(JSON.parse(raw) as string[]);
    } catch (e) {
      LLog.e(TAG, `Failed to parse enabled CDP domains: ${raw}`);
      return new Set<string>();
    }
  }

  /**
   * Replaces the enabled CDP domain set. Duplicate entries are ignored. If any
   * key is invalid, the whole update is rejected.
   */
  public static setEnabledCDPDomains(domains: Set<string> | string[]): void {
    // Take a stable snapshot before validation, persistence, and native sync.
    const domainSnapshot: string[] = Array.from(domains);
    for (const key of domainSnapshot) {
      if (!DevToolSettings.verifyCDPDomainKey(key)) {
        return;
      }
    }
    DevToolSettings.persistAndSyncEnabledCDPDomains(new Set<string>(domainSnapshot));
  }

  public static isCDPDomainEnabled(key: string): boolean {
    if (!DevToolSettings.verifyCDPDomainKey(key)) {
      return false;
    }
    return DevToolSettings.getEnabledCDPDomains().has(key);
  }

  public static setCDPDomainEnabled(key: string, enabled: boolean): void {
    if (!DevToolSettings.verifyCDPDomainKey(key)) {
      return;
    }
    const enabledDomains = DevToolSettings.getEnabledCDPDomains();
    if (enabledDomains.has(key) === enabled) {
      return;
    }
    if (enabled) {
      enabledDomains.add(key);
    } else {
      enabledDomains.delete(key);
    }
    DevToolSettings.persistAndSyncEnabledCDPDomains(enabledDomains);
  }

  private static verifyCDPDomainKey(key: string): boolean {
    if (!key.startsWith(DevToolSettings.CDP_DOMAIN_KEY_PREFIX)) {
      LLog.e(TAG, `Invalid CDP domain key: ${key}`);
      return false;
    }
    return true;
  }

  private static persistAndSyncEnabledCDPDomains(domains: Set<string>): void {
    lynx.DevToolSettingsHarmony.setPersistedString(
      DevToolSettings.SP_KEY_ACTIVATED_CDP_DOMAINS, JSON.stringify(Array.from(domains)));
    DevToolSettings.syncEnabledCDPDomainsToNative(domains);
  }

  private static syncEnabledCDPDomainsToNative(domains: Set<string>): void {
    lynx.DevToolSettingsHarmony.syncStringSetToNative(
      DevToolSettings.SP_KEY_ACTIVATED_CDP_DOMAINS, Array.from(domains));
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
