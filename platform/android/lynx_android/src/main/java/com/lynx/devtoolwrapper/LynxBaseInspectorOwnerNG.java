// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.react.bridge.Callback;
import com.lynx.tasm.provider.LynxResourceCallback;

public interface LynxBaseInspectorOwnerNG extends LynxBaseInspectorOwner {
  void sendMessage(CustomizedMessage message);

  /**
   * Subscribes to a specific type of message with a given handler.
   *
   * This method allows you to subscribe to messages of a certain type, specifying a handler that
   * will be called when such messages are received. The handler will be referenced weakly to avoid
   * potential memory leaks, so the lifecycle of the handler must be managed by the subscriber. This
   * means the handler will only be called as long as it is still alive.
   *
   * <b>Note:</b> This is a breaking change introduced in version 3.0 where the lifecycle of the
   * handler is no longer managed internally.
   *
   * @param type The type of message to subscribe to. This parameter must not be null.
   * @param handler The handler that will process the messages. The handler will be referenced
   *     weakly.
   *
   * @since 3.0
   *
   * @example Example usage:
   * ```
   * ((LynxBaseInspectorOwnerNG) owner).subscribeMessage(messageType, new MessageHandler() {
   *     @Override
   *     public void handleMessage(Message message) {
   *         // Handle message
   *     }
   * });
   * ```
   */
  void subscribeMessage(String type, MessageHandler handler);
  void unsubscribeMessage(String type);
  void onRegisterModule(LynxModuleFactory moduleFactory);
  // download resource(for example, download "update.template.js" for HotModuleReplace)
  void downloadResource(String url, LynxResourceCallback<byte[]> callback);
}
