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
   * When set to true, the containing block of absolute/fixed elements is the content area; otherwise, it is the padding area
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  absoluteInContentBound?: boolean;

  /**
   * Let mouse events have the same semantic with W3C standard
   *
   * Supported platform: macOS, Windows
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  alignMouseEventWithW3C?: boolean;

  /**
   * Controls whether Android image URL redirection is resolved asynchronously before request dispatch. When enabled, image fetchers call asyncRedirectUrl; when disabled, they use the synchronous redirect path.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  asyncRedirect?: boolean;

  /**
   * Controls whether page attach and detach automatically drive page-level `onShow` and `onHide` exposure lifecycle. When enabled, platform template renders trigger show or hide during `attachToView` and `detachFromWindow`; when disabled, those lifecycle callbacks only run through explicit exposure flows.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  autoExpose?: boolean;

  /**
   * When set to true, for compatibility, some layout behaviors are consistent with the previous incorrect behaviors.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  CSSAlignWithLegacyW3C?: boolean;

  /**
   * Custom Inheritable CSS Properties
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  customCSSInheritanceList?: string[];

  /**
   * ReactLynx cannot opt top_level_variables used in lepus. So we cannot forbid updateData when variable not in top_level_variables. User can use this config to close the check.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  dataStrictMode?: boolean;

  /**
   * Debug metadata URL for template diagnostics.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue ""
   */
  debugMetadataUrl?: string;

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
   * Disable tracing gc mode in quick context.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  disableQuickTracingGC?: boolean;

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
   * Controls whether Fiber keeps animation override styles while forward updates apply new styles. When enabled, animation fill or override styles are preserved across forward updates; when disabled, forward updates can drop those animation-preserved styles. When omitted, it falls back to settings or native config.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableAnimationForwardUpdatePreservation?: boolean;

  /**
   * Controls whether Android video engine initialization happens asynchronously instead of blocking the normal page init path. When enabled, video engine startup is deferred to an async init path; when disabled, runtime keeps synchronous initialization.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableAsyncInitVideoEngine?: boolean;

  /**
   * Controls whether Android page config exposes the async image-request scheduling flag to `LynxContext`. When enabled, platform image code can start requests from an async worker path; when disabled, image requests keep the normal request-start flow.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableAsyncRequestImage?: boolean;

  /**
   * FE Framework use this config to notify Engine that resolve subtree binding will be triggered when render DOM (Not exposed to normal user)
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.4
   *
   * @defaultValue undefined
   */
  enableAsyncResolveSubtree?: boolean;

  /**
   * Controls whether Fiber elements batch layout tasks and flush them together during sync layout. When enabled, layout work is enqueued on the element context delegate and flushed at sync-layout boundaries; when disabled, layout tasks run immediately. When omitted, it falls back to settings or native config.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue false
   */
  enableBatchLayoutTaskWithSyncLayout?: boolean;

  /**
   * Controls whether iOS exposure detection skips repeated checks while the page stays static. When enabled, the exposure observer returns early until UI movement marks the page dirty again; when disabled, exposure checks continue on the normal cadence.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableCheckExposureOptimize?: boolean;

  /**
   * Controls whether Android image loading checks local-resource URLs before applying redirect handling. When enabled, page config forwards the local-image check flag to Android image code so local resources can choose the correct redirect path; when disabled, that extra local-resource check is bypassed.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableCheckLocalImage?: boolean;

  /**
   * Controls whether JS runtime checks for circular references when converting JS values into Lynx values. When enabled, `TemplateEntry` forwards the flag to BTS or JSI runtime and circular structures take the guarded conversion path; when disabled, conversions skip that extra circular-data check.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableCircularDataCheck?: boolean;

  /**
   * Controls whether async-loaded dynamic components are decoded on child threads before they are delivered to TASM. When enabled, TemplateAssembler passes the flag to the component loader so async component bundles decode off the main path; when disabled, component decode stays on the normal delivery path. When unset, it can still fall back to LynxEnv.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableComponentAsyncDecode?: boolean;

  /**
   * Controls whether LepusNG component props preserve explicit `null` values instead of dropping them during property application. When enabled, page proxy and `RadonComponent` allow `null` props through the LepusNG component path; when disabled, component props keep the older non-null-only behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableComponentNullProp?: boolean;

  /**
   * Create Android platform UIs in lynx built-in thread-pool to optimize UI Operation Execution
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableCreateViewAsync?: boolean;

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
   * Under scoped CSS, the imported CSS declarations by import rules are lazily decoded at the first time the scope takes effect.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableCSSLazyImport?: boolean;

  /**
   * When enabled, CSS supports unified rules.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 4.0
   *
   * @defaultValue false
   */
  enableCSSRule?: boolean;

  /**
   * Controls whether iOS sends `disexposure` events when the Lynx view enters the background. When enabled, `LynxUIExposure` emits `disexposure` during background transitions; when disabled, backgrounding does not trigger that extra event.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue true
   */
  enableDisexposureWhenBackground?: boolean;

  /**
   * Controls whether Android sends `disexposure` events when `LynxView` becomes hidden without leaving the window. When enabled, `LynxObserverManager` and `UIExposure` emit `disexposure` for views hidden by visibility changes; when disabled, hiding the page does not force that extra event.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableDisexposureWhenLynxHidden?: boolean;

  /**
   * Controls whether custom events can be dispatched directly to a `LynxUI` target instead of only bubbling through the page delegate. When enabled, iOS event dispatch resolves the target UI by sign and calls `dispatchEvent:` on it; when disabled, custom events stay on the normal page-level callback path.
   *
   * Supported platform: iOS, Android
   *
   * Since: LynxSDK 3.9
   *
   * @defaultValue false
   */
  enableDispatchCustomEventForUI?: boolean;

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
   * Controls whether runtime uses the refactored event-listener and dispatch path. When enabled, element decode, Radon event processing, and touch dispatch use the newer listener registration flow that supports rebinding and interception; when disabled, runtime stays on the legacy event hookup path. If the field is omitted, decoder falls back to the settings source.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enableEventHandleRefactor?: boolean;

  /**
   * Include the element nodeIndex field in event target and currentTarget parameters.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableEventTargetInfoNodeIndex?: boolean;

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
   * Controls whether exposure-ui-margin properties expand the area used for exposure calculation. When enabled, exposure detection uses the adjusted rectangle with UI margins; when disabled, exposure checks use the raw UI bounds.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableExposureUIMargin?: boolean;

  /**
   * Controls whether Android runs exposure checks when LynxView requests layout. When enabled, layout requests can trigger exposure detection; when disabled, exposure checks wait for the normal exposure flow.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableExposureWhenLayout?: boolean;

  /**
   * Controls whether reload flows emit exposure or disexposure transitions for the old and new page state. When enabled, Android, iOS, and Harmony reload paths send `disexposure` before reload and can re-run exposure after reload init; when disabled, reload does not trigger that extra exposure cycle.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enableExposureWhenReload?: boolean;

  /**
   * Make the Lynx Fetch-API support standard http streaming
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue undefined
   */
  enableFetchAPIStandardStreaming?: boolean;

  /**
   * A better and stable position fixed handling.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableFixedNew?: boolean;

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
   * Controls whether `<frame>` data and global-props are transferred through native lepus value pointers instead of the legacy platform map path. When enabled, Android and iOS consume the native pointer to build TemplateData; when disabled, frame data and global props continue to be delivered as platform maps.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableFrameNativeData?: boolean;

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
   * Controls whether Harmony overlay components use the newer overlayManager-based overlay path. When enabled, Harmony overlays are shown and removed through overlayManager so pass-through event handling can work; when disabled, overlays keep the legacy path.
   *
   * Supported platform: HarmonyOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableHarmonyNewOverlay?: boolean;

  /**
   * Controls whether Harmony exposure detection also tracks the root visible-area-change signal. When enabled, Harmony root visibility depends on the visible-area-change event before exposure is reported; when disabled, exposure only uses the normal visibility checks.
   *
   * Supported platform: HarmonyOS
   *
   * Since: LynxSDK 3.4
   *
   * @defaultValue false
   */
  enableHarmonyVisibleAreaChangeForExposure?: boolean;

  /**
   * Controls whether the runtime binds PRIMJS ICU support into the JS environment. When enabled, TemplateEntry and BTS runtime install ICU bindings when the context supports them; when disabled, the JS environment runs without those ICU bindings.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableICU?: boolean;

  /**
   * Controls whether iOS image decoding downsamples large images before display. When enabled, `LynxUIImage` and the UI context use downsampling-aware decode paths; when disabled, images keep the normal full-resolution decode path.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableImageDownsampling?: boolean;

  /**
   * Controls whether iOS exposure position calculations include animation-layer geometry instead of only the base view frame. When enabled, exposure detection measures visibility using animation-layer positions; when disabled, it uses the normal layer position.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableiOSAnimationLayerForExposure?: boolean;

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
   * Controls whether Fiber pages run the data-processor flow on the JS thread instead of completing it during normal decode. When enabled, TemplateAssembler posts JS earlier and processes template data through the JS-thread processor path; when disabled, data processing stays on the regular decode side.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableJSDataProcessor?: boolean;

  /**
   * Does diffResult have moveAction
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableListMoveOperation?: boolean;

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
   * Controls whether list rendering enables the list plug path in Radon list platform info. When enabled, page config passes the plug flag into list nodes; when disabled, list nodes run without the plug behavior.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableListPlug?: boolean;

  /**
   * Force report lynx scroll fluency event. When setting pageConfig.enableLynxScrollFluency to a double value in the range [0, 1], we will monitor the fluency metrics for this LynxUI based on this probability. The probability indicates the likelihood of enabling fluency monitoring, and the metrics will be reported unconditionally through the applogService.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableLynxScrollFluency?: boolean | number;

  /**
   * Controls whether the JS runtime installs Lynx's microtask-based Promise polyfill. When enabled, `TemplateEntry`, JS app startup, and `setup-promise.ts` turn on the polyfill path; when disabled, runtime keeps the existing Promise implementation. When unset, native config can still decide the flag.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableMicrotaskPromisePolyfill?: boolean;

  /**
   * Controls whether pooled LepusNG MTS runtimes pre-execute their context before the page claims them. When enabled, MTSRuntimePool initializes debug tooling, deserializes the bundle, and calls `TryExecute()` during pool fill; when disabled, pooled contexts wait until normal runtime use to execute.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue false
   */
  enableMTSPreExecute?: boolean;

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
   * Controls how multi-touch events populate touches and changedTouches when compatibility with single-touch payloads is needed. When enabled, multi-touch cancel and payload generation keep single-touch compatible fields; when disabled, multi-touch events use the newer native multi-touch parameter shape.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableMultiTouchParamsCompatible?: boolean;

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
   * Controls whether element updates use Lynx's new animator path instead of relying on platform animation behavior. When enabled, page config turns on the new animator for element and Fiber animation branches; when disabled, runtime falls back to platform animation handling. When unset, it falls back to `LynxEnv`.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableNewAnimator?: boolean;

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
   * Controls whether image loading uses the newer image-service pipeline. When enabled, the UI context turns on the new image loader; when disabled, image loading stays on the legacy service path.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewImage?: boolean;

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
   * Implement the platform-level list based on scrollView on the IOS platform
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableNewListContainer?: boolean;

  /**
   * Controls whether sticky elements are rendered on the new path. When enabled, sticky elements use the new path; when disabled, they keep the legacy path.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableNewSticky?: boolean;

  /**
   * Controls whether Fiber elements use the new styling resolution pipeline instead of the legacy style resolver path. When enabled, ElementManager turns on the new styling flag and FiberElement switches to `ResolveCSSStylesNewPipeline`; when disabled, style resolution stays on the existing pipeline. If the field is omitted, native or settings fallback can still decide the flag.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.9
   *
   * @defaultValue false
   */
  enableNewStylingPipeline?: boolean;

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
   * Preserve integer values for numeric flex inputs in CSS parser.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   */
  enableParseIntFlex?: boolean;

  /**
   * Controls whether Lynx sets up the platform-gesture bridge for gesture-conflict handling. When enabled, the event handler installs custom platform gestures and delegates; when disabled, that platform-gesture layer is removed.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enablePlatformGesture?: boolean;

  /**
   * SimpleStyle mode will incrementally update styles based on properties if this config set to TRUE. Otherwise it will incrementally update based on StyleObjects. The StyleObject based updating has a better performance but not allow StyleObjects bound to the same element has intersect properties.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enablePropertyBasedSimpleStyle?: boolean;

  /**
   * Controls whether background runtime can query dynamic components synchronously. When enabled, page config passes a synchronous-query flag into TemplateEntryHolder and JS bundle holders; when disabled, component queries stay asynchronous.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableQueryComponentSync?: boolean;

  /**
   * Controls whether React pages pass only `propsId` to the JS thread instead of cloning the full props object. When enabled and `propsId` exists, `PageProxy::ProcessReactPropsForJS()` sends only `propsId` plus the `$$onlyPropsId` marker; when disabled, JS receives the full props payload.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReactOnlyPropsId?: boolean;

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
   * Controls whether reload patching triggers the LynxUI onNodeReload lifecycle on affected elements. When enabled, refresh patching walks reloaded elements and calls onNodeReload; when disabled, reload updates skip that lifecycle callback.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReloadLifecycle?: boolean;

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
   * Controls whether `loadScript` can reuse cached module exports for the same template URL and path. When enabled, TemplateEntry passes the flag into the JS app runtime and repeated `loadScript` calls can reuse prior exports; when disabled, the cache key is not created and each load runs fresh.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enableReuseLoadScriptExports?: boolean;

  /**
   * Controls whether MTS runtimes register Lynx's native Signal API for the page. When true, page config passes the signal flag into runtime creation and MTS runtime registration exposes signal support to the VM; when false, that signal registration is skipped. When unset, it falls back to the native config source.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableSignalAPI?: boolean;

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
   * Controls whether Android text measurement can use the BoringLayout fast path for eligible plain text. When enabled, simple text may use BoringLayout; when disabled, text stays on the general layout path.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableTextBoringLayout?: boolean;

  /**
   * Controls whether iOS text rendering turns on the gradient optimization path. When enabled, LynxUIText records the experiment flag and uses the optimized gradient text path; when disabled, text gradients stay on the legacy rendering flow. When unset, it falls back to external settings or native config.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue undefined
   */
  enableTextGradientOpt?: boolean;

  /**
   * Controls whether iOS rich text chooses natural alignment using the text language aware algorithm. When enabled, attributed text applies language-aware natural alignment; when disabled, it keeps the older alignment heuristic.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextLanguageAlignment?: boolean;

  /**
   * Controls whether iOS text renders through the dedicated text-layer path. When enabled, LynxUIText uses layer-based text rendering; when disabled, text stays on the legacy view-backed rendering path.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableTextLayerRender?: boolean;

  /**
   * Controls whether text layout results can be reused from the text renderer cache. When enabled, text shadow nodes can reuse cached renderers for repeated layout specs; when disabled, text layout is recomputed each time. When unset, platform env can still provide the value.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.3
   *
   * @defaultValue undefined
   */
  enableTextLayoutCache?: boolean;

  /**
   * Controls whether text layout allows noncontiguous layout on Darwin text renderers. When enabled, text renderers set allowsNonContiguousLayout for eligible text; when disabled, layout stays contiguous.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableTextNonContiguousLayout?: boolean;

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
   * Controls whether iOS touch dispatch uses the refactored gesture-conflict path. When enabled, external gestures in possible or began state no longer cancel Lynx touch handling and touchend can complete normally; when disabled, the legacy cancellation behavior remains.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableTouchRefactor?: boolean;

  /**
   * Make the touch point coordinates of Lynx or Canvas's TouchEvent to take into account the transformation.
   *
   * Supported platform: Android, HarmonyOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableTransformedTouchPosition?: boolean;

  /**
   * Controls whether ElementManager enables UI-operation batching for create-view and update work. When enabled, TemplateAssembler turns on UI operation batching and Android UI work can be grouped before flush; when disabled, runtime keeps the normal non-batched path. When unset, LynxEnv can still enable batching.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUIOperationOptimize?: boolean;

  /**
   * Controls whether runtime enables the unified pixel pipeline after page config is decoded. When true, TemplateAssembler turns on the unified pixel pipeline context and render completion goes through the unified resolve path; when false, it keeps the legacy patch-finish pipeline. When unset, it falls back to the native config source.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.4
   *
   * @defaultValue undefined
   */
  enableUnifiedPipeline?: boolean;

  /**
   * Unify behavior between old-fixed and new-fixed
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue false
   */
  enableUnifyFixedBehavior?: boolean;

  /**
   * Controls whether LepusNG context creation first tries pooled MTS runtimes instead of constructing a fresh runtime immediately. When true, TemplateEntry reuses local or global context pools and template bundles can prefill a local pool; when false, runtime skips pooled reuse and creates a new context directly. When unset, it falls back to LynxEnv.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUseContextPool?: boolean;

  /**
   * Controls whether UI props and style payloads are stored in MapBuffer instead of the legacy map container. When enabled, fragments and prop bundles can read MapBuffer-backed data; when disabled, runtime keeps the older prop container path. When unset, it falls back to LynxEnv.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUseMapBuffer?: boolean;

  /**
   * Controls whether iOS client-slide views continue receiving raw touch callbacks alongside Lynx gestures. When enabled, gesture recognizers stop canceling touches in view and touchesBegan or touchesEnded can still reach the platform view; when disabled, Lynx gestures keep canceling those raw touch callbacks.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableViewReceiveTouch?: boolean;

  /**
   * Drive the execution of UI tasks in the pipeline according to the VSync signal, bringing a certain progressive rendering effect. It is suitable for scenarios where JS-driven updates are frequent. Turn it on as needed.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK [{'Android': '3.2'}, {'iOS': '3.2'}]
   *
   * @defaultValue false
   */
  enableVsyncAlignedFlush?: boolean;

  /**
   * Controls whether iOS x-text nodes reuse a prepared text layout model instead of rebuilding it for each measure. When enabled, BDX text nodes create and reuse a text model during layout; when disabled, each measure recomputes layout data.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableXTextLayoutReused?: boolean;

  /**
   * A config to force make some special properties can be used to layout only (such as direction&text-align,etc.).
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  extendedLayoutOnlyOpt?: boolean;

  /**
   * Carries user-defined `extraInfo` through config decode into `PageConfig` and script-visible config state. When set, the manual binary config decoder stores the `lepus::Value` payload on the page config; when omitted, no extra info object is attached.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  extraInfo?: Record<string, unknown>;

  /**
   * Controls whether Android nodes can stay flattened without a backing platform `View` by default. When true, only nodes that need a real platform view create one; when false, nodes create corresponding Android views more eagerly.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  flatten?: boolean;

  /**
   * Make font scale only apply to sp units.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  fontScaleEffectiveOnlyOnSp?: boolean;

  /**
   * Controls the deprecated global iOS implicit-animation default copied into UI config during page decode. When true, layer property changes keep implicit Core Animation actions; when false, iOS disables them by default. For target SDK 2.0 and above, the default override is false.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  implicit?: boolean;

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
   * Specifies the frequency of exposure detection.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  observerFrameRate?: number;

  /**
   * Scheduler config for pipeline, including enableParallelElement/list-framework batch render and other scheduler config.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  pipelineSchedulerConfig?: number;

  /**
   * Controls the preferred frame-rate mode requested for animation-driven frame callbacks. `ElementManager` forwards this string to `ElementVsyncProxy`, so values such as `high`, `low`, or `auto` change the vsync cadence used while CSS animations request frames.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue "auto"
   */
  preferredFps?: string;

  /**
   * Controls the layout compatibility version used by manual config decode for legacy CSS behavior. During decode, boolean or version-string input is converted to a `base::Version` and written into layout configs; older compatibility versions keep legacy layout quirks while newer or default values follow target-SDK behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  quirksMode?: boolean | string;

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
   * Controls how component prop type mismatches are handled during component property application. When enabled, `RadonComponent` falls back to the declared default value for mismatched props; when disabled, runtime keeps the older mismatch behavior.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  strictPropType?: boolean;

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
   * Controls whether iOS turns on the refactored image implementation exposed through trailUseNewImage. When enabled, the iOS UI context uses the newer image pipeline; when disabled, it stays on the legacy image implementation. When unset, it falls back to native config.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  trailNewImage?: boolean;

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

  /**
   * Controls whether Android image requests run through the image post-processor path after load. When enabled, PageConfig marks image requests for post-processing; when disabled, images are delivered without that extra processing stage.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  useImagePostProcessor?: boolean;

  /**
   * Controls whether Android pages create the newer x-swiper implementation instead of the deprecated legacy swiper. When enabled, pages use the new swiper path; when disabled, runtime can still create the legacy swiper and logs a deprecation warning.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  useNewSwiper?: boolean;

  /**
   * deprecated
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  autoResumeAnimation?: boolean;

  /**
   * cli version
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  cli?: string;

  /**
   * Controls the deprecated compile-render component ownership fix inside Radon components. When enabled, `RadonComponent` also rewrites plug subtrees during component reassignment; when disabled, that extra compile-render plug adjustment is skipped.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  compileRender?: boolean;

  /**
   * Carries the deprecated custom-data JSON string from config decode into platform config info and behavior registries. When set, Android, iOS, and Harmony bridges expose the string to platform-side consumers; when empty, no customData payload is forwarded.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  customData?: string;

  /**
   * Controls the deprecated accessibility mutation-observer path for accessibility-id changes. When enabled, Android and iOS accessibility wrappers watch node mutations and refresh accessibility state when relevant ids or structure change; when disabled, those mutation observers are not installed.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableA11yIDMutationObserver?: boolean;

  /**
   * Globally enable async rendering for software rendering contents on iOS, this can largely optimize the frame rate and reduce janks.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableAsyncDisplay?: boolean;

  /**
   * Controls the deprecated iOS background-mask optimization that uses `CAShapeLayer` for rounded background and overflow masks. When enabled, `LynxBackgroundManager` and UI context prefer shape-layer masks; when disabled, iOS falls back to the older mask path. This optimization is effectively fixed in newer implementations.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableBackgroundShapeLayer?: boolean;

  /**
   * Deprecated, this is for legacy CSS selector to enable a cascading.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableCascadePseudo?: boolean;

  /**
   * Controls the deprecated Radon page data-validation step before `updatePage` applies new data. When enabled, renderer functions set the flag on `RadonPage` and update-page checks whether incoming data actually changed; when disabled, that equality check is skipped.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableCheckDataWhenUpdatePage?: boolean;

  /**
   * Controls the deprecated component-level gate for layout-only optimization. When enabled, component elements can return true from `CanBeLayoutOnly()` and skip backing views when other checks pass; when disabled, components keep real element nodes even if other layout-only conditions match.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableComponentLayoutOnly?: boolean;

  /**
   * Controls the deprecated Radon component-map scope used during component lookup and update. When enabled, `RadonComponent` reads and updates the page-global component map; when disabled, component bookkeeping stays on the per-component map path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableGlobalComponentMap?: boolean;

  /**
   * Controls whether Android painting context schedules native `CreatePaintingNode` work asynchronously instead of executing creation immediately. When enabled, `ElementManager` forwards the flag and `PaintingContextAndroid` enqueues async create-view work; when disabled, create-node work stays synchronous. When unset, it falls back to `LynxEnv`.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   * @deprecated 3.5
   */
  enableNativeScheduleCreateViewAsync?: boolean;

  /**
   * Controls the deprecated Android delegate-based accessibility implementation. When enabled, `LynxAccessibilityWrapper` prefers the newer accessibility delegate path; when disabled, accessibility stays on the older node-provider or helper path.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableNewAccessibility?: boolean;

  /**
   * Controls the deprecated extra page-config gate for layout-only optimization. When enabled, `ElementManager` combines this flag with TASM's layout-only switch before allowing layout-only nodes; when disabled, page config blocks that optimization even if TASM enables it.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableNewLayoutOnly?: boolean;

  /**
   * Controls the deprecated style-to-PropBundle optimization flag applied after config decode. When enabled, `ElementManager` turns on the optimized push-style path; when disabled, style payloads keep the older path. When unset, runtime falls back to `LynxEnv`.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   * @deprecated 3.5
   */
  enableOptPushStyleToBundle?: boolean;

  /**
   * Controls the deprecated Android accessibility overlap policy for Lynx accessibility elements. When enabled, `LynxAccessibilityNodeProvider` can expose overlapping Lynx nodes to accessibility; when disabled, overlap handling keeps the older filtered behavior.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableOverlapForAccessibilityElement?: boolean;

  /**
   * Controls the deprecated CSS import-flattening order used when the binary reader expands `@import` dependencies. When enabled, `CSSStyleSheetManager::FlatDependentCSS()` preserves the newer forward import order; when disabled, imported sheets are flattened in the older reverse order.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  fixCSSImportRuleOrder?: boolean;

  /**
   * Controls the deprecated page-level fallback for forcing Radon style recalculation on updates. When the newer compiler option is unset, manual config decode still reads this flag and `RadonNode` can recalculate styles instead of using the incremental style-invalidation path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  forceCalcNewStyle?: boolean;

  /**
   * Carries the deprecated React version string from config decode into platform config info and performance reporting. When set, Android and iOS bridges expose it in config-info payloads and perf metrics; when empty, no React version is reported.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  reactVersion?: string;

  /**
   * Carries the deprecated iOS image-size warning threshold from config decode into UI context. When a decoded image exceeds this threshold, iOS red-box diagnostics can warn; when absent, the default warning threshold is used.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   * @deprecated 3.5
   */
  redBoxImageSizeWarningThreshold?: number;

  /**
   * deprecated
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  removeComponentElement?: boolean;

  /**
   * Controls the deprecated Android image-attach timing for starting requests from the main thread. When enabled and attach happens on the main thread, image requests start immediately; when disabled, request start is posted to the next main-thread frame.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  syncImageAttach?: boolean;

  /**
   * Controls the deprecated page-level ternary switch that forwards image loading to the newer image pipeline on Android and iOS. When explicitly true or false, the decoded value is exported to platform `PageConfig`; when unset, platform code falls back to newer native flags such as `enableNewImage` or `trailNewImage`.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   * @deprecated 3.5
   */
  useNewImage?: boolean;

  /**
   * Carries the deprecated frontend page-version string from config decode into runtime reporting and platform bridges. When set, `PageConfig` forwards it to Android or iOS config info and event tracking; when empty, runtime reports no page version.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  version?: string;
}
