// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundManager.h"
#import "LynxBackgroundImageLayerInfo.h"
#import "LynxBackgroundInfo.h"
#import "LynxBackgroundRenderer.h"
#import "LynxBackgroundUtils.h"
#import "LynxBasicShape.h"
#import "LynxBoxShadowLayer.h"
#import "LynxBoxShadowManager.h"
#import "LynxColorUtils.h"
#import "LynxConvertUtils.h"
#import "LynxImageLoader.h"
#import "LynxImageProcessor.h"
#import "LynxService.h"
#import "LynxServiceImageProtocol.h"
#import "LynxSubErrorCode.h"
#import "LynxUI+Internal.h"
#import "LynxUIContext+Internal.h"
#import "LynxUnitUtils.h"

NSString* NSStringFromLynxBorderRadii(LynxBorderRadii* radii) {
  return [NSString
      stringWithFormat:@"LynxBorderRadii_%f_%ld_%f_%ld_%f_%ld_%f_%ld_%f_%ld_%f_%ld_%f_%ld_%f_%ld",
                       radii->topLeftX.val, (long)radii->topLeftX.unit, radii->topLeftY.val,
                       (long)radii->topLeftY.unit, radii->topRightX.val,
                       (long)radii->topRightX.unit, radii->topRightY.val,
                       (long)radii->topRightY.unit, radii->bottomLeftX.val,
                       (long)radii->bottomLeftX.unit, radii->bottomLeftY.val,
                       (long)radii->bottomLeftY.unit, radii->bottomRightX.val,
                       (long)radii->bottomRightX.unit, radii->bottomRightY.val,
                       (long)radii->bottomRightY.unit

  ];
}

const LynxBorderRadii LynxBorderRadiiZero = {{0, 0}, {0, 0}, {0, 0}, {0, 0},
                                             {0, 0}, {0, 0}, {0, 0}, {0, 0}};

#pragma mark LynxBackgroundSubLayer
@implementation LynxBackgroundSubLayer
@end

#pragma mark LynxBorderLayer
@implementation LynxBorderLayer
@end

#pragma mark LynxBackgroundManager
@implementation LynxBackgroundManager {
  // TODO(linxs):_isBGChangedImage&_isBGChangedNoneImage to be removed later, use LynxBackgroundInfo
  // instead!
  BOOL _isBGChangedImage, _isBGChangedNoneImage, _isMaskChanged;
  BOOL _withAnimation;
  // This backgroundSize means the area in view the background can display, most time is view size,
  // not css property background-size.
  CGSize _backgroundSize;
  BOOL _onlyGradient;
}

- (instancetype)initWithUI:(LynxUI*)ui {
  self = [super init];
  if (self) {
    _ui = ui;
    _backgroundInfo = [[LynxBackgroundInfo alloc] init];
    _isBGChangedImage = _isBGChangedNoneImage = NO;
    _opacity = 1;
    // TODO(fangzhou): move these properties to info
    _transform = CATransform3DIdentity;
    _transformOrigin = CGPointMake(0.5, 0.5);
    _implicitAnimation = true;
    _postTranslate = CGPointZero;
    _overlapRendering = NO;
    _uiBackgroundShapeLayerEnabled = LynxBgShapeLayerPropUndefine;
    _shouldRasterizeShadow = NO;
    _borderRadiusRaw = _borderRadius = LynxBorderRadiiZero;
    _onlyGradient = YES;
    _isPixelated = NO;
  }
  return self;
}

- (void)applyTransformOrigin:(CALayer*)layer {
  CGFloat oldAnchorX = layer.anchorPoint.x;
  CGFloat oldAnchorY = layer.anchorPoint.y;
  CGFloat anchorX = _transformOrigin.x;
  CGFloat anchorY = _transformOrigin.y;
  layer.anchorPoint = _transformOrigin;
  CGFloat newPositionX = layer.position.x + (anchorX - oldAnchorX) * layer.frame.size.width;
  CGFloat newPositionY = layer.position.y + (anchorY - oldAnchorY) * layer.frame.size.height;
  layer.position = CGPointMake(newPositionX, newPositionY);
}

- (BOOL)clearAllBackgroundDrawable {
  if ([_backgroundDrawable count] <= 0) {
    return NO;
  }
  [_backgroundDrawable removeAllObjects];
  _isBGChangedImage = YES;
  return YES;
}

- (void)markBackgroundDirty {
  _isBGChangedImage = YES;
}

- (void)markMaskDirty {
  _isMaskChanged = YES;
}

- (void)clearSimpleBorder {
  CALayer* layer = _ui.view.layer;
  [layer setBorderWidth:0.0];
  [layer setCornerRadius:0.0];
  [_borderLayer setBorderWidth:0.0];
  [_borderLayer setCornerRadius:0.0];
}

- (void)autoAddOpacityViewWithOpacity:(CGFloat)opacity {
  if (self.ui.view.superview == nil) {
    return;
  }
  CALayer* mainLayer = _ui.view.layer;
  CALayer* superLayer = mainLayer.superlayer;
  UIView* superView = _ui.view.superview;

  if (!_opacityView) {
    _opacityView = [[UIView alloc] init];
    CALayer* opacityLayer = _opacityView.layer;
    [superView insertSubview:_opacityView belowSubview:_ui.view];
    [superLayer insertSublayer:opacityLayer below:mainLayer];
    [mainLayer removeFromSuperlayer];
    [_opacityView addSubview:_ui.view];
    [opacityLayer addSublayer:mainLayer];
    // background layer below main layer.
    if (_backgroundLayer) {
      [_backgroundLayer removeFromSuperlayer];
      [opacityLayer insertSublayer:_backgroundLayer below:mainLayer];
    }

    // border layer should above background layer and below main layer.
    if (_borderLayer) {
      [_borderLayer removeFromSuperlayer];
      if (_ui.overflow == OVERFLOW_HIDDEN_VAL) {
        // border layer above main layer to cover border area when 'overflow:hidden
        [opacityLayer insertSublayer:_borderLayer above:mainLayer];
      } else {
        [opacityLayer insertSublayer:_borderLayer below:mainLayer];
      }
    }
    opacityLayer.frame =
        CGRectMake(0, 0, superLayer.frame.size.width, superLayer.frame.size.height);
  }
  _opacityView.layer.opaque = NO;
  _ui.view.layer.opacity = 1.0;
  _opacityView.layer.opacity = opacity;
  _opacityView.layer.masksToBounds = NO;
  _opacityView.layer.mask = _maskLayer;
}

- (void)setOpacity:(CGFloat)opacity {
  _opacity = opacity;

  if (!_overlapRendering) {
    if (_backgroundLayer) {
      _backgroundLayer.opacity = opacity;
    }
    if (_borderLayer) {
      _borderLayer.opacity = opacity;
    }
  } else {
    if ((_backgroundLayer || _borderLayer)) {
      if (!_opacityView) {
        [self autoAddOpacityViewWithOpacity:_opacity];
      }
      _ui.view.layer.opacity = 1;
      _opacityView.layer.opacity = opacity;
    }
  }
}

- (void)setIsPixelated:(BOOL)isPixelated {
  _isPixelated = isPixelated;
}

- (void)setHidden:(BOOL)hidden {
  _hidden = hidden;
  if (_backgroundLayer) {
    _backgroundLayer.hidden = hidden;
  }
  if (_borderLayer) {
    _borderLayer.hidden = hidden;
  }
}

- (void)setPostTranslate:(CGPoint)postTranslate {
  _postTranslate = postTranslate;

  CATransform3D transform = [self getTransformWithPostTranslate];
  _ui.view.layer.transform = transform;
  [self setTransformToLayers:transform];
}

- (CATransform3D)getTransformWithPostTranslate {
  CATransform3D result = _transform;
  result.m41 = result.m41 + _postTranslate.x;
  result.m42 = result.m42 + _postTranslate.y;
  return result;
}

- (void)setTransform:(CATransform3D)transform {
  _transform = transform;
  [self setTransformToLayers:transform];
}

