// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_SHADOW_NODE_TEXT_LYNXBASETEXTSHADOWNODE_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_SHADOW_NODE_TEXT_LYNXBASETEXTSHADOWNODE_H_

#import <Foundation/Foundation.h>
#import <Lynx/LynxConverter.h>
#import <Lynx/LynxShadowNode.h>
#import <Lynx/LynxTextStyle.h>

NS_ASSUME_NONNULL_BEGIN

extern NSAttributedStringKey const LynxInlineViewAttributedStringKey;
extern NSAttributedStringKey const LynxInlineTextShadowNodeSignKey;
extern NSAttributedStringKey const LynxUsedFontMetricKey;
extern NSAttributedStringKey const LynxVerticalAlignKey;

@interface LynxTextAttachment : NSTextAttachment
@property(readwrite, nonatomic, assign) NSInteger sign;
@property(readwrite, nonatomic, assign) LynxVerticalAlign verticalAlign;
@property(readwrite, nonatomic, assign) CGFloat verticalAlignLength;
@end

@interface LynxBaseTextShadowNode : LynxShadowNode

@property(nonatomic, strong) LynxTextStyle* textStyle;
@property(nonatomic, readonly) BOOL hasNonVirtualOffspring;
// TODO(zhixuan): Currently the flag below is always false, will link the flag with page config.
@property(nonatomic, readonly) BOOL enableTextRefactor;
@property(nonatomic, readonly) BOOL enableNewClipMode;
@property(nonatomic, readonly) BOOL enableTextLayoutCache;
@property(nonatomic, nullable) NSString* text;

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength __deprecated;

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength
                                  withDirection:(NSWritingDirection)direction;
- (void)alignHiddenNativeLayoutNode:(NSSet*)alignedNodeSignSet alignContext:(AlignContext*)ctx;

- (void)markStyleDirty;
@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_SHADOW_NODE_TEXT_LYNXBASETEXTSHADOWNODE_H_
