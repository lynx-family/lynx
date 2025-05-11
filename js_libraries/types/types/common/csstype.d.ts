// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * This file is auto-generated from CSS define files in the css_defines directory.
 * Each property's type is determined by:
 * 1. For enum types: Uses the values array from the CSS define file
 * 2. For properties with keywords: Uses the keywords array as enum values, with (string & {}) for open-ended types
 * 3. For other types: Uses string type
 */

import type * as CSS from 'csstype';

export type Modify<T, R> = Omit<T, keyof R> & R;

export type CSSProperties = Modify<
  CSS.Properties<string | number>,
  {
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
    backgroundClip?: 'border-box' | 'content-box' | 'padding-box' | 'text';
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
  }
>;