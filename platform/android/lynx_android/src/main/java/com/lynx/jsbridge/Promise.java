/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.jsbridge;

import androidx.annotation.Keep;
import androidx.annotation.Nullable;

/**
 * Interface that represents a JavaScript Promise which can be passed to the native module as a
 * method parameter.
 *
 * Methods annotated with {@link LynxMethod} that use {@link Promise} as type of the last parameter
 * will be marked as "promise" and will return a promise when invoked from JavaScript.
 */
@Keep
public interface Promise {
  /**
   * Successfully resolve the Promise.
   */
  @Keep void resolve(@Nullable Object value);

  /**
   * Report an error which wasn't caused by an exception.
   */
  @Keep void reject(String code, String message);

  /**
   * Report an error which wasn't caused by an exception.
   * @deprecated Prefer passing a module-specific error code to JS.
   *             Using this method will pass the error code "EUNSPECIFIED".
   */
  @Keep @Deprecated void reject(String message);
}
