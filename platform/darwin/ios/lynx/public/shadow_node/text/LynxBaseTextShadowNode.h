// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxConverter.h"
#import "LynxShadowNode.h"
#import "LynxTextStyle.h"

NS_ASSUME_NONNULL_BEGIN

extern NSAttributedStringKey const LynxInlineViewAttributedStringKey;
extern NSAttributedStringKey const LynxInlineTextShadowNodeSignKey;
extern NSAttributedStringKey const LynxUsedFontMetricKey;
extern NSAttributedStringKey const LynxVerticalAlignKey;

@interface LynxBaseTextShadowNode : LynxShadowNode

@property(nonatomic, strong) LynxTextStyle* textStyle;
@property(nonatomic, readonly) BOOL hasNonVirtualOffspring;
// TODO(zhixuan): Currently the flag below is always false, will link the flag with page config.
@property(nonatomic, readonly) BOOL enableTextRefactor;
@property(nonatomic, readonly) BOOL enableNewClipMode;
@property(nonatomic, nullable) NSString* text;

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength __deprecated;

- (NSAttributedString*)generateAttributedString:
                           (nullable NSDictionary<NSAttributedStringKey, id>*)baseTextAttribute
                              withTextMaxLength:(NSInteger)textMaxLength
                                  withDirection:(NSWritingDirection)direction;

- (void)markStyleDirty;
@end

NS_ASSUME_NONNULL_END
