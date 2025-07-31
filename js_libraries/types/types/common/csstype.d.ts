// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import * as CSS from 'csstype';

export type Modify<T, R> = Omit<T, keyof R> & R;

export type CSSProperties = Modify<
  CSS.Properties<string | number>,
  {
    position?: 'absolute' | 'relative' | 'fixed' | 'sticky';
    boxSizing?: 'border-box' | 'content-box' | 'auto';
    display?: 'none' | 'flex' | 'grid' | 'linear' | 'relative' | 'block' | 'auto';
    overflow?: 'hidden' | 'visible' | (string & {});
    whiteSpace?: 'normal' | 'nowrap';
    textAlign?: 'left' | 'center' | 'right' | 'start' | 'end';
    textOverflow?: 'clip' | 'ellipsis';
    fontWeight?: 'normal' | 'bold' | '100' | '200' | '300' | '400' | '500' | '600' | '700' | '800' | '900';
    flexDirection?: 'column' | 'row' | 'row-reverse' | 'column-reverse';
    flexWrap?: 'wrap' | 'nowrap' | 'wrap-reverse';
    alignContent?: 'flex-start' | 'flex-end' | 'center' | 'stretch' | 'space-between' | 'space-around' | 'start' | 'end';
    justifyContent?: 'flex-start' | 'center' | 'flex-end' | 'space-between' | 'space-around' | 'space-evenly' | 'stretch' | 'start' | 'end';
    fontStyle?: 'normal' | 'italic' | 'oblique';
    transform?:
      | 'translate'
      | 'translateX'
      | 'translateY'
      | 'translateZ'
      | 'translate'
      | 'translate3d'
      | 'translate3D'
      | 'rotate'
      | 'rotateX'
      | 'rotateY'
      | 'rotateZ'
      | 'scale'
      | 'scaleX'
      | 'scaleY'
      | (string & {});
    animationTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    borderStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    transformOrigin?: 'left' | 'right' | 'top' | 'bottom' | 'center' | (string & {});
    linearDirection?: 'column' | 'row' | 'column-reverse' | 'row-reverse';
    linearOrientation?: 'horizontal' | 'vertical' | 'horizontal-reverse' | 'vertical-reverse';
    linearWeight?: number;
    linearGravity?: 'none' | 'top' | 'bottom' | 'left' | 'right' | 'center-vertical' | 'center-horizontal' | 'space-between' | 'start' | 'end' | 'center';
    linearLayoutGravity?:
      | 'none'
      | 'top'
      | 'bottom'
      | 'left'
      | 'right'
      | 'center-vertical'
      | 'center-horizontal'
      | 'fill-vertical'
      | 'fill-horizontal'
      | 'center'
      | 'stretch'
      | 'start'
      | 'end';
    layoutAnimationCreateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    layoutAnimationCreateProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
    layoutAnimationDeleteTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    layoutAnimationDeleteProperty?: 'opacity' | 'scaleX' | 'scaleY' | 'scaleXY' | (string & {});
    layoutAnimationUpdateTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    textDecoration?: 'none' | 'underline' | 'line-through' | (string & {});
    visibility?: 'hidden' | 'visible' | 'none' | 'collapse';
    transitionProperty?:
      | 'none'
      | 'opacity'
      | 'scaleX'
      | 'scaleY'
      | 'scaleXY'
      | 'width'
      | 'height'
      | 'background-color'
      | 'visibility'
      | 'left'
      | 'top'
      | 'right'
      | 'bottom'
      | 'transform'
      | 'all'
      | (string & {});
    transitionTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | 'ease-in-ease-out' | 'ease' | 'ease-in-out' | 'square-bezier' | 'cubic-bezier' | (string & {});
    borderLeftStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderRightStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderTopStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    borderBottomStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    overflowX?: 'hidden' | 'visible' | (string & {});
    overflowY?: 'hidden' | 'visible' | (string & {});
    wordBreak?: 'normal' | 'break-all' | 'keep-all';
    outlineStyle?: 'solid' | 'dashed' | 'dotted' | 'double' | 'groove' | 'ridge' | 'inset' | 'outset' | 'hidden' | 'none' | (string & {});
    verticalAlign?: 'baseline' | 'sub' | 'super' | 'top' | 'text-top' | 'middle' | 'bottom' | 'text-bottom' | (string & {});
    direction?: 'normal' | 'lynx-rtl' | 'rtl' | 'ltr';
    relativeCenter?: 'none' | 'vertical' | 'horizontal' | 'both';
    linearCrossGravity?: 'none' | 'start' | 'end' | 'center' | 'stretch';
    listMainAxisGap?: 'grayscale' | (string & {});
    fontVariationSettings?: string;
    fontFeatureSettings?: string;
    fontOpticalSizing?: 'none' | 'auto';
  }
