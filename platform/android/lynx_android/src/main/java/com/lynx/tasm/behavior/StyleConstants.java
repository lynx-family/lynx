// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import androidx.annotation.IntDef;
import com.lynx.tasm.behavior.AutoGenStyleConstants;

/**
 * auto generator by css generator.
 * Please don't modify manually
 * should equals to css_type.h's defines.
 */
public final class StyleConstants implements AutoGenStyleConstants {
  // AnimationDirection
  public static final int ANIMATIONDIRECTION_NORMAL = 0; // version:1.0
  public static final int ANIMATIONDIRECTION_REVERSE = 1; // version:1.0
  public static final int ANIMATIONDIRECTION_ALTERNATE = 2; // version:1.0
  public static final int ANIMATIONDIRECTION_ALTERNATEREVERSE = 3; // version:1.0

  // AnimationFillMode
  public static final int ANIMATIONFILLMODE_NONE = 0; // version:1.0
  public static final int ANIMATIONFILLMODE_FORWARDS = 1; // version:1.0
  public static final int ANIMATIONFILLMODE_BACKWARDS = 2; // version:1.0
  public static final int ANIMATIONFILLMODE_BOTH = 3; // version:1.0

  // AnimationPlayState
  public static final int ANIMATIONPLAYSTATE_PAUSED = 0; // version:1.0
  public static final int ANIMATIONPLAYSTATE_RUNNING = 1; // version:1.0

  // AnimationIterationCount
  public static final double ANIMATIONITERATIONCOUNT_INFINITE = 1e9; // version:1.0

  // TextAlign
  public static final int TEXTALIGN_LEFT = 0; // version:1.0
  public static final int TEXTALIGN_CENTER = 1; // version:1.0
  public static final int TEXTALIGN_RIGHT = 2; // version:1.0
  public static final int TEXTALIGN_START = 3; // version:2.0
  public static final int TEXTALIGN_END = 4; // version:2.0
  public static final int TEXTALIGN_JUSTIFY = 5; // version:2.10
  @Deprecated public static final int TEXTALIGN_AUTO = TEXTALIGN_START; // version:1.0

  // Direction
  public static final int DIRECTION_NORMAL = 0; // version:1.0
  public static final int DIRECTION_RTL = 2; // version:1.0
  public static final int DIRECTION_LTR = 3; // version:2.0
  @Deprecated public static final int DIRECTION_LYNXRTL = DIRECTION_RTL; // version:1.0

  // Position
  public static final int POSITION_ABSOLUTE = 0; // version:1.0
  public static final int POSITION_RELATIVE = 1; // version:1.0
  public static final int POSITION_FIXED = 2; // version:1.0
  public static final int POSITION_STICKY = 3; // version:1.4

  public static final int OVERFLOW_VISIBLE = 0;
  public static final int OVERFLOW_HIDDEN = 1;
  public static final int OVERFLOW_SCROLL = 2;
  @IntDef({OVERFLOW_VISIBLE, OVERFLOW_HIDDEN, OVERFLOW_SCROLL})
  public @interface OverflowType {}

  public static final int TEXT_DECORATION_NONE = 0;
  public static final int TEXT_DECORATION_UNDERLINE = 1;
  public static final int TEXT_DECORATION_LINETHROUGH = 1 << 1;

  @IntDef({TEXT_DECORATION_NONE, TEXT_DECORATION_UNDERLINE, TEXT_DECORATION_LINETHROUGH})
  public @interface TextDecoration {}

  public static final int TRANSFORM_NONE = 0;
  public static final int TRANSFORM_TRANSLATE = 1;
  public static final int TRANSFORM_TRANSLATE_X = 1 << 1;
  public static final int TRANSFORM_TRANSLATE_Y = 1 << 2;
  public static final int TRANSFORM_TRANSLATE_Z = 1 << 3;
  public static final int TRANSFORM_TRANSLATE_3d = 1 << 4;
  public static final int TRANSFORM_ROTATE = 1 << 5;
  public static final int TRANSFORM_ROTATE_X = 1 << 6;
  public static final int TRANSFORM_ROTATE_Y = 1 << 7;
  public static final int TRANSFORM_ROTATE_Z = 1 << 8;
  public static final int TRANSFORM_SCALE = 1 << 9;
  public static final int TRANSFORM_SCALE_X = 1 << 10;
  public static final int TRANSFORM_SCALE_Y = 1 << 11;
  public static final int TRANSFORM_SKEW = 1 << 12;
  public static final int TRANSFORM_SKEW_X = 1 << 13;
  public static final int TRANSFORM_SKEW_Y = 1 << 14;
  public static final int TRANSFORM_MATRIX = 1 << 15;
  public static final int TRANSFORM_MATRIX_3d = 1 << 16;

  public static final int PLATFORM_LENGTH_UNIT_NUMBER = 0;
  public static final int PLATFORM_LENGTH_UNIT_PERCENT = 1;
  public static final int PLATFORM_LENGTH_UNIT_CALC = 2;

  public static final int PLATFORM_PERSPECTIVE_UNIT_NUMBER = 0;
  public static final int PLATFORM_PERSPECTIVE_UNIT_VW = 1;
  public static final int PLATFORM_PERSPECTIVE_UNIT_VH = 2;
  public static final int PLATFORM_PERSPECTIVE_UNIT_DEFAULT = 3;
  public static final int PLATFORM_PERSPECTIVE_UNIT_PX = 4;

  // BackgroundClip
  public static final int BACKGROUND_CLIP_PADDING_BOX = 0;
  public static final int BACKGROUND_CLIP_BORDER_BOX = 1;
  public static final int BACKGROUND_CLIP_CONTENT_BOX = 2;

