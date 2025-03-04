// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.generic;

/**
 * lynx streaming fetch delegate.
 */
public interface StreamDelegate {
  /**
   * Streaming load process started.
   *
   * @param contentOffset total length, -1 means unknown.
   */
  void onStart(int contentOffset);

  /**
   * streaming load process return a part of data
   * @param bytes content in resource.
   * @param offset content part offset.
   * @param length content part length.
   */
  void onData(byte[] bytes, int offset, int length);

  /**
   * streaming load process ended success.
   */
  void onEnd();

  /**
   * streaming load process ended with error.
   */
  void onError(String errMsg);
}
