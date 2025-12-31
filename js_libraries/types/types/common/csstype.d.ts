// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 *
 * ## Gradual Type Strictness
 *
 * This module exports two CSS property types with different strictness levels:
 *
 * ### CSSProperties (Default - Loose)
 * - Extends `CSS.Properties` from the `csstype` package for full web compatibility
 * - Overrides specific properties with Lynx-specific types where applicable
 * - Use this for maximum compatibility with existing CSS patterns
 * - Allows standard CSS values that may not be supported by Lynx runtime
 *
 * ### LynxCSSProperties (Strict)
 * - Contains ONLY properties defined in Lynx's css_defines
 * - Provides strict typing based on Lynx's actual supported values
 * - Use this when you want maximum type safety and only Lynx-supported properties
 * - Will cause compile errors for unsupported CSS properties
 *
 * ## Migration Path
 * 1. Start with `CSSProperties` for compatibility
 * 2. Gradually migrate to `LynxCSSProperties` for stricter type checking
 * 3. Use `LynxCSSProperties` in new code for best type safety
 */

import * as CSS from 'csstype';

// =============================================================================
// Utility Types
// =============================================================================

/**
 * Utility type to modify an existing type by overriding specific properties.
 * Used to extend csstype's Properties with Lynx-specific types.
 */
export type Modify<T, R> = Omit<T, keyof R> & R;

// =============================================================================
// LynxCSSProperties (Strict Mode)
// =============================================================================

/**
 * Strict CSS properties type generated from Lynx's css_defines.
 * Only includes properties that are explicitly supported by the Lynx runtime.
 * Use this type for maximum type safety when you want to ensure only
 * Lynx-supported CSS properties and values are used.
 *
 * @example
 * const style: LynxCSSProperties = {
 *   display: 'flex',      // ✓ Only Lynx-supported values
 *   flexDirection: 'row', // ✓ Strictly typed
 *   // display: 'inline', // ✗ Error: 'inline' not supported in Lynx
 * };
 */
