// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxCSSType.h"
#import "LynxConverter+Transform.h"
#import "LynxLog.h"
#import "LynxUIUnitUtils.h"
#import "LynxUnitUtils.h"

typedef NS_ENUM(NSUInteger, TranslateAxis) {
  TranslateAxisX = 0,
  TranslateAxisY = 1,
  TranslateAxisZ = 2
};

@implementation LynxConverter (Transform)
/*
 *      transform: rotate(45deg) translateX(200);
 *      parse for transform.
 *      maybe multi values, then each CATransform3D should be concatenated.
 */

CGFloat GetRawLengthValue(LynxTransformRaw* value, int index, TranslateAxis axis, LynxUI* ui,
                          CGRect applyFrame) {
  CGFloat parentValueOnAxis = 0.0;
  if (axis != TranslateAxisZ) {
    parentValueOnAxis = axis == TranslateAxisX ? !applyFrame.size.width ? ui.updatedFrame.size.width
                                                                        : applyFrame.size.width
                        : !applyFrame.size.height ? ui.updatedFrame.size.height
                                                  : applyFrame.size.height;
  }
  CGFloat result_value = 0.0;
  if (index == 0) {
    result_value = [value.platformLengthP0 valueWithParentValue:parentValueOnAxis];
    if (value.platformLengthP0.type == LynxPlatformLengthUnitPercentage) {
      result_value = [LynxUIUnitUtils roundPtToPhysicalPixel:result_value];
    }
  } else if (index == 1) {
    result_value = [value.platformLengthP1 valueWithParentValue:parentValueOnAxis];
    if (value.platformLengthP1.type == LynxPlatformLengthUnitPercentage) {
      result_value = [LynxUIUnitUtils roundPtToPhysicalPixel:result_value];
    }
  } else if (index == 2) {
    result_value = [value.platformLengthP2 valueWithParentValue:parentValueOnAxis];
    if (value.platformLengthP2.type == LynxPlatformLengthUnitPercentage) {
      result_value = [LynxUIUnitUtils roundPtToPhysicalPixel:result_value];
    }
  }
  return result_value;
}