- (void)setTransformToLayers:(CATransform3D)transform {
  if (_backgroundLayer != nil) {
    _backgroundLayer.transform = transform;
  }
  if (_borderLayer != nil) {
    _borderLayer.transform = transform;
  }
}

- (void)setTransformOrigin:(CGPoint)transformOrigin {
  _transformOrigin = transformOrigin;
  [self setTransformOriginToLayers:transformOrigin];
}

- (void)setTransformOriginToLayers:(CGPoint)transformOrigin {
  if (_backgroundLayer != nil) {
    [self applyTransformOrigin:_backgroundLayer];
  }
  if (_borderLayer != nil) {
    [self applyTransformOrigin:_borderLayer];
  }
}

- (void)tryToLoadImagesAutoRefresh:(BOOL)autoRefresh
                          drawable:(NSMutableArray*)curArray
                             layer:(LynxBackgroundSubBackgroundLayer*)layer
                            isMask:(BOOL)isMask {
  BOOL onlyHasGradient = NO;
  [self tryToLoadImagesAutoRefresh:autoRefresh
                          drawable:curArray
                             layer:layer
                            isMask:isMask
                      onlyGradient:&onlyHasGradient];
}

- (void)tryToLoadImagesAutoRefresh:(BOOL)autoRefresh
                          drawable:(NSMutableArray*)curArray
                             layer:(LynxBackgroundSubBackgroundLayer*)layer
                            isMask:(BOOL)isMask
                      onlyGradient:(BOOL*)onlyGradient {
  *onlyGradient = YES;
  for (int i = 0; i < (int)curArray.count; ++i) {
    id item = curArray[i];
    if (![item isKindOfClass:[LynxBackgroundImageDrawable class]]) {
      continue;
    }
    *onlyGradient = NO;
    NSURL* url = ((LynxBackgroundImageDrawable*)item).url;

    if (((LynxBackgroundImageDrawable*)item).image) {
      continue;
    }

    NSMutableArray* processors = [NSMutableArray new];
    __weak typeof(self) weakSelf = self;
    __weak LynxBackgroundSubBackgroundLayer* weakLayer = layer;
    __weak NSMutableArray* weakArray = curArray;
    __weak LynxBackgroundImageDrawable* drawable = item;
    const int currentIndex = i;
    // FIXME(linxs:) change to imageService once imageService is ready!
    LynxURL* requestUrl = [[LynxURL alloc] init];
    requestUrl.url = url;
    [[LynxImageLoader sharedInstance]
        loadImageFromLynxURL:requestUrl
                        size:self.ui.view.bounds.size
                 contextInfo:@{LynxImageFetcherContextKeyUI : self}
                  processors:processors
                imageFetcher:self.ui.context.imageFetcher
                 LynxUIImage:nil
        enableGenericFetcher:NO
                   completed:^(UIImage* _Nullable image, NSError* _Nullable error,
                               NSURL* _Nullable imageURL) {
                     __strong __typeof(weakSelf) strongSelf = weakSelf;
                     void (^complete)(UIImage*, NSError* _Nullable) = ^(UIImage* image,
                                                                        NSError* _Nullable error) {
                       if (!error) {
                         if (image != nil) {
                           if (weakArray != nil &&
                               weakArray == (isMask ? strongSelf.maskDrawable
                                                    : strongSelf.backgroundDrawable)) {
                             [drawable setImage:image];
                           }
                           if (autoRefresh && weakLayer != nil &&
                               weakLayer ==
                                   (isMask ? strongSelf.maskLayer : strongSelf.backgroundLayer)) {
                             if ((NSUInteger)currentIndex < weakLayer.imageArray.count) {
                               [drawable setImage:image];
                             }
                             [weakLayer setAnimatedPropsWithImage:image];
                             if (!isMask) {
                               [strongSelf applyComplexBackground];
                             } else {
                               [strongSelf applyComplexMask];
                             }
                           }

                           if ([strongSelf.ui.eventSet valueForKey:@"bgload"]) {
                             NSDictionary* detail = @{
                               @"height" : [NSNumber numberWithFloat:image.size.height],
                               @"width" : [NSNumber numberWithFloat:image.size.width]
                             };
                             [strongSelf.ui.context.eventEmitter
                                 dispatchCustomEvent:[[LynxDetailEvent alloc]
                                                         initWithName:@"bgload"
                                                           targetSign:strongSelf.ui.sign
                                                               detail:detail]];
                           }
                         }
                       } else {
                         // get categorized code
                         NSNumber* errorCode = [error.userInfo valueForKey:@"error_num"]
                                                   ?: [NSNumber numberWithInteger:error.code];
                         NSNumber* categorizedErrorCode = [LynxService(LynxServiceImageProtocol)
                             getMappedCategorizedPicErrorCode:errorCode];

                         // pass LynxError to LynxLifecycleDispatcher
                         NSString* errMsg = @"Load backgroundImage failed";
                         NSString* rootCause = [NSString stringWithFormat:@"%@", error];
                         NSInteger subErrCode = categorizedErrorCode
                                                    ? [categorizedErrorCode integerValue]
                                                    : ECLynxResourceImageException;
                         LynxError* err = [LynxError lynxErrorWithCode:subErrCode
                                                               message:errMsg
                                                         fixSuggestion:@""
                                                                 level:LynxErrorLevelError];
                         [err setRootCause:rootCause];
                         [strongSelf.ui.context didReceiveResourceError:err
                                                             withSource:url.absoluteString ?: @""
                                                                   type:@"image"];

                         // FE bind error
                         NSString* errorDetail =
                             [NSString stringWithFormat:@"url:%@,%@", url, [error description]];
                         NSDictionary* feErrorDic = @{
                           @"errMsg" : errorDetail ?: @"",
                           @"error_code" : errorCode ?: @-1,
                           @"lynx_categorized_code" : categorizedErrorCode ?: @-1,
                         };
                         if ([strongSelf.ui.eventSet valueForKey:@"bgerror"]) {
                           [strongSelf.ui.context.eventEmitter
                               dispatchCustomEvent:[[LynxDetailEvent alloc]
                                                       initWithName:@"bgerror"
                                                         targetSign:strongSelf.ui.sign
                                                             detail:feErrorDic]];
                         }
                       }
                     };
                     if ([NSThread isMainThread]) {
                       complete(image, error);
                     } else {
                       dispatch_async(dispatch_get_main_queue(), ^{
                         complete(image, error);
                       });
                     }
                   }];
  }
}

- (void)tryToLoadImagesAutoRefresh:(BOOL)autoRefresh
                          drawable:(NSMutableArray*)curArray
                             layer:(LynxBackgroundSubBackgroundLayer*)layer
                      onlyGradient:(BOOL*)onlyGradient {
  [self tryToLoadImagesAutoRefresh:autoRefresh
                          drawable:curArray
                             layer:layer
                            isMask:NO
                      onlyGradient:onlyGradient];
}

- (void)tryToLoadBackgroundImagesAutoRefresh:(BOOL)autoRefresh onlyGradient:(BOOL*)onlyGradient {
  if (![self hasBackgroundImageOrGradient]) {
    return;
  }

  [self tryToLoadImagesAutoRefresh:autoRefresh
                          drawable:self.backgroundDrawable
                             layer:_backgroundLayer
                      onlyGradient:onlyGradient];
}

- (void)tryToLoadMaskImagesAutoRefresh:(BOOL)autoRefresh {
  [self tryToLoadImagesAutoRefresh:autoRefresh
                          drawable:self.maskDrawable
                             layer:_maskLayer
                            isMask:YES];
}

- (BOOL)hasBackgroundImageOrGradient {
  return [self.backgroundDrawable count] != 0;
}

