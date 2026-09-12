// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;

/**
 * Receives transfer views created by {@code <transfer-view>} nodes.
 *
 * <p>Experimental API. This API is unstable and may change or be removed without notice.
 *
 * <p>A listener becomes the owner of a transfer only when {@link
 * #onCreate(String, View, ReadableMap)} returns {@code true}. Only that owner listener will receive
 * the matching dataset updates and {@link #onRemove(String, View)} callback when the transfer is
 * removed.
 */
public interface LynxTransferListener {
  /**
   * Called when a transfer view is ready to be mounted by the host.
   *
   * @param id the {@code transfer-id} declared on the {@code <transfer-view>} node
   * @param view the detached wrapper view containing the transfer subtree
   * @param dataset the dataset declared on the {@code <transfer-view>} node
   * @return {@code true} if this listener takes ownership of the view; {@code false} to leave it
   *     pending for another listener
   */
  boolean onCreate(@NonNull String id, @NonNull View view, @Nullable ReadableMap dataset);

  /**
   * Called after the properties of an owned transfer have been updated.
   *
   * @param id the {@code transfer-id} associated with the updated transfer
   * @param dataset the latest dataset declared on the {@code <transfer-view>} node
   */
  void onDatasetUpdate(@NonNull String id, @Nullable ReadableMap dataset);

  /**
   * Called when an owned transfer is removed or the host {@link LynxView} is destroyed.
   *
   * <p>This callback is sent only to the listener that previously returned {@code true} from
   * {@link #onCreate(String, View, ReadableMap)} for the same view.
   *
   * @param id the {@code transfer-id} associated with the removed transfer
   * @param view the wrapper view that will be detached from its current parent
   */
  void onRemove(@NonNull String id, @NonNull View view);
}
