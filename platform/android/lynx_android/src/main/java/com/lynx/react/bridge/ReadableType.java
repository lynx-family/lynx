/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

import androidx.annotation.Keep;

/**
 * Defines the type of an object stored in a {@link ReadableArray} or
 * {@link ReadableMap}.
 */
@Keep
public enum ReadableType {
  Null,
  Boolean,
  Int,
  Number,
  String,
  Map,
  Array,
  Long,
  ByteArray,
  PiperData,
  LynxObject
}
