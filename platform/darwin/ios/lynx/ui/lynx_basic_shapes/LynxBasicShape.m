// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBasicShape.h"
#import "LBSCoreGraphicsPathParser.h"
#import "LynxBackgroundInfo.h"
#import "LynxBackgroundUtils.h"

typedef NS_ENUM(NSInteger, LBSCornerType) {
  LBSCornerTypeDefault = 0,
  LBSCornerTypeRect = 1,
  LBSCornerTypeRounded = 2,
  LBSCornerTypeSuperElliptical = 3,
};

@implementation LynxBasicShape {
 @public
  LynxBorderRadii* _cornerRadii;
 @public
  LBSCornerType _cornerType;
 @public
  LynxBorderUnitValue* _params;
 @public
  LynxBasicShapeType _type;
  UIBezierPath* _path;
  CGSize _size;
}

- (UIBezierPath*)pathWithFrameSize:(CGSize)frameSize {
  if (_type == LynxBasicShapeTypePath) {
    // Paths are generated when set because they are irrelevant to UI size.
    return _path;
  }

  if (CGSizeEqualToSize(_size, frameSize) && _path) {
    return _path;
  }
  _size = frameSize;
  CGPathRef cPath = LBSCreatePathFromBasicShape(self, frameSize);
  if (cPath) {
    _path = [UIBezierPath bezierPathWithCGPath:cPath];
    CGPathRelease(cPath);
  }
  return _path;
}
- (void)dealloc {
  if (_cornerRadii) {
    free(_cornerRadii);
    _cornerRadii = NULL;
  }
  if (_params) {
    free(_params);
    _params = NULL;
  }
}
@end

/// Get the LynxBorderUnitValue from
/// - Parameters:
///   - value: target value
///   - args: args array
///   - position: The location of the value in the args array
void LBSGetCornerRadiiValue(LynxBorderUnitValue* value, NSArray<NSNumber*>* args, int position) {
  (*value) = (LynxBorderUnitValue){.val = [[args objectAtIndex:position] doubleValue],
                                   .unit = [[args objectAtIndex:position + 1] intValue]};
  if (value->unit == LynxBorderValueUnitPercent) {
    // Adapt to LynxCornerRadii, percentage value is in [0, 100];
    value->val *= 100;
  }
}

static LynxBasicShape* _Nullable LBSCreateInsetFromArray(NSArray<NSNumber*>* _Nonnull array) {
  LynxBasicShape* clipPath = [[LynxBasicShape alloc] init];
  // clang-format off
  //  ______________________________________________________________________________
  // |                       Meaning of Fields in `array`                           |
  // --------------------------------------------------------------------------------
  // |   0    |   1   |    2   |    3    |    4    |     5     |    6   |     7     |
  // |  type  |  top  |  unit  |  right  |   unit  |   bottom  |  unit  |   left    |
  // --------------------------------------------------------------------------------
  // |   8    |    9  |   10   |      11       |   12   |           13              |
  // |  unit  |   ex  |   ey   |   top-left-x  |  unit  |        top-left-y         |
  // --------------------------------------------------------------------------------
  // |   14   |       15       |    16   |      17      |  18  |         19         |
  // |  unit  |  top-right-x   |   unit  | top-right-y  | unit |   bottom-right-x   |
  // --------------------------------------------------------------------------------
  // |   20   |       21       |  22  |       23      |  24  |      25       |  26  |
  // |  unit  | bottom-right-y | unit | bottom-left-x | unit | bottom-left-y | unit |
  // --------------------------------------------------------------------------------
  // clang-format on
  clipPath->_params = (LynxBorderUnitValue*)malloc(sizeof(LynxBorderUnitValue) * 6);
  clipPath->_type = LynxBasicShapeTypeInset;
  unsigned long argsCount = [array count];
  switch (argsCount) {
    case 9:
      clipPath->_cornerType = LBSCornerTypeRect;
      break;
    case 25:
      clipPath->_cornerType = LBSCornerTypeRounded;
      break;
    case 27:
      clipPath->_cornerType = LBSCornerTypeSuperElliptical;
      break;
    default:
      // Error generating basic shape.
      return NULL;
  }

  // get insets values from params array
  for (int i = 0; i < 4; i++) {
    clipPath->_params[i].val = [[array objectAtIndex:(2 * i + 1)] doubleValue];
    clipPath->_params[i].unit = [[array objectAtIndex:(2 * i + 2)] doubleValue];
  }

  int radiusOffset = 9;
  switch (clipPath->_cornerType) {
    case LBSCornerTypeDefault:
    case LBSCornerTypeRect:
      break;
    case LBSCornerTypeSuperElliptical:
      // Exponents value
      clipPath->_params[4].val = [[array objectAtIndex:9] doubleValue];
      clipPath->_params[5].val = [[array objectAtIndex:10] doubleValue];
      radiusOffset = 11;
    case LBSCornerTypeRounded:
      // clang-format off
      // Get border radius value from next 16 values.
      // [top-left-x,     unit, top-left-y,     unit, top-right-x,   unit, top-right-y,   unit,
      //  bottom-right-x, unit, bottom-right-y, unit, bottom-left-x, unit, bottom-left-y, unit]
      // clang-format on
      clipPath->_cornerRadii = (LynxBorderRadii*)malloc(sizeof(LynxBorderRadii));
      LynxBorderUnitValue* values = (LynxBorderUnitValue*)(clipPath->_cornerRadii);
      // Get the border radius values
      for (int i = 0; i < 8; i++) {
        LBSGetCornerRadiiValue(values + i, array, radiusOffset + (i * 2));
      }
  }
  return clipPath;
}

