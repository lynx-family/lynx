// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIText;
@class LynxTextRenderer;

@interface LynxTextView : UIView

@property(nonatomic, strong) CALayer *contentLayer;
@property(nonatomic, weak) LynxUIText *ui;
@property(nonatomic, weak) LynxTextRenderer *textRenderer;
@property(nonatomic, assign) UIEdgeInsets border;
@property(nonatomic, assign) UIEdgeInsets padding;
@property(nonatomic, assign) BOOL enableTextSelection;
@property(nonatomic, assign) BOOL enableCustomContextMenu;
@property(nonatomic, assign) BOOL enableCustomTextSelection;
@property(nonatomic, copy) void (^selectionChangeEventCallback)(NSDictionary *);

- (void)updateSelectionColor:(UIColor *)color;
- (void)updateHandleColor:(UIColor *)color;
- (void)updateHandleSize:(CGFloat)size;

- (void)setOverflowOffset:(CGPoint)overflowOffset;

- (void)initSelectionGesture;

- (NSArray *)getTextBoundingBoxes:(NSInteger)start withEnd:(NSInteger)end;
- (NSString *)getSelectedText;
- (NSArray *)setTextSelection:(CGFloat)startX
                       startY:(CGFloat)startY
                         endX:(CGFloat)endX
                         endY:(CGFloat)endY;
- (NSArray *)getHandlesInfo;

@end

NS_ASSUME_NONNULL_END