export type LynxCSSProperties = {
  // layout
  flexFlow?: string;
  marginInlineStart?: string;
  marginInlineEnd?: string;
  paddingInlineStart?: string;
  paddingInlineEnd?: string;
  gridTemplateColumns?: string;
  gridTemplateRows?: string;
  gridAutoColumns?: string;
  gridAutoRows?: string;
  gridColumnSpan?: number;
  gridRowSpan?: number;
  gridColumnStart?: string;
  gridColumnEnd?: string;
  gridRowStart?: string;
  gridRowEnd?: string;
  gridColumnGap?: string;
  gridRowGap?: string;
  gridAutoFlow?: 'row' | 'column' | 'dense' | 'row dense' | 'column dense';
  maskPosition?: string;
  display?: 'none' | 'flex' | 'grid' | 'linear' | 'relative' | 'block' | 'auto';
  padding?: string;
  paddingLeft?: string;
  paddingRight?: string;
  paddingTop?: string;
  paddingBottom?: string;
  margin?: string;
  marginLeft?: string;
  marginRight?: string;
  marginTop?: string;
  marginBottom?: string;
  flex?: string;
  position?: 'absolute' | 'relative' | 'fixed' | 'sticky';
  flexGrow?: number;
  flexShrink?: number;
  flexBasis?: string;
  flexDirection?: 'column' | 'row' | 'row-reverse' | 'column-reverse';
  flexWrap?: 'wrap' | 'nowrap' | 'wrap-reverse';
  backgroundPosition?: string;

  // typography
  outline?: string;
  outlineColor?: string;
  outlineStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  outlineWidth?: string;
  textDecorationColor?: string;
  linearCrossGravity?: 'none' | 'start' | 'end' | 'center' | 'stretch';
  borderInlineStartColor?: string;
  borderInlineEndColor?: string;
  borderInlineStartWidth?: string;
  borderInlineEndWidth?: string;
  borderInlineStartStyle?: string;
  borderInlineEndStyle?: string;
  relativeAlignInlineStart?: string;
  relativeAlignInlineEnd?: string;
  relativeInlineStartOf?: number;
  relativeInlineEndOf?: number;
  insetInlineStart?: string;
  insetInlineEnd?: string;
  linearDirection?: string;
  textIndent?: string;
  textStroke?: string;
  textStrokeWidth?: string;
  textStrokeColor?: string;
  XAutoFontSize?: string;
  XAutoFontSizePresetSizes?: string;
  fontVariationSettings?: string;
  fontFeatureSettings?: string;
  fontOpticalSizing?: 'none' | 'auto';
  XPlaceholderFontFamily?: string;
  XPlaceholderFontSize?: string;
  XPlaceholderFontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
  XPlaceholderFontStyle?: 'normal' | 'italic' | 'oblique';
  textAlign?: 'left' | 'center' | 'right' | 'start' | 'end' | 'justify';
  lineHeight?: string;
  textOverflow?: 'clip' | 'ellipsis';
  fontSize?: string;
  fontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
  fontFamily?: string;
  fontStyle?: 'normal' | 'italic' | 'oblique';
  lineSpacing?: string;
  linearOrientation?: 'horizontal' | 'vertical' | 'horizontal-reverse' | 'vertical-reverse' | 'row' | 'column' | 'row-reverse' | 'column-reverse';
  linearWeightSum?: number;
  linearWeight?: number;
  linearGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'space-between' | 'start' | 'end' | 'center';
  linearLayoutGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'fill-vertical' | 'fill-horizontal' | 'center' | 'stretch' | 'start' | 'end';
  adaptFontSize?: string;
  textDecoration?: 'none' | 'underline' | 'line-through' | (string & {});
  textShadow?: string;

  // visual
  borderTopColor?: string;
  backgroundOrigin?: 'border-box' | 'content-box' | 'padding-box';
  backgroundRepeat?: 'no-repeat' | 'repeat-x' | 'repeat-y' | 'repeat' | 'round' | 'space';
  backgroundSize?: string;
  border?: string;
  borderRight?: string;
  borderLeft?: string;
  borderTop?: string;
  borderBottom?: string;
  borderBottomColor?: string;
  borderLeftStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  borderRightStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  borderTopStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  borderBottomStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  borderRadius?: string;
  backgroundClip?: 'border-box' | 'content-box' | 'padding-box' | 'text' | 'border-area';
  caretColor?: string;
  borderTopLeftRadius?: string;
  borderBottomLeftRadius?: string;
  borderTopRightRadius?: string;
  borderBottomRightRadius?: string;
  borderStartStartRadius?: string;
  borderEndStartRadius?: string;
  borderStartEndRadius?: string;
  borderEndEndRadius?: string;
  borderWidth?: string;
  borderLeftWidth?: string;
  borderRightWidth?: string;
  borderTopWidth?: string;
  borderBottomWidth?: string;
  XAnimationColorInterpolation?: 'auto' | 'sRGB' | 'linearRGB';
  XHandleColor?: string;
  color?: string;
  XPlaceholderColor?: string;
  background?: string;
  borderColor?: string;
  backgroundColor?: string;
  borderStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
  borderLeftColor?: string;
  borderRightColor?: string;
  backgroundImage?: string;

  // animation
  transition?: string;
  transitionProperty?: 'none' | 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | 'width' | 'height' | 'background-color' | 'visibility' | 'left' | 'top' | 'right' | 'bottom' | 'transform' | 'all' | (string & {});
  transitionDuration?: string;
  transitionDelay?: string;
  transitionTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
  implicitAnimation?: string;
  enterTransitionName?: string;
  exitTransitionName?: string;
  pauseTransitionName?: string;
  resumeTransitionName?: string;
  animation?: string;
  animationName?: string;
  animationDuration?: string;
  animationTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
  animationDelay?: string;
  animationIterationCount?: string;
  animationDirection?: 'normal' | 'reverse' | 'alternate' | 'alternate-reverse';
  animationFillMode?: 'none' | 'forwards' | 'backwards' | 'both';
  animationPlayState?: 'paused' | 'running';
  layoutAnimationCreateDuration?: string;
  layoutAnimationCreateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
  layoutAnimationCreateDelay?: string;
  layoutAnimationCreateProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
  layoutAnimationDeleteDuration?: string;
  layoutAnimationDeleteTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
  layoutAnimationDeleteDelay?: string;
  layoutAnimationDeleteProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
  layoutAnimationUpdateDuration?: string;
  layoutAnimationUpdateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
  layoutAnimationUpdateDelay?: string;

  // other
  top?: string;
  visibility?: 'hidden' | 'visible' | 'none' | 'collapse';
  content?: string;
  overflowX?: 'hidden' | 'visible' | (string & {});
  overflowY?: 'hidden' | 'visible' | (string & {});
  wordBreak?: 'normal' | 'break-all' | 'keep-all';
  verticalAlign?: 'baseline' | 'sub' | 'super' | 'top' | 'text-top' | 'middle' | 'bottom' | 'text-bottom' | (string & {});
  direction?: 'normal' | 'lynx-rtl' | 'rtl' | 'ltr';
  relativeId?: number;
  relativeAlignTop?: string;
  relativeAlignRight?: string;
  relativeAlignBottom?: string;
  relativeAlignLeft?: string;
  relativeTopOf?: number;
  relativeRightOf?: number;
  relativeBottomOf?: number;
  relativeLeftOf?: number;
  relativeLayoutOnce?: string;
  relativeCenter?: 'none' | 'vertical' | 'horizontal' | 'both';
  zIndex?: number;
  maskImage?: string;
  justifyItems?: 'start' | 'end' | 'center' | 'stretch' | 'auto';
  justifySelf?: 'start' | 'end' | 'center' | 'stretch' | 'auto';
  filter?: string;
  listMainAxisGap?: 'grayscale' | (string & {});
  listCrossAxisGap?: string;
  perspective?: 'number' | 'vw' | 'vh' | 'default' | 'px';
  cursor?: string;
  clipPath?: string;
  mask?: string;
  left?: string;
  maskRepeat?: string;
  maskClip?: string;
  maskOrigin?: string;
  maskSize?: string;
  gap?: string;
  columnGap?: string;
  rowGap?: string;
  imageRendering?: 'auto' | 'crisp-edges' | 'pixelated';
  hyphens?: 'none' | 'manual' | 'auto';
  XAppRegion?: 'none' | 'drag' | 'no-drag';
  XHandleSize?: string;
  offsetDistance?: number;
  offsetPath?: string;
  offsetRotate?: string;
  pointerEvents?: 'auto' | 'none';
  opacity?: number;
  overflow?: 'hidden' | 'visible' | (string & {});
  height?: string;
  width?: string;
  maxWidth?: string;
  minWidth?: string;
  right?: string;
  maxHeight?: string;
  minHeight?: string;
  bottom?: string;
  whiteSpace?: 'normal' | 'nowrap';
  letterSpacing?: string;
  alignItems?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline';
  alignSelf?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline';
  alignContent?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'space-between' | 'space-around' | 'start' | 'end';
  justifyContent?: 'flex-start' | 'center' | 'flex-end' | 'space-between' | 'space-around' | 'space-evenly' | 'stretch' | 'start' | 'end';
  boxSizing?: 'border-box' | 'content-box' | 'auto';
  transform?: 'translate' | 'translateX' | 'translateY' | 'translateZ' | 'translate' | 'translate3d' | 'translate3D' | 'rotate' | 'rotateX' | 'rotateY' | 'rotateZ' | 'scale' | 'scaleX' | 'scaleY' | (string & {});
  order?: number;
  boxShadow?: string;
  transformOrigin?: 'left' | 'right' | 'top' | 'bottom' | 'center' | (string & {});
  aspectRatio?: string;
};

