// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxMarkdownUI.h>

#import <stdint.h>

#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <ServalMarkdown/ServalMarkdownConstants.h>

#import "LynxMarkdownShadowNode.h"
#import "adaptor/LynxMarkdownBundle.h"
#import "adaptor/LynxServalMarkdownViewWrapper.h"

@implementation LynxMarkdownUI {
  LynxMarkdownBundle *_bundle;
}

- (LynxMarkdownView *)createView {
  return [[LynxMarkdownView alloc] init];
}

- (void)didInsertChild:(LynxUI *)child atIndex:(NSInteger)index {
  [super didInsertChild:child atIndex:index];
  [child.view setHidden:YES];
  [child.backgroundManager setHidden:YES];
}

- (void)onReceiveUIOperation:(id)value {
  [super onReceiveUIOperation:value];
  if (value != nil && [value isKindOfClass:[LynxMarkdownBundle class]]) {
    _bundle = (LynxMarkdownBundle *)value;
    [self.view setBundle:_bundle];
  }
}

- (LynxServalMarkdownViewWrapper *)markdownView {
  return _bundle != nil ? _bundle.markdownView : nil;
}

- (ServalMarkdownIndexType)toIndexType:(id)typeValue {
  if ([typeValue isKindOfClass:[NSString class]] &&
      [(NSString *)typeValue isEqualToString:@"source"]) {
    return kServalMarkdownIndexTypeSource;
  }
  return kServalMarkdownIndexTypeChar;
}

- (ServalMarkdownCharRangeType)toSelectionRangeType:(id)typeValue {
  if (![typeValue isKindOfClass:[NSString class]]) {
    return kServalMarkdownCharRangeTypeChar;
  }
  NSString *type = (NSString *)typeValue;
  if ([type isEqualToString:@"word"]) {
    return kServalMarkdownCharRangeTypeWord;
  }
  if ([type isEqualToString:@"sentence"]) {
    return kServalMarkdownCharRangeTypeSentence;
  }
  if ([type isEqualToString:@"paragraph"]) {
    return kServalMarkdownCharRangeTypeParagraph;
  }
  return kServalMarkdownCharRangeTypeChar;
}

- (NSArray<NSNumber *> *)getSelectionRange:(LynxServalMarkdownViewWrapper *)markdown
                                    StartX:(CGFloat)startX
                                    StartY:(CGFloat)startY
                                      EndX:(CGFloat)endX
                                      EndY:(CGFloat)endY
                             SelectionType:(ServalMarkdownCharRangeType)selectionType {
  NSInteger start = -1;
  NSInteger end = -1;
  if (selectionType == kServalMarkdownCharRangeTypeChar) {
    start = [markdown getCharIndexByPoint:startX y:startY indexType:kServalMarkdownIndexTypeChar];
    end = [markdown getCharIndexByPoint:endX y:endY indexType:kServalMarkdownIndexTypeChar];
  } else {
    NSRange startRange = [markdown getCharRangeByPoint:startX
                                                     y:startY
                                             indexType:kServalMarkdownIndexTypeChar
                                             rangeType:selectionType];
    NSRange endRange = [markdown getCharRangeByPoint:endX
                                                   y:endY
                                           indexType:kServalMarkdownIndexTypeChar
                                           rangeType:selectionType];
    if (startRange.location == NSNotFound || endRange.location == NSNotFound) {
      return nil;
    }
    start = (NSInteger)startRange.location;
    end = (NSInteger)(endRange.location + endRange.length);
  }
  if (start < 0 || end < 0) {
    return nil;
  }
  if (start > end) {
    NSInteger tmp = start;
    start = end;
    end = tmp;
  }
  return @[ @(start), @(end) ];
}

LYNX_UI_METHOD(getContent) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"no ui for node");
    return;
  }
  BOOL hasRange = params != nil &&
                  ([params objectForKey:@"start"] != nil || [params objectForKey:@"end"] != nil);
  NSString *content = @"";
  if (hasRange) {
    NSInteger start = 0;
    NSInteger end = INT32_MAX;
    if ([params[@"start"] isKindOfClass:[NSNumber class]]) {
      start = [params[@"start"] integerValue];
    }
    if ([params[@"end"] isKindOfClass:[NSNumber class]]) {
      end = [params[@"end"] integerValue];
    }
    if (start >= end) {
      callback(kUIMethodParamInvalid, @"start >= end");
      return;
    }
    content = [markdown getContent:(int)start
                               end:(int)end
                         indexType:[self toIndexType:params[@"indexType"]]];
  } else {
    content = [markdown getContent];
  }
  callback(kUIMethodSuccess, @{@"content" : content ?: @""});
}

