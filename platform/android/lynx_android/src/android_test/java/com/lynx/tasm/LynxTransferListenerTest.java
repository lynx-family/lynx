// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;

import android.content.Context;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxTransferListenerTest {
  private static class CountingTransferListener implements LynxTransferListener {
    private final String mAcceptedId;
    int createCount;
    int removeCount;

    CountingTransferListener(String acceptedId) {
      mAcceptedId = acceptedId;
    }

    @Override
    public boolean onCreate(String id, View view) {
      if (!mAcceptedId.equals(id)) {
        return false;
      }
      createCount++;
      return true;
    }

    @Override
    public void onRemove(String id, View view) {
      removeCount++;
    }
  }

  @Test
  public void dispatchTransferRemove_notifiesOnlyListenerThatAcceptedCreate() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxView lynxView = new LynxView(context);
    View transferView = new View(context);
    UITransfer owner = mock(UITransfer.class);
    CountingTransferListener ownerListener = new CountingTransferListener("accepted");
    CountingTransferListener unrelatedListener = new CountingTransferListener("unrelated");

    lynxView.registerTransferListener(ownerListener);
    lynxView.registerTransferListener(unrelatedListener);

    lynxView.dispatchTransferCreate("accepted", owner, transferView);
    lynxView.dispatchTransferRemove("accepted", transferView);

    assertEquals(1, ownerListener.createCount);
    assertEquals(1, ownerListener.removeCount);
    assertEquals(0, unrelatedListener.createCount);
    assertEquals(0, unrelatedListener.removeCount);
  }
}
