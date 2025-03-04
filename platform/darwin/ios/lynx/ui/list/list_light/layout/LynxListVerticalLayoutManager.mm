// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListVerticalLayoutManager.h>

@implementation LynxListVerticalLayoutManager

- (CGSize)getContentSize {
  return CGSizeMake(self.bounds.size.width, [self largestMainSize]);
}

- (BOOL)isVerticalLayout {
  return YES;
}

- (NSInteger)nearestFullspanItem:(NSInteger)currentItem {
  __block NSInteger nearestItem = -1;
  [self.fullSpanItems
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (obj.integerValue < currentItem) {
          nearestItem = obj.integerValue;
        }
      }];
  return nearestItem;
}

- (CGFloat)layoutOffsetForFullSpanItem:(NSInteger)index {
  return [self layoutOffsetForFullSpanItems:CGRectGetWidth(self.models[index].frame) -
                                            self.insets.left - self.insets.right
                                  crossSize:CGRectGetWidth(self.bounds)
                               paddingStart:self.insets.left
                                 paddingEnd:self.insets.right];
}

- (BOOL)layoutModelVisibleInIndex:(NSInteger)index {
  LynxListLayoutModelLight *model = [self attributesFromIndex:index];
  CGFloat listVisibleStart = self.bounds.origin.y;
  CGFloat listVisibleEnd = self.bounds.origin.y + self.bounds.size.height;
  CGFloat modelStart = model.frame.origin.y;
  CGFloat modelEnd = model.frame.origin.y + model.frame.size.height;
  return ((modelStart <= listVisibleStart && modelEnd >= listVisibleStart) ||
          (modelStart <= listVisibleEnd && modelEnd >= listVisibleEnd) ||
          (modelStart >= listVisibleStart && modelEnd <= listVisibleEnd));
}

- (CGRect)generateModelFrame:(CGFloat)mainAxisOffset
             crossAxisOffset:(CGFloat)crossAxisOffset
                itemMainSize:(CGFloat)itemMainSize
               itemCrossSize:(CGFloat)itemCrossSize {
  return CGRectMake(crossAxisOffset, mainAxisOffset, itemCrossSize, itemMainSize);
}

- (CGFloat)crossSize:(BOOL)isFullSpan {
  CGFloat cellContentAreaSize = CGRectGetWidth(self.bounds) -
                                (self.insets.left + self.insets.right) -
                                (self.numberOfColumns - 1) * self.crossAxisGap;
  CGFloat normalItemCrossSize = floorf((cellContentAreaSize) / self.numberOfColumns);
  return isFullSpan ? CGRectGetWidth(self.bounds) : normalItemCrossSize;
}

- (CGFloat)mainSize:(NSInteger)index {
  return [self.models objectAtIndex:index].frame.size.height;
}

- (CGFloat)crossUpperInset {
  return self.insets.left;
}

- (CGRect)defaultModelFrame:(NSNumber *)index {
  if (self.estimatedHeights[index]) {
    return CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width,
                      self.estimatedHeights[index].floatValue);
  } else {
    return CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width,
                      [UIScreen mainScreen].bounds.size.height);
  }
}

@end
