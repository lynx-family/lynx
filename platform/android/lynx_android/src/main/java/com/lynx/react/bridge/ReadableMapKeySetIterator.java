/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;

/**
 * Interface of a iterator for a {@link NativeMap}'s key set.
 */
@Keep
public interface ReadableMapKeySetIterator {
  boolean hasNextKey();
  String nextKey();
}
