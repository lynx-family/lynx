// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBackgroundDrawable.h"
#import "LynxCSSType.h"
#import "LynxUIUnitUtils.h"
#include "base/include/log/logging.h"

// Layers that implicit animations are disabled.
@interface LBGGradientLayer : CAGradientLayer
@end

@implementation LBGGradientLayer

- (id<CAAction>)actionForKey:(NSString *)event {
  return [NSNull null];
}

@end

@interface LBGReplicatorLayer : CAReplicatorLayer
@end

@implementation LBGReplicatorLayer
- (id<CAAction>)actionForKey:(NSString *)event {
  return [NSNull null];
}
@end

#pragma mark - LynxBackgroundSize
@implementation LynxBackgroundSize

- (instancetype)init {
  return [self initWithLength:nil];
}

- (instancetype)initWithLength:(LynxPlatformLength *)value {
  self = [super init];
  if (self) {
    self.value = value;
  }
  return self;
}

- (BOOL)isCover {
  return (NSInteger)self.value.numberValue == LynxBackgroundSizeCover;
}

- (BOOL)isContain {
  return (NSInteger)self.value.numberValue == LynxBackgroundSizeContain;
}

- (BOOL)isAuto {
  return (NSInteger)self.value.numberValue == LynxBackgroundSizeAuto;
}

- (CGFloat)apply:(CGFloat)parentValue currentValue:(CGFloat)currentValue {
  if ([self isAuto]) {
    return currentValue;
  } else {
    return [self.value valueWithParentValue:parentValue];
  }
}

@end

#pragma mark - LynxBackgroundPosition

@implementation LynxBackgroundPosition

- (instancetype)initWithValue:(LynxPlatformLength *)value {
  self = [super init];
  if (self) {
    self.value = value;
  }
  return self;
}

- (CGFloat)apply:(CGFloat)availableValue {
  return [self.value valueWithParentValue:availableValue];
}

@end

#pragma mark - LynxBackgroundDrawable

@interface LynxBackgroundDrawable ()
- (void)onDraw:(CGContextRef)ctx rect:(CGRect)rect;
@end

@implementation LynxBackgroundDrawable

- (instancetype)init {
  self = [super init];
  if (self) {
    self.repeatX = LynxBackgroundRepeatRepeat;
    self.repeatY = LynxBackgroundRepeatRepeat;
    self.origin = LynxBackgroundOriginPaddingBox;
    self.clip = LynxBackgroundClipBorderBox;
    self.sizeX = nil;
    self.sizeY = nil;
    self.posX = nil;
    self.posY = nil;
    self.bounds = CGRectZero;
  }
  return self;
}

- (LynxBackgroundImageType)type {
  return LynxBackgroundImageNone;
}

- (CGFloat)getImageWidth {
  return self.bounds.size.width;
}

- (CGFloat)getImageHeight {
  return self.bounds.size.height;
}

- (BOOL)isReady {
  return YES;
}

- (BOOL)isGradient {
  return NO;
}

- (CGSize)computeBackgroundSizeWithImageSize:(const CGSize *_Nonnull)imageSize
                             andPaintBoxSize:(const CGSize *_Nonnull)paintBoxSize {
  CGFloat selfWidth = paintBoxSize->width;
  CGFloat selfHeight = paintBoxSize->height;
  // adjust the size
  CGFloat width = imageSize->width;
  CGFloat height = imageSize->height;

  // early return size zero.
  if (ABS(width) < FLT_EPSILON || ABS(height) < FLT_EPSILON) {
    return CGSizeZero;
  }

  CGFloat aspect = width / height;
  if ([self.sizeX isCover]) {
    width = selfWidth;
    height = width / aspect;
    if (height < selfHeight) {
      height = selfHeight;
      width = aspect * height;
    }
  } else if ([self.sizeX isContain]) {
    width = selfWidth;
    height = width / aspect;
    if (height > selfHeight) {
      height = selfHeight;
      width = aspect * height;
    }
  } else if (self.sizeX != nil && self.sizeY != nil) {
    width = [self.sizeX apply:selfWidth currentValue:width];
    height = [self.sizeY apply:selfHeight currentValue:height];

    if ([self.sizeX isAuto]) {
      if ([self isGradient]) {
        width = paintBoxSize->width;
      } else {
        width = aspect * height;
      }
    }

    if ([self.sizeY isAuto]) {
      if ([self isGradient]) {
        height = paintBoxSize->height;
      } else {
        height = width / aspect;
      }
    }
  }
  return (CGSize){width, height};
}

