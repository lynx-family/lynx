// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.performance.fsptracing.axial;

import android.graphics.Rect;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.annotation.RestrictTo;
import com.lynx.tasm.performance.fsptracing.FSPContentInfo;
import com.lynx.tasm.performance.fsptracing.FSPSnapshot;
import com.lynx.tasm.performance.fsptracing.IFSPTracerImpl;
import com.lynx.tasm.performance.fsptracing.axial.FSPAxialConfig;
import com.lynx.tasm.performance.fsptracing.axial.FSPAxialSnapshot;
import java.util.ArrayList;
import java.util.BitSet;

/**
 * Implementation of FSP tracer using axial projection method.
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class FSPAxialTracerImpl implements IFSPTracerImpl {
  private final FSPAxialConfig mConfig;

  public FSPAxialTracerImpl(@NonNull FSPAxialConfig config) {
    mConfig = config;
  }

  @Override
  public FSPSnapshot createSnapshot() {
    return new FSPAxialSnapshot();
  }

  @Override
  public void fillSnapshot(@NonNull FSPSnapshot snapshot, @NonNull FSPContentInfo contentInfo) {
    assert snapshot instanceof FSPAxialSnapshot;

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

    FSPAxialSnapshot axialSnapshot = (FSPAxialSnapshot) snapshot;
    BitSet xProjections = axialSnapshot.getXProjections();
    BitSet yProjections = axialSnapshot.getYProjections();

    int minX = Math.max(0, Math.min(contentRect.left, w));
    minX = minX * FSPAxialSnapshot.X_PROJECTIONS_LEN / w;
    minX = Math.min(minX, FSPAxialSnapshot.X_PROJECTIONS_LEN - 1);

    int maxX = Math.max(0, Math.min(contentRect.right, w));
    maxX = maxX * FSPAxialSnapshot.X_PROJECTIONS_LEN / w;
    maxX = Math.min(maxX, FSPAxialSnapshot.X_PROJECTIONS_LEN - 1);

    for (int i = minX; i <= maxX; i++) {
      xProjections.set(i);
    }

    int minY = Math.max(0, Math.min(contentRect.top, h));
    minY = minY * FSPAxialSnapshot.Y_PROJECTIONS_LEN / h;
    minY = Math.min(minY, FSPAxialSnapshot.Y_PROJECTIONS_LEN - 1);

    int maxY = Math.max(0, Math.min(contentRect.bottom, h));
    maxY = maxY * FSPAxialSnapshot.Y_PROJECTIONS_LEN / h;
    maxY = Math.min(maxY, FSPAxialSnapshot.Y_PROJECTIONS_LEN - 1);

    for (int i = minY; i <= maxY; i++) {
      yProjections.set(i);
    }
  }

  @Override
  public boolean diffSnapshots(@NonNull FSPSnapshot current, FSPSnapshot previous) {
    assert current instanceof FSPAxialSnapshot;
    assert previous == null || previous instanceof FSPAxialSnapshot;

    FSPAxialSnapshot currentAxial = (FSPAxialSnapshot) current;

    int projectionW = (int) (currentAxial.getXProjections().cardinality()
        * (long) currentAxial.getContainerWidth() / FSPAxialSnapshot.X_PROJECTIONS_LEN);
    int projectionH = (int) (currentAxial.getYProjections().cardinality()
        * (long) currentAxial.getContainerHeight() / FSPAxialSnapshot.Y_PROJECTIONS_LEN);

    currentAxial.setProjectionWidth(projectionW);
    currentAxial.setProjectionHeight(projectionH);

    // Check against completion rates
    double compRateW = (double) projectionW / currentAxial.getContainerWidth();
    double compRateH = (double) projectionH / currentAxial.getContainerHeight();

    if (compRateW < mConfig.getMinimumCompletionRateX()
        || compRateH < mConfig.getMinimumCompletionRateY()) {
      return false;
    }

    if (previous == null) {
      // Initial snapshot always considered stable.
      return true;
    }

    FSPAxialSnapshot prevAxial = (FSPAxialSnapshot) previous;

    // diff in pixels/points in dimensions
    int diffW = projectionW - prevAxial.getProjectionWidth();
    int diffH = projectionH - prevAxial.getProjectionHeight();

    if (diffW >= mConfig.getAcceptablePixelDifferencePerSnapshot()
        || diffH >= mConfig.getAcceptablePixelDifferencePerSnapshot()) {
      return false;
    }

    double diffT = (currentAxial.getLastChangeTimestampNs() - prevAxial.getLastChangeTimestampNs())
        / 1000_000.0;

    // Check if the interval is less than the minimum interval.
    if (diffT <= mConfig.getMinimumDiffIntervalMs()) {
      return false;
    }

    // diff in pixels/points in dimensions per second
    double rateW = diffW / diffT;
    double rateH = diffH / diffT;

    if (rateW >= mConfig.getAcceptablePixelDifferencePerMs()
        || rateH >= mConfig.getAcceptablePixelDifferencePerMs()) {
      return false;
    }

    BitSet xDiff = (BitSet) currentAxial.getXProjections().clone();
    xDiff.xor(prevAxial.getXProjections());
    BitSet yDiff = (BitSet) currentAxial.getYProjections().clone();
    yDiff.xor(prevAxial.getYProjections());

    // diff in pixels/points in projected dimensions per second
    int diffX =
        xDiff.cardinality() * currentAxial.getContainerWidth() / FSPAxialSnapshot.X_PROJECTIONS_LEN;
    int diffY = yDiff.cardinality() * currentAxial.getContainerHeight()
        / FSPAxialSnapshot.Y_PROJECTIONS_LEN;

    double rateX = diffX / diffT;
    double rateY = diffY / diffT;

    if (rateX >= mConfig.getAcceptablePixelDifferencePerMs()
        || rateY >= mConfig.getAcceptablePixelDifferencePerMs()) {
      return false;
    }

    return true;
  }

  @Override
  public ArrayList<String> inspectSnapshot(@NonNull FSPSnapshot current, FSPSnapshot previous) {
    // TODO: Implement inspection of snapshot detail.
    return new ArrayList<>();
  }
}
