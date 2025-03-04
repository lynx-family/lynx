// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxUIMethodProcessor.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxCollectionInvalidationContext;
@class LynxCollectionViewLayout;

@interface LynxCollectionScroll : NSObject
@property(nonatomic) NSInteger initialScrollIndex;
@property(nonatomic) BOOL horizontalLayout;

- (instancetype)init;
- (void)scrollCollectionView:(UICollectionView *)collectionView
                    position:(NSInteger)position
                      offset:(CGFloat)offset
                     alignTo:(NSString *)alignTo
                      smooth:(BOOL)smooth
                 useScroller:(BOOL)useScroller
                    callback:(LynxUIMethodCallbackBlock)callback;
- (void)updateWithInvalidationContext:(LynxCollectionInvalidationContext *)context;
- (void)updateWithCellLoadedAtIndexPath:(NSIndexPath *)indexPath;
- (void)initialScrollCollectionViewIfNeeded:(LynxCollectionViewLayout *)layout;
- (void)updateLastIndexPathWithValidLayoutAttributes:(LynxCollectionInvalidationContext *)context;
- (void)collectionViewDidScroll:(UICollectionView *)collectionView;
- (void)collectionViewDidEndScrollingAnimation:(UICollectionView *)collectionView;
- (void)collectionViewWillBeginDragging:(UICollectionView *)collectionView;

@end

NS_ASSUME_NONNULL_END