- (void)computeBackgroundPosition:(CGFloat *_Nonnull)offsetX
                          offsetY:(CGFloat *_Nonnull)offsetY
                         paintBox:(const CGRect)paintBox
                             size:(const CGSize)size {
  *offsetX = paintBox.origin.x;
  *offsetY = paintBox.origin.y;

  if (self.posX != nil && self.posY != nil) {
    CGSize deltaSize =
        CGSizeMake(paintBox.size.width - size.width, paintBox.size.height - size.height);
    *offsetX += [self.posX apply:deltaSize.width];
    *offsetY += [self.posY apply:deltaSize.height];
  }
}

- (void)drawInContext:(CGContextRef)ctx
           borderRect:(CGRect)borderRect
          paddingRect:(CGRect)paddingRect
          contentRect:(CGRect)contentRect {
  if (![self isReady]) {
    return;
  }
  // decide painting area
  CGRect paintBox = paddingRect;
  switch (self.origin) {
    case LynxBackgroundOriginBorderBox:
      paintBox = borderRect;
      break;
    case LynxBackgroundOriginContentBox:
      paintBox = contentRect;
      break;
    default:
      paintBox = paddingRect;
      break;
  }
  // adjust the size
  CGSize imageSize = paintBox.size;
  if (![self isGradient]) {
    imageSize.width = [self getImageWidth];
    imageSize.height = [self getImageHeight];
  }

  CGSize size = [self computeBackgroundSizeWithImageSize:&imageSize
                                         andPaintBoxSize:&(paintBox.size)];

  // OPTME:(tangruiwen) see issue:#4190
  if (size.width <= 0.01 || size.height <= 0.01) {
    return;
  }

  [self setBounds:CGRectMake(0, 0, size.width, size.height)];

  // decide position
  CGFloat offsetX;
  CGFloat offsetY;
  [self computeBackgroundPosition:&offsetX offsetY:&offsetY paintBox:paintBox size:size];

  // repeat type
  CGContextSaveGState(ctx);
  if (self.repeatX == LynxBackgroundRepeatNoRepeat &&
      self.repeatY == LynxBackgroundRepeatNoRepeat) {
    CGContextTranslateCTM(ctx, offsetX, offsetY);
    [self onDraw:ctx rect:CGRectMake(0, 0, size.width, size.height)];
  } else {
    CGFloat endX = MAX((paintBox.origin.x + paintBox.size.width),
                       (borderRect.origin.x + borderRect.size.width));
    CGFloat endY = MAX((paintBox.origin.y + paintBox.size.height),
                       (borderRect.origin.y + borderRect.size.height));
    CGFloat startX =
        (self.repeatX == LynxBackgroundRepeatRepeat || self.repeatX == LynxBackgroundRepeatRepeatX)
            ? offsetX - ceil(offsetX / size.width) * size.width
            : offsetX;
    CGFloat startY =
        (self.repeatY == LynxBackgroundRepeatRepeat || self.repeatY == LynxBackgroundRepeatRepeatY)
            ? offsetY - ceil(offsetY / size.height) * size.height
            : offsetY;

    for (CGFloat x = startX; x < endX; x += size.width) {
      for (CGFloat y = startY; y < endY; y += size.height) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, x, y);
        [self onDraw:ctx rect:CGRectMake(0, 0, size.width, size.height)];
        CGContextRestoreGState(ctx);

        if (self.repeatY == LynxBackgroundRepeatNoRepeat) {
          break;
        }
      }
      if (self.repeatX == LynxBackgroundRepeatNoRepeat) {
        break;
      }
    }
  }
  CGContextRestoreGState(ctx);
}

- (void)onDraw:(CGContextRef)ctx rect:(CGRect)rect {
}