- (NSMutableArray*)generateImageLayerWithBound:(CGSize)bound
                                      position:(NSMutableArray*)pos
                                          size:(NSMutableArray*)size
                                        origin:(NSMutableArray*)origin
                                          clip:(NSMutableArray*)clip
                                        repeat:(NSMutableArray*)repeat
                                          info:(NSMutableArray*)drawingInfo
                                          mask:(BOOL)isMask {
  if ([drawingInfo count] == 0) {
    _backgroundLayer.isAnimated = NO;
    _backgroundLayer.animatedImageDuration = 0;
    return nil;
  }

  NSMutableArray* layerInfoArray = [[NSMutableArray alloc] init];
  CGRect borderRect = {.size = bound};
  CGRect paddingRect = [_backgroundInfo getPaddingRect:bound];
  CGRect contentRect = [_backgroundInfo getContentRect:paddingRect];
  UIEdgeInsets paddingInsets = [_backgroundInfo getPaddingInsets];

  CGRect clipRect = borderRect;
  LynxCornerInsets cornerInsets =
      LynxGetCornerInsets(borderRect, [_backgroundInfo borderRadius], UIEdgeInsetsZero);
  LynxBackgroundClipType clipType = LynxBackgroundClipBorderBox;

  NSMutableArray* curArray = drawingInfo;
  for (int i = 0; i < (int)curArray.count; ++i) {
    LynxBackgroundImageLayerInfo* layerInfo = [[LynxBackgroundImageLayerInfo alloc] init];
    [layerInfo setItem:curArray[i]];
    [layerInfoArray addObject:layerInfo];
    if ([layerInfo.item isKindOfClass:[LynxBackgroundImageDrawable class]]) {
      [_backgroundLayer
          setAnimatedPropsWithImage:((LynxBackgroundImageDrawable*)layerInfo.item).image];
    }

    // decide origin position
    CGRect paintingBox = paddingRect;
    LynxBackgroundOriginType usedOrigin =
        isMask ? LynxBackgroundOriginBorderBox : LynxBackgroundOriginPaddingBox;
    if ([origin count] != 0) {
      int usedOriginIndex = i % [origin count];
      usedOrigin = [origin[usedOriginIndex] integerValue];
      switch (usedOrigin) {
        case LynxBackgroundOriginPaddingBox:
          paintingBox = paddingRect;
          break;
        case LynxBackgroundOriginBorderBox:
          paintingBox = borderRect;
          break;
        case LynxBackgroundOriginContentBox:
          paintingBox = contentRect;
          break;
        default:
          break;
      }
    }

    if ([clip count] != 0) {
      int usedClipIndex = i % [clip count];
      clipType = [clip[usedClipIndex] integerValue];
      switch (clipType) {
        case LynxBackgroundClipPaddingBox:
          clipRect = paddingRect;
          cornerInsets = LynxGetCornerInsets(borderRect, [_backgroundInfo borderRadius],
                                             [self getAdjustedBorderWidth]);
          break;
        case LynxBackgroundClipContentBox:
          clipRect = contentRect;
          cornerInsets =
              LynxGetCornerInsets(borderRect, [_backgroundInfo borderRadius], paddingInsets);
          break;
        default:
          clipRect = borderRect;
          cornerInsets =
              LynxGetCornerInsets(borderRect, [_backgroundInfo borderRadius], UIEdgeInsetsZero);
          break;
      }
    }

    layerInfo.backgroundOrigin = usedOrigin;
    layerInfo.paintingRect = paintingBox;
    layerInfo.clipRect = clipRect;
    layerInfo.contentRect = contentRect;
    layerInfo.borderRect = borderRect;
    layerInfo.paddingRect = paddingRect;
    layerInfo.backgroundClip = clipType;
    layerInfo.cornerInsets = cornerInsets;
    if (isMask) {
      LynxPlatformLength* len =
          [[LynxPlatformLength alloc] initWithValue:@"0.5" type:LynxPlatformLengthUnitPercentage];
      layerInfo.backgroundPosX = [[LynxBackgroundPosition alloc] initWithValue:len];
      layerInfo.backgroundPosY = [[LynxBackgroundPosition alloc] initWithValue:len];
    }

    // set background image size
    if ([size count] >= 2) {
      if ((NSUInteger)i * 2 >= [size count]) {
        layerInfo.backgroundSizeX = size[size.count - 2];
        layerInfo.backgroundSizeY = size[size.count - 1];
      } else {
        layerInfo.backgroundSizeX = size[i * 2];
        layerInfo.backgroundSizeY = size[i * 2 + 1];
      }
    }

    // set background repeatType
    LynxBackgroundRepeatType repeatXType = LynxBackgroundRepeatRepeat;
    LynxBackgroundRepeatType repeatYType = LynxBackgroundRepeatRepeat;
    if ([repeat count] >= 2) {
      const int usedRepeatIndex = i % ([repeat count] / 2);
      repeatXType = [((NSNumber*)repeat[usedRepeatIndex * 2]) integerValue];
      repeatYType = [((NSNumber*)repeat[usedRepeatIndex * 2 + 1]) integerValue];
    }
    layerInfo.repeatXType = repeatXType;
    layerInfo.repeatYType = repeatYType;

    // set background position
    if ([pos count] >= 2) {
      int usedPosIndex = i % ([pos count] / 2);
      layerInfo.backgroundPosX = pos[usedPosIndex * 2];
      layerInfo.backgroundPosY = pos[usedPosIndex * 2 + 1];
    }
  }
  return layerInfoArray;
}

- (NSMutableArray*)generateBackgroundImageLayerWithSize:(CGSize)size
                                                   info:(NSMutableArray*)drawingInfo {
  return [self generateImageLayerWithBound:size
                                  position:self.backgroundPosition
                                      size:self.backgroundImageSize
                                    origin:self.backgroundOrigin
                                      clip:self.backgroundClip
                                    repeat:self.backgroundRepeat
                                      info:drawingInfo
                                      mask:NO];
}

- (NSMutableArray*)generateMaskImageLayerWithSize:(CGSize)size info:(NSMutableArray*)drawingInfo {
  return [self generateImageLayerWithBound:size
                                  position:self.maskPosition
                                      size:self.maskSize
                                    origin:self.maskOrigin
                                      clip:self.maskClip
                                    repeat:self.maskRepeat
                                      info:drawingInfo
                                      mask:YES];
}

- (void)removeAllAnimations {
  if (_backgroundLayer != nil) {
    [_backgroundLayer removeAllAnimations];
  }
  if (_borderLayer != nil) {
    [_borderLayer removeAllAnimations];
  }
}

- (void)addAnimationToViewAndLayers:(CAAnimation*)anim forKey:(nullable NSString*)key {
  [_ui.view.layer addAnimation:anim forKey:key];

  if (_backgroundLayer != nil) {
    [_backgroundLayer addAnimation:anim forKey:key];
  }
  if (_borderLayer != nil) {
    [_borderLayer addAnimation:anim forKey:key];
  }
}

- (void)setWithAnimation {
  if (_withAnimation) {
    return;
  }

  _withAnimation = YES;

  if ((_backgroundLayer == nil || !(_backgroundLayer.type == LynxBgTypeComplex)) &&
      LynxHasBorderRadii([_backgroundInfo borderRadius])) {
    if (_backgroundLayer == nil) {
      [self autoAddBackgroundLayer:YES];
    } else {
      _backgroundLayer.type = LynxBgTypeComplex;
      _backgroundLayer.cornerRadius = 0;
      _backgroundLayer.backgroundColor = [UIColor clearColor].CGColor;
    }
    [self applyComplexBackground];
  }
}

- (void)addAnimation:(CAAnimation*)anim forKey:(nullable NSString*)key {
  if ([key isEqualToString:@"DUP-transition-opacity"] && _overlapRendering) {
    if (_opacityView) {
      [_opacityView.layer addAnimation:anim forKey:key];
    }
  } else {
    if (_backgroundLayer != nil) {
      [_backgroundLayer addAnimation:anim forKey:key];
    }

    if (_borderLayer != nil) {
      [_borderLayer addAnimation:anim forKey:key];
    }
  }
}

- (int)animationOptions {
  int opts = 0;
  if (_borderLayer != nil) {
    opts |= LynxAnimOptHasBorderLayer;
    if (LynxBgTypeSimple != _borderLayer.type) {
      opts |= LynxAnimOptHasBorderComplex;
    }
  }
  if (_backgroundLayer != nil) {
    opts |= LynxAnimOptHasBGLayer;
    if (_backgroundLayer.type == LynxBgTypeComplex && !_onlyGradient) {
      opts |= LynxAnimOptHasBGComplex;
    }
  }
  return opts;
}

