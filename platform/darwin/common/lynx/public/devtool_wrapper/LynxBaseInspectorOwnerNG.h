// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "CustomizedMessage.h"
#import "LynxBaseInspectorOwner.h"

@protocol MessageHandler <NSObject>

@required
- (void)onMessage:(nonnull NSString *)message;

@end

@protocol LynxBaseInspectorOwnerNG <LynxBaseInspectorOwner>

@required
- (void)sendMessage:(nonnull CustomizedMessage *)message;

/**
 * Subscribes to a specific type of message(eg: "CDP") with a given handler.
 *
 * This method allows you to subscribe to messages of a certain type, specifying a handler that will
 * be called when such messages are received. The handler will be referenced weakly to avoid
 * potential memory leaks, so the lifecycle of the handler must be managed by the subscriber. This
 * means the handler will only be called as long as it is still alive.
 *
 * <b>Note:</b> This is a breaking change introduced in version 3.0 where the lifecycle of the
 * handler is no longer managed internally.
 *
 * @param type The type of message to subscribe to. This parameter must not be nil.
 * @param handler The handler that will process the messages. The handler will be referenced weakly.
 *
 * @since 3.0
 *
 * @note Example usage:
 *
 * ```
 * [(id<LynxBaseInspectorOwnerNG>)owner subscribeMessage:messageType
 *                         withHandler:messageHandler];
 * ```
 */
- (void)subscribeMessage:(nonnull NSString *)type withHandler:(nonnull id<MessageHandler>)handler;
- (void)unsubscribeMessage:(nonnull NSString *)type;

- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(nullable NSString *)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;

@end
