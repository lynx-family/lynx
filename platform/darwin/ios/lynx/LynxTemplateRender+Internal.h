// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRuntimeLifecycleListener.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxUIOwner.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxEvent;
@class LynxError;
@class LynxEngineProxy;
@class LynxGestureArenaManager;
@class LynxPerformanceController;
@class LynxShadowNodeOwner;

@protocol LynxUIRendererProtocol;

@interface LynxTemplateRender ()

/**
 * used by FrameView
 */
- (instancetype)initWithBuilderBlock:(void (^_Nullable)(NS_NOESCAPE LynxViewBuilder *_Nonnull))block
                       containerView:(UIView<LUIBodyView> *_Nullable)containerView;

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

- (LynxShadowNodeOwner *)shadowNodeOwner;

- (LynxUIOwner *)uiOwner;

- (LynxPerformanceController *)performanceController;

- (void)setAttachLynxPageUICallback:(attachLynxPageUI)callback;

/**
 * updateFrame for setUp and FrameView
 */
- (void)updateFrame:(CGRect)frame;

- (LynxViewBuilderBlock)getLynxViewBuilderBlock;

- (void)detachLynxEngine;

- (void)DispatchPlatformInputEvent:(NSArray *)iEventData withData:(NSArray *)fEventData;

- (int)GetPlatformEventHandlerState;

@end

NS_ASSUME_NONNULL_END
