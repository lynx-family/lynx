// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.shapes;

import android.graphics.Path;
import android.graphics.PathMeasure;
import com.lynx.tasm.utils.LRUHashMap;
import java.util.ArrayList;
import java.util.List;

public class LynxOffsetCalculator {
  private static final int MAX_CACHE_SIZE = 10;
  private static final LRUHashMap<Path, PathLengthCache> lruPathCache =
      new LRUHashMap<>(MAX_CACHE_SIZE);

  // Path length cache
  private static class PathLengthCache {
    float totalLength;
    List<Float> segmentLengths;

    PathLengthCache() {
      segmentLengths = new ArrayList<>();
    }
  }

  /**
   * Gets the point coordinates of the specified progress on the path
   * @param path The path to calculate
   * @param progress The progress (0.0 ~ 1.0)
   * @return Coordinates of the corresponding progress points on the path
   */
  public static float[] pointAtProgress(Path path, float progress) {
    if (path == null) {
      return new float[] {0, 0, 0};
    }
    progress = Math.max(0, Math.min(1, progress));

    // Get or create a cache
    PathLengthCache cache = lruPathCache.get(path);
    if (cache == null) {
      cache = new PathLengthCache();

      // Measure all path segments
      PathMeasure pathMeasure = new PathMeasure(path, false);
      do {
        float length = pathMeasure.getLength();
        cache.segmentLengths.add(length);
        cache.totalLength += length;
      } while (pathMeasure.nextContour());

      // LRU cache
      lruPathCache.put(path, cache);
    }

    // Calculate target distance
    float targetDistance = cache.totalLength * progress;

    // Find the path segment where the target point is located
    float accumulatedLength = 0;
    int targetSegment = 0;

    for (int i = 0; i < cache.segmentLengths.size(); i++) {
      float nextAccLength = accumulatedLength + cache.segmentLengths.get(i);
      if (targetDistance <= nextAccLength) {
        targetSegment = i;
        break;
      }
      accumulatedLength = nextAccLength;
    }

    // Reinitialize PathMeasure to the target path segment
    PathMeasure pathMeasure = new PathMeasure(path, false);
    for (int i = 0; i < targetSegment; i++) {
      pathMeasure.nextContour();
    }

    // Calculate the relative distance on the current path segment
    float distanceInSegment = targetDistance - accumulatedLength;

    // An array to store the result [x, y]
    float[] pos = new float[2];
    float[] tan = new float[2];

    // Get point coordinates at specified distance on the path
    pathMeasure.getPosTan(distanceInSegment, pos, tan);

    float angle = (float) ((Math.toDegrees(Math.atan2(tan[0], tan[1])) + 360) % 180);
    return new float[] {pos[0], pos[1], angle};
  }
}
