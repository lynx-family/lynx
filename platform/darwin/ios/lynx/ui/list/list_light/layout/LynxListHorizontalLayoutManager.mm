// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxListHorizontalLayoutManager.h>

@implementation LynxListHorizontalLayoutManager

- (CGSize)getContentSize {
  return CGSizeMake([self largestMainSize], self.bounds.size.height);
}

- (BOOL)isVerticalLayout {
  [super isVerticalLayout];
  return NO;
}

- (CGFloat)layoutOffsetForFullSpanItem:(NSInteger)index {
  return [self layoutOffsetForFullSpanItems:CGRectGetHeight(self.models[index].frame) -
                                            self.insets.top - self.insets.bottom
                                  crossSize:CGRectGetHeight(self.bounds)
                               paddingStart:self.insets.top
                                 paddingEnd:self.insets.bottom];
}

- (BOOL)layoutModelVisibleInIndex:(NSInteger)index {
  LynxListLayoutModelLight *model = [self attributesFromIndex:index];
  CGFloat listVisibleStart = self.bounds.origin.x;
  CGFloat listVisibleEnd = self.bounds.origin.x + self.bounds.size.width;
  CGFloat modelStart = model.frame.origin.x;
  CGFloat modelEnd = model.frame.origin.x + model.frame.size.width;
  return ((modelStart <= listVisibleStart && modelEnd >= listVisibleStart) ||
          (modelStart <= listVisibleEnd && modelEnd >= listVisibleEnd) ||
          (modelStart >= listVisibleStart && modelEnd <= listVisibleEnd));
}

- (CGRect)generateModelFrame:(CGFloat)mainAxisOffset
             crossAxisOffset:(CGFloat)crossAxisOffset
                itemMainSize:(CGFloat)itemMainSize
               itemCrossSize:(CGFloat)itemCrossSize {
  return CGRectMake(mainAxisOffset, crossAxisOffset, itemMainSize, itemCrossSize);
}

- (CGFloat)crossSize:(BOOL)isFullSpan {
  CGFloat cellContentAreaSize = CGRectGetHeight(self.bounds) -
                                (self.insets.top + self.insets.bottom) -
                                (self.numberOfColumns - 1) * self.crossAxisGap;
  CGFloat normalItemCrossSize = floorf((cellContentAreaSize) / self.numberOfColumns);
  return isFullSpan ? CGRectGetHeight(self.bounds) : normalItemCrossSize;
}

- (CGFloat)mainSize:(NSInteger)index {
  return [self.models objectAtIndex:index].frame.size.width;
}

- (CGFloat)crossUpperInset {
  return self.insets.top;
}

- (CGRect)defaultModelFrame:(NSNumber *)index {
  if (self.estimatedHeights[index]) {
    return CGRectMake(0, 0, self.estimatedHeights[index].floatValue,
                      [UIScreen mainScreen].bounds.size.height);
  } else {
    return CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width,
                      [UIScreen mainScreen].bounds.size.height);
  }
}

@end
