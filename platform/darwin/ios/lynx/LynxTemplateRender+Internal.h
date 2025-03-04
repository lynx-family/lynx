// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIOwner.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxEvent;
@class LynxError;
@class LynxEngineProxy;
@class LynxGestureArenaManager;

@protocol LynxUIRendererProtocol;

@interface LynxTemplateRender ()

- (NSInteger)logBoxImageSizeWarningThreshold;

- (void)didMoveToWindow:(BOOL)windowIsNil;
- (BOOL)enableNewListContainer;

- (void)runOnTasmThread:(dispatch_block_t)task;
- (BOOL)onLynxEvent:(LynxEvent *)event;
/**
 * Dispatch error to LynxTemplateRender
 */
- (void)onErrorOccurred:(LynxError *)error;
/// Generated in the LynxShell, id of template instance.
/// instanceId is a value greater than or equal to 0, the initial value is -1.
- (int32_t)instanceId;

- (LynxGestureArenaManager *)getGestureArenaManager;

- (LynxEngineProxy *)getEngineProxy;

- (void)markDirty;

- (id<LynxUIRendererProtocol>)lynxUIRenderer;

- (LynxUIOwner *)uiOwner;

@end

NS_ASSUME_NONNULL_END
