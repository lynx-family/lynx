// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * The Lynx compiler options to set.
 *
 * @public
 */

export enum CompilerOptionAirMode {
  AIR_MODE_OFF = 0,
  AIR_MODE_TTML_WITHOUT_JS,
  AIR_MODE_NATIVE_SCRIPT,
  AIR_MODE_STRICT,
  AIR_MODE_FIBER,
}

export interface CompilerOptions {
  /**
   * @defaultValue ""
   *
   */
  bundleModuleMode?: string

  /**
   * @defaultValue false
   *
   */
  debugInfoOutside?: boolean

  /**
   * Since: LynxSDK 2.2
   *
   * @defaultValue true
   *
   */
  defaultDisplayLinear?: boolean

  /**
   * Since: LynxSDK 1.6
   *
   * @defaultValue true
   *
   */
  defaultOverflowVisible?: boolean

  /**
   * Since: LynxSDK 2.0
   *
   * @defaultValue false
   *
   */
  disableMultipleCascadeCSS?: boolean

  /**
   * Enable this switch to use all raw css styles(no parse in encode), not used in runtime.
   *
   * @defaultValue true
   *
   */
  enableAirRawCSS?: boolean

  /**
   * Since: LynxSDK 2.6
   *
   * @defaultValue false
   *
   */
  enableComponentConfig?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableCSSAsyncDecode?: boolean

  /**
   * Since: LynxSDK 2.1
   *
   * @defaultValue false
   *
   */
  enableCSSClassMerge?: boolean

  /**
   * @defaultValue true
   *
   */
  enableCSSEngine?: boolean

  /**
   * Since: LynxSDK 1.6
   *
   * @defaultValue true
   *
   */
  enableCssExternalClass?: boolean

  /**
   * If enable CSS invalidation we use RuleInvalidationSet to gather the selector invalidation.
   *
   * Since: LynxSDK 2.10
   *
   * @defaultValue false
   *
   */
  enableCSSInvalidation?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableCSSLazyDecode?: boolean

  /**
   * This switch will enable the css module in blink standard mode.
   *
   * Since: LynxSDK 2.8
   *
   * @defaultValue false
   *
   */
  enableCSSSelector?: boolean

  /**
   * Since: LynxSDK 1.6
   *
   * @defaultValue false
   *
   */
  enableCSSStrictMode?: boolean

  /**
   * Since: LynxSDK 2.0
   *
   * @defaultValue true
   *
   */
  enableCSSVariable?: boolean

  /**
   * Since: LynxSDK 1.6
   *
   * @defaultValue true
   *
   */
  enableDynamicComponent?: boolean

  /**
   * Since: LynxSDK 2.5
   *
   * @defaultValue undefined
   *
   */
  enableEventRefactor?: boolean

  /**
   * @defaultValue false
   *
   */
  enableFiberArch?: boolean

  /**
   * If enable this value, the template will be encoded as flexible template.
   *
   * Since: LynxSDK 2.8
   *
   * @defaultValue false
   *
   */
  enableFlexibleTemplate?: boolean

  /**
   * Since: LynxSDK 2.1
   *
   * @defaultValue false
   *
   */
  enableKeepPageData?: boolean

  /**
   * Allow async decode lepus chunk.
   *
   * @defaultValue false
   *
   */
  enableLepusChunkAsyncDecode?: boolean

  /**
   * @defaultValue false
   *
   */
  enableLepusDebug?: boolean

  /**
   * Since: LynxSDK 2.3
   *
   * @defaultValue false
   *
   */
  enableLynxAir?: boolean

  /**
   * @defaultValue false
   *
   */
  enableParallelElement?: boolean

  /**
   * @defaultValue false
   *
   */
  enableRadon?: boolean

  /**
   * @defaultValue false
   *
   */
  enableRemoveCSSScope?: boolean

  /**
   * Enable this switch to reuse lepus::Context between DynamicComponent and page can only be used in lepusng.
   *
   * Since: LynxSDK 2.18
   *
   * @defaultValue false
   *
   */
  enableReuseContext?: boolean

  /**
   * Using the simplified styling module.
   *
   * Since: LynxSDK 3.3
   *
   * @defaultValue false
   *
   */
  enableSimpleStyling?: boolean

  /**
   * Allow encoding quickjs bytecode instead of source code in template.
   *
   * Since: LynxSDK 2.12
   *
   * @defaultValue false
   *
   */
  experimental_encodeQuickjsBytecode?: boolean

  /**
   * Since: LynxSDK 2.6
   *
   * @defaultValue undefined
   *
   */
  forceCalcNewStyle?: boolean

  /**
   * Since: LynxSDK 1.4
   *
   * @defaultValue false
   *
   */
  implicitAnimation?: boolean

  /**
   * Since: LynxSDK 2.11
   *
   * @defaultValue undefined
   *
   */
  lynxAirMode?: CompilerOptionAirMode

  /**
   * @defaultValue false
   *
   */
  removeCSSParserLog?: boolean

  /**
   * @defaultValue ""
   *
   */
  targetSdkVersion?: string

  /**
   * @defaultValue ""
   *
   */
  templateDebugUrl?: string

  /**
   * @defaultValue undefined
   *
   */
  trialOptions?: Record<string, unknown>

  /**
   * Since: LynxSDK 2.1
   *
   * @defaultValue false
   *
   */
  useLepusNG?: boolean

}