- (int)animationLayerCount {
  int count = 0;
  if (_borderLayer != nil) {
    ++count;
  }
  if (_backgroundLayer != nil) {
    ++count;
  }
  return count;
}

- (void)removeAnimationForKey:(NSString*)key {
  if (_backgroundLayer != nil) {
    [_backgroundLayer removeAnimationForKey:key];
  }
  if (_borderLayer != nil) {
    [_borderLayer removeAnimationForKey:key];
  }
}

- (void)removeBorderLayer {
  if (_borderLayer != nil) {
    [_borderLayer removeAllAnimations];
    [_borderLayer removeFromSuperlayer];
    _borderLayer = nil;
  }
}

/// Apply border related props to the corresponding layer. When the view's 4 borders have same
/// `border-color`, `border-width`, `border-radius` and `border-style`, the border can be presented
/// via `CALayer`'s props. We use `borderLayer` to manage the layer hierarchy, `borderLayer` could
/// be above or below `view.layer`. Always put borders on `borderLayer` if it exists.
- (void)applySimpleBorder {
  CALayer* layer = _ui.view.layer;
  // If borderLayer exists, all border related props (radius, borderWidth, borderColor) should be
  // applied to borderLayer.
  if (_borderLayer) {
    // Clear border on view.layer and apply border on _borderLayer.
    [layer setBorderWidth:.0f];
    layer = _borderLayer;
  }
  if ([_backgroundInfo borderLeftStyle] != LynxBorderStyleNone &&
      [_backgroundInfo borderLeftStyle] != LynxBorderStyleHidden) {
    layer.borderWidth = [self getAdjustedBorderWidth].bottom;
  } else {
    layer.borderWidth = 0.0;
  }
  // Adjust radius, radius should smaller than half of the corresponding edge's length.
  // Simple border means all cornerRadius are the same.
  layer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                   bySize:layer.frame.size];
  if ([_backgroundInfo borderBottomColor]) {
    layer.borderColor = [_backgroundInfo borderBottomColor].CGColor;
  }
}

- (CALayer*)autoAddBorderLayer:(LynxBgTypes)type {
  if (_borderLayer == nil) {
    _borderLayer = [[LynxBorderLayer alloc] init];
    _borderLayer.delegate = self;
    _borderLayer.type = LynxBgTypeSimple;
    [_ui.animationManager notifyBGLayerAdded];
  }
  _borderLayer.masksToBounds = NO;
  _borderLayer.hidden = _hidden;
  _borderLayer.type = type;
  _borderLayer.transform = CATransform3DIdentity;
  _borderLayer.frame = _ui.view.layer.frame;
  _borderLayer.transform = _transform;
  _borderLayer.allowsEdgeAntialiasing = _allowsEdgeAntialiasing;
  if (_ui.enableNewTransformOrigin) {
    [self applyTransformOrigin:_borderLayer];
  }
  CALayer* superLayer = _ui.view.layer.superlayer;
  if (superLayer != nil && _borderLayer.superlayer != superLayer) {
    if (_ui.overflow != 0) {
      [superLayer insertSublayer:_borderLayer below:_ui.view.layer];
      if (_backgroundLayer != nil) {
        [superLayer insertSublayer:_backgroundLayer below:_borderLayer];
      }
    } else {
      [superLayer insertSublayer:_borderLayer above:_ui.view.layer];
    }
  }
  return _borderLayer;
}

- (void)autoAddOutlineLayer {
  if (self.ui.view.superview == nil || self.borderLayer == nil) {
    return;
  }

  if (_outlineLayer != nil) {
    [_outlineLayer removeFromSuperlayer];
  }
  if (_backgroundInfo.outlineStyle != LynxBorderStyleNone && _backgroundInfo.outlineWidth > 0) {
    if (_outlineLayer == nil) {
      _outlineLayer = [[CALayer alloc] init];
      _outlineLayer.allowsEdgeAntialiasing = _allowsEdgeAntialiasing;
    }
    if (LynxUpdateOutlineLayer(_outlineLayer, _backgroundSize, _backgroundInfo.outlineStyle,
                               _backgroundInfo.outlineColor, _backgroundInfo.outlineWidth)) {
      [_borderLayer addSublayer:_outlineLayer];
    } else {
      _outlineLayer = nil;
    }
  } else {
    _outlineLayer = nil;
  }
}

- (void)removeBackgroundLayer {
  if (_backgroundLayer != nil) {
    [_backgroundLayer removeAllAnimations];
    [_backgroundLayer removeFromSuperlayer];
    // set delegate to nil to remove displayLlink
    _backgroundLayer.delegate = nil;
    _backgroundLayer = nil;
  }
}

- (void)removeMaskLayer {
  if (_maskLayer) {
    [_maskLayer removeAllAnimations];
    [_maskLayer removeFromSuperlayer];
    _maskLayer = nil;
  }
}

- (CALayer*)addMaskLayer {
  if (!_maskLayer) {
    _maskLayer = [[LynxBackgroundSubBackgroundLayer alloc] init];
    _maskLayer.delegate = self;
    [_ui.animationManager notifyBGLayerAdded];
  }
  _maskLayer.masksToBounds = NO;
  _maskLayer.opacity = _opacity;
  _maskLayer.hidden = _hidden;
  _maskLayer.type = LynxBgTypeComplex;
  _maskLayer.transform = CATransform3DIdentity;
  _maskLayer.frame = self.ui.view.layer.frame;
  _maskLayer.transform = _transform;
  if (_ui.enableNewTransformOrigin) {
    [self applyTransformOrigin:_maskLayer];
  }
  _maskLayer.allowsEdgeAntialiasing = _allowsEdgeAntialiasing;

  if (!_opacityView) {
    [self autoAddOpacityViewWithOpacity:_opacity];
  }
  return _maskLayer;
}

- (CALayer*)autoAddBackgroundLayer:(BOOL)complex {
  if (!_backgroundLayer) {
    _backgroundLayer = [[LynxBackgroundSubBackgroundLayer alloc] init];
    _backgroundLayer.delegate = self;
    [_ui.animationManager notifyBGLayerAdded];
  }
  _backgroundLayer.masksToBounds = NO;
  _backgroundLayer.hidden = _hidden;
  _backgroundLayer.type = complex ? LynxBgTypeComplex : LynxBgTypeSimple;
  // reset transform before change frame, otherwise the result value of frame is undefined.
  _backgroundLayer.transform = CATransform3DIdentity;
  _backgroundLayer.frame = self.ui.view.layer.frame;
  _backgroundLayer.transform = _transform;
  if (_ui.enableNewTransformOrigin) {
    [self applyTransformOrigin:_backgroundLayer];
  }
  _backgroundLayer.allowsEdgeAntialiasing = _allowsEdgeAntialiasing;
  _backgroundLayer.enableAsyncDisplay = self.ui.enableAsyncDisplay;
  _backgroundLayer.backgroundColorClip =
      [self.backgroundClip count] == 0
          ? LynxBackgroundClipBorderBox
          : (LynxBackgroundClipType)[[self.backgroundClip lastObject] integerValue];
  _backgroundLayer.paddingWidth = _backgroundInfo.paddingWidth;

  _ui.view.backgroundColor = [UIColor clearColor];
  if (!complex) {
    _backgroundLayer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                                bySize:_backgroundLayer.frame.size];
    _backgroundLayer.backgroundColor = _backgroundInfo.backgroundColor.CGColor;
  }

  CALayer* superLayer = _ui.view.layer.superlayer;

  if (superLayer != nil && _backgroundLayer.superlayer != superLayer) {
    if (_ui.overflow != 0 && _borderLayer != nil) {
      [superLayer insertSublayer:_borderLayer below:_ui.view.layer];
      [superLayer insertSublayer:_backgroundLayer below:_borderLayer];
    } else {
      [superLayer insertSublayer:_backgroundLayer below:_ui.view.layer];
    }
  }
  return _backgroundLayer;
}