static LynxBasicShape* _Nullable LBSCreateCircleFromArray(NSArray<NSNumber*>* _Nonnull array) {
#define SIZE_CIRCLE_RAW_ARRAY 7    // [type, radius, unit, position x, unit, postion y, unit]
#define SIZE_CIRCLE_PARAM_ARRAY 3  // [radius, position x, position y]
  if ([array count] != SIZE_CIRCLE_RAW_ARRAY) {
    // illegal argument for generating circle
    return NULL;
  }
  LynxBasicShape* clipPath = [[LynxBasicShape alloc] init];
  clipPath->_type = LynxBasicShapeTypeCircle;
  clipPath->_params = malloc(sizeof(LynxBorderUnitValue) *
                             SIZE_CIRCLE_PARAM_ARRAY);  // radius, position x, position y
  LynxBorderUnitValue* it = clipPath->_params;

  for (int i = 0; i < SIZE_CIRCLE_PARAM_ARRAY; i++) {
    *(it + i) = (LynxBorderUnitValue){
        .val = [[array objectAtIndex:(2 * i + 1)] doubleValue],
        .unit = (LynxBorderValueUnit)[[array objectAtIndex:2 * i + 2] intValue]};
  }
  return clipPath;
#undef SIZE_CIRCLE_PARAM_ARRAY
#undef SIZE_CIRCLE_RAW_ARRAY
};

LynxBasicShape* _Nullable LBSCreateBasicShapeFromArray(NSArray<NSNumber*>* array) {
  if (!array || [array count] < 1) {
    return NULL;
  }
  LynxBasicShapeType type = [[array objectAtIndex:0] intValue];
  if (type == LynxBasicShapeTypeInset) {
    return LBSCreateInsetFromArray(array);
  } else if (type == LynxBasicShapeTypeCircle) {
    return LBSCreateCircleFromArray(array);
  }
  return NULL;
}

/// Add lame curve to target Path
/// - Parameters:
///   - path: target path
///   - rx: radius on x axis
///   - ry: radius on y axis
///   - cx: center position on x axis
///   - cy: center position on y axis
///   - ex: exponent for x
///   - ey: exponent for y
///   - quadrant: which quadrant the curve located in.
void LBSAddLameCurveToPath(CGMutablePathRef path, CGFloat rx, CGFloat ry, CGFloat cx, CGFloat cy,
                           CGFloat ex, CGFloat ey, int quadrant) {
  double cosI, sinI, x, y;
  float fx = (quadrant == 1 || quadrant == 4) ? 1 : -1;
  float fy = (quadrant == 1 || quadrant == 2) ? 1 : -1;
  for (float i = (float)(M_PI_2 * (quadrant - 1)); i < M_PI_2 * quadrant; i += 0.01) {
    // abs for cos and sin
    cosI = fx * cos(i);
    sinI = fy * sin(i);
    x = fx * rx * pow(cosI, 2 / ex) + cx;
    y = fy * ry * pow(sinI, 2 / ey) + cy;
    if (i == 0) {
      CGPathMoveToPoint(path, nil, x, y);
    } else {
      CGPathAddLineToPoint(path, nil, x, y);
    }
  }
}

CGFloat LBSBorderUnitToFloat(LynxBorderUnitValue val, CGFloat lengthContext) {
  return val.unit == LynxBorderValueUnitPercent ? val.val * lengthContext : val.val;
}