// =============================================================================
// CSSProperties (Loose Mode - Default)
// =============================================================================

/**
 * Lynx-specific property overrides for CSSProperties.
 * These override the loose csstype definitions with Lynx-specific types
 * while maintaining compatibility with standard CSS.
 */
type LynxCSSOverrides = {
    // layout
    gridColumnSpan?: number;
    gridRowSpan?: number;
    gridAutoFlow?: 'row' | 'column' | 'dense' | 'row dense' | 'column dense';
    display?: 'none' | 'flex' | 'grid' | 'linear' | 'relative' | 'block' | 'auto';
    position?: 'absolute' | 'relative' | 'fixed' | 'sticky';
    flexGrow?: number;
    flexShrink?: number;
    flexDirection?: 'column' | 'row' | 'row-reverse' | 'column-reverse';
    flexWrap?: 'wrap' | 'nowrap' | 'wrap-reverse';

    // typography
    outlineStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    linearCrossGravity?: 'none' | 'start' | 'end' | 'center' | 'stretch';
    relativeInlineStartOf?: number;
    relativeInlineEndOf?: number;
    fontOpticalSizing?: 'none' | 'auto';
    XPlaceholderFontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
    XPlaceholderFontStyle?: 'normal' | 'italic' | 'oblique';
    textAlign?: 'left' | 'center' | 'right' | 'start' | 'end' | 'justify';
    textOverflow?: 'clip' | 'ellipsis';
    fontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
    fontStyle?: 'normal' | 'italic' | 'oblique';
    linearOrientation?: 'horizontal' | 'vertical' | 'horizontal-reverse' | 'vertical-reverse' | 'row' | 'column' | 'row-reverse' | 'column-reverse';
    linearWeightSum?: number;
    linearWeight?: number;
    linearGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'space-between' | 'start' | 'end' | 'center';
    linearLayoutGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'fill-vertical' | 'fill-horizontal' | 'center' | 'stretch' | 'start' | 'end';
    textDecoration?: 'none' | 'underline' | 'line-through' | (string & {});

    // visual
    backgroundOrigin?: 'border-box' | 'content-box' | 'padding-box';
    backgroundRepeat?: 'no-repeat' | 'repeat-x' | 'repeat-y' | 'repeat' | 'round' | 'space';
    borderLeftStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderRightStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderTopStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderBottomStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    backgroundClip?: 'border-box' | 'content-box' | 'padding-box' | 'text' | 'border-area';
    XAnimationColorInterpolation?: 'auto' | 'sRGB' | 'linearRGB';
    borderStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});

    // animation
    transitionProperty?: 'none' | 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | 'width' | 'height' | 'background-color' | 'visibility' | 'left' | 'top' | 'right' | 'bottom' | 'transform' | 'all' | (string & {});
    transitionTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    animationTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    animationDirection?: 'normal' | 'reverse' | 'alternate' | 'alternate-reverse';
    animationFillMode?: 'none' | 'forwards' | 'backwards' | 'both';
    animationPlayState?: 'paused' | 'running';
    layoutAnimationCreateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    layoutAnimationCreateProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
    layoutAnimationDeleteTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    layoutAnimationDeleteProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
    layoutAnimationUpdateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});

    // other
    visibility?: 'hidden' | 'visible' | 'none' | 'collapse';
    overflowX?: 'hidden' | 'visible' | (string & {});
    overflowY?: 'hidden' | 'visible' | (string & {});
    wordBreak?: 'normal' | 'break-all' | 'keep-all';
    verticalAlign?: 'baseline' | 'sub' | 'super' | 'top' | 'text-top' | 'middle' | 'bottom' | 'text-bottom' | (string & {});
    direction?: 'normal' | 'lynx-rtl' | 'rtl' | 'ltr';
    relativeId?: number;
    relativeTopOf?: number;
    relativeRightOf?: number;
    relativeBottomOf?: number;
    relativeLeftOf?: number;
    relativeCenter?: 'none' | 'vertical' | 'horizontal' | 'both';
    zIndex?: number;
    justifyItems?: 'start' | 'end' | 'center' | 'stretch' | 'auto';
    justifySelf?: 'start' | 'end' | 'center' | 'stretch' | 'auto';
    listMainAxisGap?: 'grayscale' | (string & {});
    perspective?: 'number' | 'vw' | 'vh' | 'default' | 'px';
    imageRendering?: 'auto' | 'crisp-edges' | 'pixelated';
    hyphens?: 'none' | 'manual' | 'auto';
    XAppRegion?: 'none' | 'drag' | 'no-drag';
    offsetDistance?: number;
    pointerEvents?: 'auto' | 'none';
    opacity?: number;
    overflow?: 'hidden' | 'visible' | (string & {});
    whiteSpace?: 'normal' | 'nowrap';
    alignItems?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline';
    alignSelf?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline';
    alignContent?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'space-between' | 'space-around' | 'start' | 'end';
    justifyContent?: 'flex-start' | 'center' | 'flex-end' | 'space-between' | 'space-around' | 'space-evenly' | 'stretch' | 'start' | 'end';
    boxSizing?: 'border-box' | 'content-box' | 'auto';
    transform?: 'translate' | 'translateX' | 'translateY' | 'translateZ' | 'translate' | 'translate3d' | 'translate3D' | 'rotate' | 'rotateX' | 'rotateY' | 'rotateZ' | 'scale' | 'scaleX' | 'scaleY' | (string & {});
    order?: number;
    transformOrigin?: 'left' | 'right' | 'top' | 'bottom' | 'center' | (string & {});
};

