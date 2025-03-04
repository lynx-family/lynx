// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIOwner.h>
#include <objc/NSObjCRuntime.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUITreeHelper : NSObject

- (void)attachLynxUIOwner:(nullable LynxUIOwner *)uiOwner;

/*
 *find the minimum ui node which the point falls in
 *
 *Parameter:
 * x,y is coordinate to the screen of the point
 * uiSign is the id of the starting search node, lynxView or overlay view
 * thus,before calling view's hitTest function, We need to first convert the coordinates relative to
 *the screen into coordinates relative to the view
 *
 * Return Value:
 * the id of the found node , return 0 if not found
 */
- (int)findNodeIdForLocationWithX:(float)x withY:(float)y fromUI:(int)uiSign mode:(NSString *)mode;

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString *)mode;

- (CGPoint)convertPointFromScreen:(CGPoint)point ToView:(UIView *)view;

- (void)scrollIntoView:(int)node_id;

- (CGRect)getRectToWindow;

- (CGPoint)getViewLocationOnScreen;

/*
 * get LynxUI Tree rooted at LynxRootUI
 *
 * Parameter: none
 *
 * Return value:
 * the string converted from jsonObject of LynxUI Tree
 */
- (NSString *)getLynxUITree;

/*
 * get the information of the specified LynxUI node
 *
 * Parameter:
 * id : id of the LynxUI node you want to get information
 *
 * Return value:
 * the string converted from jsonObject of LynxUI information
 */
- (NSString *)getUINodeInfo:(int)id;

/*
 *set UI given style
 *
 *Parameter:
 * id: id of ui
 * name: style name which you wan't to set. only support
 *frame/border/margin/border-color/background-color/isVisible content: the style value you wan't to
 *set
 *
 *Return Value:
 * if name is supported editable style and content's format is right, then set style success and
 *return 0 otherwise, set style fail, and return -1
 */
- (int)setUIStyle:(int)id withStyleName:(NSString *)name withStyleContent:(NSString *)content;

/*
 *for frame/margin/border style
 *styleContent is 4 num string, such as "3,2, 3, 5", split num by "," and whitespace is allowed
 */
- (int)setFrame:(NSString *)styleContent ofUI:(LynxUI *)ui;
- (int)setMargin:(NSString *)styleContent ofUI:(LynxUI *)ui;
- (int)setBorder:(NSString *)styleContent ofUI:(LynxUI *)ui;

// for background-color/border-color style
// styleContent should be "#RRGGBBAA"
- (int)setBackgroundColor:(NSString *)styleContent ofUI:(LynxUI *)ui;
- (int)setBorderColor:(NSString *)styleContent ofUI:(LynxUI *)ui;

// for visibility style
// styleContent is "true"/"false"
- (int)setVisibility:(NSString *)styleContent ofUI:(LynxUI *)ui;

- (NSArray<NSNumber *> *)getTransformValue:(NSInteger)sign
                 withPadBorderMarginLayout:(NSArray<NSNumber *> *)arrayLayout;

@end

NS_ASSUME_NONNULL_END
