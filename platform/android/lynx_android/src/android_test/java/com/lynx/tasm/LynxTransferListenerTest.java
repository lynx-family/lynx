// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.view.View;
import android.widget.FrameLayout;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxTransferListenerTest {
  private Context mContext;
  private UITransfer mOwner;

  @Before
  public void setUp() {
    mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxEnv.inst().init((Application) mContext.getApplicationContext(), null, null, null, null);
    mOwner = mock(UITransfer.class);
    when(mOwner.getLynxContext()).thenReturn(mock(LynxContext.class));
  }

  private static class CountingTransferListener implements LynxTransferListener {
    private final String mAcceptedId;
    int createCount;
    int datasetUpdateCount;
    int removeCount;
    ReadableMap createDataset;
    ReadableMap updatedDataset;

    CountingTransferListener(String acceptedId) {
      mAcceptedId = acceptedId;
    }

    @Override
    public boolean onCreate(String id, View view, ReadableMap dataset) {
      if (!mAcceptedId.equals(id)) {
        return false;
      }
      createCount++;
      createDataset = dataset;
      return true;
    }

    @Override
    public void onDatasetUpdate(String id, ReadableMap dataset) {
      datasetUpdateCount++;
      updatedDataset = dataset;
    }

    @Override
    public void onRemove(String id, View view) {
      removeCount++;
    }
  }

  @Test
  @UiThreadTest
  public void dispatchTransferRemove_notifiesOnlyListenerThatAcceptedCreate() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxView lynxView = new LynxView(context);
    View transferView = new View(context);
    CountingTransferListener ownerListener = new CountingTransferListener("accepted");
    CountingTransferListener unrelatedListener = new CountingTransferListener("unrelated");

    lynxView.registerTransferListener(ownerListener);
    lynxView.registerTransferListener(unrelatedListener);

    lynxView.dispatchTransferCreate("accepted", mOwner, transferView);
    lynxView.dispatchTransferRemove("accepted", transferView);

    assertEquals(1, ownerListener.createCount);
    assertEquals(1, ownerListener.removeCount);
    assertEquals(0, unrelatedListener.createCount);
    assertEquals(0, unrelatedListener.removeCount);
    lynxView.destroy();
  }

  @Test
  @UiThreadTest
  public void dispatchTransferDatasetUpdate_notifiesOwnerWithCurrentDatasets() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxView lynxView = new LynxView(context);
    View transferView = new View(context);
    JavaOnlyMap initialDataset = new JavaOnlyMap();
    initialDataset.putString("phase", "create");
    JavaOnlyMap updatedDataset = new JavaOnlyMap();
    updatedDataset.putString("phase", "update");
    when(mOwner.getDataset()).thenReturn(initialDataset);
    CountingTransferListener listener = new CountingTransferListener("accepted");

    lynxView.registerTransferListener(listener);
    lynxView.dispatchTransferCreate("accepted", mOwner, transferView);
    lynxView.dispatchTransferDatasetUpdate(transferView, updatedDataset);

    assertSame(initialDataset, listener.createDataset);
    assertEquals(1, listener.datasetUpdateCount);
    assertSame(updatedDataset, listener.updatedDataset);
    lynxView.destroy();
  }

  @Test
  @UiThreadTest
  public void lateListenerClaimsPendingContentOnlyOnce() {
    LynxTransferManager manager = new LynxTransferManager();
    View content = new View(mContext);
    FrameLayout parent = new FrameLayout(mContext);
    parent.addView(content);
    assertFalse(manager.dispatchTransferCreate("pending", mOwner, content));
    assertNull(content.getParent());
    CountingTransferListener rejecting = new CountingTransferListener("other");
    CountingTransferListener accepting = new CountingTransferListener("pending");
    manager.registerTransferListener(rejecting);
    manager.dispatchTransferDatasetUpdate(content, new JavaOnlyMap());
    manager.registerTransferListener(accepting);
    manager.registerTransferListener(accepting);
    assertEquals(1, accepting.createCount);
    assertEquals(0, rejecting.createCount);
    manager.dispatchTransferDatasetUpdate(content, new JavaOnlyMap());
    assertEquals(1, accepting.datasetUpdateCount);
    manager.clearTransfers();
    assertEquals(1, accepting.removeCount);
  }

  @Test
  @UiThreadTest
  public void unregisterSuppressesCallbacksAndRemovalStillDetachesContent() {
    LynxTransferManager manager = new LynxTransferManager();
    CountingTransferListener listener = new CountingTransferListener("owned");
    manager.registerTransferListener(listener);
    View content = new View(mContext);
    assertTrue(manager.dispatchTransferCreate("owned", mOwner, content));
    manager.unregisterTransferListener(listener);
    manager.dispatchTransferDatasetUpdate(content, new JavaOnlyMap());
    FrameLayout parent = new FrameLayout(mContext);
    parent.addView(content);
    manager.dispatchTransferRemove("owned", content);
    assertNull(content.getParent());
    assertEquals(0, listener.datasetUpdateCount);
    assertEquals(0, listener.removeCount);
    parent.addView(content);
    manager.dispatchTransferRemove("unknown", content);
    assertNull(content.getParent());
    assertFalse(manager.dispatchTransferCreate("pending", mOwner, content));
    parent.addView(content);
    manager.dispatchTransferRemove("pending", content);
    assertNull(content.getParent());
  }

  @Test
  @UiThreadTest
  public void clearDetachesActiveAndPendingContentAndForgetsListeners() {
    LynxTransferManager manager = new LynxTransferManager();
    CountingTransferListener listener = new CountingTransferListener("owned");
    manager.registerTransferListener(listener);
    View active = new View(mContext);
    View pending = new View(mContext);
    assertTrue(manager.dispatchTransferCreate("owned", mOwner, active));
    assertFalse(manager.dispatchTransferCreate("pending", mOwner, pending));
    FrameLayout parent = new FrameLayout(mContext);
    parent.addView(active);
    parent.addView(pending);
    manager.clearTransfers();
    manager.clearTransfers();
    assertEquals(0, parent.getChildCount());
    assertEquals(1, listener.removeCount);
    manager.dispatchTransferDatasetUpdate(active, new JavaOnlyMap());
    assertEquals(0, listener.datasetUpdateCount);
    assertFalse(manager.dispatchTransferCreate("owned", mOwner, active));
    manager.clearTransfers();
  }
}
