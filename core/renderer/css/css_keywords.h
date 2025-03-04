// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_KEYWORDS_H_
#define CORE_RENDERER_CSS_CSS_KEYWORDS_H_

namespace lynx {
namespace tasm {

enum class TokenType {
  /* tokens begin */
  LEFT_PAREN,   // (
  RIGHT_PAREN,  // )
  COMMA,        // ,
  COLON,        // :
  DOT,          // .
  MINUS,        // -
  PLUS,         // +
  SEMICOLON,    // ;
  SLASH,        // /
  SHARP,        // #
  PERCENTAGE,   // %
  EQUAL,        // =
  IDENTIFIER,   // {a-z, A-Z, or _}, { a-z A-Z 0-9 _ }
  STRING,       // ''
  NUMBER,       // number
  DIMENSION,    // number + ident
  HEX,          // #
  ERROR,
  TOKEN_EOF,
  WHITESPACE,
  UNKNOWN,
  /* tokens end */

  // Keywords below are ident-like tokens, as new token type for compatibility
  RGB,              // rgb
  RGBA,             // rgba
  HSL,              // hsl
  HSLA,             // hsla
  URL,              // url
  NONE,             // none
  TO,               // to
  CENTER,           // center
  PX,               // px
  RPX,              // rpx
  REM,              // rem
  EM,               // em
  VW,               // vw
  VH,               // vh
  PPX,              // ppx
  FR,               // fr
  SP,               // sp
  MAX_CONTENT,      // max-content
  DEG,              // deg
  GRAD,             // GRAD
  RAD,              // rad
  TURN,             // turn
  AUTO,             // auto
  COVER,            // cover
  CONTAIN,          // contain
  REPEAT_X,         // repeat-x
  REPEAT_Y,         // repeat-y
  REPEAT,           // repeat
  NO_REPEAT,        // no-repeat
  SPACE,            // space
  ROUND,            // round
  BORDER_BOX,       // border-box
  PADDING_BOX,      // padding-box
  CONTENT_BOX,      // content-box
  LINEAR_GRADIENT,  // linear-gradient
  RADIAL_GRADIENT,  // radial-gradient
  CLOSEST_SIDE,     // closest-side
  CLOSEST_CORNER,   // closest-corner
  FARTHEST_SIDE,    // farthest-side
  FARTHEST_CORNER,  // farthest-corner
  ELLIPSE,          // ellipse
  CIRCLE,           // circle
  POLYGON,          // polygon
  SUPER_ELLIPSE,    // super-ellipse
  PATH,             // path
  AT,               // at
  DATA,             // data
  /* border keywords */
  THIN,    // thin
  MEDIUM,  // medium
  THICK,   // thick
  HIDDEN,  // hidden
  DOTTED,  // dotted
  DASHED,  // dashed
  SOLID,   // solid
  DOUBLE,  // double
  GROOVE,  // groove
  RIDGE,   // ridge
  INSET,   // inset
  OUTSET,  // outset
  /* text-decoration */
  UNDERLINE,
  LINE_THROUGH,
  WAVY,
  /* font-face */
  FORMAT,
  LOCAL,
  NORMAL,
  BOLD,
  /* compatible history keywords */
  TOBOTTOM,  // tobottom
  TOLEFT,    // toleft
  TORIGHT,   // toright
  TOTOP,     // totop
  /* function begin */
  CALC,         // calc(1px + 12px)
  ENV,          // env(safe-area-inset-right)
  FIT_CONTENT,  // fit-content(20px)
  BLUR,         // blur(2px)

  /* transform begin */
  ROTATE,
  ROTATE_X,
  ROTATE_Y,
  ROTATE_Z,
  TRANSLATE,
  TRANSLATE_3D,
  TRANSLATE_X,
  TRANSLATE_Y,
  TRANSLATE_Z,
  SCALE,
  SCALE_X,
  SCALE_Y,
  SKEW,
  SKEW_X,
  SKEW_Y,
  MATRIX,
  MATRIX_3D,
  /* transform end */

  SQUARE_BEZIER,  // square-bezier
  CUBIC_BEZIER,   // cubic-bezier
  STEPS,          // steps

  GRAYSCALE,  // grayscale(10%)
  /* function end */

