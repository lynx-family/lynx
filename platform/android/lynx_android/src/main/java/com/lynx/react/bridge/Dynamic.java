/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.react.bridge;

/**
 * Type representing a piece of data with unknown runtime type. Useful for allowing javascript to
 * pass one of multiple types down to the native layer.
 */
public interface Dynamic {
  boolean isNull();
  boolean asBoolean();
  double asDouble();
  int asInt();
  long asLong();
  String asString();
  ReadableArray asArray();
  ReadableMap asMap();
  byte[] asByteArray();
  ReadableType getType();
  @Deprecated void recycle();
}
