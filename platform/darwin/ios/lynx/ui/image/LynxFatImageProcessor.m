// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFatImageProcessor.h"
#import "LynxUI+Internal.h"
#import "LynxUIUnitUtils.h"

@implementation LynxFatImageProcessor {
  CGSize _size;
  UIEdgeInsets _padding;
  UIEdgeInsets _border;
  UIViewContentMode _contentMode;
}

- (instancetype)initWithSize:(CGSize)size
                     padding:(UIEdgeInsets)padding
                      border:(UIEdgeInsets)border
                 contentMode:(UIViewContentMode)contentMode {
  self = [super init];
  if (self) {
    _size = size;
    _padding = padding;
    _border = border;
    _contentMode = contentMode;
  }
  return self;
}

- (NSString*)cacheKey {
  return [NSString stringWithFormat:@"LynxFatImageProcessor_%@_%@_%@_%ld",
                                    NSStringFromCGSize(_size), NSStringFromUIEdgeInsets(_padding),
                                    NSStringFromUIEdgeInsets(_border), (long)_contentMode];
}

- (void)drawImage:(UIImage* _Nonnull)image toContext:(CGContextRef)ctx {
  CGFloat availableWidth =
      _size.width - _padding.left - _padding.right - _border.left - _border.right;
  CGFloat availableHeight =
      _size.height - _padding.top - _padding.bottom - _border.top - _border.bottom;
  CGRect contentRect = CGRectMake(_padding.left + _border.left, _padding.top + _border.top,
                                  availableWidth, availableHeight);
  CGContextClipToRect(ctx, contentRect);
  CGContextTranslateCTM(ctx, _padding.left + _border.left, _padding.top + _border.top);

  CGFloat offsetStartX = 0;
  CGFloat offsetStartY = 0;
  // source width and height
  CGFloat sourceWidth = image.size.width;
  CGFloat sourceHeight = image.size.height;
  CGRect imageRect = CGRectZero;
  // Scale image
  if (_contentMode == UIViewContentModeScaleToFill) {
    // Scale to fix the width and height
    imageRect = CGRectMake(0, 0, availableWidth, availableHeight);
  } else if (_contentMode == UIViewContentModeScaleAspectFit) {
    // Accroding to the width and height factor to shrink the image displaying center
    CGFloat w_rate = availableWidth / sourceWidth;
    CGFloat h_rate = availableHeight / sourceHeight;
    if (w_rate > h_rate) {
      CGFloat finalWidth = sourceWidth * h_rate;
      CGFloat startW = (availableWidth - finalWidth) / 2 + offsetStartX;
      CGFloat startH = offsetStartY;
      imageRect = CGRectMake(startW, startH, finalWidth, availableHeight);
    } else {
      CGFloat finalHeight = sourceHeight * w_rate;
      CGFloat startW = offsetStartX;
      CGFloat startH = (availableHeight - finalHeight) / 2 + offsetStartY;
      imageRect = CGRectMake(startW, -startH, availableWidth, finalHeight);
    }
  } else if (_contentMode == UIViewContentModeScaleAspectFill) {
    // Accroding to the width and height factor to enlarge the image displaying center
    float w_rate = availableWidth / sourceWidth;
    float h_rate = availableHeight / sourceHeight;
    if (w_rate > h_rate) {
      CGFloat finalHeight = sourceHeight * w_rate;
      CGFloat translateY = (availableHeight - finalHeight) / 2 + offsetStartY;
      imageRect = CGRectMake(offsetStartX, -translateY, availableWidth, finalHeight);
    } else {
      CGFloat finalWidth = sourceWidth * h_rate;
      CGFloat translateX = (availableWidth - finalWidth) / 2 + offsetStartX;
      imageRect = CGRectMake(translateX, offsetStartY, finalWidth, availableHeight);
    }
  } else {
    CGFloat translateY = (availableHeight - sourceHeight) / 2 + offsetStartY;
    CGFloat translateX = (availableWidth - sourceWidth) / 2 + offsetStartX;
    // Center bitmap in view, no scaling.
    imageRect = CGRectMake(translateX, -translateY, sourceWidth, sourceHeight);
  }
  // The direction of the Y axis is upward and the image is inverted drawing
  // so we should invert it here.
  CGContextTranslateCTM(ctx, 0, imageRect.size.height);
  CGContextScaleCTM(ctx, 1.0, -1.0);
  CGContextDrawImage(ctx, imageRect, image.CGImage);
}

- (UIImage*)processImage:(UIImage*)image {
  if (_size.width == 0 || _size.height == 0) {
    return image;
  }
  UIImage* filterImage = [LynxUI
      imageWithActionBlock:^(CGContextRef _Nonnull context) {
        [self drawImage:image toContext:context];
      }
                    opaque:NO
                     scale:[LynxUIUnitUtils screenScale]
                      size:_size];
  return filterImage;
}

@end