static void LBSAddInsetPathToPath(CGMutablePathRef path, LynxBasicShape* _Nonnull shape,
                                  const CGSize* viewport) {
  CGRect bounds = CGRectMake(0, 0, viewport->width, viewport->height);
  UIEdgeInsets insets;
  insets.top = LBSBorderUnitToFloat(shape->_params[0], viewport->height);
  insets.right = LBSBorderUnitToFloat(shape->_params[1], viewport->width);
  insets.bottom = LBSBorderUnitToFloat(shape->_params[2], viewport->height);
  insets.left = LBSBorderUnitToFloat(shape->_params[3], viewport->width);
  UIEdgeInsets adjustedInsets = LynxGetEdgeInsets(bounds, insets, 1.0);
  CGRect innerBounds = LynxGetRectWithEdgeInsets(bounds, adjustedInsets);
  switch (shape->_cornerType) {
    case LBSCornerTypeRounded:
      LynxPathAddRoundedRect(
          path, innerBounds,
          LynxGetCornerInsets(innerBounds, *(shape->_cornerRadii), UIEdgeInsetsZero));
      break;
    case LBSCornerTypeSuperElliptical: {
      LynxCornerInsets cornerRadius =
          LynxGetCornerInsets(innerBounds, *(shape->_cornerRadii), UIEdgeInsetsZero);
      float left = innerBounds.origin.x;
      float top = innerBounds.origin.y;
      float right = left + innerBounds.size.width;
      float bottom = top + innerBounds.size.height;
      float rx = cornerRadius.bottomRight.width;
      float ry = cornerRadius.bottomRight.height;
      float cx = right - rx;
      float cy = bottom - ry;
      float ex = shape->_params[4].val;
      float ey = shape->_params[5].val;
      LBSAddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 1);

      rx = cornerRadius.bottomLeft.width;
      ry = cornerRadius.bottomLeft.height;
      cx = left + rx;
      cy = bottom - ry;
      LBSAddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 2);

      rx = cornerRadius.topLeft.width;
      ry = cornerRadius.topLeft.height;
      cx = left + rx;
      cy = top + ry;
      LBSAddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 3);

      rx = cornerRadius.topRight.width;
      ry = cornerRadius.topRight.height;
      cx = right - rx;
      cy = top + ry;
      LBSAddLameCurveToPath(path, rx, ry, cx, cy, ex, ey, 4);
      CGPathCloseSubpath(path);
      break;
    }
    default:
      CGPathAddRect(path, NULL, innerBounds);
      break;
  }
}

static void LBSAddCircleToPath(CGMutablePathRef path, LynxBasicShape* _Nonnull shape,
                               const CGSize* viewport) {
#define INDEX_CIRCLE_RADIUS 0
#define INDEX_CIRCLE_POSITION_X 1
#define INDEX_CIRCLE_POSITION_Y 2
  double radius = LBSBorderUnitToFloat(
      shape->_params[INDEX_CIRCLE_RADIUS],
      sqrt(viewport->height * viewport->height + viewport->width * viewport->width) / M_SQRT2);
  double position_x =
      LBSBorderUnitToFloat(shape->_params[INDEX_CIRCLE_POSITION_X], viewport->width);
  double position_y =
      LBSBorderUnitToFloat(shape->_params[INDEX_CIRCLE_POSITION_Y], viewport->height);
  CGPathAddArc(path, nil, position_x, position_y, radius, 0, M_PI * 2, YES);
#undef INDEX_CIRCLE_RADIUS
#undef INDEX_CIRCLE_POSITION_X
#undef INDEX_CIRCLE_POSITION_Y
}

CGPathRef LBSCreatePathFromBasicShape(LynxBasicShape* shape, CGSize viewport) {
  CGMutablePathRef path = NULL;
  if (shape->_type == LynxBasicShapeTypeInset) {
    path = CGPathCreateMutable();
    LBSAddInsetPathToPath(path, shape, &viewport);
  } else if (shape->_type == LynxBasicShapeTypeCircle) {
    path = CGPathCreateMutable();
    LBSAddCircleToPath(path, shape, &viewport);
  }
  return path;
}

LynxBasicShape* LBSCreateBasicShapeFromPathData(NSString* data) {
  LynxBasicShape* path = [[LynxBasicShape alloc] init];
  path->_type = LynxBasicShapeTypePath;
  const char* cData = [data UTF8String];
  CGPathRef cPath = LBSCreatePathFromData(cData);
  path->_path = [UIBezierPath bezierPathWithCGPath:cPath];
  CGPathRelease(cPath);
  return path;
}
