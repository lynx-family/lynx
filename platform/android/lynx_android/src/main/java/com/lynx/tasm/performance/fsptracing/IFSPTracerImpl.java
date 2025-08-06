// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing;

import androidx.annotation.NonNull;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPContentInfo;
import java.util.ArrayList;

/**
 * Interface for FSP tracer implementations.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public interface IFSPTracerImpl {
  /**
   * Create a new snapshot instance.
   */
  @NonNull FSPSnapshot createSnapshot();

  /**
   * Compare current snapshot with a previous one to determine if the UI is stable.
   * @param current The current snapshot
   * @param previous The previous snapshot, may be null
   * @return true if the UI is considered stable, false otherwise
   */
  boolean diffSnapshots(@NonNull FSPSnapshot current, FSPSnapshot previous);

  /**
   * Fill the snapshot with content information.
   * @param snapshot The snapshot to fill
   * @param contentInfo The content information
   */
  void fillSnapshot(@NonNull FSPSnapshot snapshot, @NonNull FSPContentInfo contentInfo);

  /**
   * Inspect the current snapshot for devtool.
   * @param current The current snapshot
   * @param previous The previous snapshot, may be null
   * @return List of strings with snapshot details
   */
  ArrayList<String> inspectSnapshot(@NonNull FSPSnapshot current, FSPSnapshot previous);
}
