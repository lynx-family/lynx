// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxLayoutTick.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxUILayoutTick : LynxLayoutTick

- (instancetype)initWithRoot:(UIView<LUIBodyView>*)root block:(nonnull LynxOnLayoutBlock)block;

/**
 * attach view for request layout
 * @param root root view
 */
- (void)attach:(UIView<LUIBodyView>* _Nonnull)root;

@end

NS_ASSUME_NONNULL_END
