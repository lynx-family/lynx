// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxMarkdownLinkExposureUI.h"

@implementation LynxMarkdownLinkExposureUIV2 {
  NSString *_uniqueID;
  CGRect _rect;
  NSString *_url;
  NSString *_content;
}

- (instancetype)initWithRect:(CGRect)rect
                    uniqueID:(NSString *)uniqueID
                         url:(NSString *)url
                     content:(NSString *)content {
  self = [super initWithView:[UIView new]];
  if (self != nil) {
    _uniqueID = uniqueID;
    _rect = rect;
    _url = url;
    _content = content;
    self.frame = rect;
  }
  return self;
}

- (BOOL)isVisible {
  return true;
}

- (CGRect)getBoundingClientRectToScreen {
  CGRect rect = [[self getExposeReceiveTarget] getBoundingClientRectToScreen];
  rect.origin.x += _rect.origin.x;
  rect.origin.y += _rect.origin.y;
  rect.size = _rect.size;
  return rect;
}

- (NSString *)getUniqueID {
  return _uniqueID;
}

- (NSDictionary *)getData {
  NSMutableDictionary *dic = [NSMutableDictionary dictionary];
  [dic setValue:_url forKey:@"url"];
  [dic setValue:_content forKey:@"content"];

  return dic;
}

- (NSDictionary *)getOption {
  NSMutableDictionary *dic = [NSMutableDictionary dictionary];
  [dic setValue:@YES forKey:@"sendCustom"];
  [dic setValue:@YES forKey:@"specifyTarget"];
  [dic setValue:@"childrenexpose" forKey:@"bindEventName"];

  return dic;
}

- (LynxUI *)getExposeReceiveTarget {
  return [self parent];
}

- (BOOL)enableExposureUIMargin {
  return [[self getExposeReceiveTarget] enableExposureUIMargin];
}

- (CGFloat)exposureMarginTop {
  return [self getExposeReceiveTarget].exposureMarginTop;
}

- (CGFloat)exposureMarginRight {
  return [self getExposeReceiveTarget].exposureMarginRight;
}

- (CGFloat)exposureMarginBottom {
  return [self getExposeReceiveTarget].exposureMarginBottom;
}

- (CGFloat)exposureMarginLeft {
  return [self getExposeReceiveTarget].exposureMarginLeft;
}

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return [self getExposeReceiveTarget];
}

@end
