// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.performance.memory.MemoryRecord;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Memory attribution for one completed Lynx instance query.
 */
public final class LynxInstanceMemoryUsage {
  private final int mInstanceId;
  @Nullable private final String mPageId;
  @Nullable private final String mUrl;
  private final long mTotalBytes;
  private final long mElementBytes;
  private final long mElementNodeCount;
  private final long mViewBytes;
  @NonNull private final Map<String, MemoryRecord> mViewDetail;
  private final long mMainThreadRuntimeBytes;
  private final long mBackgroundThreadRuntimeBytes;
  @Nullable private final String mBtsRuntimeGroupId;

  LynxInstanceMemoryUsage(int instanceId, @Nullable String pageId, @Nullable String url,
      long totalBytes, long elementBytes, long elementNodeCount, long viewBytes,
      @Nullable Map<String, MemoryRecord> viewDetail, long mainThreadRuntimeBytes,
      long backgroundThreadRuntimeBytes, @Nullable String btsRuntimeGroupId) {
    mInstanceId = instanceId;
    mPageId = pageId;
    mUrl = url;
    mTotalBytes = totalBytes;
    mElementBytes = elementBytes;
    mElementNodeCount = elementNodeCount;
    mViewBytes = viewBytes;
    if (viewDetail == null || viewDetail.isEmpty()) {
      mViewDetail = Collections.emptyMap();
    } else {
      mViewDetail = Collections.unmodifiableMap(new HashMap<>(viewDetail));
    }
    mMainThreadRuntimeBytes = mainThreadRuntimeBytes;
    mBackgroundThreadRuntimeBytes = backgroundThreadRuntimeBytes;
    mBtsRuntimeGroupId = btsRuntimeGroupId;
  }

  /** LynxShell instance id. -1 means the instance was not fully attached. */
  public int getInstanceId() {
    return mInstanceId;
  }

  /** Temporary page identity for the instance memory sample. */
  @Nullable
  public String getPageId() {
    return mPageId;
  }

  /** Current template URL captured when the instance fetcher completes. */
  @Nullable
  public String getUrl() {
    return mUrl;
  }

  /** Instance-attributed total: element + view + main runtime + background runtime. */
  public long getTotalBytes() {
    return mTotalBytes;
  }

  /** Element tree memory reported from the native element manager. */
  public long getElementBytes() {
    return mElementBytes;
  }

  /** Number of element nodes used as a context signal for elementBytes. */
  public long getElementNodeCount() {
    return mElementNodeCount;
  }

  /** Android view memory reported by LynxUIOwner. */
  public long getViewBytes() {
    return mViewBytes;
  }

  /**
   * Per-view-category memory details reported by LynxUIOwner.
   *
   * <p>The map key is the view category/tag. Each {@link MemoryRecord} value is a detached snapshot
   * containing category, sizeBytes, instanceCount, and optional category-specific detail. This
   * keeps the Android API aligned with the "lynxsdk_memory_usage" detail shape, for example:
   * {@code {"x-viewpager-ng": {"category": "x-viewpager-ng", "sizeBytes": 0,
   * "instanceCount": 2}}}.
   *
   * <p>The returned map is immutable. Its {@link MemoryRecord} values are detached snapshots copied
   * from LynxUIOwner on the UI thread.
   */
  @NonNull
  public Map<String, MemoryRecord> getViewDetail() {
    return mViewDetail;
  }

  /** Main-thread runtime heap snapshot bytes. */
  public long getMainThreadRuntimeBytes() {
    return mMainThreadRuntimeBytes;
  }

  /** Background-thread runtime heap snapshot bytes. */
  public long getBackgroundThreadRuntimeBytes() {
    return mBackgroundThreadRuntimeBytes;
  }

  /** Background runtime group id used by global result deduplication. */
  @Nullable
  public String getBtsRuntimeGroupId() {
    return mBtsRuntimeGroupId;
  }
}
