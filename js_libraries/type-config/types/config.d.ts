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
   * Controls iOS long-press recognition after scroll gestures. When enabled, Lynx requires long press to wait for a decelerating scroll view pan recognizer to fail, suppressing long press during inertial scrolling.
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
   * Controls whether runtime treats inline style values as CSS variables and resolves them through the inline-variable path. When enabled, decoded page config turns on inline CSS variable support for runtime CSS readers and style resolution; when disabled, inline style values are handled as ordinary static styles. If the field is omitted, the setting can still be driven by native or settings fallback.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableCSSInlineVariables?: boolean;

  /**
   * Controls unified CSS rule support in template CSS encoding and decoding. When enabled, CSS rules such as style, media, supports, keyframes, font-face, and layer rules are parsed and decoded through the unified rule path.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 4.0
   *
   * @defaultValue false
   */
  enableCSSRule?: boolean;

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
   * Controls whether touches on the root area can pass through the Lynx page instead of being consumed by Lynx. When enabled, root touch dispatch returns false and overlay or host views underneath can receive the event; when disabled, Lynx keeps normal touch consumption.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableEventThrough?: boolean;

  /**
   * Controls standard HTTP streaming support for the Lynx Fetch API. When enabled through page or native config, Fetch requests use the standard streaming response path; when disabled or unset, Fetch keeps the non-standard streaming fallback unless a legacy streaming flag is present.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue undefined
   */
  enableFetchAPIStandardStreaming?: boolean;

  /**
   * When enabled, flex-basis defaults to 0% instead of 0 when omitted in flex shorthand, matching web browser behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableFlexBasisZeroPercent?: boolean;

  /**
   * Controls whether the CSS parser accepts grid-column and grid-row shorthand syntax. When enabled, grid shorthand handlers parse placement shorthands into style values; when disabled, those shorthand declarations are rejected.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.9
   *
   * @defaultValue false
   */
  enableGridPlacementShorthands?: boolean;

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
   * Controls whether Lynx touch events carry multi-finger state instead of collapsing to single-touch behavior. When enabled, touch events include information for multiple active fingers; when disabled, runtime keeps the single-touch event model.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableMultiTouch?: boolean;

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
   * Controls whether iOS Lynx tap gestures can fire at the same time as outer native tap gestures. When enabled, LynxTap and host tap gestures are allowed to recognize together; when disabled, Lynx keeps the default mutual-exclusion behavior.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableSimultaneousTap?: boolean;

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
   * Controls whether touch event coordinates account for element transforms. When enabled, Lynx and Canvas touch dispatch convert root touch points into the transformed target view coordinate space; when disabled, touch positions keep the legacy untransformed calculation.
   *
   * Supported platform: Android, HarmonyOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableTransformedTouchPosition?: boolean;

  /**
   * Controls whether font scale applies only to `sp` units. When enabled, non-sp font-relevant lengths use a font scale of 1 while `sp` values keep font-scale behavior; when disabled, font scale can affect broader text sizing.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  fontScaleEffectiveOnlyOnSp?: boolean;

  /**
   * Controls whether Android text measurement includes the font's top and bottom padding. When enabled, page config forwards `includeFontPadding` to text shadow nodes and text height or vertical centering includes font padding; when disabled, text metrics exclude that padding. When omitted, Android falls back to the historical SDK-based default (`true` for 2.4 <= target SDK < 2.9).
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  includeFontPadding?: boolean;

  /**
   * Controls the Android PageConfig flag returned by `useRelativeKeyboardHeightApi()` for keyboard height callbacks. When enabled, keyboard APIs can report height relative to the Lynx view bottom to handle decor-view offset changes; when disabled, callbacks keep the default height semantics.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  keyboardCallbackPassRelativeHeight?: boolean;

  /**
   * Controls the platform long-press timeout before Lynx fires a `longpress` event. Smaller values make long-press recognition fire sooner, while the default or negative handling keeps the platform timeout behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  longPressDuration?: number;

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

  /**
   * Controls the movement threshold used by platform gesture recognizers before a tap is canceled. When pointer movement exceeds this distance, Android or Harmony tap handling no longer treats the interaction as a tap; smaller movement keeps the tap path eligible.
   *
   * Supported platform: Android, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue "50px"
   */
  tapSlop?: string;

  /**
   * Controls whether `vw` and `vh` values use the unified viewport calculation path across layout and dynamic style updates. When enabled, Fiber elements and dynamic CSS updates recompute viewport units from the current viewport consistently; when disabled, some properties keep the older behavior. From target SDK 2.3 onward, the default override is enabled.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  unifyVWVHBehavior?: boolean;
}