>;

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
  "outlineColor" | "outlineStyle" | "outlineWidth" | "textDecorationColor" | "linearCrossGravity" | "borderInlineStartColor" | "borderInlineEndColor" | "borderInlineStartWidth" | "borderInlineEndWidth" | "borderInlineStartStyle" | "borderInlineEndStyle" | "relativeAlignInlineStart" | "relativeAlignInlineEnd" | "relativeInlineStartOf" | "relativeInlineEndOf" | "insetInlineStart" | "insetInlineEnd" | "linearDirection" | "textIndent" | "textStroke" | "textStrokeWidth" | "textStrokeColor" | "XAutoFontSize" | "XAutoFontSizePresetSizes" | "fontVariationSettings" | "fontFeatureSettings" | "fontOpticalSizing" | "textAlign" | "lineHeight" | "textOverflow" | "fontSize" | "fontWeight" | "fontFamily" | "fontStyle" | "lineSpacing" | "linearOrientation" | "linearWeightSum" | "linearWeight" | "linearGravity" | "linearLayoutGravity" | "adaptFontSize" | "textShadow" |
  // visual
  "borderTopColor" | "backgroundOrigin" | "backgroundRepeat" | "backgroundSize" | "borderBottomColor" | "borderLeftStyle" | "borderRightStyle" | "borderTopStyle" | "borderBottomStyle" | "backgroundClip" | "caretColor" | "borderTopLeftRadius" | "borderBottomLeftRadius" | "borderTopRightRadius" | "borderBottomRightRadius" | "borderStartStartRadius" | "borderEndStartRadius" | "borderStartEndRadius" | "borderEndEndRadius" | "borderLeftWidth" | "borderRightWidth" | "borderTopWidth" | "borderBottomWidth" | "XAnimationColorInterpolation" | "XHandleColor" | "color" | "backgroundColor" | "borderLeftColor" | "borderRightColor" | "backgroundImage" |
  // animation
  "transitionProperty" | "transitionDuration" | "transitionDelay" | "transitionTimingFunction" | "implicitAnimation" | "enterTransitionName" | "exitTransitionName" | "pauseTransitionName" | "resumeTransitionName" | "animationName" | "animationDuration" | "animationTimingFunction" | "animationDelay" | "animationIterationCount" | "animationDirection" | "animationFillMode" | "animationPlayState" | "layoutAnimationCreateDuration" | "layoutAnimationCreateTimingFunction" | "layoutAnimationCreateDelay" | "layoutAnimationCreateProperty" | "layoutAnimationDeleteDuration" | "layoutAnimationDeleteTimingFunction" | "layoutAnimationDeleteDelay" | "layoutAnimationDeleteProperty" | "layoutAnimationUpdateDuration" | "layoutAnimationUpdateTimingFunction" | "layoutAnimationUpdateDelay" |
  // other
  "top" | "visibility" | "content" | "overflowX" | "overflowY" | "wordBreak" | "verticalAlign" | "direction" | "relativeId" | "relativeAlignTop" | "relativeAlignRight" | "relativeAlignBottom" | "relativeAlignLeft" | "relativeTopOf" | "relativeRightOf" | "relativeBottomOf" | "relativeLeftOf" | "relativeLayoutOnce" | "relativeCenter" | "zIndex" | "maskImage" | "justifyItems" | "justifySelf" | "filter" | "listMainAxisGap" | "listCrossAxisGap" | "perspective" | "cursor" | "clipPath" | "left" | "maskRepeat" | "maskClip" | "maskOrigin" | "maskSize" | "columnGap" | "rowGap" | "imageRendering" | "hyphens" | "XAppRegion" | "XHandleSize" | "offsetDistance" | "offsetPath" | "offsetRotate" | "opacity" | "height" | "width" | "maxWidth" | "minWidth" | "right" | "maxHeight" | "minHeight" | "bottom" | "letterSpacing" | "alignItems" | "alignSelf" | "alignContent" | "justifyContent" | "boxSizing" | "transform" | "order" | "boxShadow" | "transformOrigin" | "aspectRatio";

// Since `Shorthands` and `Longhands` are auto generated, there may be properties
// such as `gridColumnSpan` is not manually defined in `CSSProperties` yet.
// Use `& keyof CSSProperties` to ensure only the defined keys are included to avoid type error.
export type CSSPropertiesWithShorthands = Pick<CSSProperties, Shorthands & keyof CSSProperties>;
export type CSSPropertiesWithLonghands = Pick<CSSProperties, Longhands & keyof CSSProperties>;
