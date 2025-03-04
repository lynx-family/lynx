// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestbenchDumpFileHelper.h"
#import <Lynx/LynxUIImage.h>
#import <objc/runtime.h>

#pragma mark - TestbenchDumpFileHelper
@implementation TestbenchDumpFileHelper

+ (NSString*)getViewTree:(nonnull UIView*)rootView {
  NSError* jsonError = nil;

  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:[self getViewTreeRecursive:rootView]
                                                     options:0
                                                       error:&jsonError];
  ;
  NSString* jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  return jsonString;
}

+ (NSMutableDictionary*)getViewTreeRecursive:(UIView*)view {
  NSMutableDictionary* node = [NSMutableDictionary dictionary];

  NSString* className = NSStringFromClass([view class]);
  [node setValue:className forKey:@"name"];

  [node setValue:@(view.frame.origin.x) forKey:@"left"];
  [node setValue:@(view.frame.origin.y) forKey:@"top"];
  [node setValue:@(view.frame.size.width) forKey:@"width"];
  [node setValue:@(view.frame.size.height) forKey:@"height"];

  NSArray* subviews = [view subviews];
  if ([subviews count] != 0) {
    NSMutableArray* childrenNodes = [NSMutableArray array];
    for (UIView* child in subviews) {
      [childrenNodes addObject:[self getViewTreeRecursive:child]];
    }
    [node setValue:childrenNodes forKey:@"children"];
  }

  return node;
}

+ (NSString*)getUITree:(nonnull LynxUI*)rootUI {
  NSError* jsonError = nil;

  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:[self getUITreeRecursive:rootUI]
                                                     options:0
                                                       error:&jsonError];
  ;
  NSString* jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  return jsonString;
}

+ (NSMutableDictionary*)getUITreeRecursive:(LynxUI*)ui {
  NSMutableDictionary* node = [NSMutableDictionary dictionary];

  NSString* className = NSStringFromClass([ui class]);
  [node setValue:className forKey:@"name"];

  [node setValue:@(ui.view.frame.origin.x) forKey:@"left"];
  [node setValue:@(ui.view.frame.origin.y) forKey:@"top"];
  [node setValue:@(ui.view.frame.size.width) forKey:@"width"];
  [node setValue:@(ui.view.frame.size.height) forKey:@"height"];

  [self getSpecificInfo:ui jsonNode:node];

  NSMutableArray* children = [ui children];
  if ([children count] != 0) {
    NSMutableArray* childrenNodes = [NSMutableArray array];
    for (LynxUI* child in children) {
      [childrenNodes addObject:[self getUITreeRecursive:child]];
    }
    [node setValue:childrenNodes forKey:@"children"];
  }

  return node;
}

+ (void)getSpecificInfo:(LynxUI*)ui jsonNode:node {
  NSString* className = NSStringFromClass([ui class]);
  if ([className isEqual:@"LynxUIImage"]) {
    LynxURL* src = [ui valueForKey:@"_src"];
    [node setValue:src.url.absoluteString forKey:@"src"];

  } else if ([className isEqual:@"LynxUIText"]) {
    NSInteger textAlignment =
        [[ui valueForKeyPath:@"_renderer._layoutSpec._textStyle._textAlignment"] integerValue];
    CGSize size = [[ui valueForKeyPath:@"_renderer._textSize"] CGSizeValue];

    [node setValue:@(textAlignment) forKey:@"textAlignment"];
    NSInteger textHeight = size.height * 1000;
    CGFloat roundTextHeight = textHeight * 1.0 / 1000;
    [node setValue:@(roundTextHeight) forKey:@"textSize"];
  }
}
@end