- (void)drawTextBackgroundInContext:(CGContextRef)ctx contentRect:(CGRect)contentRect {
  if (![self isReady]) {
    return;
  }
  // decide painting area
  CGRect paintBox = contentRect;
  CGFloat selfWidth = paintBox.size.width;
  CGFloat selfHeight = paintBox.size.height;
  // adjust the size
  CGFloat width = paintBox.size.width;
  CGFloat height = paintBox.size.height;
  CGFloat widthSrc = [self getImageWidth];
  CGFloat heightSrc = [self getImageHeight];

  if ([self isGradient]) {
    width = paintBox.size.width;
    height = paintBox.size.height;
  } else {
    width = widthSrc;
    height = heightSrc;
  }
  CGFloat aspect = widthSrc / heightSrc;
  if ([self.sizeX isCover]) {
    width = selfWidth;
    height = width / aspect;
    if (height < selfHeight) {
      height = selfHeight;
      width = aspect * height;
    }
  } else if ([self.sizeX isContain]) {
    width = selfWidth;
    height = width / aspect;
    if (height > selfHeight) {
      height = selfHeight;
      width = aspect * height;
    }
  } else if (self.sizeX != nil && self.sizeY != nil) {
    width = [self.sizeX apply:selfWidth currentValue:width];
    height = [self.sizeY apply:selfHeight currentValue:height];

    if ([self.sizeX isAuto]) {
      if ([self isGradient]) {
        width = paintBox.size.width;
      } else {
        width = aspect * height;
      }
    }

    if ([self.sizeY isAuto]) {
      if ([self isGradient]) {
        height = paintBox.size.height;
      } else {
        height = width / aspect;
      }
    }
  }

  if (width <= 0.01 || height <= 0.01) {
    return;
  }

  [self setBounds:CGRectMake(0, 0, width, height)];

  // decide position
  CGFloat offsetX = paintBox.origin.x;
  CGFloat offsetY = paintBox.origin.y;

  if (self.posX != nil && self.posY != nil) {
    CGSize deltaSize = CGSizeMake(paintBox.size.width - width, paintBox.size.height - height);
    offsetX += [self.posX apply:deltaSize.width];
    offsetY += [self.posY apply:deltaSize.height];
  }

  // repeat type
  CGContextSaveGState(ctx);
  if (self.repeatX == LynxBackgroundRepeatNoRepeat &&
      self.repeatY == LynxBackgroundRepeatNoRepeat) {
    CGContextTranslateCTM(ctx, offsetX, offsetY);
    [self onDraw:ctx rect:CGRectMake(0, 0, width, height)];
  } else {
    CGFloat endX = paintBox.origin.x + paintBox.size.width;
    CGFloat endY = paintBox.origin.y + paintBox.size.height;
    CGFloat startX =
        (self.repeatX == LynxBackgroundRepeatRepeat || self.repeatX == LynxBackgroundRepeatRepeatX)
            ? offsetX - 1.0
            : offsetX;
    CGFloat startY =
        (self.repeatY == LynxBackgroundRepeatRepeat || self.repeatY == LynxBackgroundRepeatRepeatY)
            ? offsetY - 1.0
            : offsetY;

    for (CGFloat x = startX; x < endX; x += width) {
      for (CGFloat y = startY; y < endY; y += height) {
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, x, y);
        [self onDraw:ctx rect:CGRectMake(0, 0, width, height)];
        CGContextRestoreGState(ctx);

        if (self.repeatY == LynxBackgroundRepeatNoRepeat) {
          break;
        }
      }
      if (self.repeatX == LynxBackgroundRepeatNoRepeat) {
        break;
      }
    }
  }
  CGContextRestoreGState(ctx);
}

@end

#pragma mark - LynxBackgroundImageDrawable
@interface LynxBackgroundImageDrawable ()
@property(nonatomic) NSUInteger currentFrame;
@property(nonatomic) NSUInteger *stepArray;
@property(nonatomic) NSUInteger currentStepIndex;
@end

@implementation LynxBackgroundImageDrawable
unsigned long stepArrayLen = 0;

- (LynxBackgroundImageType)type {
  return LynxBackgroundImageURL;
}

- (void)dealloc {
  if (_stepArray) {
    free(_stepArray);
  }
}

/*
  Returns how many frames should be skipped after drawingInContext;
 */
- (NSUInteger)nextStep {
  if ([_image.images count] / [_image duration] < 30) return 1;
  if (!_stepArray) {
    // Lazy initialization to make sure image is ready.
    [self generateStepArrayWithFPS:([_image.images count] / [_image duration]) andTargetFPS:30];
  }
  _currentStepIndex = _currentStepIndex % stepArrayLen;
  return _stepArray[_currentStepIndex++];
}

- (void)generateStepArrayWithFPS:(NSUInteger)FPS andTargetFPS:(NSUInteger)targetFPS {
  // Every FPS / gcd_ frames should draw targetFPS / gcd_ frames.
  // stepArray[i] means after draw the ith frame should skip next stepArray[i] frames.
  NSUInteger gcdInt, arrayLen, remains;
  gcdInt = gcd(FPS, targetFPS);
  arrayLen = targetFPS / gcdInt;
  remains = FPS / gcdInt;

  unsigned long base = remains / arrayLen;
  remains = remains % arrayLen;

  _stepArray = (NSUInteger *)malloc(arrayLen * sizeof(NSUInteger));
  for (unsigned long i = 0; i < remains; ++i) {
    _stepArray[i] = base + 1;
  }
  for (unsigned long i = remains; i < arrayLen; ++i) {
    _stepArray[i] = base;
  }
  stepArrayLen = arrayLen;
}

