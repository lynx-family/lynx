// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <QuartzCore/QuartzCore.h>

#include "base/include/vector.h"
#include "gfx/public/display_list_reader.h"
#include "gfx/public/rounded_rectangle.h"

@class LynxRendererContext;

// Keeps drawing tiles in host coordinates while host decorations follow the
// renderer's position, transform, opacity, and visibility.
@interface LynxSCGPaintGroup : CALayer
@end

@interface LynxDisplayListSurfacePainter : NSObject

- (instancetype)initWithContext:(LynxRendererContext *)context
                    insertLayer:(void (^)(CALayer *, BOOL hostDecoration))insertLayer
                     invalidate:(dispatch_block_t)invalidate;
- (void)beginFrameWithBounds:(CGRect)bounds scale:(CGFloat)scale;
- (BOOL)paintItem:(const lynx::tasm::DisplayListItem &)item
            reader:(const lynx::tasm::DisplayListReader &)reader
             boxes:(const lynx::base::Vector<lynx::tasm::RoundedRectangle> &)boxes
            offset:(CGPoint)offset
    hostDecoration:(BOOL)hostDecoration;
- (void)endGroup;
// Returns NO when the list exceeds the bounded surface allocation budget.
- (BOOL)endFrame;
- (void)invalidateImages;
- (void)reset;

@end
