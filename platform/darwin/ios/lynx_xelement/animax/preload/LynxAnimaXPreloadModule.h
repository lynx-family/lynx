// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

@class AnimaXPlayer;
@class LynxContext;

// Page-owned initialization, allocated only by prepare or an AnimaX component.
// Initialization never constructs resource services; loading requests do that.
@interface LynxAnimaXPreloadModule : NSObject

+ (instancetype)moduleForContext:(LynxContext *)context;
// Main only. Composition player state for the AnimaX component, which reaches
// these APIs only through this module. None of them creates a module or host.
+ (void)setCompositionMode:(BOOL)enabled forPlayer:(AnimaXPlayer *)player;
// The playercreated event payload, or nil when the player has no native player.
+ (NSDictionary *)compositionReceiverInfoForPlayer:(AnimaXPlayer *)player;
+ (void)cancelPendingCompositionBindingForPlayer:(AnimaXPlayer *)player;
// Completion may run on the calling thread, JS or Main. Concurrent callers
// share initialization; a ready runtime does not dispatch back through Main.
- (void)ensureReady:(void (^)(BOOL ready))callback;
// Main only, after ensureReady succeeds. Does not create resource services.
- (BOOL)attachCompositionPlayer:(AnimaXPlayer *)player;

@end
