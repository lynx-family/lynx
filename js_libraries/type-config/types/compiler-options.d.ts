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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  debugInfoOutside?: boolean

  /**
   * NA
   *
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableCSSAsyncDecode?: boolean

  /**
   * NA
   *
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSEngine?: boolean

  /**
   * NA
   *
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableCSSLazyDecode?: boolean

  /**
   * This switch will enable the css module in blink standard mode.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSSelector?: boolean

  /**
   * CSS Length should be <number> follows a unit. Under strict mode, invalid <length> values are dropped.
   *
   * Supported platform: Android, HarmonyOS, iOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableFiberArch?: boolean

  /**
   * If enable this value, the template will be encoded as flexible template.
   *
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableRemoveCSSScope?: boolean

  /**
   * Using the simplified styling module.
   *
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
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
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  removeCSSParserLog?: boolean

  /**
   * NA
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   *
   */
  targetSdkVersion?: string

  /**
   * NA
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   *
   */
  templateDebugUrl?: string

}