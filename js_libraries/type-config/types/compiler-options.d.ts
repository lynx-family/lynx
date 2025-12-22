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
  enableCSSExternalClass?: boolean

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

}