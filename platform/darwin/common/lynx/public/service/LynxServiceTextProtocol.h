// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <LynxServiceAPI/ServiceAPI.h>
NS_ASSUME_NONNULL_BEGIN

@class LynxUIOwner;
@class LynxTextInfo;

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
 * destroy page object
 * @param page cpp Page object pointer
 */
- (void)destroyPage:(void *)page;
/**
 * draw page on a core graphics context
 * @param page cpp Page object pointer
 * @param context core graphics context to draw
 */
- (void)drawPage:(void *)page OnContext:(CGContextRef)context;
/**
 * get text layout info
 * @param text text content
 * @param fontSize font size with unit
 * @param fontFamily font family
 * @param maxWidth max measure width with unit
 * @param maxLine max measure line count
 * @return platform text info object
 */
- (nullable LynxTextInfo *)getTextInfo:(NSString *)text
                              fontSize:(NSString *)fontSize
                            fontFamily:(nullable NSString *)fontFamily
                              maxWidth:(nullable NSString *)maxWidth
                               maxLine:(NSInteger)maxLine;
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

@interface LynxTextInfo : NSObject

@property(nonatomic, readonly) CGFloat width;
@property(nonatomic, readonly) CGFloat height;
@property(nonatomic, copy, readonly, nullable) NSArray<NSString *> *content;

- (instancetype)initWithWidth:(CGFloat)width;
- (instancetype)initWithWidth:(CGFloat)width content:(nullable NSArray<NSString *> *)content;
- (instancetype)initWithWidth:(CGFloat)width height:(CGFloat)height;
- (instancetype)initWithWidth:(CGFloat)width
                       height:(CGFloat)height
                      content:(nullable NSArray<NSString *> *)content;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICETEXTPROTOCOL_H_