- (void)applyComplexBackground {
  _backgroundLayer.backgroundColor = [UIColor clearColor].CGColor;
  _backgroundLayer.cornerRadius = 0;
  if (_isBGChangedImage || [_backgroundInfo BGChangedImage]) {
    _backgroundLayer.isAnimated = NO;
    [self tryToLoadBackgroundImagesAutoRefresh:YES onlyGradient:&_onlyGradient];
    _backgroundLayer.imageArray =
        [self generateBackgroundImageLayerWithSize:_backgroundSize info:self.backgroundDrawable];
  }
  [_backgroundLayer markDirtyWithSize:self->_backgroundSize
                                radii:self->_borderRadius
                         borderInsets:self->_borderWidth
                      backgroundColor:self->_backgroundColor
                           drawToEdge:NO
                            capInsets:self->_backgroundCapInsets.capInsets
                       isGradientOnly:_onlyGradient && (_uiBackgroundShapeLayerEnabled !=
                                                        LynxBgShapeLayerPropDisabled)
                          isPixelated:_isPixelated];
}

- (void)applyComplexMask {
  _maskLayer.backgroundColor = [UIColor clearColor].CGColor;
  _maskLayer.cornerRadius = 0;
  if (_isMaskChanged) {
    [self tryToLoadMaskImagesAutoRefresh:YES];

    _maskLayer.imageArray = [self generateMaskImageLayerWithSize:_backgroundSize
                                                            info:self.maskDrawable];
  }
  [_maskLayer markDirtyWithSize:self->_backgroundSize
                          radii:self->_borderRadius
                   borderInsets:self->_borderWidth
                backgroundColor:[UIColor clearColor]
                     drawToEdge:NO
                      capInsets:self->_backgroundCapInsets.capInsets
                 isGradientOnly:false
                    isPixelated:self->_isPixelated];
}

- (void)getBackgroundImageAsync:(NSArray*)imageArrayInfo withCompletion:completionBlock {
  if (!imageArrayInfo) {
    imageArrayInfo = [[NSArray alloc] initWithArray:self->_backgroundLayer.imageArray];
  }

  // deep copy: thread safe
  CGSize backgroundSizeCopy = self->_backgroundSize;
  LynxBorderRadii borderRadiusCopy = [_backgroundInfo borderRadius];
  UIEdgeInsets borderWidthCopy = [self getAdjustedBorderWidth];
  UIColor* backgroundColorCopy = [[_backgroundInfo backgroundColor] copy];
  BOOL isPixelated = self->_isPixelated;

  __weak LynxBackgroundManager* weakSelf = self;
  lynx_async_get_background_image_block_t displayBlock = ^{
    __strong LynxBackgroundManager* strongSelf = weakSelf;
    if (strongSelf) {
      return LynxGetBackgroundImage(backgroundSizeCopy, borderRadiusCopy, borderWidthCopy,
                                    backgroundColorCopy.CGColor, NO, imageArrayInfo, isPixelated);
    }
    return (UIImage*)nil;
  };
  [self.ui displayComplexBackgroundAsynchronouslyWithDisplay:displayBlock
                                                  completion:completionBlock];
}

- (void)dealloc {
  if (@available(iOS 13.0, *)) {
    // Prevent modifying CALayer's delegate on a background thread.
    return;
  }

  // The circular retain needs to be break under iOS 12. Otherwise an UAF crash will occur.
  if (_backgroundLayer && _backgroundLayer.delegate == self) {
    _backgroundLayer.delegate = nil;
  }
  if (_borderLayer && _borderLayer.delegate == self) {
    _borderLayer.delegate = nil;
  }
}

- (BOOL)toAddSubLayerOnBorderLayer {
  if (_backgroundInfo.outlineStyle != LynxBorderStyleNone) {
    // outlines are attached onto border the layer
    return YES;
  }

  return NO;
}

- (BOOL)toAddSubLayerOnBackgroundLayer:(BOOL)hasBorderLayer {
  if ([_shadowArray count] != 0) {
    // shadows are attached onto background layer
    return YES;
  }

  if (_ui.overflow != 0 && hasBorderLayer) {
    return YES;
  }

  return NO;
}

- (BOOL)isSimpleBackground {
  if ([self.backgroundClip count] != 0) {
    // should clip background color
    return NO;
  }

  if ([_backgroundInfo hasDifferentBorderRadius]) {
    // normal layer do not support different radius
    return NO;
  }

  if ([self.backgroundDrawable count] != 0) {
    return NO;
  }

  if ([_shadowArray count] != 0) {
    for (LynxBoxShadow* shadow in _shadowArray) {
      if (shadow.inset) {
        // background of view will hide the inset shader, so we use complex mode
        return NO;
      }
    }
  }

  return YES;
}

- (id<CAAction>)actionForLayer:(CALayer*)layer forKey:(NSString*)event {
  if (!_implicitAnimation)
    return (id)[NSNull null];  // disable all implicit animations
  else
    return nil;  // allow implicit animations
}

- (float)adjustRadius:(float)radius bySize:(CGSize)size {
  if (radius + radius > size.width) {
    radius = size.width * 0.5f;
  }
  if (radius + radius > size.height) {
    radius = size.height * 0.5f;
  }
  return radius;
}

- (UIEdgeInsets)getAdjustedBorderWidth {
  CGFloat top = (_backgroundInfo.borderTopStyle == LynxBorderStyleNone ||
                 _backgroundInfo.borderTopStyle == LynxBorderStyleHidden)
                    ? 0
                    : _backgroundInfo.borderWidth.top;
  CGFloat left = (_backgroundInfo.borderLeftStyle == LynxBorderStyleNone ||
                  _backgroundInfo.borderLeftStyle == LynxBorderStyleHidden)
                     ? 0
                     : _backgroundInfo.borderWidth.left;
  CGFloat bottom = (_backgroundInfo.borderBottomStyle == LynxBorderStyleNone ||
                    _backgroundInfo.borderBottomStyle == LynxBorderStyleHidden)
                       ? 0
                       : _backgroundInfo.borderWidth.bottom;
  CGFloat right = (_backgroundInfo.borderRightStyle == LynxBorderStyleNone ||
                   _backgroundInfo.borderRightStyle == LynxBorderStyleHidden)
                      ? 0
                      : _backgroundInfo.borderWidth.right;
  return UIEdgeInsetsMake(top, left, bottom, right);
}

// CAShapeLayer is enabled to substitute CALayer for rendering background & border.
- (BOOL)isBackgroundShapeLayerEnabled {
  bool ret = false;
  switch (_uiBackgroundShapeLayerEnabled) {
    case LynxBgShapeLayerPropUndefine:
      ret = _ui.context.enableBackgroundShapeLayer;
      break;
    case LynxBgShapeLayerPropEnabled:
      ret = true;
      break;
    case LynxBgShapeLayerPropDisabled:
      ret = false;
      break;
  }
  return ret;
}

- (void)extractBorderRadiusValue:(const CGSize*)newViewSize {
  CGRect rect = {.size = *newViewSize};
  // Use borderRadiusRaw for calculation and borderRadius for rendering.
  LynxCornerInsets cornerInsets = LynxGetCornerInsetsA(rect, _borderRadiusRaw, UIEdgeInsetsZero,
                                                       _backgroundInfo->borderRadiusCalc);
  LynxBorderRadii radii = (LynxBorderRadii){.topLeftX.val = cornerInsets.topLeft.width,
                                            .topLeftX.unit = LynxBorderValueUnitDefault,
                                            .topLeftY.val = cornerInsets.topLeft.height,
                                            .topLeftY.unit = LynxBorderValueUnitDefault,
                                            .topRightX.val = cornerInsets.topRight.width,
                                            .topRightX.unit = LynxBorderValueUnitDefault,
                                            .topRightY.val = cornerInsets.topRight.height,
                                            .topRightY.unit = LynxBorderValueUnitDefault,
                                            .bottomRightX.val = cornerInsets.bottomRight.width,
                                            .bottomRightX.unit = LynxBorderValueUnitDefault,
                                            .bottomRightY.val = cornerInsets.bottomRight.height,
                                            .bottomRightY.unit = LynxBorderValueUnitDefault,
                                            .bottomLeftX.val = cornerInsets.bottomLeft.width,
                                            .bottomLeftX.unit = LynxBorderValueUnitDefault,
                                            .bottomLeftY.val = cornerInsets.bottomLeft.height,
                                            .bottomLeftY.unit = LynxBorderValueUnitDefault};
  bool changed = NO;
  LynxBorderUnitValue* l = (LynxBorderUnitValue*)&radii;
  LynxBorderUnitValue* r = (LynxBorderUnitValue*)&_borderRadius;
  for (int i = 0; i < 8; i++) {
    if (!isBorderUnitEqual(l[i], r[i])) {
      changed = YES;
      break;
    }
  }
  if (changed) {
    self.borderRadius = radii;
    // radius changed, background and border need updating.
    _isBGChangedNoneImage = _isBGChangedImage = YES;
  }
}

