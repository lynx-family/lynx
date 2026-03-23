// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import androidx.annotation.Nullable;
import com.lynx.debugrouter.app.MessageHandleResult;
import com.lynx.debugrouter.app.MessageHandler;
import java.util.Map;

public class ExplorerAppMessageHandler implements MessageHandler {
  private static final String ACTION_OPEN_PAGE = "App.openPage";
  private static final String ACTION_CLOSE_PAGE = "App.closePage";

  private final String mActionName;

  private ExplorerAppMessageHandler(String actionName) {
    mActionName = actionName;
  }

  public static MessageHandler createOpenPageHandler() {
    return new ExplorerAppMessageHandler(ACTION_OPEN_PAGE);
  }

  public static MessageHandler createClosePageHandler() {
    return new ExplorerAppMessageHandler(ACTION_CLOSE_PAGE);
  }

  @Override
  public MessageHandleResult handle(Map<String, String> params) {
    try {
      if (ACTION_OPEN_PAGE.equals(mActionName)) {
        String url = params == null ? null : params.get("url");
        if (url == null || url.trim().isEmpty()) {
          return fail("Parameter `url` is required.");
        }
        LynxModuleAdapter.getInstance().openSchemaSync(url);
        return success(null);
      }

      if (ACTION_CLOSE_PAGE.equals(mActionName)) {
        if (!LynxModuleAdapter.getInstance().hasClosablePage()) {
          return fail("No active Explorer page can be closed.");
        }
        LynxModuleAdapter.getInstance().closeCurrentPageSync();
        return success(null);
      }
    } catch (RuntimeException e) {
      return fail(e.getMessage());
    }

    return new MessageHandleResult(
        MessageHandleResult.CODE_NOT_IMPLEMENTED, "Unsupported action: " + mActionName);
  }

  @Override
  public String getName() {
    return mActionName;
  }

  private MessageHandleResult success(@Nullable String message) {
    return new MessageHandleResult(MessageHandleResult.CODE_HANDLE_SUCCESSFULLY, message);
  }

  private MessageHandleResult fail(@Nullable String message) {
    return new MessageHandleResult(
        MessageHandleResult.CODE_HANDLE_FAILED, message == null ? "Unknown error." : message);
  }
}
