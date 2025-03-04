// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.list;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.lynx.tasm.EventEmitter;

interface AppearEventCourierInterface {
  void onListNodeAttached(@NonNull ListViewHolder holder);
  void onListNodeDetached(@NonNull ListViewHolder holder);
  void holderAttached(ListViewHolder holder);

  void onListLayout();
}

public class AppearEventCourier implements AppearEventCourierInterface {
  private EventEmitter mEventEmitter;
  private RecyclerView mRecyclerView;
  private AppearEventCourierInterface mImpl;
  private boolean mUseNew;

  public AppearEventCourier(
      @NonNull EventEmitter eventEmitter, @NonNull RecyclerView recyclerView) {
    mEventEmitter = eventEmitter;
    mRecyclerView = recyclerView;
    mUseNew = false;
    mImpl = new AppearEventCourierImpl(eventEmitter);
  }

  final void setNewAppear(boolean useNew) {
    if (useNew == mUseNew) {
      return;
    }
    mUseNew = useNew;
    if (useNew) {
      mImpl = new AppearEventCourierImplV2(mEventEmitter, mRecyclerView);
    } else {
      mImpl = new AppearEventCourierImpl(mEventEmitter);
    }
  }

  final void setDisappear(boolean enableDisappear) {
    if (mImpl instanceof AppearEventCourierImpl) {
      ((AppearEventCourierImpl) mImpl).setDisappear(enableDisappear);
    }
  }

  public void onListNodeAttached(@NonNull ListViewHolder holder) {
    mImpl.onListNodeAttached(holder);
  }
  public void onListNodeDetached(@NonNull ListViewHolder holder) {
    mImpl.onListNodeDetached(holder);
  }
  public void holderAttached(ListViewHolder holder) {
    mImpl.holderAttached(holder);
  }
  public void onListLayout() {
    mImpl.onListLayout();
  }
}