// for detailed comments, see them on LynxBackgroundManager.h
- (void)applyEffect {
  if (!_ui || !_ui.view) {
    return;
  }

  // to get the real size, reset the transform
  CATransform3D layerTransform = _ui.view.layer.transform;
  _ui.view.layer.transform = CATransform3DIdentity;
  const CGSize newViewSize = CGSizeMake(_ui.view.frame.size.width, _ui.view.frame.size.height);
  const BOOL isSizeChanged = !CGSizeEqualToSize(_backgroundSize, newViewSize);

  if (isSizeChanged) {
    // size changed, should use the new reference box calculate the borderRadius.
    // TODO(renzhongyue): we can skip this if no percentage or calc values in the raw radius value
    // from CSSComputedStyle. But it requires a more complicate state management.
    [self extractBorderRadiusValue:&newViewSize];
  }

  if ([_backgroundInfo borderChanged] &&
      (isSizeChanged || _isBGChangedNoneImage || [_backgroundInfo BGChangedNoneImage])) {
    [_ui updateLayerMaskOnFrameChanged];
  }
  _ui.view.layer.transform = layerTransform;

  const BOOL hasDifferentBorderRadius = [_backgroundInfo hasDifferentBorderRadius];
  const BOOL isSimpleBorder = [_backgroundInfo isSimpleBorder];
  const BOOL noBorderLayer = isSimpleBorder && ![self toAddSubLayerOnBorderLayer] &&
                             (OVERFLOW_HIDDEN_VAL == _ui.overflow || ![_backgroundInfo hasBorder]);
  const BOOL isSimpleBackground = [self isSimpleBackground];
  const BOOL noBackgroundLayer =
      isSimpleBackground && ![self toAddSubLayerOnBackgroundLayer:!noBorderLayer];
  // noMaskLayer = the view shouldn't contain a mask layer
  const BOOL noMaskLayer = [self.maskDrawable count] == 0 ? YES : NO;

  BOOL borderChanged = NO;
  if (isSizeChanged) {
    _backgroundSize = newViewSize;
    if (!noBorderLayer) {
      // size changed, we need to recalculate border size
      borderChanged = YES;
    }
    if (!noBackgroundLayer) {
      _isBGChangedImage = YES;
    }
    if (!noMaskLayer) {
      _isMaskChanged = YES;
    }
  }

  LynxBgTypes borderType =
      isSimpleBorder ? LynxBgTypeSimple
      : ([_backgroundInfo canUseBorderShapeLayer] && [self isBackgroundShapeLayerEnabled])
          ? LynxBgTypeShape
          : LynxBgTypeComplex;

  if (borderChanged || [_backgroundInfo borderChanged]) {
    if (noBorderLayer) {
      [self removeBorderLayer];
      [self applySimpleBorder];
    } else if (_backgroundSize.width > 0 || _backgroundSize.height > 0 || isSizeChanged) {
      [self autoAddBorderLayer:borderType];
      if (LynxBgTypeSimple == borderType) {
        [_borderLayer setContents:nil];
        [_borderLayer setPath:nil];
        [self applySimpleBorder];
      } else if (LynxBgTypeShape == borderType) {
        CGPathRef borderPath = [_backgroundInfo getBorderLayerPathWithSize:_backgroundSize];
        // Clear simple border and complex border
        [self clearSimpleBorder];
        [_borderLayer setContents:nil];
        LynxUpdateBorderLayerWithPath(_borderLayer, borderPath, _backgroundInfo);
        CGPathRelease(borderPath);
      } else {
        [self clearSimpleBorder];
        [_borderLayer setPath:nil];
        // TODO(fangzhou):move this function to draw module
        UIImage* image = [_backgroundInfo getBorderLayerImageWithSize:_backgroundSize];
        LynxUpdateLayerWithImage(_borderLayer, image);
      }
      [self autoAddOutlineLayer];
    }
    borderChanged = NO;
    _backgroundInfo.borderChanged = NO;
  }

  // If UI previously has no border, background layer will not be created. If then the borders are
  // added, we need create backgroundLayer to ensure background is below border.
  const BOOL needToCreateBackgroundLayer = _backgroundLayer == nil && !noBackgroundLayer;
  if (_isBGChangedImage || _isBGChangedNoneImage || [_backgroundInfo BGChangedImage] ||
      [_backgroundInfo BGChangedNoneImage] || needToCreateBackgroundLayer) {
    if (noBackgroundLayer) {
      [self removeBackgroundLayer];
      _ui.view.backgroundColor = [_backgroundInfo backgroundColor];
      _ui.view.layer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                                bySize:newViewSize];
      _isBGChangedImage = _isBGChangedNoneImage = NO;
      _backgroundInfo.BGChangedImage = _backgroundInfo.BGChangedNoneImage = NO;
    } else if (_backgroundSize.width > 0 || _backgroundSize.height > 0 || isSizeChanged) {
      if (nil != _backgroundLayer && self.backgroundDrawable.count == 0) {
        _backgroundLayer.contents = nil;
        [_backgroundLayer detachAllGradientLayers];
      }
      [self autoAddBackgroundLayer:!isSimpleBackground];
      if (!isSimpleBackground) {
        [self applyComplexBackground];
      }
      [self updateShadow];
      _isBGChangedImage = _isBGChangedNoneImage = NO;
      _backgroundInfo.BGChangedImage = _backgroundInfo.BGChangedNoneImage = NO;
    }
  }

  if (_isMaskChanged) {
    if (noMaskLayer) {
      [self removeMaskLayer];
    } else if (_backgroundSize.width > 0 || _backgroundSize.height > 0 || isSizeChanged) {
      [self addMaskLayer];
      [self applyComplexMask];
    }
  }

  if (hasDifferentBorderRadius) {
    _ui.view.layer.cornerRadius = 0;
  } else {
    // if radius values of all corners are same, just use layer cornerRadius
    _ui.view.layer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                              bySize:newViewSize];
  }

  if (!CGSizeEqualToSize(newViewSize, CGSizeZero) || _withAnimation) {
    // if lynxUI has sticky attribute, transform = _transform + stickyTransform
    // else transform is default _transform
    CATransform3D transformWithSticky = [self getTransformWithPostTranslate];

    _ui.view.layer.transform = CATransform3DIdentity;
    if (_backgroundLayer != nil) {
      // reset transform before change frame, otherwise the result value of frame is undefined.

      _backgroundLayer.transform = CATransform3DIdentity;
      // All properties should set with the original frame size without UI transformation.
      _backgroundLayer.frame = _ui.view.layer.frame;
      _backgroundLayer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                                  bySize:_backgroundLayer.frame.size];
      _backgroundLayer.mask = nil;
      if (_ui.clipPath) {
        CAShapeLayer* mask = [[CAShapeLayer alloc] init];
        UIBezierPath* path = [_ui.clipPath pathWithFrameSize:_ui.frameSize];
        mask.path = path.CGPath;
        _backgroundLayer.mask = mask;
      }
      _backgroundLayer.transform = transformWithSticky;
    }

    if (_borderLayer != nil) {
      _borderLayer.transform = CATransform3DIdentity;
      // All properties should set with the original frame size without UI transformation.
      _borderLayer.frame = _ui.view.layer.frame;
      _borderLayer.cornerRadius = [self adjustRadius:[_backgroundInfo borderRadius].topLeftX.val
                                              bySize:_borderLayer.frame.size];
      _borderLayer.mask = nil;
      if (_ui.clipPath) {
        CAShapeLayer* mask = [[CAShapeLayer alloc] init];
        UIBezierPath* path = [_ui.clipPath pathWithFrameSize:_ui.frameSize];
        mask.path = path.CGPath;
        _borderLayer.mask = mask;
      }
      _borderLayer.transform = transformWithSticky;
    }

    if (!CATransform3DIsIdentity(transformWithSticky)) {
      _ui.view.layer.transform = transformWithSticky;
    }
  }

  // apply opacity on layers.
  [self setOpacity:_opacity];
}

