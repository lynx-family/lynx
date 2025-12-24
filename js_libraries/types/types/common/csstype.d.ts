// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 * 
 * Type System Design:
 * ===================
 * This module provides a gradual type strengthening system for inline styles:
 * 
 * 1. CSSProperties (default, loose mode):
 *    - Lynx-specific properties have strict types for better autocomplete
 *    - Standard CSS properties accept both specific values AND loose string types
 *    - Backward compatible with existing code
 *    - Recommended for most use cases
 * 
 * 2. StrictCSSProperties (strict mode):
 *    - All properties have strict enum types
 *    - Better type safety but may require code changes
 *    - Use when you want maximum type checking
 * 
 * 3. Compatibility with csstype:
 *    - CSSProperties can be used alongside CSS.Properties from 'csstype'
 *    - Uses the Modify helper type to override specific properties
 * 
 * Migration Path:
 * ===============
 * - Start with CSSProperties (loose mode) for backward compatibility
 * - Gradually migrate to StrictCSSProperties as your codebase matures
 * - Use utility types (Shorthands, Longhands) for specific use cases
 */

import type * as CSS from 'csstype';

export type Modify<T, R> = Omit<T, keyof R> & R;

/**
 * Lynx-specific CSS properties defined strictly.
 * These properties are not part of standard CSS.
 */
type LynxSpecificProperties = {
    // typography
    linearCrossGravity?: 'none' | 'start' | 'end' | 'center' | 'stretch';
    relativeAlignInlineStart?: string;
    relativeAlignInlineEnd?: string;
    relativeInlineStartOf?: number;
    relativeInlineEndOf?: number;
    linearDirection?: string;
    XAutoFontSize?: string;
    XAutoFontSizePresetSizes?: string;
    XPlaceholderFontFamily?: string;
    XPlaceholderFontSize?: string;
    XPlaceholderFontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
    XPlaceholderFontStyle?: 'normal' | 'italic' | 'oblique';
    linearOrientation?: 'horizontal' | 'vertical' | 'horizontal-reverse' | 'vertical-reverse' | 'row' | 'column' | 'row-reverse' | 'column-reverse';
    linearWeightSum?: number;
    linearWeight?: number;
    linearGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'space-between' | 'start' | 'end' | 'center';
    linearLayoutGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'fill-vertical' | 'fill-horizontal' | 'center' | 'stretch' | 'start' | 'end';

    // visual
    XAnimationColorInterpolation?: 'auto' | 'sRGB' | 'linearRGB';
    XHandleColor?: string;
    XPlaceholderColor?: string;

    // other
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
    XAppRegion?: 'none' | 'drag' | 'no-drag';
    XHandleSize?: string;
};

/**
 * Override types for standard CSS properties that Lynx implements differently.
 * Uses loose typing to allow both Lynx-specific values and standard CSS values.
 */
