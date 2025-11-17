// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * The Lynx compile options to set.
 *
 * @public
 */

export enum CompileOptionAirMode {
  AIR_MODE_OFF = 0,
  AIR_MODE_TTML_WITHOUT_JS,
  AIR_MODE_NATIVE_SCRIPT,
  AIR_MODE_STRICT,
  AIR_MODE_FIBER,
}

export interface CompileOptions {
  /**
   * @defaultValue undefined
   *
   */
  debugInfoOutside?: boolean

  /**
   * Since: 2.2
   *
   * @defaultValue undefined
   *
   */
  defaultDisplayLinear?: boolean

  /**
   * Since: 1.6
   *
   * @defaultValue undefined
   *
   */
  defaultOverflowVisible?: boolean

  /**
   * Since: 2.0
   *
   * @defaultValue undefined
   *
   */
  disableMultipleCascadeCSS?: boolean

  /**
   * Enable this switch to use all raw css styles(no parse in encode), not used in runtime.
   *
   * @defaultValue undefined
   *
   */
  enableAirRawCSS?: boolean

  /**
   * Since: 2.6
   *
   * @defaultValue undefined
   *
   */
  enableComponentConfig?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableCSSAsyncDecode?: boolean

  /**
   * Since: 2.1
   *
   * @defaultValue undefined
   *
   */
  enableCSSClassMerge?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableCSSEngine?: boolean

  /**
   * Since: 1.6
   *
   * @defaultValue undefined
   *
   */
  enableCssExternalClass?: boolean

  /**
   * If enable CSS invalidation we use RuleInvalidationSet to gather the selector invalidation.
   *
   * Since: 2.10
   *
   * @defaultValue undefined
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
   * Since: 2.8
   *
   * @defaultValue undefined
   *
   */
  enableCSSSelector?: boolean

  /**
   * Since: 1.6
   *
   * @defaultValue undefined
   *
   */
  enableCSSStrictMode?: boolean

  /**
   * Since: 2.0
   *
   * @defaultValue undefined
   *
   */
  enableCSSVariable?: boolean

  /**
   * Since: 1.6
   *
   * @defaultValue undefined
   *
   */
  enableDynamicComponent?: boolean

  /**
   * Since: 2.5
   *
   * @defaultValue undefined
   *
   */
  enableEventRefactor?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableFiberArch?: boolean

  /**
   * If enable this value, the template will be encoded as flexible template.
   *
   * Since: 2.8
   *
   * @defaultValue undefined
   *
   */
  enableFlexibleTemplate?: boolean

  /**
   * Since: 2.1
   *
   * @defaultValue undefined
   *
   */
  enableKeepPageData?: boolean

  /**
   * Allow async decode lepus chunk.
   *
   * @defaultValue undefined
   *
   */
  enableLepusChunkAsyncDecode?: boolean

  /**
   * Since: 2.3
   *
   * @defaultValue undefined
   *
   */
  enableLynxAir?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableRadon?: boolean

  /**
   * @defaultValue undefined
   *
   */
  enableRemoveCSSScope?: boolean

  /**
   * Enable this switch to reuse lepus::Context between DynamicComponent and page can only be used in lepusng.
   *
   * Since: 2.18
   *
   * @defaultValue undefined
   *
   */
  enableReuseContext?: boolean

  /**
   * Using the simplified styling module.
   *
   * Since: 3.3
   *
   * @defaultValue undefined
   *
   */
  enableSimpleStyling?: boolean

  /**
   * Allow encoding quickjs bytecode instead of source code in template.
   *
   * Since: 2.12
   *
   * @defaultValue undefined
   *
   */
  experimental_encodeQuickjsBytecode?: boolean

  /**
   * Since: 2.6
   *
   * @defaultValue undefined
   *
   */
  forceCalcNewStyle?: boolean

  /**
   * Since: 1.4
   *
   * @defaultValue undefined
   *
   */
  implicitAnimation?: boolean

  /**
   * Since: 2.11
   *
   * @defaultValue undefined
   *
   */
  lynxAirMode?: CompileOptionAirMode

  /**
   * @defaultValue undefined
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
   * Since: 2.1
   *
   * @defaultValue undefined
   *
   */
  useLepusNG?: boolean

}