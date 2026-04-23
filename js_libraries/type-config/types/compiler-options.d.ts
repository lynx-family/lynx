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
   * Controls whether LepusNG keeps function-level debug info as external debug metadata instead of only inline source metadata. When enabled, encode and runtime keep external debug records for inspector and JS error reporting; when disabled, only the basic inline debug payload is available.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  debugInfoOutside?: boolean;

  /**
   * Controls the default root layout mode applied during page-config decode. When enabled and targetSdkVersion is at least 2.2, decoded pages default to linear layout; when disabled, the root layout keeps the legacy default.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  defaultDisplayLinear?: boolean;

  /**
   * Controls the default page-level overflow behavior applied during config decode. When enabled and targetSdkVersion is at least 2.0, decoded page config defaults overflow to visible; when disabled, page layout falls back to the legacy hidden-overflow default.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  defaultOverflowVisible?: boolean;

  /**
   * Controls whether the CSS encoder keeps cascade selectors with three or more selector levels. When enabled, the parser drops those multi-level cascade tokens during encode; when disabled, they remain in the encoded style output.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  disableMultipleCascadeCSS?: boolean;

  /**
   * Controls whether template metadata carries component-config support into runtime decode. When enabled, generated bundles can expose component config records such as extra-data removal and component-element removal; when disabled, that component-config feature flag is off.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableComponentConfig?: boolean;

  /**
   * Controls whether CSS fragments are decoded on a concurrent thread after the CSS descriptor is parsed. When enabled, the binary reader schedules asynchronous CSS fragment decode and treats CSS loading as lazy; when disabled, CSS decoding stays on the synchronous reader path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableCSSAsyncDecode?: boolean;

  /**
   * Controls how the CSS encoder and binary reader handle repeated class or id selectors while building style fragments. When enabled, duplicate selectors merge their declarations into one token; when disabled, later tokens replace earlier ones with the same selector key.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSClassMerge?: boolean;

  /**
   * Controls whether compile options bind the dedicated CSS-engine flag into runtime config. When enabled, generated page config carries the CSS engine switch for downstream style parsing and resolution; when disabled, runtime keeps the legacy flag state.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSEngine?: boolean;

  /**
   * Controls whether template compile metadata keeps support for external classes. When enabled, MetaFactory serializes the flag so downstream template and CSS stages can treat external classes as supported; when disabled, that capability flag is cleared.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSExternalClass?: boolean;

  /**
   * Controls whether CSS fragments build selector invalidation data alongside CSS selector rules. When enabled, the CSS reader creates RuleInvalidationSet data and Radon can invalidate affected styles incrementally; when disabled, selector matching skips invalidation metadata and style updates fall back to broader recalculation.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSInvalidation?: boolean;

  /**
   * Controls whether CSS fragments are decoded lazily in the template binary reader. When enabled, the reader defers CSS fragment decoding until they are needed; when disabled, CSS is decoded eagerly during descriptor parsing. When unset, runtime falls back to the disable_lazy_css_decode experiment setting.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableCSSLazyDecode?: boolean;

  /**
   * Controls whether compile, CSS encode, and CSS decode keep the Blink-style CSS selector module. When enabled, the parser and binary CSS reader preserve selector tokens for selector matching and invalidation; when disabled, selector parsing stays on the legacy non-selector path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableCSSSelector?: boolean;

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
  enableCSSStrictMode?: boolean;

  /**
   * Controls whether CSS custom properties are encoded and decoded in the CSS reader pipeline. When enabled for targetSdkVersion at least 2.0, CSS variable tokens and their default-value payloads are preserved; when disabled, CSS values are treated as plain static values.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue true
   *
   */
  enableCSSVariable?: boolean;

  /**
   * Controls the event-refactor path written into page config before runtime starts. When enabled, or left unset, the decoder turns on the refactored event behavior; when disabled, runtime falls back to the legacy event path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  enableEventRefactor?: boolean;

  /**
   * Controls whether template encode and decode switch from the Radon path to the Fiber architecture path. When enabled, MetaFactory validates the SDK gate, the encoder reads Fiber CSS sources, and runtime uses Fiber-specific bundle and page-config behavior; when disabled, templates stay on the Radon path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableFiberArch?: boolean;

  /**
   * Controls whether the template binary uses the flexible-template body format instead of the legacy section layout. When enabled, the writer emits the flexible template body and the decoder reads that format; when disabled, encode and decode stay on the legacy non-flexible template layout. Fiber and Air arch paths force this on.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableFlexibleTemplate?: boolean;

  /**
   * Controls whether decoded page config keeps saved page data for render-page reuse. When enabled, page config turns on page-data retention and virtual pages can reuse mould data instead of rebuilding from scratch; when disabled, page data is not retained between renders.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableKeepPageData?: boolean;

  /**
   * Controls whether the encoder asks the Lepus VM to generate optimized bytecode instead of the default bytecode form. When enabled and targetSdkVersion is at least 3.8, encode turns on SetOptBytecode; when disabled, or below that SDK gate, the VM emits the normal bytecode path.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.8
   *
   * @defaultValue false
   *
   */
  enableOptLepusBytecode?: boolean;

  /**
   * Controls whether runtime component rendering removes the generated CSS scope when building virtual pages. When enabled, the page proxy skips scoped descendant-style isolation for component pages; when disabled, component CSS keeps the generated scope.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  enableRemoveCSSScope?: boolean;

  /**
   * Controls whether encode and decode use the SimpleStyle path for style objects, keyframes, and font faces. When enabled, MetaFactory collects `styleObjects`, the writer emits simple-style sections, and runtime installs simple-style keyframes and font faces; when disabled, those sections are skipped.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.3
   *
   * @defaultValue false
   *
   */
  enableSimpleStyling?: boolean;

  /**
   * Controls whether the encoder writes QuickJS bytecode into the JS section instead of serializing JS source text. When enabled, the binary writer emits JS bytecode; when disabled, it writes source code strings. The experimental path is intended to be gated by a sufficiently high target SDK.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  experimental_encodeQuickjsBytecode?: boolean;

  /**
   * Controls whether page config forces the Radon runtime to recalculate styles instead of using CSS invalidation and cached style diffs. When enabled, nodes rebuild style state on updates; when disabled, Radon can reuse invalidation results and cached styles.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue undefined
   *
   */
  forceCalcNewStyle?: boolean;

  /**
   * Controls the default implicit-animation behavior exposed to page config on Darwin. When enabled, background and layer property changes keep implicit Core Animation actions by default; when disabled, layer updates disable implicit actions unless another animation path turns them back on.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  implicitAnimation?: boolean;

  /**
   * Controls whether compile metadata asks downstream CSS parsing stages to suppress parser diagnostics. When enabled, MetaFactory serializes a no-parser-log flag into compile options; when disabled, parser logging remains available.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue false
   *
   */
  removeCSSParserLog?: boolean;

  /**
   * Sets the engine version used across template encode, binary decode, and runtime initialization for compatibility checks. Higher versions enable newer codec and renderer paths such as CSS value parsing, CSS variables, page-data retention defaults, and Fiber gating, while lower versions keep legacy behavior.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   *
   */
  targetSdkVersion?: string;

  /**
   * Sets the external debug-info URL attached to the generated Lepus or QuickJS bundle during encode and runtime initialization. When set, the runtime exposes this URL to the inspector and appends it to stack traces and JS error reports; when empty, no external debug-info URL is reported.
   *
   * Supported platform: Android, iOS, HarmonyOS
   *
   * Since: LynxSDK 3.2
   *
   * @defaultValue ""
   *
   */
  templateDebugUrl?: string;
}
