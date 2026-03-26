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
   * Enable Android Lynx Image URL asynchronous redirect
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  asyncRedirect?: boolean;

  /**
   * if this flag is true, onShow/onHide will be automatically triggered by attachToView/detachFromWindow calls
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
   * Enable Android A11y
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableA11y?: boolean;

  /**
   * Indicates whether all ui  enable a11y
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableAccessibilityElement?: boolean;

  /**
   * Whether to enable asynchronous initialization of videoEngine
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableAsyncInitVideoEngine?: boolean;

  /**
   * If true, supports initiating image requests in asynchronous threads.
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
   * Enable batch layout task with sync layout.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue false
   */
  enableBatchLayoutTaskWithSyncLayout?: boolean;

  /**
   * Enable exposure detection optimization so that exposure detection is not performed when the page is static.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableCheckExposureOptimize?: boolean;

  /**
   * Determine whether redirection is needed for local image resources.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableCheckLocalImage?: boolean;

  /**
   * If this flag is true, circular data check will enable when convert js value to other vale.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableCircularDataCheck?: boolean;

  /**
   * Enable dynamic components to be decoded in child threads before they are delivered into tasm in async-loading.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableComponentAsyncDecode?: boolean;

  /**
   * Support component can be passed null props, null props only supported in LepusNG now. Open this switch to support lepus use null prop.
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
   * Enable CSS inheritance
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableCSSInheritance?: boolean;

  /**
   * Enable CSS inline variables.
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
   * Enables the disexposure event to fire when LynxView is in the background.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue true
   */
  enableDisexposureWhenBackground?: boolean;

  /**
   * Enables the disexposure event to fire when LynxView is in the hidden state.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableDisexposureWhenLynxHidden?: boolean;

  /**
   * Enable the Lynx touch event to be triggered normally after the last finger is lifted in a multi-finger scenario.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableEndGestureAtLastFingerUp?: boolean;

  /**
   * Enable new event processing logic to support dynamic registration of event listeners, interception of events, etc.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enableEventHandleRefactor?: boolean;

  /**
   * Enable the client-slide touch event penetrate Lynx when touching the root node area of the Lynx page.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableEventThrough?: boolean;

  /**
   * Enable exposure-ui-margin-* properties to take effect.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableExposureUIMargin?: boolean;

  /**
   * Enable exposure check when LynxView is layoutRequest
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableExposureWhenLayout?: boolean;

  /**
   * Enables exposure and disexposure events to be triggered when reloading the Lynx page.
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
   * Enable harmony new overlay based overlayManager to handle events pass through.
   *
   * Supported platform: HarmonyOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableHarmonyNewOverlay?: boolean;

  /**
   * Enable Harmony to detect exposure with visible area change event.
   *
   * Supported platform: HarmonyOS
   *
   * Since: LynxSDK 3.4
   *
   * @defaultValue false
   */
  enableHarmonyVisibleAreaChangeForExposure?: boolean;

  /**
   * Enable Bind PRIMJS-ICU
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableICU?: boolean;

  /**
   * If true,  downsampling will be enabled for all images on this LynxView
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableImageDownsampling?: boolean;

  /**
   * Control whether to consider animation layers in position calculation during exposure detection.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.6
   *
   * @defaultValue false
   */
  enableiOSAnimationLayerForExposure?: boolean;

  /**
   * Enable js binding api throw exception
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableJsBindingApiThrowException?: boolean;

  /**
   * Enable data processor on JS thread
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
   * Indicates whether to use new architecture for the  platform list
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableListNewArchitecture?: boolean;

  /**
   * Indicates whether to use list plug
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
   * This configuration item enableMicrotaskPromisePolyfill is used to determine whether to enable the micro - task Promise polyfill. Its value type is TernaryBool, and the default value is TernaryBool::UNDEFINE_VALUE.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableMicrotaskPromisePolyfill?: boolean;

  /**
   * Enable MTS VM pre execute code.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.7
   *
   * @defaultValue false
   */
  enableMTSPreExecute?: boolean;

  /**
   * Enable support multi-finger events so that event parameters can contain information about multiple fingers.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableMultiTouch?: boolean;

  /**
   * Enable the parameters of multi-finger events compatible with single-finger events
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableMultiTouchParamsCompatible?: boolean;

  /**
   * Indicates whether use c++ list.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableNativeList?: boolean;

  /**
   * If this flag is false, will use platform animation ability.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableNewAnimator?: boolean;

  /**
   * Whether enable new clip mode.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewClipMode?: boolean;

  /**
   * if want to use gesture handler api, you need to set true
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableNewGesture?: boolean;

  /**
   * Enable load image from image service
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableNewImage?: boolean;

  /**
   * Enable the new IntersectionObserver detection logic so that observe can be triggered normally without binding to scroll events.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
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
   * If this flag is true, new transform origin algorithm will apply.
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
   * Enable shadow platform gesture to handle gesture conflict.
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
   * Enable query component sync in background runtime
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableQueryComponentSync?: boolean;

  /**
   * If we got propsId, we could only pass propsId and a flag to JS thread. JS thread will use a propsMap to get correct props
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReactOnlyPropsId?: boolean;

  /**
   * If this flag is true, do not copy init data, instead copy Object.keys of init data for efficiency.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReduceInitDataCopy?: boolean;

  /**
   * Enable LynxUI onNodeReload lifecycle.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableReloadLifecycle?: boolean;

  /**
   * If this flag is true, remove the globalProps & systemInfo from component data for efficiency.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableRemoveComponentExtraData?: boolean;

  /**
   * Enable reuse loadScript's result.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue false
   */
  enableReuseLoadScriptExports?: boolean;

  /**
   * Enable using native signal API
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableSignalAPI?: boolean;

  /**
   * Enable the client's tap gesture to be triggered simultaneously with Lynx's tap gesture.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableSimultaneousTap?: boolean;

  /**
   * Enable using BoringLayout on Android.
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableTextBoringLayout?: boolean;

  /**
   * Enable text gradient optimization for iOS.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.5
   *
   * @defaultValue undefined
   */
  enableTextGradientOpt?: boolean;

  /**
   * Enable a more accurate text alignment judgment method, but it will increase the time taken for text layout.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextLanguageAlignment?: boolean;

  /**
   * Enable using text layer render on iOS.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableTextLayerRender?: boolean;

  /**
   * Whether enable text layout cache.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.3
   *
   * @defaultValue undefined
   */
  enableTextLayoutCache?: boolean;

  /**
   * Whether enable text noncontiguous layout
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  enableTextNonContiguousLayout?: boolean;

  /**
   * Set text overflow as visible if true.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextOverflow?: boolean;

  /**
   * Enable text refactor, with behavior more aligned with web.
   *
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  enableTextRefactor?: boolean;

  /**
   * Enable Lynx's touchend event to be triggered normally.
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
   * Enable the optimization about UIOperation batching and CreateViewAsync at Android.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUIOperationOptimize?: boolean;

  /**
   * Enable the unified pixel pipeline for Lynx Engine.
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
   * Enable Use Context Pool For LepusNG VM creation.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUseContextPool?: boolean;

  /**
   * Enable the mapbuffer structure for LynxProps.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  enableUseMapBuffer?: boolean;

  /**
   * Enable touchesBegan and other methods to be triggered when touching the client-slide custom components.
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
   * Whether enable x-text layout reused.
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
   * user defined extraInfo.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  extraInfo?: Record<string, unknown>;

  /**
   * Control whether the node requires creating a corresponding Android View. Default is true.
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
   * Control whether implicit animations are allowed on the iOS platform.
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  implicit?: boolean;

  /**
   * Control the top and bottom padding of text on Android, which affects the text's height and vertical centering effect.
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
   * Specify the interval for triggering the long press event.
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
   * Control the frame rate of CSS animations
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue "auto"
   */
  preferredFps?: string;

  /**
   * Supports string value. It is equal to TargetSDKVersion by default. The mode is for compatibility with old pages that do not conform to CSS layout specifications produced.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  quirksMode?: boolean | string;

  /**
   * If false, descendant selector only works in component scope.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   */
  removeDescendantSelectorScope?: boolean;

  /**
   * if this flag is true, if component's prop type mismatch, will use default value.
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  strictPropType?: boolean;

  /**
   * Specify the sliding distance threshold at which the tap event is not triggered.
   *
   * Supported platform: Android, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue "50px"
   */
  tapSlop?: string;

  /**
   * Enable the iOS image refactor
   *
   * Supported platform: iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   */
  trailNewImage?: boolean;

  /**
   * Control whether the vw and vh units dynamically adjust with the viewport in properties like font-size; enabled by default for targetSdkVersion >= 2.3
   *
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  unifyVWVHBehavior?: boolean;

  /**
   * Whether use image post processor
   *
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   */
  useImagePostProcessor?: boolean;

  /**
   * Indicates whether to use new swiper
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  compileRender?: boolean;

  /**
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  customData?: string;

  /**
   * Enable MutationObserver for accessibility.
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
   * Enable iOS background manager to apply shape layer optimization. Deprecated and to be removed since this optimization becomes a fixed setting.
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
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableCheckDataWhenUpdatePage?: boolean;

  /**
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableComponentLayoutOnly?: boolean;

  /**
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableGlobalComponentMap?: boolean;

  /**
   * Enable create ui async form C++ PaintingContext
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
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   * @deprecated 3.5
   */
  enableNewAccessibility?: boolean;

  /**
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableNewLayoutOnly?: boolean;

  /**
   * Enable using PropBundleStyleWriter to write style to PropBundle.
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
   * Supported platform: Android
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  enableOverlapForAccessibilityElement?: boolean;

  /**
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  fixCSSImportRuleOrder?: boolean;

  /**
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   * @deprecated 3.5
   */
  forceCalcNewStyle?: boolean;

  /**
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  reactVersion?: string;

  /**
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
   * If true and on the main thread, image requests will be initiated immediately; otherwise, image requests will be posted after the next frame of the main thread to delay the requests.
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
   * Supported platform: Android, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   * @deprecated 3.5
   */
  useNewImage?: boolean;

  /**
   * Supported platform: Android, HarmonyOS, iOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   * @deprecated 3.5
   */
  version?: string;
}
