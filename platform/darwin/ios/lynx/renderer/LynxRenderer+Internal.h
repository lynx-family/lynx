// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRenderer.h>
#include "core/renderer/ui_wrapper/painting/platform_renderer.h"
#include "gfx/public/display_list.h"

@interface LynxRenderer ()

- (void)setPlatformRenderer:(const lynx::fml::WeakPtr<lynx::tasm::PlatformRenderer> &)renderer;

- (void)updateDisplayList:(lynx::tasm::DisplayList *)list;

- (lynx::tasm::DisplayList *)getDisplayList;

- (void)publishLayoutPosition:(CGPoint)position;
- (void)publishScrollOffset:(CGPoint)offset;
- (void)publishStickyOffset:(CGPoint)offset;
- (void)publishIsOnTree:(BOOL)isOnTree;
- (void)publishSiblingOrder:(int32_t)order;

- (void)detachHostDecorationLayers;

- (void)reattachHostDecorationLayers;

- (void)syncHostDecorationLayers;

- (CGPoint)rendererLayoutOrigin;

- (void)updateRendererLayoutFrame:(CGRect)frame;

- (void)updateRendererLayoutSize:(CGSize)size;

- (void)updateRendererLayoutCenter:(CGPoint)center;

- (void)updateLayoutOffsetIfNeeded:(CGPoint)offset;

// SubtreeProperties support
- (void)applySubtreeProperties:(const lynx::tasm::SubtreeProperty *)properties count:(size_t)count;
- (void)applyTransform:(const float *)transform;
- (void)applyOpacity:(float)opacity;
- (void)applyFilter:(int32_t)type amount:(float)amount;

@end
