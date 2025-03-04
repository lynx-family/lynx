// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxUIMethodProcessor.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LynxCollectionScrollerHolderDelegate <NSObject>

- (void)removeScroller;

@end

@interface LynxCollectionScroller : NSObject

@property(nonatomic) BOOL horizontalLayout;

- (instancetype)initWithTargetIndexPath:(NSIndexPath *)targetIndexPath
                             scrollDown:(BOOL)willScrollDown
                  scrollToInvisibleRect:(BOOL)willScrollToInvisibleRect
                         scrollPosition:(UICollectionViewScrollPosition)scrollPosition
                                 offset:(CGFloat)offset
                                 sticky:(BOOL)sticky
                               delegate:(id<LynxCollectionScrollerHolderDelegate>)delegate
                             completion:(nonnull void (^)(BOOL success))completion;

- (void)collectionViewDidScroll:(UICollectionView *)collectionView;
- (void)collectionViewDidEndScrollingAnimation:(UICollectionView *)collectionView;
- (void)collectionViewWillBeginDragging:(UICollectionView *)collectionView;

- (void)collectionViewStartScroll:(UICollectionView *)collectionView animated:(BOOL)animated;
- (void)stopScroll;

@end

NS_ASSUME_NONNULL_END