NSUInteger gcd(NSUInteger a, NSUInteger b) {
  NSUInteger temp;
  while (b != 0) {
    temp = a % b;
    a = b;
    b = temp;
  }
  return a;
}

- (void)drawInContext:(CGContextRef)ctx
           borderRect:(CGRect)borderRect
          paddingRect:(CGRect)paddingRect
          contentRect:(CGRect)contentRect {
  if (self.image && self.image.images) {
    _currentFrame = _currentFrame % [self.image.images count];
    [super drawInContext:ctx borderRect:borderRect paddingRect:paddingRect contentRect:contentRect];
    _currentFrame += [self nextStep];
  } else {
    [super drawInContext:ctx borderRect:borderRect paddingRect:paddingRect contentRect:contentRect];
  }
}

- (instancetype)initWithURL:(NSURL *)url {
  self = [super init];
  if (self) {
    self.url = url;
    self.image = nil;
  }
  return self;
}

- (instancetype)initWithString:(NSString *)string {
  self = [super init];
  if (self) {
    self.url = [NSURL URLWithString:[self illegalUrlHandler:string]];
    self.image = nil;
  }
  return self;
}

- (NSString *)illegalUrlHandler:(NSString *)value {
  // To handle some illegal symbols, such as chinese characters and [], etc
  // Query + Path characterset will cover all other urlcharacterset
  if (![[NSURL alloc] initWithString:value]) {
    NSMutableCharacterSet *characterSetForEncode = [[NSMutableCharacterSet alloc] init];
    [characterSetForEncode formUnionWithCharacterSet:[NSCharacterSet URLQueryAllowedCharacterSet]];
    [characterSetForEncode formUnionWithCharacterSet:[NSCharacterSet URLPathAllowedCharacterSet]];
    value = [value stringByAddingPercentEncodingWithAllowedCharacters:characterSetForEncode];
  }
  return value;
}

- (BOOL)isReady {
  return self.image != nil;
}

- (CGFloat)getImageWidth {
  if (self.image == nil) {
    return 0;
  }
  return self.image.size.width;
}

- (CGFloat)getImageHeight {
  if (self.image == nil) {
    return 0;
  }
  return self.image.size.height;
}

- (void)onDraw:(CGContextRef)ctx rect:(CGRect)rect {
  UIGraphicsPushContext(ctx);
  if (_image.images && _image.images.count > 0) {
    [_image.images[_currentFrame] drawInRect:rect];
  } else {
    [self.image drawInRect:rect];
  }
  UIGraphicsPopContext();
}
@end

#pragma mark - LynxBackgroundGradientDrawable

@implementation LynxBackgroundGradientDrawable
- (LynxBackgroundImageType)type {
  return LynxBackgroundImageLinearGradient;
}
- (BOOL)isGradient {
  return YES;
}

- (BOOL)isReady {
  return self.gradient != nil;
}

- (void)onDraw:(CGContextRef)ctx rect:(CGRect)rect {
  CGContextSaveGState(ctx);
  CGContextAddRect(ctx, rect);
  CGContextClip(ctx);
  [self.gradient draw:ctx withRect:rect];
  CGContextRestoreGState(ctx);
}
- (void)onPrepareGradientWithSize:(CGSize)gradientSize {
  // Abstract, implemented in LynxBackgroundLinearGradientDrawable and
  // LynxBackgroundRadialGradientDrawable.
}

// The offset is calculated from 'background-position' property, which means the offset from the top
// left point of border-box.
- (void)handleBackgroundRepeat:(const CGRect &)borderBox
                          size:(const CGSize &)size
                        height:(CGFloat)height
                         width:(CGFloat)width
                       offsetX:(CGFloat)offsetX
                       offsetY:(CGFloat)offsetY {
  if (ABS(width) < FLT_EPSILON || ABS(height) < FLT_EPSILON) {
    return;
  }

  if (!_verticalRepeatLayer) {
    _verticalRepeatLayer = [LBGReplicatorLayer layer];
    _verticalRepeatLayer.masksToBounds = YES;
  }

  if (!_horizontalRepeatLayer) {
    _horizontalRepeatLayer = [LBGReplicatorLayer layer];
    _horizontalRepeatLayer.masksToBounds = NO;
  }

  // Handle background repeat
  [_horizontalRepeatLayer addSublayer:_gradientLayer];
  _horizontalRepeatLayer.frame = borderBox;

  _horizontalRepeatLayer.instanceTransform = CATransform3DMakeTranslation(width, 0, 0);

  _verticalRepeatLayer.frame = borderBox;
  _verticalRepeatLayer.instanceTransform = CATransform3DMakeTranslation(0, height, 0);
  [_verticalRepeatLayer addSublayer:_horizontalRepeatLayer];

  CGFloat startX = offsetX, startY = offsetY;
  if (self.repeatX == LynxBackgroundRepeatRepeat) {
    int backwardCount = ceil(offsetX / width);
    // replicator only support one way transform, move the start point back out of the left most
    // point of visible area.
    startX = offsetX - backwardCount * width;
    // plus one to ensure fully cover the visible area
    _horizontalRepeatLayer.instanceCount = ceil(size.width / width) + 1;
  }

  if (self.repeatY == LynxBackgroundRepeatRepeat) {
    int backwardCount = ceil(offsetY / height);
    // replicator only support one way transform, move the start point back out of the top most
    // point of visible area.
    startY = offsetY - backwardCount * height;
    // plus one to ensure fully cover the visible area
    _verticalRepeatLayer.instanceCount = ceil(size.height / height) + 1;
  }
  _gradientLayer.frame = CGRectMake(startX, startY, width, height);
}