- (void)getBackgroundImageForContentsAnimationAsync:(void (^)(UIImage*))completionBlock
                                           withSize:(CGSize)size {
  [self setWithAnimation];
  NSMutableArray* bgImgInfoArr =
      [self generateBackgroundImageLayerWithSize:size info:self.backgroundDrawable];
  [self getBackgroundImageAsync:bgImgInfoArr withCompletion:completionBlock];
}

- (UIImage*)getBackgroundImageForContentsAnimationWithSize:(CGSize)size {
  [self setWithAnimation];
  NSMutableArray* bgImgInfoArr =
      [self generateBackgroundImageLayerWithSize:size info:self.backgroundDrawable];
  UIImage* image = LynxGetBackgroundImage(
      size, [_backgroundInfo borderRadius], [self getAdjustedBorderWidth],
      [_backgroundInfo backgroundColor].CGColor, NO, bgImgInfoArr, _isPixelated);
  return image;
}

- (UIImage*)getBorderImageForContentsAnimationWithSize:(CGSize)size {
  UIImage* image = [_backgroundInfo getBorderLayerImageWithSize:size];
  return image;
}

- (CGPathRef)getBorderPathForAnimationWithSize:(CGSize)size {
  CGPathRef path = [_backgroundInfo getBorderLayerPathWithSize:size];
  return path;
}

- (UIImage*)getBackgroundImageForContentsAnimation {
  return [self getBackgroundImageForContentsAnimationWithSize:_backgroundSize];
}

- (void)removeShadowLayers {
  for (LynxBoxShadow* shadow in _shadowArray) {
    if (shadow.layer != nil) {
      [shadow.layer removeFromSuperlayer];
    }
  }
}

- (void)setShadowArray:(NSArray<LynxBoxShadow*>*)shadowArrayIn {
  const NSUInteger count = [_shadowArray count];
  if (count == [shadowArrayIn count]) {
    bool allSame = true;
    for (size_t i = 0; i < count; ++i) {
      if (![_shadowArray[i] isEqualToBoxShadow:shadowArrayIn[i]]) {
        allSame = false;
        break;
      }
    }
    if (allSame) return;
  }

  [self removeShadowLayers];

  _shadowArray = shadowArrayIn;

  _isBGChangedNoneImage = YES;
}

- (void)updateShadow {
  if (self.ui.view.superview == nil || self.backgroundLayer == nil) {
    return;
  }

  CALayer* lastInsetLayer = nil;
  const bool hasBorderRadii = LynxHasBorderRadii([_backgroundInfo borderRadius]);

  for (LynxBoxShadow* shadow in _shadowArray) {
    if (shadow.layer != nil) {
      [shadow.layer removeFromSuperlayer];
    }

    // TODO(renzhongyue): rasterize shadow with spreadRadius. Now the shouldRasterizeShadow will
    // only shadows without spread radius on bitmap backends.
    // -[LynxBackgroundManager shouldRasterize] is an attribute set by front end.
    const BOOL hasSpreadRadius = shadow.spreadRadius != 0;
    const BOOL rasterizeShadow = _shouldRasterizeShadow && !hasSpreadRadius;

    CALayer* layer;
    if (!rasterizeShadow) {
      layer = [CALayer new];
      layer.shadowColor = shadow.shadowColor.CGColor;
      layer.shadowOpacity = 1.0f;
      layer.shadowRadius = shadow.blurRadius * 0.5f;
      layer.shadowOffset = CGSizeMake(shadow.offsetX, shadow.offsetY);
    } else {
      layer = [[LynxBoxShadowLayer alloc] initWithUi:_ui];
      [(LynxBoxShadowLayer*)layer setCustomShadowBlur:shadow.blurRadius];
      [(LynxBoxShadowLayer*)layer setCustomShadowColor:shadow.shadowColor];
      [(LynxBoxShadowLayer*)layer setCustomShadowOffset:CGSizeMake(shadow.offsetX, shadow.offsetY)];
      [(LynxBoxShadowLayer*)layer setInset:shadow.inset];
    }

    // Common props for rasterized shadow and UIKit shadowPath.
    shadow.layer = layer;
    layer.frame = self.backgroundLayer.bounds;  // sub layer
    _backgroundLayer.shadowsBounds = self.backgroundLayer.bounds;
    layer.masksToBounds = NO;
    layer.backgroundColor = [UIColor clearColor].CGColor;
    layer.allowsEdgeAntialiasing = _allowsEdgeAntialiasing;

    const float maxInset = MIN(layer.bounds.size.width, layer.bounds.size.height) * 0.5f;
    const float inset = MAX(shadow.spreadRadius, -maxInset);

    if (shadow.inset) {
      CGMutablePathRef maskPath = CGPathCreateMutable(), path = CGPathCreateMutable();
      const CGRect innerRect = CGRectInset(layer.bounds, inset, inset);
      if (!hasBorderRadii) {
        LynxPathAddRect(path, innerRect, false);
        CGPathAddRect(maskPath, nil, layer.bounds);
      } else {
        UIEdgeInsets borders;
        borders.top = borders.right = borders.bottom = borders.left = inset;
        LynxPathAddRoundedRect(
            path, LynxGetRectWithEdgeInsets(layer.bounds, borders),
            LynxGetCornerInsets(layer.bounds, [_backgroundInfo borderRadius], borders));
        LynxPathAddRoundedRect(
            maskPath, layer.bounds,
            LynxGetCornerInsets(layer.bounds, [_backgroundInfo borderRadius], UIEdgeInsetsZero));
      }
      // inverse outer rect large enough is ok
      const CGRect outerRect = CGRectInset(
          CGRectUnion(innerRect, CGRectOffset(layer.bounds, -shadow.offsetX, -shadow.offsetY)),
          -300, -300);
      LynxPathAddRect(path, outerRect, true);

      // Set path to layer, LynxShadowLayer use customized rendering function to avoid off-screen
      // rendering, don't set value to CALayer's props. But CoreGraphics don't have blur effect or
      // shadow spread effect. Shadows with spreadRadius should still use CALayer's shadowPath
      // property.
      if (!rasterizeShadow) {
        layer.shadowPath = path;
        // clip by the real round rect
        CAShapeLayer* shapeLayer = [[CAShapeLayer alloc] init];
        shapeLayer.path = maskPath;
        layer.mask = shapeLayer;
      } else {
        [(LynxBoxShadowLayer*)layer setCustomShadowPath:path];
        [(LynxBoxShadowLayer*)layer setMaskPath:maskPath];
        layer.frame = outerRect;
        [(LynxBoxShadowLayer*)layer invalidate];
      }

      // add above all background images, keep the order
      if (lastInsetLayer != nil) {
        [_backgroundLayer insertSublayer:layer below:lastInsetLayer];
      } else {
        [_backgroundLayer addSublayer:layer];
      }
      lastInsetLayer = layer;
      CGPathRelease(path);
      CGPathRelease(maskPath);
    } else {
      CGMutablePathRef maskPath = CGPathCreateMutable();
      CGPathRef path = nil;
      if (!hasBorderRadii) {
        CGPathAddRect(maskPath, nil, layer.bounds);
        path = CGPathRetain(
            [UIBezierPath bezierPathWithRect:CGRectInset(layer.bounds, -shadow.spreadRadius,
                                                         -shadow.spreadRadius)]
                .CGPath);

      } else {
        path = LynxPathCreateWithRoundedRect(
            layer.bounds,
            LynxGetCornerInsets(layer.bounds, [_backgroundInfo borderRadius], UIEdgeInsetsZero));
        CGPathAddPath(maskPath, nil, path);
        CGPathRelease(path);

        UIEdgeInsets borders;
        borders.top = borders.right = borders.bottom = borders.left = -shadow.spreadRadius;
        path = LynxPathCreateWithRoundedRect(
            LynxGetRectWithEdgeInsets(layer.bounds, borders),
            LynxGetCornerInsets(layer.bounds, [_backgroundInfo borderRadius], borders));
      }
      const float inset = -3 * (MAX(shadow.blurRadius, 0) + MAX(shadow.spreadRadius, 0));
      const CGRect shadowOuterRect =
          CGRectOffset(CGRectInset(layer.bounds, inset, inset), shadow.offsetX, shadow.offsetY);
      CGPathAddRect(maskPath, nil, CGRectInset(CGRectUnion(layer.bounds, shadowOuterRect), -5, -5));

      if (!rasterizeShadow) {
        // clip area between outerRect and real shadow inner rect
        CAShapeLayer* shapeLayer = [[CAShapeLayer alloc] init];
        shapeLayer.path = maskPath;
        shapeLayer.fillRule = kCAFillRuleEvenOdd;
        layer.mask = shapeLayer;
        layer.shadowPath = path;
      } else {
        [(LynxBoxShadowLayer*)layer setCustomShadowPath:path];
        [(LynxBoxShadowLayer*)layer setMaskPath:maskPath];
        layer.frame = shadowOuterRect;
        [(LynxBoxShadowLayer*)layer invalidate];
      }

      // always below border-layer, image layers, keep the order
      [self.backgroundLayer insertSublayer:layer atIndex:0];
      CGPathRelease(maskPath);
      CGPathRelease(path);
    }
  }
}

