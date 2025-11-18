// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui;
import android.graphics.Rect;

/**
 * The ILynxUIMeaningfulContent defines the interface for LynxUI to report meaningful content
 * loading status.
 */
public interface ILynxUIMeaningfulContent {
  enum MeaningfulContentStatus {
    /**
     * Represents a UI component that does not render meaningful content and is irrelevant to FSP
     * calculation (e.g., a container view).
     */
    IRRELEVANT,
    /**
     * Represents a UI component that contains meaningful content but is still pending load or
     * render (e.g., an Image is downloading).
     */
    PENDING,
    /**
     * Represents a UI component that has finished loading and has presented its meaningful content
     * to the user.
     */
    PRESENTED
  }

  /**
   * Gets the current meaningful content loading status of the UI component. See
   * `MeaningfulContentStatus` for details.
   */
  MeaningfulContentStatus getMeaningfulContentStatus();

  /**
   * Optionally provides the timestamp in microseconds when the first meaningful content was
   * presented.
   *
   * <p>This method is optional. If not implemented, it defaults to -1, indicating that no specific
   * timestamp is provided.
   *
   * @return The timestamp in microseconds, or -1 if not implemented.
   */
  long getFirstMeaningfulContentPresentedTimestampMicros();
}
