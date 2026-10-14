/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.lynx.jsbridge;

import androidx.annotation.Keep;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.base.CalledByNative;
import java.nio.ByteBuffer;

/**
 * Interface that represents a bytecode generate result.
 */
@Keep
public interface LynxBytecodeCallback {
  /**
   * response when bytecode generate finish.
   * @param errorMsg when success, errorMsg will be null, otherwise is error.
   * @param buffers when success, this will be bytecode result.Each key is sourceUrl, value is
   * bytecode result.Note: Please note that the current ByteBuffer is constructed using
   * DirectByteBuffer. If you need to use it, please make a copy yourself and do not store this
   * object directly.
   */
  @CalledByNative void onResponse(String errorMsg, JavaOnlyMap buffers);

  /**
   * Called once per decoded js file right before its bytecode would be generated.
   * It is invoked on a background thread.
   * @param jsName the name of the decoded js file, the same as its key in the buffers of
   * onResponse.
   * @param md5 the md5 of the js file's source content.
   * @return true to continue generating bytecode for this file; false to skip it, e.g. when the
   * caller already holds the bytecode for this md5. Defaults to true so existing implementations
   * keep generating every file.
   */
  @CalledByNative
  default boolean continueGeneration(String jsName, String md5) {
    return true;
  }
}
