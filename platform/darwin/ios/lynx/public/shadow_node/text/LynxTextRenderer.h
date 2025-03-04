// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxEventTargetSpan.h"
#import "LynxTextLayoutSpec.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxTextAttachmentInfo : NSObject

- (instancetype)initWithSign:(NSInteger)sign andFrame:(CGRect)frame;

@property(nonatomic, readonly) NSInteger sign;
@property(nonatomic, readonly) CGRect frame;
@property(nonatomic, assign) BOOL nativeAttachment;

@end

@interface LynxTextRenderer : NSObject

- (instancetype)initWithAttributedString:(NSAttributedString *)attrStr
                              layoutSpec:(LynxLayoutSpec *)spec;

@property(nonatomic, readonly) NSAttributedString *attrStr;
@property(nonatomic, strong) NSAttributedString *truncationToken;
@property(nonatomic, readonly) LynxLayoutSpec *layoutSpec;

@property(nonatomic, readonly) NSLayoutManager *layoutManager;
@property(nonatomic, readonly) NSTextStorage *textStorage;

@property(nonatomic, readonly) NSArray<LynxEventTargetSpan *> *subSpan;
@property(nonatomic, strong, nullable) UIColor *selectionColor;

@property(nonatomic, strong, nullable) NSArray<LynxTextAttachmentInfo *> *attachments;
@property(nonatomic) CGFloat baseline;
// for bindlayout
@property(nonatomic) NSInteger ellipsisCount;

@property(nonatomic, copy) void (^layoutTruncationBlock)(NSMutableAttributedString *);

/**
 Returns the computed size of the renderer given the constrained size and other parameters.
 */
- (CGSize)size;
- (CGSize)textsize;
- (CGFloat)maxFontSize;
- (CGFloat)textContentOffsetX;

- (void)drawRect:(CGRect)bounds padding:(UIEdgeInsets)padding border:(UIEdgeInsets)border;
- (void)genSubSpan;

/// ensure LayoutManager layout text
- (void)ensureTextRenderLayout;

@end

NS_ASSUME_NONNULL_END
