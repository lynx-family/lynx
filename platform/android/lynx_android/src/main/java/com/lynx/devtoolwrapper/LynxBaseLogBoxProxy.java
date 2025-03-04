// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtoolwrapper;

import android.content.Context;
import com.lynx.tasm.LynxError;

/**
 * Interface for handling LogBox operation
 */
public interface LynxBaseLogBoxProxy {
  /**
   * Displays a log message.
   *
   * @param error The error object containing details about the log message.
   */
  void showLogMessage(final LynxError error);
  /**
   * Displays the console.
   */
  void showConsole();

  /**
   * Called when the template is loaded.
   */
  void onLoadTemplate();

  /**
   * Attaches a context to the proxy.
   *
   * @param context The context to attach.
   */
  void attachContext(Context context);
}
