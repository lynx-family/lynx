// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * The Lynx config to set.
 *
 * @public
 */

export interface Config {
  /**
   * Prevent the long press event from being triggered during inertial scrolling.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  disableLongpressAfterScroll?: boolean;

  /**
   * Controls whether Android turns on the helper-based Lynx accessibility path for the page. When enabled, LynxAccessibilityWrapper initializes the accessibility helper and a11y-id based lookup path; when disabled, runtime falls back to the default or delegate-based accessibility behavior.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableA11y?: boolean;

  /**
   * Controls the default accessibility-element status for Android Lynx views. When enabled, views whose own a11y status is default inherit important-for-accessibility behavior from page config; when disabled, those views stay out of the default accessibility tree unless explicitly marked.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableAccessibilityElement?: boolean;

  /**
   * Controls whether the style system applies the CSS inheritance path during style propagation. When enabled, Fiber style propagation and inherited-property updates respect CSS inheritance; when disabled, inherited style propagation stays off.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableCSSInheritance?: boolean;

  /**
   * Controls whether iOS multi-finger touch handling ends only after the last finger is lifted. When enabled, the touch recognizer reports ended or cancelled when the final finger leaves; when disabled, the legacy earlier-ending behavior remains.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableEndGestureAtLastFingerUp?: boolean;

  /**
   * Controls whether JS binding APIs surface binding failures as JS exceptions. When enabled, the runtime bundle turns on throw-exception behavior for JSI bindings; when disabled, bindings keep the older non-throwing behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableJsBindingApiThrowException?: boolean;

  /**
   * Controls whether list nodes are created through the newer Radon diff list architecture. When enabled, renderer functions can build RadonDiffListNode2 and decoder falls back to the settings value if the page omits it; when disabled, runtime keeps the older list architecture.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableListNewArchitecture?: boolean;

  /**
   * Controls whether list rendering prefers the native C++ list implementation instead of the older platform list path. When enabled, list elements resolve to native-list mode in shell or page config; when disabled, runtime keeps the legacy platform implementation. When unset, native config can still decide the flag.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableNativeList?: boolean;

  /**
   * Controls whether text and layout clipping use the newer clip-mode behavior. When enabled, text layout specs and UI context use the new clip mode; when disabled, clipping stays on the legacy behavior.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewClipMode?: boolean;

  /**
   * Controls whether runtime enables the new gesture arena and handler integration instead of the legacy touch-only gesture path. When enabled, platform UI owners initialize new gesture handlers and Radon or Fiber gesture updates take the new path; when disabled, scrolling and touch handling keep the legacy behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableNewGesture?: boolean;

  /**
   * Controls whether intersection observers use the newer detection logic instead of the older scroll-bound path. When enabled, intersection managers add observers to the dedicated run loop and observe without depending on scroll-event binding; when disabled, runtime keeps the legacy observer logic. When unset, settings can still decide the flag.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewIntersectionObserver?: boolean;

  /**
   * Controls whether iOS uses the newer transform-origin calculation for transforms and background positioning. When enabled, `LynxUI`, `LynxConverter+Transform`, and background handling use the new origin algorithm; when disabled, iOS keeps the legacy transform-origin math. From target SDK 2.6 onward, the default override is enabled.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewTransformOrigin?: boolean;

  /**
   * Controls whether TTML init data passed to JS is reduced to `Object.keys(data)` instead of cloning the full object. When enabled, `PageProxy::ProcessInitDataForJS()` sends only non-nil keys for non-React pages; when disabled, JS receives the full init-data payload.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReduceInitDataCopy?: boolean;

  /**
   * Controls whether component data strips `__globalProps` and `SystemInfo` before component render and updates. When enabled, decoder and runtime omit those extra values from component data for `RadonPage` and `RadonComponent`; when disabled, components keep the older extra-data payload.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableRemoveComponentExtraData?: boolean;

  /**
   * Controls whether platform text renderers treat text overflow as visible instead of clipping to bounds. When enabled, page config forwards the flag to platform text contexts and iOS or Harmony text rendering allows visible overflow; when disabled, text keeps the older clipped overflow behavior. From target SDK 2.8 onward, the default override is enabled.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextOverflow?: boolean;

  /**
   * Controls whether text measurement and rendering use the refactored text path that is closer to web behavior. When enabled, text layout specs, baseline handling, and rich-text processing use the refactored rules; when disabled, text keeps the legacy Lynx behavior.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextRefactor?: boolean;

  /**
   * The outer decor view that wraps lynx view may change due to that virtual navigation bar is shielded or drawn. Change the returning value of keyboard event to return absolute keyboard height and the offset from keyboard to to lynx view bottom
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  keyboardCallbackPassRelativeHeight?: boolean;

  /**
   * Controls whether descendant selectors are allowed to cross component scope boundaries during style resolution. When enabled, `AttributeHolder` and `StyleResolver` stop constraining descendant selectors to the current component scope; when disabled, descendant selectors only match inside the component scope. In Fiber, manual decode keeps the default off unless explicitly set.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  removeDescendantSelectorScope?: boolean;
}
