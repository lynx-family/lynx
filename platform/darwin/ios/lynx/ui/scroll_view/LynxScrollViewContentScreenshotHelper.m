// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollViewContentScreenshotHelper.h"

#import <Lynx/LynxUI.h>
#import <math.h>

static NSArray<NSNumber *> *LynxBuildContentScreenshotOffsets(CGFloat maxOffset,
                                                              CGFloat viewportSize) {
  NSMutableArray<NSNumber *> *offsets = [NSMutableArray arrayWithObject:@0];
  if (maxOffset <= 0 || viewportSize <= 0) {
    return offsets;
  }
  CGFloat offset = 0;
  while (offset < maxOffset) {
    offset = MIN(offset + viewportSize, maxOffset);
    if (fabs(offset - offsets.lastObject.doubleValue) > 0.5) {
      [offsets addObject:@(offset)];
    }
  }
  return offsets;
}

static void LynxDrawContentScreenshotChildren(UIScrollView *scrollView, NSArray<LynxUI *> *children,
                                              CGSize contentSize) {
  UIColor *backgroundColor = scrollView.backgroundColor ?: UIColor.clearColor;
  [backgroundColor setFill];
  UIRectFill(CGRectMake(0, 0, contentSize.width, contentSize.height));

  CGContextRef context = UIGraphicsGetCurrentContext();
  for (LynxUI *child in children) {
    UIView *childView = child.view;
    if (!childView || childView.hidden || childView.alpha <= 0) {
      continue;
    }
    CGRect frame = childView.frame;
    if (CGRectIsEmpty(frame)) {
      continue;
    }

    [childView setNeedsLayout];
    [childView layoutIfNeeded];

    CGContextSaveGState(context);
    CGContextClipToRect(context, CGRectMake(0, 0, contentSize.width, contentSize.height));
    CGContextTranslateCTM(context, frame.origin.x, frame.origin.y);
    BOOL drawn =
        [childView drawViewHierarchyInRect:CGRectMake(0, 0, frame.size.width, frame.size.height)
                        afterScreenUpdates:NO];
    if (!drawn) {
      [childView.layer renderInContext:context];
    }
    CGContextRestoreGState(context);
  }
}

@implementation LynxScrollViewContentScreenshotHelper

+ (void)takeContentScreenshotForScrollView:(UIScrollView *)scrollView
                                  children:(NSArray<LynxUI *> *)children
                                    params:(NSDictionary *)params
                                  callback:(LynxUIMethodCallbackBlock)callback {
  if (!scrollView || scrollView.bounds.size.width <= 0 || scrollView.bounds.size.height <= 0) {
    return callback(kUIMethodNoUiForNode, @{});
  }

  BOOL usePng = [[params objectForKey:@"format"] isEqualToString:@"png"];
  CGFloat scale = 1.f;
  if ([[params allKeys] containsObject:@"scale"]) {
    scale = ((NSNumber *)[params objectForKey:@"scale"]).floatValue;
  }
  if (scale <= 0) {
    return callback(kUIMethodParamInvalid, @"scale must be greater than 0");
  }

  CGSize viewportSize = scrollView.bounds.size;
  CGSize contentSize = CGSizeMake(MAX(scrollView.contentSize.width, viewportSize.width),
                                  MAX(scrollView.contentSize.height, viewportSize.height));
  if (contentSize.width <= 0 || contentSize.height <= 0) {
    return callback(kUIMethodOperationError, @"content size is invalid");
  }

  CGPoint originalOffset = scrollView.contentOffset;
  UIGraphicsBeginImageContextWithOptions(contentSize, NO, [UIScreen mainScreen].scale * scale);
  @try {
    if (children.count > 0) {
      LynxDrawContentScreenshotChildren(scrollView, children, contentSize);
    } else {
      NSArray<NSNumber *> *yOffsets = LynxBuildContentScreenshotOffsets(
          contentSize.height - viewportSize.height, viewportSize.height);
      NSArray<NSNumber *> *xOffsets = LynxBuildContentScreenshotOffsets(
          contentSize.width - viewportSize.width, viewportSize.width);
      for (NSNumber *yValue in yOffsets) {
        for (NSNumber *xValue in xOffsets) {
          CGPoint offset = CGPointMake(xValue.doubleValue, yValue.doubleValue);
          scrollView.contentOffset = offset;
          [scrollView layoutIfNeeded];
          BOOL drawn =
              [scrollView drawViewHierarchyInRect:CGRectMake(offset.x, offset.y, viewportSize.width,
                                                             viewportSize.height)
                               afterScreenUpdates:NO];
          if (!drawn) {
            CGContextRef context = UIGraphicsGetCurrentContext();
            CGContextSaveGState(context);
            CGContextTranslateCTM(context, offset.x, offset.y);
            [scrollView.layer renderInContext:context];
            CGContextRestoreGState(context);
          }
        }
      }
    }
  } @finally {
    scrollView.contentOffset = originalOffset;
  }

  UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
  UIGraphicsEndImageContext();
  if (!image) {
    return callback(kUIMethodUnknown, @{});
  }

  NSData *data = usePng ? UIImagePNGRepresentation(image) : UIImageJPEGRepresentation(image, 1.0);
  if (!data) {
    return callback(kUIMethodOperationError, @"image encode failed");
  }
  NSString *base64 = [data base64EncodedStringWithOptions:0];
  if (!base64) {
    return callback(kUIMethodOperationError, @"image base64 encode failed");
  }
  NSString *header = usePng ? @"data:image/png;base64," : @"data:image/jpeg;base64,";
  NSString *str = [header stringByAppendingString:base64];
  callback(kUIMethodSuccess, @{
    @"width" : @(image.size.width * image.scale),
    @"height" : @(image.size.height * image.scale),
    @"data" : str
  });
}

@end