static CATransform3D convertSingleTransform(LynxTransformRaw* value, char* rotationType,
                                            CGFloat* rotationX, CGFloat* rotationY,
                                            CGFloat* rotationZ, LynxUI* ui, CGRect applyFrame) {
  CATransform3D transform = CATransform3DIdentity;
  LynxTransformType type = (LynxTransformType)value.type;
  CGFloat scaleX;
  CGFloat scaleY;
  CGFloat scale;
  CGAffineTransform shear;
  switch (type) {
    case LynxTransformTypeRotateX:
      *rotationX = value.p0 * M_PI / 180;
      *rotationType |= LynxTransformRotationX;
      transform = CATransform3DRotate(transform, *rotationX, 1, 0, 0);
      break;
    case LynxTransformTypeRotateY:
      *rotationY = value.p0 * M_PI / 180;
      *rotationType |= LynxTransformRotationY;
      transform = CATransform3DRotate(transform, *rotationY, 0, 1, 0);
      break;
    case LynxTransformTypeRotate:
    case LynxTransformTypeRotateZ:
      *rotationZ = value.p0 * M_PI / 180;
      *rotationType |= LynxTransformRotationZ;
      transform = CATransform3DRotate(transform, *rotationZ, 0, 0, 1);
      break;
    case LynxTransformTypeTranslate:
      transform = CATransform3DTranslate(
          transform, GetRawLengthValue(value, 0, TranslateAxisX, ui, applyFrame),
          GetRawLengthValue(value, 1, TranslateAxisY, ui, applyFrame),
          GetRawLengthValue(value, 2, TranslateAxisZ, ui, applyFrame));
      break;
    case LynxTransformTypeTranslate3d:
      transform = CATransform3DTranslate(
          transform, GetRawLengthValue(value, 0, TranslateAxisX, ui, applyFrame),
          GetRawLengthValue(value, 1, TranslateAxisY, ui, applyFrame),
          GetRawLengthValue(value, 2, TranslateAxisZ, ui, applyFrame));
      break;
    case LynxTransformTypeTranslateX:
      transform = CATransform3DTranslate(
          transform, GetRawLengthValue(value, 0, TranslateAxisX, ui, applyFrame), 0, 0);
      break;
    case LynxTransformTypeTranslateY:
      transform = CATransform3DTranslate(
          transform, 0, GetRawLengthValue(value, 0, TranslateAxisY, ui, applyFrame), 0);
      break;
    case LynxTransformTypeTranslateZ:
      transform = CATransform3DTranslate(transform, 0, 0, value.p0);
      transform = CATransform3DTranslate(
          transform, 0, 0, GetRawLengthValue(value, 0, TranslateAxisZ, ui, applyFrame));
      break;
    case LynxTransformTypeScale:
      scaleX = value.p0;
      scaleY = value.p1;
      scaleX = ABS(scaleX) < FLT_EPSILON ? FLT_EPSILON : scaleX;
      scaleY = ABS(scaleY) < FLT_EPSILON ? FLT_EPSILON : scaleY;
      transform = CATransform3DScale(transform, scaleX, scaleY, 1);
      break;
    case LynxTransformTypeScaleX:
      scale = value.p0;
      scale = ABS(scale) < FLT_EPSILON ? FLT_EPSILON : scale;
      transform = CATransform3DScale(transform, scale, 1, 1);
      break;
    case LynxTransformTypeScaleY:
      scale = value.p0;
      scale = ABS(scale) < FLT_EPSILON ? FLT_EPSILON : scale;
      transform = CATransform3DScale(transform, 1, scale, 1);
      break;
    case LynxTransformTypeSkew:
      shear =
          CGAffineTransformMake(1, tan(value.p1 * M_PI / 180), tan(value.p0 * M_PI / 180), 1, 0, 0);
      transform = CATransform3DMakeAffineTransform(shear);
      break;
    case LynxTransformTypeSkewX:
      shear = CGAffineTransformMake(1, 0, tan(value.p0 * M_PI / 180), 1, 0, 0);
      transform = CATransform3DMakeAffineTransform(shear);
      break;
    case LynxTransformTypeSkewY:
      shear = CGAffineTransformMake(1, tan(value.p0 * M_PI / 180), 0, 1, 0, 0);
      transform = CATransform3DMakeAffineTransform(shear);
      break;
    case LynxTransformTypeMatrix:
    case LynxTransformTypeMatrix3d:
      *rotationX = 0;
      *rotationY = 0;
      *rotationZ = 0;
      transform = value.transformMatrix;
    default:
      break;
  }
  return transform;
}

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)rotationX
                       rotationY:(CGFloat*)rotationY
                       rotationZ:(CGFloat*)rotationZ
                              ui:(LynxUI*)ui {
  return [self toCATransform3D:value
                            ui:ui
                      newFrame:CGRectZero
                  rotationType:rotationType
                     rotationX:rotationX
                     rotationY:rotationY
                     rotationZ:rotationZ];
}

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                              ui:(LynxUI*)ui
                        newFrame:(CGRect)frame
          transformWithoutRotate:(CATransform3D*)transformWithoutRotate
        transformWithoutRotateXY:(CATransform3D*)transformWithoutRotateXY
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)rotationX
                       rotationY:(CGFloat*)rotationY
                       rotationZ:(CGFloat*)rotationZ {
  *rotationType = LynxTransformRotationNone;

  if (!value || [value isEqual:[NSNull null]]) {
    *transformWithoutRotate = CATransform3DIdentity;
    *transformWithoutRotateXY = CATransform3DIdentity;
    return CATransform3DIdentity;
  }

  __block CATransform3D transform = CATransform3DIdentity;
  *transformWithoutRotate = CATransform3DIdentity;
  *transformWithoutRotateXY = CATransform3DIdentity;
  if (ui.perspective != nil && ui.perspective.count > 1 &&
      (LynxPerspectiveLengthUnit)[ui.perspective[1] intValue] != LynxPerspectiveLengthUnitDefault) {
    transform.m34 = -1.0 / [self getPerspectiveLength:ui perspective:ui.perspective];
    (*transformWithoutRotate).m34 = transform.m34;
    (*transformWithoutRotateXY).m34 = transform.m34;
  }

  if (frame.size.width == 0 || frame.size.height == 0) {
    frame = ui.updatedFrame;
  }
  CGFloat originXDiffValue = 0;
  CGFloat originYDiffValue = 0;
  LynxTransformOriginRaw* transformOriginRaw = ui.transformOriginRaw;
  if (!ui.enableNewTransformOrigin) {
    if (![self isDefaultTransformOrigin:transformOriginRaw]) {
      NSMutableArray* transformOriginPair = [self toTransformOrigin:transformOriginRaw frame:frame];
      originXDiffValue =
          [[transformOriginPair objectAtIndex:0] floatValue] - (frame.size.width / 2);
      originYDiffValue =
          [[transformOriginPair objectAtIndex:1] floatValue] - (frame.size.height / 2);
    }
  }
  if (value && value.count > 0) {
    [value enumerateObjectsUsingBlock:^(LynxTransformRaw* _Nonnull obj, NSUInteger idx,
                                        BOOL* _Nonnull stop) {
      if (obj) {
        CATransform3D preTranslate = CATransform3DIdentity;
        if (!ui.enableNewTransformOrigin) {
          preTranslate =
              CATransform3DTranslate(preTranslate, originXDiffValue, originYDiffValue, 0);
        }
        CATransform3D singleTransform;
        if ([obj isMatrix]) {
          singleTransform = obj.transformMatrix;
        } else {
          singleTransform =
              convertSingleTransform(obj, rotationType, rotationX, rotationY, rotationZ, ui, frame);
        }
        if (!ui.enableNewTransformOrigin) {
          singleTransform = CATransform3DConcat(singleTransform, preTranslate);
          CATransform3D postTranslate = CATransform3DIdentity;
          postTranslate = CATransform3DTranslate(postTranslate, originXDiffValue * -1,
                                                 originYDiffValue * -1, 0);
          singleTransform = CATransform3DConcat(postTranslate, singleTransform);
        }

        transform = CATransform3DConcat(singleTransform, transform);
        if (![obj isRotate]) {
          *transformWithoutRotate = CATransform3DConcat(singleTransform, *transformWithoutRotate);
        }
        if (![obj isRotateXY]) {
          *transformWithoutRotateXY =
              CATransform3DConcat(singleTransform, *transformWithoutRotateXY);
        }
      }
    }];
  }
  return transform;
}

