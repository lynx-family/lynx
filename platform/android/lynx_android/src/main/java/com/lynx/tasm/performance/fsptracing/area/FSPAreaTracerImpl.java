// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.area;

import android.graphics.Rect;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPContentInfo;
import com.lynx.tasm.performance.fsptracing.FSPSnapshot;
import com.lynx.tasm.performance.fsptracing.IFSPTracerImpl;
import com.lynx.tasm.performance.fsptracing.area.FSPAreaConfig;
import com.lynx.tasm.performance.fsptracing.area.FSPAreaSnapshot;
import java.util.ArrayList;
import java.util.BitSet;

/**
 * Implementation of FSP tracer using area projection method.
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAreaTracerImpl implements IFSPTracerImpl {
  private final FSPAreaConfig mConfig;

  public FSPAreaTracerImpl(FSPAreaConfig config) {
    mConfig = config;
  }

  @Override
  public FSPSnapshot createSnapshot() {
    return new FSPAreaSnapshot();
  }

  @Override
  public void fillSnapshot(@NonNull FSPSnapshot snapshot, @NonNull FSPContentInfo contentInfo) {
    assert snapshot instanceof FSPAreaSnapshot;

    Rect contentRect = contentInfo.getRect();
    int uiSign = contentInfo.getUiSign();
    long changeTimestampNs = contentInfo.getChangeTimestampNs();

    if (contentRect.isEmpty() || snapshot.getContainerWidth() <= 0
        || snapshot.getContainerHeight() <= 0) {
      return;
    }

    snapshot.updateTimestamp(changeTimestampNs, uiSign);

    int w = snapshot.getContainerWidth();
    int h = snapshot.getContainerHeight();

    FSPAreaSnapshot areaSnapshot = (FSPAreaSnapshot) snapshot;
    BitSet projections = areaSnapshot.getProjections();

    int minX = Math.max(0, Math.min(contentRect.left, w));
    minX = minX * FSPAreaSnapshot.X_PROJECTIONS_LEN / w;
    minX = Math.min(minX, FSPAreaSnapshot.X_PROJECTIONS_LEN - 1);

    int maxX = Math.max(0, Math.min(contentRect.right, w));
    maxX = maxX * FSPAreaSnapshot.X_PROJECTIONS_LEN / w;
    maxX = Math.min(maxX, FSPAreaSnapshot.X_PROJECTIONS_LEN - 1);

    int minY = Math.max(0, Math.min(contentRect.top, h));
    minY = minY * FSPAreaSnapshot.Y_PROJECTIONS_LEN / h;
    minY = Math.min(minY, FSPAreaSnapshot.Y_PROJECTIONS_LEN - 1);

    int maxY = Math.max(0, Math.min(contentRect.bottom, h));
    maxY = maxY * FSPAreaSnapshot.Y_PROJECTIONS_LEN / h;
    maxY = Math.min(maxY, FSPAreaSnapshot.Y_PROJECTIONS_LEN - 1);
    
    for (int y = minY; y <= maxY; y++) {
      for (int x = minX; x <= maxX; x++) {
        projections.set(y * FSPAreaSnapshot.X_PROJECTIONS_LEN + x);
      }
    }
  }

  @Override
  public boolean diffSnapshots(@NonNull FSPSnapshot current, FSPSnapshot previous) {
    assert current instanceof FSPAreaSnapshot;
    assert previous == null || previous instanceof FSPAreaSnapshot;

    FSPAreaSnapshot currentArea = (FSPAreaSnapshot) current;

    long containerArea = (long) currentArea.getContainerWidth() * currentArea.getContainerHeight();
    int projectionArea = (int) (currentArea.getProjections().cardinality() * containerArea
        / FSPAreaSnapshot.PROJECTIONS_LEN);
    currentArea.setProjectionArea(projectionArea);

    // Check against completion rates
    double compRate = (double) projectionArea / containerArea;
    if (compRate < mConfig.getMinimumCompletionRate()) {
      return false;
    }

    if (previous == null) {
      // Initial snapshot always considered stable.
      return true;
    }

    FSPAreaSnapshot prevArea = (FSPAreaSnapshot) previous;

    // diff area changes
    int diffArea = projectionArea - prevArea.getProjectionArea();
    if (diffArea >= mConfig.getAcceptableDifferencePerSnapshot()) {
      return false;
    }

    double diffT =
        (currentArea.getLastChangeTimestampNs() - prevArea.getLastChangeTimestampNs()) / 1000_000.0;

    // Check if the interval is less than the minimum interval.
    if (diffT <= mConfig.getMinimumDiffIntervalMs()) {
      return false;
    }

    // diff area change rates
    double rateArea = diffArea / diffT;
    if (rateArea >= mConfig.getAcceptableDifferencePerMs()) {
      return false;
    }

    // diff content change rates
    BitSet diffProjections = (BitSet) currentArea.getProjections().clone();
    diffProjections.xor(prevArea.getProjections());
    int diffContent =
        (int) (diffProjections.cardinality() * containerArea / FSPAreaSnapshot.PROJECTIONS_LEN);
    double rateContent = diffContent / diffT;

    if (rateContent >= mConfig.getAcceptableDifferencePerMs()) {
      return false;
    }

    return true;
  }

  @Override
  public ArrayList<String> inspectSnapshot(@NonNull FSPSnapshot current, FSPSnapshot previous) {
    assert current instanceof FSPAreaSnapshot;
    assert previous == null || previous instanceof FSPAreaSnapshot;

    // TODO: Implement inspection of snapshot detail.
    return new ArrayList<>();
  }
}