LYNX_UI_METHOD(pauseAnimation) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"animation not start");
    return;
  }
  // TODO: send to layout thread
  [markdown pauseAnimation];
  callback(
      kUIMethodSuccess,
      @{@"animationStep" : @([markdown getAnimationStep])});
}

LYNX_UI_METHOD(resumeAnimation) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"animation not start");
    return;
  }
  // TODO: send to layout thread
  NSInteger animationStep = -1;
  if ([params[@"animationStep"] isKindOfClass:[NSNumber class]]) {
    animationStep = [params[@"animationStep"] integerValue];
  }
  if (animationStep >= 0) {
    [markdown resumeAnimation:(int)animationStep];
  } else {
    [markdown resumeAnimation];
  }
  callback(kUIMethodSuccess, nil);
}

LYNX_UI_METHOD(getTextBoundingRect) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"no ui for node");
    return;
  }
  NSNumber *start = [params objectForKey:@"start"];
  NSNumber *end = [params objectForKey:@"end"];
  if (start == nil || end == nil || start.integerValue < 0 || end.integerValue < 0 ||
      start.integerValue > end.integerValue) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }
  NSArray<NSValue *> *boxes =
      [markdown getTextBoundingRect:start.intValue
                                end:end.intValue
                          indexType:[self toIndexType:params[@"indexType"]]];
  if (boxes.count == 0) {
    callback(kUIMethodUnknown, @"Can not find text bounding rect.");
    return;
  }
  CGRect rect = [self getRelativeBoundingClientRect:params];
  NSDictionary *result = [self getTextBoundingRectFromBoxes:boxes textRect:rect];
  callback(kUIMethodSuccess, result);
}

LYNX_UI_METHOD(getCharIndexByPoint) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"no ui for node");
    return;
  }
  NSNumber *x = params[@"x"];
  NSNumber *y = params[@"y"];
  if (![x isKindOfClass:[NSNumber class]] || ![y isKindOfClass:[NSNumber class]]) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }

  CGFloat translateX = self.padding.left + self.border.left;
  CGFloat translateY = self.padding.top + self.border.top;
  NSInteger index = [markdown getCharIndexByPoint:(float)(x.doubleValue - translateX)
                                                y:(float)(y.doubleValue - translateY)
                                        indexType:[self toIndexType:params[@"indexType"]]];
  if (index < 0) {
    callback(kUIMethodUnknown, @"can not find char index");
    return;
  }
  callback(
      kUIMethodSuccess,
      @{@"index" : @(index)});
}

LYNX_UI_METHOD(setTextSelection) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"no ui for node");
    return;
  }
  NSNumber *startX = [params objectForKey:@"startX"];
  NSNumber *startY = [params objectForKey:@"startY"];
  NSNumber *endX = [params objectForKey:@"endX"];
  NSNumber *endY = [params objectForKey:@"endY"];
  if (![startX isKindOfClass:[NSNumber class]] || ![startY isKindOfClass:[NSNumber class]] ||
      ![endX isKindOfClass:[NSNumber class]] || ![endY isKindOfClass:[NSNumber class]]) {
    callback(kUIMethodParamInvalid, @"parameter is invalid");
    return;
  }

  CGFloat translateX = self.padding.left + self.border.left;
  CGFloat translateY = self.padding.top + self.border.top;
  NSArray<NSNumber *> *range =
      [self getSelectionRange:markdown
                       StartX:startX.doubleValue - translateX
                       StartY:startY.doubleValue - translateY
                         EndX:endX.doubleValue - translateX
                         EndY:endY.doubleValue - translateY
                SelectionType:[self toSelectionRangeType:params[@"selectionTextType"]]];
  if (range == nil || range.count < 2) {
    callback(kUIMethodUnknown, @"Can not set text selection.");
    return;
  }

  [markdown setTextSelection:range[0].intValue end:range[1].intValue];
  NSArray<NSValue *> *boxes = [markdown getSelectedLineBoundingRect];
  if (boxes.count == 0) {
    callback(kUIMethodSuccess, @{});
    return;
  }
  CGRect rect = [self getRelativeBoundingClientRect:params];
  NSMutableDictionary *result = [[self getTextBoundingRectFromBoxes:boxes
                                                           textRect:rect] mutableCopy];

  CGPoint handle = [markdown getSelectionHandlePosition];
  if (handle.x >= 0 && handle.y >= 0) {
    NSDictionary *handleMap = [self getHandleMap:handle.x
                                               Y:handle.y
                                          Radius:[markdown getSelectionHandleRadius]
                                        TextRect:rect];
    result[@"handles"] = @[ handleMap ];
  }
  callback(kUIMethodSuccess, [result copy]);
}

