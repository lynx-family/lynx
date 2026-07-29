// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

final class LynxTransferManager {
  // Registered host listeners. A listener owns a transfer only after its onCreate returns true.
  private final List<LynxTransferListener> mTransferListeners = new ArrayList<>();

  // Transfers that have been created by UITransfer but no listener has accepted yet.
  private final Map<View, TransferRecord> mPendingTransfers = new HashMap<>();

  // Transfers currently mounted outside LynxView by the listener stored in TransferRecord.
  private final Map<View, TransferRecord> mActiveTransfers = new HashMap<>();

  private static final class TransferRecord {
    final String id;
    final UITransfer owner;
    final View view;

    // Weak to avoid making host listener lifetime depend on transfer records.
    @Nullable final WeakReference<LynxTransferListener> listenerRef;

    TransferRecord(
        String id, UITransfer owner, View view, @Nullable LynxTransferListener listener) {
      this.id = id;
      this.owner = owner;
      this.view = view;
      this.listenerRef = listener == null ? null : new WeakReference<>(listener);
    }

    @Nullable
    LynxTransferListener getListener() {
      return listenerRef == null ? null : listenerRef.get();
    }
  }

  void registerTransferListener(@NonNull LynxTransferListener listener) {
    if (mTransferListeners.contains(listener)) {
      return;
    }
    mTransferListeners.add(listener);
    if (mPendingTransfers.isEmpty()) {
      return;
    }

    // Late listeners get a chance to claim existing pending transfers synchronously.
    List<TransferRecord> pending = new ArrayList<>(mPendingTransfers.values());
    for (TransferRecord record : pending) {
      removeFromParent(record.view);
      if (listener.onCreate(record.id, record.view)) {
        mPendingTransfers.remove(record.view);
        mActiveTransfers.put(
            record.view, new TransferRecord(record.id, record.owner, record.view, listener));
      }
    }
  }

  void unregisterTransferListener(@NonNull LynxTransferListener listener) {
    mTransferListeners.remove(listener);
  }

  boolean dispatchTransferCreate(
      @NonNull String id, @NonNull UITransfer owner, @NonNull View view) {
    // A fresh create for the same wrapper replaces stale state from prior ids or retries.
    mPendingTransfers.remove(view);
    mActiveTransfers.remove(view);
    removeFromParent(view);

    for (LynxTransferListener listener : getTransferListenersSnapshot()) {
      if (listener.onCreate(id, view)) {
        mActiveTransfers.put(view, new TransferRecord(id, owner, view, listener));
        return true;
      }
    }

    // No listener accepted this transfer yet; keep it for future listener registration.
    mPendingTransfers.put(view, new TransferRecord(id, owner, view, null));
    return false;
  }

  void dispatchTransferRemove(@NonNull String id, @NonNull View view) {
    // Active transfers notify only the listener that accepted onCreate; pending transfers only
    // detach.
    TransferRecord record = mActiveTransfers.remove(view);
    if (record == null) {
      record = mPendingTransfers.remove(view);
    }
    if (record == null) {
      removeFromParent(view);
      return;
    }
    LynxTransferListener listener = record.getListener();
    if (listener != null && mTransferListeners.contains(listener)) {
      listener.onRemove(id, view);
    }
    removeFromParent(view);
  }

  void clearTransfers() {
    // LynxView destroy clears both mounted and unclaimed transfers in one pass.
    List<TransferRecord> transferRecords = new ArrayList<>(mActiveTransfers.values());
    transferRecords.addAll(mPendingTransfers.values());
    List<LynxTransferListener> transferListeners = getTransferListenersSnapshot();
    mActiveTransfers.clear();
    mPendingTransfers.clear();
    for (TransferRecord record : transferRecords) {
      LynxTransferListener listener = record.getListener();
      if (listener != null && transferListeners.contains(listener)) {
        listener.onRemove(record.id, record.view);
      }
      removeFromParent(record.view);
    }
    mTransferListeners.clear();
  }

  @NonNull
  private List<LynxTransferListener> getTransferListenersSnapshot() {
    return new ArrayList<>(mTransferListeners);
  }

  private static void removeFromParent(@NonNull View view) {
    if (view.getParent() instanceof ViewGroup) {
      ((ViewGroup) view.getParent()).removeView(view);
    }
  }
}