type LynxCSSOverrides = {
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
    gridAutoFlow?: 'row' | 'column' | 'dense' | 'row dense' | 'column dense' | (string & {});
    maskPosition?: string;
    display?: 'none' | 'flex' | 'grid' | 'linear' | 'relative' | 'block' | 'auto' | (string & {});
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
    position?: 'absolute' | 'relative' | 'fixed' | 'sticky' | (string & {});
    flexGrow?: number;
    flexShrink?: number;
    flexBasis?: string;
    flexDirection?: 'column' | 'row' | 'row-reverse' | 'column-reverse' | (string & {});
    flexWrap?: 'wrap' | 'nowrap' | 'wrap-reverse' | (string & {});
    backgroundPosition?: string;

    // typography
    outline?: string;
    outlineColor?: string;
    outlineStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    outlineWidth?: string;
    textDecorationColor?: string;
    borderInlineStartColor?: string;
    borderInlineEndColor?: string;
    borderInlineStartWidth?: string;
    borderInlineEndWidth?: string;
    borderInlineStartStyle?: string;
    borderInlineEndStyle?: string;
    insetInlineStart?: string;
    insetInlineEnd?: string;
    textIndent?: string;
    textStroke?: string;
    textStrokeWidth?: string;
    textStrokeColor?: string;
    fontVariationSettings?: string;
    fontFeatureSettings?: string;
    fontOpticalSizing?: 'none' | 'auto' | (string & {});
    textAlign?: 'left' | 'center' | 'right' | 'start' | 'end' | 'justify' | (string & {});
    lineHeight?: string;
    textOverflow?: 'clip' | 'ellipsis' | (string & {});
    fontSize?: string;
    fontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900' | (string & {});
    fontFamily?: string;
    fontStyle?: 'normal' | 'italic' | 'oblique' | (string & {});
    lineSpacing?: string;
    adaptFontSize?: string;
    textDecoration?: 'none' | 'underline' | 'line-through' | (string & {});
    textShadow?: string;

    // visual
    borderTopColor?: string;
    backgroundOrigin?: 'border-box' | 'content-box' | 'padding-box' | (string & {});
    backgroundRepeat?: 'no-repeat' | 'repeat-x' | 'repeat-y' | 'repeat' | 'round' | 'space' | (string & {});
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
    backgroundClip?: 'border-box' | 'content-box' | 'padding-box' | 'text' | 'border-area' | (string & {});
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
    color?: string;
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
    animationDirection?: 'normal' | 'reverse' | 'alternate' | 'alternate-reverse' | (string & {});
    animationFillMode?: 'none' | 'forwards' | 'backwards' | 'both' | (string & {});
    animationPlayState?: 'paused' | 'running' | (string & {});
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
    visibility?: 'hidden' | 'visible' | 'none' | 'collapse' | (string & {});
    content?: string;
    overflowX?: 'hidden' | 'visible' | (string & {});
    overflowY?: 'hidden' | 'visible' | (string & {});
    wordBreak?: 'normal' | 'break-all' | 'keep-all' | (string & {});
    verticalAlign?: 'baseline' | 'sub' | 'super' | 'top' | 'text-top' | 'middle' | 'bottom' | 'text-bottom' | (string & {});
    direction?: 'normal' | 'lynx-rtl' | 'rtl' | 'ltr' | (string & {});
    zIndex?: number;
    maskImage?: string;
    justifyItems?: 'start' | 'end' | 'center' | 'stretch' | 'auto' | (string & {});
    justifySelf?: 'start' | 'end' | 'center' | 'stretch' | 'auto' | (string & {});
    filter?: string;
    listMainAxisGap?: 'grayscale' | (string & {});
    listCrossAxisGap?: string;
    perspective?: 'number' | 'vw' | 'vh' | 'default' | 'px' | (string & {});
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
    imageRendering?: 'auto' | 'crisp-edges' | 'pixelated' | (string & {});
    hyphens?: 'none' | 'manual' | 'auto' | (string & {});
    offsetDistance?: number;
    offsetPath?: string;
    offsetRotate?: string;
    pointerEvents?: 'auto' | 'none' | (string & {});
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
    whiteSpace?: 'normal' | 'nowrap' | (string & {});
    letterSpacing?: string;
    alignItems?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline' | (string & {});
    alignSelf?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'auto' | 'start' | 'end' | 'baseline' | (string & {});
    alignContent?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'space-between' | 'space-around' | 'start' | 'end' | (string & {});
    justifyContent?: 'flex-start' | 'center' | 'flex-end' | 'space-between' | 'space-around' | 'space-evenly' | 'stretch' | 'start' | 'end' | (string & {});
    boxSizing?: 'border-box' | 'content-box' | 'auto' | (string & {});
    transform?: 'translate' | 'translateX' | 'translateY' | 'translateZ' | 'translate' | 'translate3d' | 'translate3D' | 'rotate' | 'rotateX' | 'rotateY' | 'rotateZ' | 'scale' | 'scaleX' | 'scaleY' | (string & {});
    order?: number;
    boxShadow?: string;
    transformOrigin?: 'left' | 'right' | 'top' | 'bottom' | 'center' | (string & {});
    aspectRatio?: string;
};

/**
 * Default CSSProperties type with loose typing for backward compatibility.
 * Recommended for most use cases.
 * 
 * This type:
 * - Inherits from csstype's CSS.Properties for standard CSS support
 * - Overrides specific properties with Lynx implementations (loose mode)
 * - Adds Lynx-specific properties
 */
export type CSSProperties = Modify<
  CSS.Properties<string | number>,
  LynxCSSOverrides
> & LynxSpecificProperties;

/**
 * Strict CSSProperties type for maximum type safety.
 * Use when you want strict enum checking for all properties.
 * 
 * This type:
 * - Uses strict enum types for all properties
 * - Better autocomplete and type checking
 * - May require code changes for existing projects
 */
export type StrictCSSProperties = {
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
