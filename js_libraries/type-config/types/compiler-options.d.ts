// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * The Lynx compiler options to set.
 *
 * @public
 */

export interface CompilerOptions {
  /**
   * NA
   *
   * @defaultValue false
   *
   */
  debugInfoOutside?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  defaultDisplayLinear?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  defaultOverflowVisible?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  disableMultipleCascadeCSS?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableComponentConfig?: boolean

  /**
   * NA
   *
   * @defaultValue undefined
   *
   */
  enableCSSAsyncDecode?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSClassMerge?: boolean

  /**
   * NA
   *
   * @defaultValue true
   *
   */
  enableCSSEngine?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSExternalClass?: boolean

  /**
   * If enable CSS invalidation we use RuleInvalidationSet to gather the selector invalidation.
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSInvalidation?: boolean

  /**
   * NA
   *
   * @defaultValue undefined
   *
   */
  enableCSSLazyDecode?: boolean

  /**
   * This switch will enable the css module in blink standard mode.
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSSelector?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSStrictMode?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSVariable?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableEventRefactor?: boolean

  /**
   * NA
   *
   * @defaultValue false
   *
   */
  enableFiberArch?: boolean

  /**
   * If enable this value, the template will be encoded as flexible template.
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableFlexibleTemplate?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableKeepPageData?: boolean

  /**
   * NA
   *
   * @defaultValue false
   *
   */
  enableRemoveCSSScope?: boolean

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
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  experimental_encodeQuickjsBytecode?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  forceCalcNewStyle?: boolean

  /**
   * NA
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  implicitAnimation?: boolean

  /**
   * NA
   *
   * @defaultValue false
   *
   */
  removeCSSParserLog?: boolean

  /**
   * NA
   *
   * @defaultValue ""
   *
   */
  targetSdkVersion?: string

  /**
   * NA
   *
   * @defaultValue ""
   *
   */
  templateDebugUrl?: string

}