// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxUICollection.h>
#import <XElement/LynxUIRefresh.h>
#import <XElement/LynxUIRefreshHeader.h>
#import <XElement/LynxUIRefreshShadowNode.h>

@implementation LynxUIRefreshShadowNode

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  if (self = [super initWithSign:sign tagName:tagName]) {
    // enable custom-layout by default
    self.hasCustomLayout = YES;
  }
  return self;
}

- (MeasureResult)customMeasureLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)context {
  NSArray *child = self.children;
  [child enumerateObjectsUsingBlock:^(LynxShadowNode *_Nonnull obj, NSUInteger idx,
                                      BOOL *_Nonnull stop) {
    BOOL supported = YES;

    Class clz = [LynxComponentRegistry uiClassWithName:obj.tagName accessible:&supported];

    if (supported && [obj isKindOfClass:[LynxNativeLayoutNode class]]) {
      LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)obj;

      MeasureParam *customizedParam;

      if ([self isRefreshHeader:clz]) {
        customizedParam = [[MeasureParam alloc] initWithWidth:param.width
                                                    WidthMode:param.widthMode
                                                       Height:param.height
                                                   HeightMode:LynxMeasureModeIndefinite];
      } else {
        customizedParam = [[MeasureParam alloc] initWithWidth:param.width
                                                    WidthMode:param.widthMode
                                                       Height:param.height
                                                   HeightMode:LynxMeasureModeDefinite];
      }
      [child measureWithMeasureParam:customizedParam MeasureContext:context];
    }
  }];
  return (MeasureResult){CGSizeMake(param.width, param.height)};
}

- (void)customAlignLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context {
  [self.children enumerateObjectsUsingBlock:^(LynxShadowNode *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    if ([obj isKindOfClass:[LynxNativeLayoutNode class]]) {
      LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)obj;
      AlignParam *param = [[AlignParam alloc] init];
      [param SetAlignOffsetWithLeft:0 Top:0];
      [child alignWithAlignParam:param AlignContext:context];
    }
  }];
}

- (BOOL)isRefreshHeader:(Class)clz {
  return [clz isEqual:([LynxUIRefreshHeader class])];
}

@end
