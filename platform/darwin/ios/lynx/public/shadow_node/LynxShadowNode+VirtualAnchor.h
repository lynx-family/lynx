// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxShadowNode.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * Help to record ui hierarchy and determin the real offset in ui of child
 * shadow node.
 *
 * An virtual anchor that not virtual has a root ui container for the offspring
 * with ui.
 *
 * For example:
 *
 *   shadow node hierarchy:           ->                 lynx ui hierarchy:
 *
 *             TextShadowNode                                       LynxUIText
 *             (VirtualAnchor)                                          |
 *              /          \                                            |
 *  RawTextShadowNode  InlineTextShadowNode                         LynxUIImage
 *      (Virtual)    (Virtual & VirtualAnchor)
 *                              \
 *                          ImageShadowNode
 *
 */
@interface LynxShadowNode (VirtualAnchor)

// Record the count of all offspring which has ui (is not virtual).
@property(nonatomic) NSInteger totalCountOfChildrenThatHaveUI;

/**
 * Update totalCountOfChildrenThatHaveUI by checking the adjusting of ui
 * hierarchy while inserting a non-virtual child node or a virtual achor.
 */
- (void)adjustUIHierarchyWhileInsertNode:(LynxShadowNode*)child atIndex:(NSInteger)index;

/**
 * Update totalCountOfChildrenThatHaveUI by checking the adjusting of ui
 * hierarchy while removing a non-virtual child node or a virtual achor.
 */
- (void)adjustUIHierarchyWhileRemoveNode:(LynxShadowNode*)child atIndex:(NSInteger)index;

/**
 * Get the correct index in parent ui hierarchy for the child
 *
 * @return child index in parent ui
 */
- (NSInteger)mapToUIIndexWithChild:(LynxShadowNode*)child;

/**
 * Find the virtual anchor with ui
 *
 * @rturn node which isVirtualAnchor and isNonVirtual
 */
- (LynxShadowNode*)nodeThatHasUI;

@end

NS_ASSUME_NONNULL_END
