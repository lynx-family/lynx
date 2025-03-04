// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

/**
 * Interface for handling actions related to Lynx card.
 */
public interface LynxDevtoolCardListener {
  /**
   * Opens a given URL card.
   *
   * @param url The URL to open.
   */
  void open(String url);
}