+ (CATransform3D)toCATransform3D:(NSArray<LynxTransformRaw*>*)value
                              ui:(LynxUI*)ui
                        newFrame:(CGRect)frame
                    rotationType:(char*)rotationType
                       rotationX:(CGFloat*)rotationX
                       rotationY:(CGFloat*)rotationY
                       rotationZ:(CGFloat*)rotationZ {
  CATransform3D transformWithoutRotate = CATransform3DIdentity;
  CATransform3D transformWithoutRotateXY = CATransform3DIdentity;
  return [self toCATransform3D:value
                            ui:ui
                      newFrame:frame
        transformWithoutRotate:&transformWithoutRotate
      transformWithoutRotateXY:&transformWithoutRotateXY
                  rotationType:rotationType
                     rotationX:rotationX
                     rotationY:rotationY
                     rotationZ:rotationZ];
}

+ (CATransform3D)toCATransform3D:(id)value ui:(LynxUI*)ui {
  __block char rotationType;
  __block CGFloat currentRotationX;
  __block CGFloat currentRotationY;
  __block CGFloat currentRotationZ;
  return [self toCATransform3D:value
                  rotationType:&rotationType
                     rotationX:&currentRotationX
                     rotationY:&currentRotationY
                     rotationZ:&currentRotationZ
                            ui:ui];
}

+ (NSMutableArray*)toTransformOrigin:(LynxTransformOriginRaw*)transformOrigin frame:(CGRect)frame {
  NSNumber* initOriginXValue = [NSNumber numberWithFloat:frame.size.width / 2];
  NSNumber* initOriginYValue = [NSNumber numberWithFloat:frame.size.height / 2];
  NSMutableArray* transformOriginPair =
      [[NSMutableArray alloc] initWithObjects:initOriginXValue, initOriginYValue, nil];
  if (!transformOrigin || ![transformOrigin isValid]) {
    return transformOriginPair;
  }
  if ([transformOrigin isP0Valid]) {
    CGFloat anchor = frame.size.width;
    if ([transformOrigin isP0Percent]) {
      anchor *= transformOrigin.p0;
    } else {
      anchor = transformOrigin.p0;
    }
    NSNumber* originXValue = [NSNumber numberWithFloat:anchor];
    [transformOriginPair replaceObjectAtIndex:0 withObject:originXValue];
  }
  if ([transformOrigin isP1Valid]) {
    CGFloat anchor = frame.size.height;
    if ([transformOrigin isP1Percent]) {
      anchor *= transformOrigin.p1;
    } else {
      anchor = transformOrigin.p1;
    }
    NSNumber* originYValue = [NSNumber numberWithFloat:anchor];
    [transformOriginPair replaceObjectAtIndex:1 withObject:originYValue];
  }
  return transformOriginPair;
}

+ (BOOL)isDefaultTransformOrigin:(LynxTransformOriginRaw*)transformOrigin {
  return transformOrigin == nil || [transformOrigin isDefault];
}

+ (CGFloat)getPerspectiveLength:(LynxUI*)ui perspective:(NSArray*)perspective {
  if ((LynxPerspectiveLengthUnit)[perspective[1] intValue] == LynxPerspectiveLengthUnitVw) {
    return [perspective[0] floatValue] / 100.0f * ui.context.rootView.frame.size.width;
  } else if ((LynxPerspectiveLengthUnit)[perspective[1] intValue] == LynxPerspectiveLengthUnitVh) {
    return [perspective[0] floatValue] / 100.0f * ui.context.rootView.frame.size.height;
  }
  return [perspective[0] floatValue];
}

@end
