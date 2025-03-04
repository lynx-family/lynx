// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.ref;

import androidx.annotation.Nullable;
import java.util.Collection;
import java.util.HashSet;
import java.util.concurrent.atomic.AtomicInteger;

public final class ShareRef<T> implements Cloneable {
  private static final Collection<Wrap> LIVE_OBJECTS = new HashSet<>();
  private final AtomicInteger mRefCount;
  private volatile ResourceReleaser<T> mReleaser;
  private volatile Wrap mWrap;
  private volatile boolean mReleased;

  public ShareRef(T obj, ResourceReleaser<T> releaser) {
    mRefCount = new AtomicInteger(1);
    mReleaser = releaser;
    mWrap = new Wrap(obj);
    synchronized (LIVE_OBJECTS) {
      LIVE_OBJECTS.add(mWrap);
    }
  }

  private ShareRef(AtomicInteger refCount, ResourceReleaser<T> releaser, Wrap wrap) {
    mRefCount = refCount;
    mReleaser = releaser;
    mWrap = wrap;
  }

  public int getRefCount() {
    return mRefCount.get();
  }

  public synchronized ShareRef<T> clone() {
    if (mReleased) {
      throw new IllegalStateException("already released");
    }
    mRefCount.incrementAndGet();
    return new ShareRef<>(mRefCount, mReleaser, mWrap);
  }
  @Nullable
  public synchronized T get() {
    if (mReleased) {
      return null;
    }
    return mWrap.get();
  }

  public synchronized void release() {
    if (mReleased) {
      return;
    }
    mReleased = true;
    releaseIfNeeded();
    mWrap = null;
    mReleaser = null;
  }

  @Override
  protected void finalize() throws Throwable {
    if (!mReleased) {
      releaseIfNeeded();
    }
    super.finalize();
  }

  private void releaseIfNeeded() {
    if (mRefCount.decrementAndGet() != 0) {
      return;
    }
    T obj = mWrap.get();
    synchronized (LIVE_OBJECTS) {
      LIVE_OBJECTS.remove(mWrap);
    }
    if (mReleaser != null) {
      mReleaser.release(obj);
    }
  }
}