- (void)prepareGradientWithBorderBox:(CGRect)borderBox
                         andPaintBox:(CGRect)paintBox
                         andClipRect:(CGRect)clipRect {
  if (!_gradientLayer) {
    _gradientLayer = [LBGGradientLayer layer];
  }
  // set gradient to gradient layer
  NSMutableArray<NSNumber *> *locations =
      _gradient.positionCount > 0 ? [[NSMutableArray alloc] init] : nil;
  for (NSUInteger i = 0; i < _gradient.positionCount; i++) {
    [locations addObject:@(_gradient.positions[i])];
  }
  _gradientLayer.locations = locations;

  NSMutableArray<id> *colors = [[NSMutableArray alloc] init];
  for (NSUInteger i = 0; i < _gradient.colors.count; i++) {
    [colors addObject:(__bridge id)((UIColor *)(_gradient.colors[i])).CGColor];
  }
  _gradientLayer.colors = colors;

  const CGSize paintBoxSize = paintBox.size;
  const CGSize clipSize = clipRect.size;
  CGSize gradientSize = [self computeBackgroundSizeWithImageSize:&clipSize
                                                 andPaintBoxSize:&paintBoxSize];
  CGFloat width = gradientSize.width;
  CGFloat height = gradientSize.height;

  CGFloat offsetX, offsetY;
  [self computeBackgroundPosition:&offsetX offsetY:&offsetY paintBox:paintBox size:gradientSize];

  [self onPrepareGradientWithSize:gradientSize];

  // Clip rect is the visible area size, use the clipRect size to make least replicates of gradient.
  [self handleBackgroundRepeat:borderBox
                          size:clipRect.size
                        height:height
                         width:width
                       offsetX:offsetX
                       offsetY:offsetY];
}
@end

@implementation LynxBackgroundRadialGradientDrawable
- (LynxBackgroundImageType)type {
  return LynxBackgroundImageRadialGradient;
}

- (void)onPrepareGradientWithSize:(CGSize)gradientSize {
  if (ABS(gradientSize.width) < FLT_EPSILON || ABS(gradientSize.height) < FLT_EPSILON) {
    return;
  }

  CGPoint center =
      [(LynxRadialGradient *)self.gradient calculateCenterWithWidth:gradientSize.width
                                                          andHeight:gradientSize.height];
  CGPoint radius =
      [(LynxRadialGradient *)self.gradient calculateRadiusWithCenter:&center
                                                               sizeX:gradientSize.width
                                                               sizeY:gradientSize.height];
  const int w = MAX(gradientSize.width, 1), h = MAX(gradientSize.height, 1);
  CGPoint startPoint = CGPointMake(center.x / w, center.y / h);
  CGPoint endPoint = CGPointMake((center.x + radius.x) / w, (center.y + radius.y) / h);
  self.gradientLayer.startPoint = startPoint;
  self.gradientLayer.endPoint = endPoint;
  self.gradientLayer.type = kCAGradientLayerRadial;
}

- (instancetype)initWithArray:(NSArray *)array {
  self = [super init];
  if (self) {
    if (array == nil) {
      LOGE("radial gradient native parse error, array is null");
    } else if ([array count] < 3) {
      LOGE("radial gradient native parse error, array must have 3 element");
    } else {
      self.gradient = [[LynxRadialGradient alloc] initWithArray:array];
    }
  }
  return self;
}
@end

@implementation LynxBackgroundNoneDrawable

- (void)onDraw:(CGContextRef)ctx rect:(CGRect)rect {
  // nothing to do here
}
@end
