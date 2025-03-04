/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;

/**
 * Interface that represent javascript callback function which can be passed to
 * the native module as a method parameter.
 */
@Keep
public interface Callback {
  /**
   * Schedule javascript function execution represented by this {@link Callback}
   * instance
   *
   * @param args arguments passed to javascript callback method via bridge
   */
  @Keep void invoke(Object... args);
}
