// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRenderer.h>
#ifdef __cplusplus
#include "gfx/public/display_list.h"
#endif

@interface LynxRenderer ()

#ifdef __cplusplus
- (void)updateDisplayList:(lynx::tasm::DisplayList *)list;

- (lynx::tasm::DisplayList *)getDisplayList;
#endif

- (void)detachHostDecorationLayers;

- (void)reattachHostDecorationLayers;

- (void)syncHostDecorationLayers;

- (CGPoint)rendererLayoutOrigin;

- (void)updateRendererLayoutFrame:(CGRect)frame;

- (void)updateRendererLayoutSize:(CGSize)size;

- (void)updateRendererLayoutCenter:(CGPoint)center;

- (void)updateLayoutOffsetIfNeeded:(CGPoint)offset;

// SubtreeProperties support
#ifdef __cplusplus
- (void)applySubtreeProperties:(const lynx::tasm::SubtreeProperty *)properties count:(size_t)count;
#endif
- (void)applyTransform:(const float *)transform;
- (void)applyOpacity:(float)opacity;
- (void)applyFilter:(int32_t)type amount:(float)amount;

@end