- (void)removeAssociateLayers {
  if (_borderLayer) {
    [_borderLayer removeFromSuperlayer];
  }
  if (_backgroundLayer) {
    [_backgroundLayer removeFromSuperlayer];
  }
}

- (void)setFilters:(nullable NSArray*)array {
  _backgroundLayer.filters = array;
  _borderLayer.filters = array;
  _outlineLayer.filters = array;
}

#pragma mark getter

- (NSMutableArray*)backgroundDrawable {
  if (!_backgroundDrawable) {
    _backgroundDrawable = [[NSMutableArray alloc] init];
  }
  return _backgroundDrawable;
}

- (NSMutableArray*)backgroundOrigin {
  if (!_backgroundOrigin) {
    _backgroundOrigin = [[NSMutableArray alloc] init];
  }
  return _backgroundOrigin;
}

- (NSMutableArray*)backgroundPosition {
  if (!_backgroundPosition) {
    _backgroundPosition = [[NSMutableArray alloc] init];
  }
  return _backgroundPosition;
}

- (NSMutableArray*)backgroundRepeat {
  if (!_backgroundRepeat) {
    _backgroundRepeat = [[NSMutableArray alloc] init];
  }
  return _backgroundRepeat;
}

- (NSMutableArray*)backgroundClip {
  if (!_backgroundClip) {
    _backgroundClip = [[NSMutableArray alloc] init];
  }
  return _backgroundClip;
}

- (NSMutableArray*)backgroundImageSize {
  if (!_backgroundImageSize) {
    _backgroundImageSize = [[NSMutableArray alloc] init];
  }
  return _backgroundImageSize;
}

- (NSMutableArray*)maskDrawable {
  if (!_maskDrawable) {
    _maskDrawable = [[NSMutableArray alloc] init];
  }
  return _maskDrawable;
}

- (NSMutableArray*)maskOrigin {
  if (!_maskOrigin) {
    _maskOrigin = [[NSMutableArray alloc] init];
  }
  return _maskOrigin;
}

- (NSMutableArray*)maskPosition {
  if (!_maskPosition) {
    _maskPosition = [[NSMutableArray alloc] init];
  }
  return _maskPosition;
}

- (NSMutableArray*)maskRepeat {
  if (!_maskRepeat) {
    _maskRepeat = [[NSMutableArray alloc] init];
  }
  return _maskRepeat;
}

- (NSMutableArray*)maskClip {
  if (!_maskClip) {
    _maskClip = [[NSMutableArray alloc] init];
  }
  return _maskClip;
}

- (NSMutableArray*)maskSize {
  if (!_maskSize) {
    _maskSize = [[NSMutableArray alloc] init];
  }
  return _maskSize;
}

#pragma mark duplicate utils functions
+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii
                                  edgeInsets:(UIEdgeInsets)edgeInsets {
  return [LynxBackgroundUtils createBezierPathWithRoundedRect:bounds
                                                  borderRadii:borderRadii
                                                   edgeInsets:edgeInsets];
}

+ (CGPathRef)createBezierPathWithRoundedRect:(CGRect)bounds
                                 borderRadii:(LynxBorderRadii)borderRadii {
  return [LynxBackgroundUtils createBezierPathWithRoundedRect:bounds borderRadii:borderRadii];
}

#pragma mark duplicate info functions
- (void)makeCssDefaultValueToFitW3c {
  [_backgroundInfo makeCssDefaultValueToFitW3c];
}

- (BOOL)hasDifferentBorderRadius {
  return [_backgroundInfo hasDifferentBorderRadius];
}

- (BOOL)hasDifferentBackgroundColor:(UIColor*)color {
  return _backgroundInfo.backgroundColor && _backgroundInfo.backgroundColor != color;
}

- (void)setBackgroundColor:(UIColor*)color {
  _backgroundColor = color;
  _backgroundInfo.backgroundColor = color;
}

- (void)setBorderRadius:(LynxBorderRadii)borderRadius {
  _borderRadius = borderRadius;
  [_backgroundInfo setBorderRadius:borderRadius];
}

- (void)setBorderWidth:(UIEdgeInsets)width {
  _borderWidth = width;
  [_backgroundInfo setBorderWidth:width];
}

- (void)setBorderTopColor:(UIColor*)borderTopColor {
  _borderTopColor = borderTopColor;
  [_backgroundInfo updateBorderColor:LynxBorderTop value:borderTopColor];
}

- (void)setBorderLeftColor:(UIColor*)borderLeftColor {
  _borderLeftColor = borderLeftColor;
  [_backgroundInfo updateBorderColor:LynxBorderLeft value:borderLeftColor];
}

- (void)setBorderRightColor:(UIColor*)borderRightColor {
  _borderRightColor = borderRightColor;
  [_backgroundInfo updateBorderColor:LynxBorderRight value:borderRightColor];
}

- (void)setBorderBottomColor:(UIColor*)borderBottomColor {
  _borderBottomColor = borderBottomColor;
  [_backgroundInfo updateBorderColor:LynxBorderBottom value:borderBottomColor];
}

- (BOOL)updateOutlineWidth:(CGFloat)outlineWidth {
  return [_backgroundInfo updateOutlineWidth:outlineWidth];
}

- (BOOL)updateOutlineColor:(UIColor*)outlineColor {
  return [_backgroundInfo updateOutlineColor:outlineColor];
}
- (BOOL)updateOutlineStyle:(LynxBorderStyle)outlineStyle {
  return [_backgroundInfo updateOutlineStyle:outlineStyle];
}

- (void)updateBorderColor:(LynxBorderPosition)position value:(UIColor*)color {
  [_backgroundInfo updateBorderColor:position value:color];
}

- (BOOL)updateBorderStyle:(LynxBorderPosition)position value:(LynxBorderStyle)style {
  return [_backgroundInfo updateBorderStyle:position value:style];
}
@end

@implementation LynxConverter (LynxBorderStyle)

+ (LynxBorderStyle)toLynxBorderStyle:(id)value {
  if (!value || [value isEqual:[NSNull null]]) {
    return LynxBorderStyleSolid;
  }
  return (LynxBorderStyle)[value intValue];
}
@end
