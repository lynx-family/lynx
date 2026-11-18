// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.*;
import static org.mockito.Mockito.*;

import android.app.Application;
import android.content.Context;
import android.view.View;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.transfer.UITransfer;
import com.lynx.tasm.transfer.LynxTransferClient;
import com.lynx.tasm.transfer.LynxTransferManager;
import com.lynx.tasm.transfer.LynxTransferredView;
import com.lynx.tasm.transfer.TransferReleaseReason;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxTransferredViewTest {
  private Context mContext;
  private LynxView mOwner;
  private LynxTransferManager mManager;
  private UITransfer mNativeOwner;
  private final List<String> mTypes = new ArrayList<>();
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
    doAnswer(invocation -> {
      mTypes.add(invocation.getArgument(0));
      mData.add(invocation.getArgument(1));
      return null;
    })
        .when(mOwner)
        .dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    doAnswer(invocation -> {
      mManager.releaseTransferredView(invocation.getArgument(0));
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
  public void nativeEventsTargetTheMainThreadContext() {
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    doCallRealMethod().when(render).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    JavaOnlyMap data = new JavaOnlyMap();
    doAnswer(invocation -> {
      JavaOnlyMap event = invocation.getArgument(0);
      assertEquals("Native", event.getString("origin"));
      assertEquals("CoreContext", event.getString("target"));
      assertEquals("__CreateTransfer", event.getString("type"));
      assertSame(data, event.getMap("data"));
      return null;
    })
        .when(render)
        .dispatchMessageEvent(any(JavaOnlyMap.class));
    render.dispatchTransferEvent("__CreateTransfer", data);
    verify(render).dispatchMessageEvent(any(JavaOnlyMap.class));
  }

  @Test
  @UiThreadTest
  public void loadRequiresACompletedTemplate() {
    doCallRealMethod().when(mOwner).loadTransfer(anyString(), any(JavaOnlyMap.class));
    try {
      mOwner.loadTransfer("A", new JavaOnlyMap());
      fail("An unloaded LynxView must reject transfer creation.");
    } catch (IllegalStateException expected) {
    }
    verify(mOwner, never()).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
  }

  @Test
  @UiThreadTest
  public void controlPayloadsMatchTheFlatFrontendContract() {
    LynxTemplateRender original = mOwner.mLynxTemplateRender;
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    when(render.isTransferReady()).thenReturn(true);
    doCallRealMethod().when(mOwner).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    doCallRealMethod().when(render).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    doAnswer(call -> {
      JavaOnlyMap envelope = call.getArgument(0);
      assertEquals("Native", envelope.getString("origin"));
      assertEquals("CoreContext", envelope.getString("target"));
      mTypes.add(envelope.getString("type"));
      mData.add((JavaOnlyMap) envelope.getMap("data"));
      return null;
    })
        .when(render)
        .dispatchMessageEvent(any(JavaOnlyMap.class));
    mOwner.mLynxTemplateRender = render;
    try {
      JavaOnlyMap initial = new JavaOnlyMap();
      initial.putString("text", "hello");
      LynxTransferredView view = mManager.loadTransfer(mOwner, "Message", initial);
      JavaOnlyMap updated = new JavaOnlyMap();
      updated.putString("text", "next");
      view.setProps(updated);
      view.release();

      assertEquals(
          Arrays.asList("__CreateTransfer", "__SetTransferProps", "__DestroyTransfer"), mTypes);
      assertEquals(new HashSet<>(Arrays.asList("transferId", "componentName", "initialProps")),
          mData.get(0).asHashMap().keySet());
      assertEquals(
          new HashSet<>(Arrays.asList("transferId", "props")), mData.get(1).asHashMap().keySet());
      assertEquals(new HashSet<>(Arrays.asList("transferId")), mData.get(2).asHashMap().keySet());
      String id = mData.get(0).getString("transferId");
      assertNotNull(id);
      assertFalse(id.isEmpty());
      assertEquals("Message", mData.get(0).getString("componentName"));
      assertSame(initial, mData.get(0).getMap("initialProps"));
      assertSame(updated, mData.get(1).getMap("props"));
      assertEquals(id, mData.get(1).getString("transferId"));
      assertEquals(id, mData.get(2).getString("transferId"));

      mManager.clearTransfers();
      mManager.loadTransfer(mOwner, "Message", initial);
      assertNotEquals(id, mData.get(3).getString("transferId"));
    } finally {
      mOwner.mLynxTemplateRender = original;
    }
  }

  @Test
  @UiThreadTest
  public void propsAreForwardedAndInstancesHaveSeparateIdentities() {
    JavaOnlyMap props = new JavaOnlyMap();
    props.putString("text", "initial");
    LynxTransferredView a = mManager.loadTransfer(mOwner, "A", props);
    LynxTransferredView b = mManager.loadTransfer(mOwner, "B", new JavaOnlyMap());
    assertSame(props, mData.get(0).getMap("initialProps"));
    assertNotEquals(mData.get(0).getString("transferId"), mData.get(1).getString("transferId"));
    JavaOnlyMap nextProps = new JavaOnlyMap();
    a.setProps(nextProps);
    assertEquals("__SetTransferProps", mTypes.get(2));
    assertSame(nextProps, mData.get(2).getMap("props"));
    assertEquals(0, mData.get(2).getMap("props").size());
    assertEquals(mData.get(0).getString("transferId"), mData.get(2).getString("transferId"));
    b.release();
    a.release();
  }

  @Test
  @UiThreadTest
  public void releaseBeforeCreateRejectsLateContentAndPreservesOtherInstances() {
    LynxTransferredView a = mManager.loadTransfer(mOwner, "A", new JavaOnlyMap());
    String aId = mData.get(0).getString("transferId");
    LynxTransferredView b = mManager.loadTransfer(mOwner, "B", new JavaOnlyMap());
    String bId = mData.get(1).getString("transferId");
    a.release();
    a.release();
    assertEquals(3, mTypes.size());
    assertEquals("__DestroyTransfer", mTypes.get(2));
    View late = new View(mContext);
    mManager.dispatchTransferCreate(aId, mNativeOwner, late);
    assertNull(late.getParent());
    View content = new View(mContext);
    mManager.dispatchTransferCreate(bId, mNativeOwner, content);
    assertSame(b, content.getParent());
    a.setProps(new JavaOnlyMap());
    assertEquals(3, mTypes.size());
    b.setProps(new JavaOnlyMap());
    assertEquals(4, mTypes.size());
    assertEquals("__SetTransferProps", mTypes.get(3));
    b.release();
  }

  @Test
  @UiThreadTest
  public void oldRemovalDoesNotRemoveReplacementContent() {
    LynxTransferredView view = mManager.loadTransfer(mOwner, "A", new JavaOnlyMap());
    String id = mData.get(0).getString("transferId");
    View oldContent = new View(mContext);
    View newContent = new View(mContext);
    mManager.dispatchTransferCreate(id, mNativeOwner, oldContent);
    mManager.dispatchTransferCreate(id, mNativeOwner, newContent);
    mManager.dispatchTransferRemove(id, oldContent);
    assertEquals(1, view.getChildCount());
    assertSame(newContent, view.getChildAt(0));
    mManager.clearTransfers();
    assertNull(newContent.getParent());
    view.release();
    assertEquals(1, mTypes.size());
    view.setProps(new JavaOnlyMap());
    assertEquals(1, mTypes.size());
  }

  @Test
  @UiThreadTest
  public void unavailableTemplateIgnoresTransferCommands() {
    LynxTemplateRender original = mOwner.mLynxTemplateRender;
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    doCallRealMethod().when(mOwner).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
    try {
      mOwner.mLynxTemplateRender = render;
      for (String command :
          Arrays.asList("__CreateTransfer", "__SetTransferProps", "__DestroyTransfer")) {
        mOwner.dispatchTransferEvent(command, new JavaOnlyMap());
      }
      verify(render, never()).dispatchTransferEvent(anyString(), any(JavaOnlyMap.class));
      mOwner.mLynxTemplateRender = null;
      for (String command :
          Arrays.asList("__CreateTransfer", "__SetTransferProps", "__DestroyTransfer")) {
        mOwner.dispatchTransferEvent(command, new JavaOnlyMap());
      }
      assertTrue(mTypes.isEmpty());
    } finally {
      mOwner.mLynxTemplateRender = original;
    }
  }

  @Test
  @UiThreadTest
  public void reloadRejectsReentrantTransfersUntilTheNextLoad() {
    LynxView owner = new LynxView(mContext, new LynxViewBuilder()) {
      @Override
      public void dispatchTransferEvent(String type, JavaOnlyMap data) {
        mTypes.add(type);
        mData.add(data);
      }
    };
    LynxTemplateRender render = owner.mLynxTemplateRender;
    // Stop at the Java reload boundary, before an unloaded engine enters native reload.
    RuntimeException stopBeforeNativeReload = new RuntimeException("Stop before native reload");
    LynxViewClientV2 client = new LynxViewClientV2() {
      @Override
      public void onPageStarted(LynxView owner, LynxPipelineInfo info) {
        throw stopBeforeNativeReload;
      }
    };
    TemplateData data = TemplateData.fromString("{}");
    owner.addLynxViewClientV2(client);
    try {
      render.new TASMCallback().onLoaded(0);
      assertTrue(render.isTransferReady());
      int[] releases = {0};
      int[] rejectedLoads = {0};
      LynxTransferredView view =
          owner.loadTransfer("Message", new JavaOnlyMap(), new LynxTransferClient() {
            @Override
            public void onReleased(LynxTransferredView view, TransferReleaseReason reason) {
              assertEquals(TransferReleaseReason.OWNER_DESTROY, reason);
              assertEquals(0, view.getChildCount());
              releases[0]++;
              try {
                owner.loadTransfer("Message", new JavaOnlyMap());
                fail("Reload cleanup must reject transfer creation inside onReleased.");
              } catch (IllegalStateException expected) {
                rejectedLoads[0]++;
              }
            }
          });
      View content = new View(mContext);
      owner.dispatchTransferCreate(mData.get(0).getString("transferId"), mNativeOwner, content);
      assertSame(view, content.getParent());
      try {
        render.reloadTemplate(data, null);
        fail("The reload boundary must be reached.");
      } catch (RuntimeException actual) {
        assertSame(stopBeforeNativeReload, actual);
      }
      assertEquals(1, releases[0]);
      assertEquals(1, rejectedLoads[0]);
      assertNull(content.getParent());
      assertFalse(render.isTransferReady());
      view.setProps(new JavaOnlyMap());
      assertEquals(Arrays.asList("__CreateTransfer"), mTypes);
      try {
        owner.loadTransfer("Message", new JavaOnlyMap());
        fail("Transfer creation must wait for the next load callback.");
      } catch (IllegalStateException expected) {
      }

      render.new TASMCallback().onLoaded(0);
      assertTrue(render.isTransferReady());
      owner.loadTransfer("Message", new JavaOnlyMap()).release();
      assertEquals(
          Arrays.asList("__CreateTransfer", "__CreateTransfer", "__DestroyTransfer"), mTypes);
      assertEquals(1, releases[0]);
    } finally {
      owner.removeLynxViewClientV2(client);
      data.recycle();
      owner.destroy();
    }
  }
}
