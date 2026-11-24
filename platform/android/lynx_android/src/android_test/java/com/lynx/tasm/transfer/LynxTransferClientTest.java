// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.transfer;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

import android.app.Application;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxTransferredViewTestActivity;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxTransferClientTest {
  private Context mContext;
  private LynxView mOwner;
  private UITransfer mNativeOwner;
  private LynxTransferManager mManager;
  private final List<String> mCommands = new ArrayList<>();
  private final List<JavaOnlyMap> mData = new ArrayList<>();

  @Before
  public void setUp() {
    mContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LynxEnv.inst().init((Application) mContext.getApplicationContext(), null, null, null, null);
    UIThreadUtils.runOnUiThreadSync(() -> {
      mOwner = spy(new LynxView(mContext));
      LynxContext context = mock(LynxContext.class);
      when(context.getInstanceId()).thenReturn(37);
      doReturn(context).when(mOwner).getLynxContext();
      mNativeOwner = mock(UITransfer.class);
      when(mNativeOwner.getLynxContext()).thenReturn(context);
    });
    mManager = new LynxTransferManager();
    doAnswer(call -> {
      mCommands.add(call.getArgument(0));
      mData.add(call.getArgument(1));
      return null;
    })
        .when(mOwner)
        .dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    doAnswer(call -> {
      mManager.releaseTransferredView(call.getArgument(0));
      return null;
    })
        .when(mOwner)
        .releaseTransferredView(anyString());
  }

  @After
  public void tearDown() {
    UIThreadUtils.runOnUiThreadSync(() -> {
      mManager.clearTransfers();
      mNativeOwner.destroy();
      mOwner.destroy();
    });
  }

  @Test
  @UiThreadTest
  public void callbacksAreSynchronousAndLoadSuccessAndReleaseAreOnceOnly() {
    List<String> events = new ArrayList<>();
    LynxTransferredView view =
        mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
          @Override
          public void onLoadStart(LynxTransferredView view) {
            assertTrue(mCommands.isEmpty());
            events.add("start");
          }
          @Override
          public void onLoadSuccess(LynxTransferredView view) {
            assertEquals(1, view.getChildCount());
            events.add("loaded");
          }
          @Override
          public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
            assertEquals(0, view.getChildCount());
            assertEquals(TransferReleaseReason.EXPLICIT_RELEASE, reason);
            events.add("released");
            view.release();
          }
        });
    assertEquals(Arrays.asList("start"), events);
    String id = view.createEventData().getString("transferId");
    mManager.dispatchTransferCreate(id, mNativeOwner, new View(mContext));
    assertEquals(Arrays.asList("start", "loaded"), events);
    mManager.dispatchTransferCreate(id, mNativeOwner, new View(mContext));
    view.release();
    view.release();
    assertEquals(Arrays.asList("start", "loaded", "released"), events);
    assertEquals(Arrays.asList("__CreateTransfer", "__DestroyTransfer"), mCommands);
  }

  @Test
  @UiThreadTest
  public void releaseInLoadStartDoesNotCreateOrAcceptLateContent() {
    List<String> events = new ArrayList<>();
    LynxTransferredView view =
        mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
          @Override
          public void onLoadStart(LynxTransferredView view) {
            events.add("start");
            view.release();
          }
          @Override
          public void onLoadSuccess(LynxTransferredView view) {
            fail("Released transfer must not load.");
          }
          @Override
          public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
            events.add("released");
          }
        });
    assertTrue(mCommands.isEmpty());
    View late = new View(mContext);
    mManager.dispatchTransferCreate(
        view.createEventData().getString("transferId"), mNativeOwner, late);
    assertNull(late.getParent());
    assertEquals(Arrays.asList("start", "released"), events);
  }

  @Test
  @UiThreadTest
  public void loadStartPropsBecomeTheInitialProps() {
    JavaOnlyMap updated = new JavaOnlyMap();
    updated.putString("text", "updated during start");
    mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
      @Override
      public void onLoadStart(LynxTransferredView view) {
        view.updateProps(new JavaOnlyMap());
        view.updateProps(updated);
      }
    });
    assertEquals(Arrays.asList("__CreateTransfer"), mCommands);
    assertSame(updated, mData.get(0).getMap("initialProps"));
    assertEquals(3, mData.get(0).size());
  }

  @Test
  @UiThreadTest
  public void releaseInLoadSuccessPreventsFirstScreen() {
    AtomicInteger draws = new AtomicInteger();
    LynxTransferredView view =
        mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
          @Override
          public void onLoadSuccess(LynxTransferredView view) {
            view.release();
          }
          @Override
          public void onFirstScreen(LynxTransferredView view) {
            draws.incrementAndGet();
          }
        });
    View content = new View(mContext);
    mManager.dispatchTransferCreate(
        view.createEventData().getString("transferId"), mNativeOwner, content);
    assertNull(content.getParent());
    view.layout(0, 0, 100, 100);
    Bitmap bitmap = Bitmap.createBitmap(100, 100, Bitmap.Config.ARGB_8888);
    view.draw(new Canvas(bitmap));
    bitmap.recycle();
    assertEquals(0, draws.get());
  }

  @Test
  @UiThreadTest
  public void ownerCleanupAndCreateFailureHaveDistinctReasons() {
    List<TransferReleaseReason> reasons = new ArrayList<>();
    LynxTransferClient client = new LynxTransferClient() {
      @Override
      public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
        reasons.add(reason);
      }
    };
    LynxTransferredView first = mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), client);
    mManager.clearTransferredViews();
    first.release();
    RuntimeException failure = new IllegalStateException("dispatch failed");
    doThrow(failure).when(mOwner).dispatchTransferEvent(
        eq("__CreateTransfer"), any(JavaOnlyMap.class));
    try {
      mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), client);
      fail("Dispatch failure must propagate.");
    } catch (RuntimeException actual) {
      assertSame(failure, actual);
    }
    mManager.clearTransferredViews();
    assertEquals(
        Arrays.asList(TransferReleaseReason.OWNER_DESTROY, TransferReleaseReason.LOAD_FAILED),
        reasons);
  }

  @Test
  @UiThreadTest
  public void callbackExceptionsDoNotInterruptLifecycle() {
    LynxTransferredView view =
        mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
          @Override
          public void onLoadStart(LynxTransferredView view) {
            throw new IllegalStateException("business callback failed");
          }
          @Override
          public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
            throw new IllegalStateException("business callback failed");
          }
        });
    view.release();
    assertEquals(Arrays.asList("__CreateTransfer", "__DestroyTransfer"), mCommands);
  }

  @Test
  @UiThreadTest
  public void optionalClientOverloadsKeepTheTemplatePrerequisite() {
    LynxTransferClient client = mock(LynxTransferClient.class);
    for (int call = 0; call < 3; call++) {
      try {
        if (call == 0) {
          mOwner.loadTransfer("Message", new JavaOnlyMap());
        } else {
          mOwner.loadTransfer("Message", new JavaOnlyMap(), call == 1 ? null : client);
        }
        fail("An unloaded owner must reject creation.");
      } catch (IllegalStateException expected) {
      }
    }
    verifyNoInteractions(client);
    assertTrue(mCommands.isEmpty());
    mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), null).release();
    assertEquals(Arrays.asList("__CreateTransfer", "__DestroyTransfer"), mCommands);
  }

  @Test
  public void firstDrawIsAfterChildDrawingAndIsNotRepeatedAfterReattach() throws Exception {
    AtomicInteger draws = new AtomicInteger();
    AtomicInteger childDraws = new AtomicInteger();
    CountDownLatch firstDraw = new CountDownLatch(1);
    AtomicReference<LynxTransferredView> transfer = new AtomicReference<>();
    try (ActivityScenario<LynxTransferredViewTestActivity> scenario =
             ActivityScenario.launch(LynxTransferredViewTestActivity.class)) {
      scenario.onActivity(activity -> {
        LynxTransferredView view =
            mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
              @Override
              public void onFirstScreen(LynxTransferredView view) {
                assertTrue(childDraws.get() > 0);
                draws.incrementAndGet();
                firstDraw.countDown();
              }
            });
        transfer.set(view);
        TextView content = new TextView(activity) {
          @Override
          protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            childDraws.incrementAndGet();
          }
        };
        content.setText("Managed transfer first draw");
        mManager.dispatchTransferCreate(
            view.createEventData().getString("transferId"), mNativeOwner, content);
        FrameLayout host = new FrameLayout(activity);
        host.addView(view, new FrameLayout.LayoutParams(300, 200));
        activity.setContentView(host);
      });
      assertTrue("The attached content must draw", firstDraw.await(10, TimeUnit.SECONDS));
      scenario.onActivity(activity -> {
        LynxTransferredView view = transfer.get();
        FrameLayout parent = (FrameLayout) view.getParent();
        parent.removeView(view);
        assertFalse(view.isReleased());
        parent.addView(view);
        Bitmap bitmap = Bitmap.createBitmap(300, 200, Bitmap.Config.ARGB_8888);
        view.draw(new Canvas(bitmap));
        bitmap.recycle();
        view.updateProps(new JavaOnlyMap());
        view.release();
      });
      assertEquals(1, draws.get());
    }
  }

  @Test
  public void releaseInsideFirstDrawIsSynchronousAndOnceOnly() throws Exception {
    List<String> events = new ArrayList<>();
    CountDownLatch released = new CountDownLatch(1);
    try (ActivityScenario<LynxTransferredViewTestActivity> scenario =
             ActivityScenario.launch(LynxTransferredViewTestActivity.class)) {
      scenario.onActivity(activity -> {
        LynxTransferredView view =
            mManager.loadTransfer(mOwner, "Message", new JavaOnlyMap(), new LynxTransferClient() {
              @Override
              public void onLoadStart(LynxTransferredView view) {
                events.add("start");
              }
              @Override
              public void onLoadSuccess(LynxTransferredView view) {
                events.add("loaded");
              }
              @Override
              public void onFirstScreen(LynxTransferredView view) {
                events.add("drawn");
                view.release();
                assertEquals("released", events.get(events.size() - 1));
              }
              @Override
              public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
                assertEquals(TransferReleaseReason.EXPLICIT_RELEASE, reason);
                assertEquals(0, view.getChildCount());
                events.add("released");
                view.release();
                released.countDown();
              }
            });
        TextView text = new TextView(activity);
        text.setText("Release after first draw");
        mManager.dispatchTransferCreate(
            view.createEventData().getString("transferId"), mNativeOwner, text);
        activity.setContentView(view);
      });
      assertTrue("First draw must release the instance", released.await(10, TimeUnit.SECONDS));
      InstrumentationRegistry.getInstrumentation().waitForIdleSync();
      assertEquals(Arrays.asList("start", "loaded", "drawn", "released"), events);
      assertEquals(Arrays.asList("__CreateTransfer", "__DestroyTransfer"), mCommands);
    }
  }
}
