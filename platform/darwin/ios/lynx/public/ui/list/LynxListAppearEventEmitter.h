// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventEmitter.h"
#import "LynxUICollection+Internal.h"
#import "LynxUIComponent.h"

@class LynxUICollection;

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString *const kLynxListNodeAppearEvent;
FOUNDATION_EXPORT NSString *const kLynxListNodeDisappearEvent;

/**
 * This EventEmitter using the `itemKey` of `LynxUIComponent` to avoid duplicated appear / disappear
 * events.
 */
@interface LynxListAppearEventEmitter : NSObject

- (instancetype)initWithEmitter:(LynxEventEmitter *)emitter;

/**
 * send the `nodeappear` event when a LynxUIComponent is appearing.
 * You call this method in the UIScrollView, UICollectionView delegate methods, for example,
 * `collectionView:willDisplayCell:atIndexPath:`
 * @param ui the LynxUIComponent that is appearing
 * @param indexPath the indexPath for the LynxUIComponent
 */
- (void)onCellAppear:(LynxUIComponent *)ui atIndexPath:(NSIndexPath *)indexPath;

/**
 * send the `nodedisappear` event when a LynxUIComponent is disappearing.
 * You call this method in the UIScrollView, UICollectionView delegate methods, for example,
 * `collectionView:didEndDisplayCell:atIndexPath:`
 * @param ui the LynxUIComponent that is disappearing
 * @param indexPath the indexPath for the LynxUIComponent
 */
- (void)onCellDisappear:(LynxUIComponent *)ui atIndexPath:(NSIndexPath *)indexPath;

/**
 * @abstract invalidate the LynxListAppearEventEmitter
 * You call this method when the view is moving to background or removed from its superview.
 */
- (void)invalidate;

- (void)setListUI:(LynxUICollection *)ui;
@end

NS_ASSUME_NONNULL_END