LYNX_UI_METHOD(getSelectedText) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"no ui for node");
    return;
  }
  callback(kUIMethodSuccess, @{@"selectedText" : [markdown getSelectedText] ?: @""});
}

LYNX_UI_METHOD(getParseResult) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"parse not finished");
    return;
  }
  NSArray *tags = params[@"tags"];
  if (![tags isKindOfClass:[NSArray class]] || tags.count == 0) {
    callback(kUIMethodParamInvalid, @"param invalid: no tags");
    return;
  }

  NSMutableDictionary *result = [NSMutableDictionary dictionary];
  for (id item in tags) {
    if (![item isKindOfClass:[NSString class]]) {
      continue;
    }
    NSString *tag = (NSString *)item;
    NSArray<NSValue *> *array = [markdown getSyntaxSourceRanges:tag];
    NSMutableArray *resultArray = [NSMutableArray array];
    for (NSValue *value in array) {
      NSRange range = value.rangeValue;
      if (range.location == NSNotFound) {
        continue;
      }
      [resultArray addObject:@{
        @"start" : @((NSInteger)range.location),
        @"end" : @((NSInteger)(range.location + range.length)),
      }];
    }
    result[tag] = resultArray;
  }

  NSString *contentID = [markdown getContentID];
  if (contentID.length == 0) {
    LynxShadowNode *node = [self.context.nodeOwner nodeWithSign:self.sign];
    if ([node isKindOfClass:[LynxMarkdownShadowNode class]]) {
      contentID = [(LynxMarkdownShadowNode *)node currentContentID];
    }
  }

  callback(kUIMethodSuccess, @{
    @"id" : contentID ?: @"",
    @"result" : [result copy],
  });
}

LYNX_UI_METHOD(getImages) {
  LynxServalMarkdownViewWrapper *markdown = [self markdownView];
  if (markdown == nil) {
    callback(kUIMethodUnknown, @"parse not finished");
    return;
  }
  callback(
      kUIMethodSuccess,
      @{@"images" : [markdown getAllImageUrl] ?: @[]});
}

- (NSDictionary *)getHandleMap:(CGFloat)x
                             Y:(CGFloat)y
                        Radius:(CGFloat)radius
                      TextRect:(CGRect)rect {
  return @{
    @"x" : @(rect.origin.x + x + self.padding.left + self.border.left),
    @"y" : @(rect.origin.y + y + self.padding.top + self.border.top),
    @"radius" : @(radius),
  };
}

- (NSMutableDictionary *)getTextBoundingRectFromBoxes:(NSArray<NSValue *> *)boxes
                                             textRect:(CGRect)textRect {
  NSMutableDictionary *result = [NSMutableDictionary dictionary];
  if (boxes.count == 0) {
    return result;
  }
  CGRect boundingRect = boxes.firstObject.CGRectValue;
  for (NSValue *value in boxes) {
    boundingRect = CGRectUnion(boundingRect, value.CGRectValue);
  }
  result[@"boundingRect"] = [self getMapFromRect:textRect lineBox:boundingRect];

  NSMutableArray *boxList = [NSMutableArray array];
  for (NSValue *value in boxes) {
    [boxList addObject:[self getMapFromRect:textRect lineBox:value.CGRectValue]];
  }
  result[@"boxes"] = boxList;
  return result;
}

- (NSDictionary *)getMapFromRect:(CGRect)textRect lineBox:(CGRect)lineBox {
  return @{
    @"left" : @(textRect.origin.x + CGRectGetMinX(lineBox) + self.padding.left + self.border.left),
    @"top" : @(textRect.origin.y + CGRectGetMinY(lineBox) + self.padding.top + self.border.top),
    @"right" : @(textRect.origin.x + CGRectGetMaxX(lineBox) + self.padding.left + self.border.left),
    @"bottom" : @(textRect.origin.y + CGRectGetMaxY(lineBox) + self.padding.top + self.border.top),
    @"width" : @(CGRectGetWidth(lineBox)),
    @"height" : @(CGRectGetHeight(lineBox)),
  };
}

@end