/**
 * CSS properties type that extends standard web CSS with Lynx-specific overrides.
 * This is the recommended default type for most use cases as it provides:
 * - Full compatibility with standard CSS properties from csstype
 * - Lynx-specific type refinements for properties that have limited values in Lynx
 * - Smooth migration path for web developers
 *
 * @example
 * const style: CSSProperties = {
 *   display: 'flex',           // ✓ Lynx-specific type
 *   color: 'red',              // ✓ Falls back to csstype
 *   backgroundColor: '#fff',   // ✓ Falls back to csstype
 * };
 */
export type CSSProperties = Modify<CSS.Properties<string | number>, LynxCSSOverrides>;

// =============================================================================
// Shorthand and Longhand Property Types
// =============================================================================

export type Shorthands = 
  // layout
  "flexFlow" | "padding" | "margin" | "flex" | "backgroundPosition" |
  // typography
  "outline" | "textDecoration" |
  // visual
  "border" | "borderRight" | "borderLeft" | "borderTop" | "borderBottom" | "borderRadius" | "borderWidth" | "background" | "borderColor" | "borderStyle" |
  // animation
  "transition" | "animation" |
  // other
  "mask" | "gap" | "overflow" | "whiteSpace";
export type Longhands = 
  // layout
  "marginInlineStart" | "marginInlineEnd" | "paddingInlineStart" | "paddingInlineEnd" | "gridTemplateColumns" | "gridTemplateRows" | "gridAutoColumns" | "gridAutoRows" | "gridColumnSpan" | "gridRowSpan" | "gridColumnStart" | "gridColumnEnd" | "gridRowStart" | "gridRowEnd" | "gridColumnGap" | "gridRowGap" | "gridAutoFlow" | "maskPosition" | "display" | "paddingLeft" | "paddingRight" | "paddingTop" | "paddingBottom" | "marginLeft" | "marginRight" | "marginTop" | "marginBottom" | "position" | "flexGrow" | "flexShrink" | "flexBasis" | "flexDirection" | "flexWrap" |
  // typography
  "outlineColor" | "outlineStyle" | "outlineWidth" | "textDecorationColor" | "linearCrossGravity" | "borderInlineStartColor" | "borderInlineEndColor" | "borderInlineStartWidth" | "borderInlineEndWidth" | "borderInlineStartStyle" | "borderInlineEndStyle" | "relativeAlignInlineStart" | "relativeAlignInlineEnd" | "relativeInlineStartOf" | "relativeInlineEndOf" | "insetInlineStart" | "insetInlineEnd" | "linearDirection" | "textIndent" | "textStroke" | "textStrokeWidth" | "textStrokeColor" | "XAutoFontSize" | "XAutoFontSizePresetSizes" | "fontVariationSettings" | "fontFeatureSettings" | "fontOpticalSizing" | "XPlaceholderFontFamily" | "XPlaceholderFontSize" | "XPlaceholderFontWeight" | "XPlaceholderFontStyle" | "textAlign" | "lineHeight" | "textOverflow" | "fontSize" | "fontWeight" | "fontFamily" | "fontStyle" | "lineSpacing" | "linearOrientation" | "linearWeightSum" | "linearWeight" | "linearGravity" | "linearLayoutGravity" | "adaptFontSize" | "textShadow" |
  // visual
  "borderTopColor" | "backgroundOrigin" | "backgroundRepeat" | "backgroundSize" | "borderBottomColor" | "borderLeftStyle" | "borderRightStyle" | "borderTopStyle" | "borderBottomStyle" | "backgroundClip" | "caretColor" | "borderTopLeftRadius" | "borderBottomLeftRadius" | "borderTopRightRadius" | "borderBottomRightRadius" | "borderStartStartRadius" | "borderEndStartRadius" | "borderStartEndRadius" | "borderEndEndRadius" | "borderLeftWidth" | "borderRightWidth" | "borderTopWidth" | "borderBottomWidth" | "XAnimationColorInterpolation" | "XHandleColor" | "color" | "XPlaceholderColor" | "backgroundColor" | "borderLeftColor" | "borderRightColor" | "backgroundImage" |
  // animation
  "transitionProperty" | "transitionDuration" | "transitionDelay" | "transitionTimingFunction" | "implicitAnimation" | "enterTransitionName" | "exitTransitionName" | "pauseTransitionName" | "resumeTransitionName" | "animationName" | "animationDuration" | "animationTimingFunction" | "animationDelay" | "animationIterationCount" | "animationDirection" | "animationFillMode" | "animationPlayState" | "layoutAnimationCreateDuration" | "layoutAnimationCreateTimingFunction" | "layoutAnimationCreateDelay" | "layoutAnimationCreateProperty" | "layoutAnimationDeleteDuration" | "layoutAnimationDeleteTimingFunction" | "layoutAnimationDeleteDelay" | "layoutAnimationDeleteProperty" | "layoutAnimationUpdateDuration" | "layoutAnimationUpdateTimingFunction" | "layoutAnimationUpdateDelay" |
  // other
  "top" | "visibility" | "content" | "overflowX" | "overflowY" | "wordBreak" | "verticalAlign" | "direction" | "relativeId" | "relativeAlignTop" | "relativeAlignRight" | "relativeAlignBottom" | "relativeAlignLeft" | "relativeTopOf" | "relativeRightOf" | "relativeBottomOf" | "relativeLeftOf" | "relativeLayoutOnce" | "relativeCenter" | "zIndex" | "maskImage" | "justifyItems" | "justifySelf" | "filter" | "listMainAxisGap" | "listCrossAxisGap" | "perspective" | "cursor" | "clipPath" | "left" | "maskRepeat" | "maskClip" | "maskOrigin" | "maskSize" | "columnGap" | "rowGap" | "imageRendering" | "hyphens" | "XAppRegion" | "XHandleSize" | "offsetDistance" | "offsetPath" | "offsetRotate" | "pointerEvents" | "opacity" | "height" | "width" | "maxWidth" | "minWidth" | "right" | "maxHeight" | "minHeight" | "bottom" | "letterSpacing" | "alignItems" | "alignSelf" | "alignContent" | "justifyContent" | "boxSizing" | "transform" | "order" | "boxShadow" | "transformOrigin" | "aspectRatio";

// Since `Shorthands` and `Longhands` are auto generated, there may be properties
// such as `gridColumnSpan` is not manually defined in `CSSProperties` yet.
// Use `& keyof CSSProperties` to ensure only the defined keys are included to avoid type error.
export type CSSPropertiesWithShorthands = Pick<CSSProperties, Shorthands & keyof CSSProperties>;
export type CSSPropertiesWithLonghands = Pick<CSSProperties, Longhands & keyof CSSProperties>;

// Strict versions for LynxCSSProperties
export type LynxCSSPropertiesWithShorthands = Pick<LynxCSSProperties, Shorthands & keyof LynxCSSProperties>;
export type LynxCSSPropertiesWithLonghands = Pick<LynxCSSProperties, Longhands & keyof LynxCSSProperties>;