  /* color begin, the order is associated with CSSColor */
  TRANSPARENT,
  ALICEBLUE,
  ANTIQUEWHITE,
  AQUA,
  AQUAMARINE,
  AZURE,
  BEIGE,
  BISQUE,
  BLACK,
  BLANCHEDALMOND,
  BLUE,
  BLUEVIOLET,
  BROWN,
  BURLYWOOD,
  CADETBLUE,
  CHARTREUSE,
  CHOCOLATE,
  CORAL,
  CORNFLOWERBLUE,
  CORNSILK,
  CRIMSON,
  CYAN,
  DARKBLUE,
  DARKCYAN,
  DARKGOLDENROD,
  DARKGRAY,
  DARKGREEN,
  DARKGREY,
  DARKKHAKI,
  DARKMAGENTA,
  DARKOLIVEGREEN,
  DARKORANGE,
  DARKORCHID,
  DARKRED,
  DARKSALMON,
  DARKSEAGREEN,
  DARKSLATEBLUE,
  DARKSLATEGRAY,
  DARKSLATEGREY,
  DARKTURQUOISE,
  DARKVIOLET,
  DEEPPINK,
  DEEPSKYBLUE,
  DIMGRAY,
  DIMGREY,
  DODGERBLUE,
  FIREBRICK,
  FLORALWHITE,
  FORESTGREEN,
  FUCHSIA,
  GAINSBORO,
  GHOSTWHITE,
  GOLD,
  GOLDENROD,
  GRAY,
  GREEN,
  GREENYELLOW,
  GREY,
  HONEYDEW,
  HOTPINK,
  INDIANRED,
  INDIGO,
  IVORY,
  KHAKI,
  LAVENDER,
  LAVENDERBLUSH,
  LAWNGREEN,
  LEMONCHIFFON,
  LIGHTBLUE,
  LIGHTCORAL,
  LIGHTCYAN,
  LIGHTGOLDENRODYELLOW,
  LIGHTGRAY,
  LIGHTGREEN,
  LIGHTGREY,
  LIGHTPINK,
  LIGHTSALMON,
  LIGHTSEAGREEN,
  LIGHTSKYBLUE,
  LIGHTSLATEGRAY,
  LIGHTSLATEGREY,
  LIGHTSTEELBLUE,
  LIGHTYELLOW,
  LIME,
  LIMEGREEN,
  LINEN,
  MAGENTA,
  MAROON,
  MEDIUMAQUAMARINE,
  MEDIUMBLUE,
  MEDIUMORCHID,
  MEDIUMPURPLE,
  MEDIUMSEAGREEN,
  MEDIUMSLATEBLUE,
  MEDIUMSPRINGGREEN,
  MEDIUMTURQUOISE,
  MEDIUMVIOLETRED,
  MIDNIGHTBLUE,
  MINTCREAM,
  MISTYROSE,
  MOCCASIN,
  NAVAJOWHITE,
  NAVY,
  OLDLACE,
  OLIVE,
  OLIVEDRAB,
  ORANGE,
  ORANGERED,
  ORCHID,
  PALEGOLDENROD,
  PALEGREEN,
  PALETURQUOISE,
  PALEVIOLETRED,
  PAPAYAWHIP,
  PEACHPUFF,
  PERU,
  PINK,
  PLUM,
  POWDERBLUE,
  PURPLE,
  RED,
  ROSYBROWN,
  ROYALBLUE,
  SADDLEBROWN,
  SALMON,
  SANDYBROWN,
  SEAGREEN,
  SEASHELL,
  SIENNA,
  SILVER,
  SKYBLUE,
  SLATEBLUE,
  SLATEGRAY,
  SLATEGREY,
  SNOW,
  SPRINGGREEN,
  STEELBLUE,
  TAN,
  TEAL,
  THISTLE,
  TOMATO,
  TURQUOISE,
  VIOLET,
  WHEAT,
  WHITE,
  WHITESMOKE,
  YELLOW,
  YELLOWGREEN,
  /* color end */

  /* time */
  SECOND,
  MILLISECOND,

  /* transition begin */
  // NONE,
  OPACITY,  // opacity
  // SCALE_X,
  // SCALE_Y,
  SCALE_XY,             // scaleXY
  WIDTH,                // width
  HEIGHT,               // height
  BACKGROUND_COLOR,     // background-color
  COLOR,                // color
  VISIBILITY,           // visibility
  LEFT,                 // left
  TOP,                  // top
  RIGHT,                // right
  BOTTOM,               // bottom
  TRANSFORM,            // transform
  ALL,                  // all
  MAX_WIDTH,            // max-width
  MAX_HEIGHT,           // max-height
  MIN_WIDTH,            // min-width
  MIN_HEIGHT,           // min-height
  PADDING_LEFT,         // padding-left
  PADDING_RIGHT,        // padding-right
  PADDING_TOP,          // padding-top
  PADDING_BOTTOM,       // padding-bottom
  MARGIN_LEFT,          // margin-left
  MARGIN_RIGHT,         // margin-right
  MARGIN_TOP,           // margin-top
  MARGIN_BOTTOM,        // margin-bottom
  BORDER_LEFT_COLOR,    // border-left-color
  BORDER_RIGHT_COLOR,   // border-right-color
  BORDER_TOP_COLOR,     // border-top-color
  BORDER_BOTTOM_COLOR,  // border-bottom-color
  BORDER_LEFT_WIDTH,    // border-left-width
  BORDER_RIGHT_WIDTH,   // border-right-width
  BORDER_TOP_WIDTH,     // border-top-width
  BORDER_BOTTOM_WIDTH,  // border-bottom-width
  FLEX_BASIS,           // flex-basis
  FLEX_GROW,            // flex-grow
  BORDER_WIDTH,         // border-width
  BORDER_COLOR,         // border-color
  PADDING,              // padding
  MARGIN,               // margin
  FILTER,               // filter
  /* transition end*/

  /* timing function begin */
  LINEAR,            // linear
  EASE_IN,           // ease-in
  EASE_OUT,          // ease-out
  EASE_IN_EASE_OUT,  // ease-in-ease-out
  EASE,              // ease
  EASE_IN_OUT,       // ease-in-out
  STEP_START,        // step-start
  STEP_END,          // step-end

  /* timing function end */
  REVERSE,
  ALTERNATE,
  ALTERNATE_REVERSE,

  FORWARDS,
  BACKWARDS,
  BOTH,

  INFINITE,

  PAUSED,
  RUNNING,

  TOKEN_TRUE,
  TOKEN_FALSE,
};

struct TokenValue {
  const char* name;
  TokenType type;
};

const struct TokenValue* GetTokenValue(const char* str, unsigned len);

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_CSS_CSS_KEYWORDS_H_
