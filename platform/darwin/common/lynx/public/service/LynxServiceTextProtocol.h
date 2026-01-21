// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxUIOwner.h>
#import <LynxServiceAPI/ServiceAPI.h>
NS_ASSUME_NONNULL_BEGIN

@protocol LynxServiceTextProtocol <LynxServiceProtocol>
/**
 * create cpp TextLayoutAPI object
 * @param context resource context for TextLayoutAPI
 * @return TextLayoutAPI object pointer
 */
- (void *)createTextLayoutAPIFromContext:(LynxUIOwner *)context;
/**
 * destroy TextLayoutAPI object
 * @param api TextLayoutAPI object
 */
- (void)destroyTextLayoutAPI:(void *)api;
/**
 * draw page on a core graphics context
 * @param page cpp Page object pointer
 * @param context core graphics context to draw
 */
- (void)drawPage:(void *)page OnContext:(CGContextRef)context;
/**
 * get char index by a position relative to page origin
 * @param page cpp Page object pointer
 * @param point position relative to page origin
 */
- (int)getSelectionCharIndexOfPage:(void *)page ByTouchPosition:(CGPoint)point;
/**
 * get line rects by char range
 * @param page cpp Page object pointer
 * @param range char range on page
 * @return rect array for each line, filled by CGRect
 */
- (NSArray *)getSelectionRectsOfPage:(void *)page ByCharRange:(NSRange)range;
@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_