  // BackgroundOrigin
  public static final int BACKGROUND_ORIGIN_PADDING_BOX = 0;
  public static final int BACKGROUND_ORIGIN_BORDER_BOX = 1;
  public static final int BACKGROUND_ORIGIN_CONTENT_BOX = 2;

  // BackgroundImageType
  public static final int BACKGROUND_IMAGE_NONE = 0;
  // BACKGROUND_IMAGE_URL is used in ElementManager::ResolveStaticAssetPath by number. Make sure to
  // keep consistent if you change the value of LynxBackgroundImageURL
  public static final int BACKGROUND_IMAGE_URL = 1;
  public static final int BACKGROUND_IMAGE_LINEAR_GRADIENT = 2;
  public static final int BACKGROUND_IMAGE_RADIAL_GRADIENT = 3;

  // BackgroundPositionType
  public static final int BACKGROUND_POSITION_TOP = -(1 << 5);
  public static final int BACKGROUND_POSITION_RIGHT = -(1 << 5) - 1;
  public static final int BACKGROUND_POSITION_BOTTOM = -(1 << 5) - 2;
  public static final int BACKGROUND_POSITION_LEFT = -(1 << 5) - 3;
  public static final int BACKGROUND_POSITION_CENTER = -(1 << 5) - 4;

  // BackgroundSizeType
  public static final int BACKGROUND_SIZE_AUTO = -(1 << 5);
  public static final int BACKGROUND_SIZE_COVER = -(1 << 5) - 1;
  public static final int BACKGROUND_SIZE_CONTAIN = -(1 << 5) - 2;

  // VerticalAlign
  public static final int VERTICAL_ALIGN_DEFAULT = 0;
  public static final int VERTICAL_ALIGN_BASELINE = 1;
  public static final int VERTICAL_ALIGN_SUB = 2;
  public static final int VERTICAL_ALIGN_SUPER = 3;
  public static final int VERTICAL_ALIGN_TOP = 4;
  public static final int VERTICAL_ALIGN_TEXT_TOP = 5;
  public static final int VERTICAL_ALIGN_MIDDLE = 6;
  public static final int VERTICAL_ALIGN_BOTTOM = 7;
  public static final int VERTICAL_ALIGN_TEXT_BOTTOM = 8;
  public static final int VERTICAL_ALIGN_LENGTH = 9;
  public static final int VERTICAL_ALIGN_PERCENT = 10;
  public static final int VERTICAL_ALIGN_CENTER = 11;

  //  enum class ShadowOption { kNone = 0, kInset = 1, kInitial = 2, kInherit = 3 };
  public static final int SHADOW_OPTION_NONE = 0;
  public static final int SHADOW_OPTION_INSET = 1;
  public static final int SHADOW_OPTION_INITIAL = 2;
  public static final int SHADOW_OPTION_INHERIT = 3;

  // Fiber text style
  public static final int TEXT_PROP_ID_FONT_SIZE = 1;
  public static final int TEXT_PROP_ID_COLOR = 2;
  public static final int TEXT_PROP_ID_WHITESPACE = 3;
  public static final int TEXT_PROP_ID_TEXT_OVERFLOW = 4;
  public static final int TEXT_PROP_ID_FONT_WEIGHT = 5;
  public static final int TEXT_PROP_ID_FONT_STYLE = 6;
  public static final int TEXT_PROP_ID_LINE_HEIGHT = 7;
  public static final int TEXT_PROP_ID_ENABLE_FONTSCALING = 8;
  public static final int TEXT_PROP_ID_LETTER_SPACING = 9;
  public static final int TEXT_PROP_ID_LINE_SPACING = 10;
  public static final int TEXT_PROP_ID_TEXT_ALIGN = 11;
  public static final int TEXT_PROP_ID_WORD_BREAK = 12;
  public static final int TEXT_PROP_ID_UNDERLINE = 13;
  public static final int TEXT_PROP_ID_LINETHROUGH = 14;
  public static final int TEXT_PROP_ID_HAS_TEXTSHADOW = 15;
  public static final int TEXT_PROP_ID_SHADOW_HOFFSET = 16;
  public static final int TEXT_PROP_ID_SHADOW_VOFFSET = 17;
  public static final int TEXT_PROP_ID_SHADOW_BLUR = 18;
  public static final int TEXT_PROP_ID_SHADOW_COLOR = 19;
  public static final int TEXT_PROP_ID_VERTICAL_ALIGN = 20;
  public static final int TEXT_PROP_ID_VERTICAL_ALIGN_LENGTH = 21;

  public static final int FILTER_TYPE_NONE = 0;
  public static final int FILTER_TYPE_GRAYSCALE = 1;
  public static final int FILTER_TYPE_BLUR = 2;

  public static final int BASIC_SHAPE_TYPE_UNKNOWN = 0;
  public static final int BASIC_SHAPE_TYPE_CIRCLE = 1;
  public static final int BASIC_SHAPE_TYPE_ELLIPSE = 2;
  public static final int BASIC_SHAPE_TYPE_PATH = 3;
  public static final int BASIC_SHAPE_TYPE_SUPER_ELLIPSE = 4;
  public static final int BASIC_SHAPE_TYPE_INSET = 5;

  public static final int BORDER_SIDE_TOP = 0;
  public static final int BORDER_SIDE_RIGHT = 1;
  public static final int BORDER_SIDE_BOTTOM = 2;
  public static final int BORDER_SIDE_LEFT = 3;
}
